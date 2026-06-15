---
comet_change: device-property-push-match
role: technical-design
canonical_spec: openspec
archived-with: 2026-06-15-device-property-push-match
status: final
---

# 设备属性推送匹配 — 技术设计

## 概述

扩展 JSON 解析面板，使其支持 `thing/product/{sn}/state` 和 `sys/product/{sn}/status` 两个 topic 的自动字段翻译。同时修复 `mRawJsonCache` 设备级存储导致的同设备多 topic 数据覆盖问题。

## 技术方案

### 1. 重命名 OsdParsePanel → TopicParsePanel

**当前问题**：`OsdParsePanel` 命名暗示仅处理 OSD 数据，但实际实现已是 topic 无关的通用 JSON 解析面板。

**改动文件**：

| 文件 | 变更 |
|------|------|
| `src/ui/OsdParsePanel.h` → `src/ui/TopicParsePanel.h` | 类名 + include guard |
| `src/ui/OsdParsePanel.cpp` → `src/ui/TopicParsePanel.cpp` | 类名 + include |
| `src/ui/MainWindow.h` | `#include` + 成员类型 `mOsdParsePanel` → `mTopicParsePanel` |
| `src/ui/MainWindow.cpp` | 所有 `OsdParsePanel` 引用 → `TopicParsePanel` |
| `CMakeLists.txt` | 源文件列表 |

**接口不变**：`setTopic()`、`refresh()`、`setDeviceManager()`、`setTopicMapping()` 签名保持不变。

### 2. DeviceManager per-topic 缓存

**当前问题**：
```cpp
// 问题：同设备多个 topic 互相覆盖
QMap<QString, QString> mRawJsonCache;           // sn → json (❌)
QString latestRawJson(const QString& sn) const;  // 无法区分 topic
```

**修改后**：
```cpp
// 修复：二级 map，sn → topic → json
QMap<QString, QMap<QString, QString>> mRawJsonCache;  // sn → topic → json (✅)
QString latestRawJson(const QString& sn, const QString& topic = QString()) const;
```

**`latestRawJson` 实现逻辑**：
- 传入有效 topic → 精确返回 `mRawJsonCache[sn][topic]`
- topic 为空 → 返回该设备任意一条缓存数据（兼容 `OsdPanel` 旧调用）

**`parseAndRoute` 写入变更**：
```cpp
// 之前
mRawJsonCache[sn] = formatted;
// 之后
mRawJsonCache[sn][topic] = formatted;
```

**`DeviceManager.h` 接口声明变更**：
- `QString latestRawJson(const QString& sn) const;` → `QString latestRawJson(const QString& sn, const QString& topic = QString()) const;`
- `QMap<QString, QString> mRawJsonCache;` → `QMap<QString, QMap<QString, QString>> mRawJsonCache;`

### 3. TopicParsePanel 传入 topic

**`refresh()` 修改**：
```cpp
// 之前
QString rawJson = mDevMgr->latestRawJson(mDeviceSn);
// 之后
QString rawJson = mDevMgr->latestRawJson(mDeviceSn, mTopic);
```

仅一行变更。

### 4. topic_mappings.json 扩展

**`thing/product/{sn}/state`**：与 `thing/product/{sn}/osd` 使用相同的 `fields` 和 `groups`。在 JSON 中独立复制一份（而非引用），理由：
- 两个 topic 的字段集未来可能因 DJI 协议升级而分化
- JSON 不支持引用语法，复制保持格式一致

**`sys/product/{sn}/status`**：基于 dock-status.md 定义新映射。字段结构：

```
domain, type, sub_type, device_secret, nonce, thing_version
sub_devices[]
  └─ sn, domain, type, sub_type, index, device_secret, nonce, thing_version
```

分组策略：字段数量少（~15 个），使用单一分组"网关信息"，子设备数组不展开（数组元素在 flattenJson 中自然处理）。

**部署**：`topic_mappings.json` 随 `deploy/DjiCloudApi.exe` 一同推送。

## 数据流

```
MQTT message (topic: thing/product/dock_001/state)
  │
  ▼
parseAndRoute()
  ├─ mRawJsonCache["dock_001"]["thing/product/dock_001/state"] = json
  └─ emit deviceOsdUpdated("dock_001", "thing/product/dock_001/state", json)
        │
        ▼
MainWindow::onOsdUpdated()
  ├─ RawJsonPanel 按 topic 过滤显示
  └─ TopicParsePanel::refresh()
        ├─ latestRawJson("dock_001", "thing/product/dock_001/state")
        ├─ mappingForTopic("thing/product/dock_001/state")
        │   → 模式匹配 thing/product/{sn}/state → 返回映射配置
        └─ renderGroups(data) → 中文分组渲染
```

## 错误处理

| 场景 | 处理 |
|------|------|
| topic 无映射配置 | 显示"该 topic 暂无映射配置" |
| JSON 解析失败 | `qWarning` + 跳过本次刷新 |
| `data` 字段为空 | 跳过渲染 |
| topic 为旧的 ostopic 但缓存无数据 | `latestRawJson` 返回空字符串，`refresh()` 提前返回 |

## 测试策略

- **编译验证**：每个 task 完成后 `cmake --build build_mingw` 确保编译通过
- **手动功能测试**：
  1. 订阅 `thing/product/{sn}/state` → TopicParsePanel 选中该 topic → 验证字段翻译
  2. 订阅 `sys/product/{sn}/status` → TopicParsePanel 选中该 topic → 验证字段翻译
  3. 同一设备同时订阅 `osd` + `state` → 切换 topic 验证数据不混淆
- **回归测试**：确认 `osd` topic 解析 + `OsdPanel` 设备信息显示正常
