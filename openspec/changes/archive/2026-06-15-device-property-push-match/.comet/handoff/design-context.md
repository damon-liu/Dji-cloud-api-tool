# Comet Design Handoff

- Change: device-property-push-match
- Phase: design
- Mode: compact
- Context hash: e7349358e32dd12feabb3fa3911d32b489030443c04c0c51509bcf2bba34548f

Generated-by: comet-handoff.sh

OpenSpec remains the canonical capability spec. This handoff is a deterministic, source-traceable context pack, not an agent-authored summary.

## openspec/changes/device-property-push-match/proposal.md

- Source: openspec/changes/device-property-push-match/proposal.md
- Lines: 1-29
- SHA256: f6c03db7b2965e3a5fddb9c6559b7877c54f0b79e89bcd614fe6e1e98879f9f0

```md
# 设备属性推送匹配

## 背景

当前系统仅对 `thing/product/{sn}/osd` topic 提供 JSON 解析功能。当用户手动订阅 `thing/product/{sn}/state`（设备属性推送）或 `sys/product/{gateway_sn}/status`（网关设备状态）时，JSON 解析面板显示"该 topic 暂无映射配置"，无法自动翻译 DJI 协议字段。

## 目标

扩展 JSON 解析面板（OsdParsePanel → TopicParsePanel），使其支持以下 topic 的自动字段翻译：

1. **`thing/product/{sn}/state`** — 复用 dock-osd.md 中定义的属性字段映射（与 osd topic 共用同一套设备属性定义）
2. **`sys/product/{sn}/status`** — 基于 dock-status.md 中定义的网关设备状态字段

## 范围

### 代码
- 重命名 `OsdParsePanel` → `TopicParsePanel`（类名 + 文件 + CMakeLists + MainWindow 引用）
- `DeviceManager::mRawJsonCache` 从设备级改为设备+Topic 级存储
- `DeviceManager::latestRawJson()` 增加 topic 参数
- `TopicParsePanel::refresh()` 传入当前 topic

### 数据
- `topic_mappings.json` 新增 `thing/product/{sn}/state` 映射
- `topic_mappings.json` 新增 `sys/product/{sn}/status` 映射

## 非目标
- 不新增 C++ 数据结构（StateData / StatusData）
- 不修改 MQTT 订阅逻辑
- 不修改 UI 布局
```

## openspec/changes/device-property-push-match/design.md

- Source: openspec/changes/device-property-push-match/design.md
- Lines: 1-72
- SHA256: 13f59c0dc11b27edd0d8ac516d4d7bd997afd20fc1124ec0ab6fe75a92e26b56

```md
# 设计文档

## 架构决策

### 1. 重命名 OsdParsePanel → TopicParsePanel

`OsdParsePanel` 设计上已经是 topic 无关的——它通过 `TopicMapping` 类做字段翻译，不依赖特定 topic 类型。重命名消除误导，表达其通用性。

**影响文件**：
- `src/ui/OsdParsePanel.h` → `src/ui/TopicParsePanel.h`
- `src/ui/OsdParsePanel.cpp` → `src/ui/TopicParsePanel.cpp`
- `src/ui/MainWindow.h`（成员类型声明）
- `src/ui/MainWindow.cpp`（构造、连接、include）
- `CMakeLists.txt`（源文件列表）

### 2. mRawJsonCache 改为 per-topic 存储

**现状问题**：`QMap<QString, QString> mRawJsonCache` 按设备 SN 存一条 JSON。当设备同时订阅 `osd` + `state` 时，后到的消息覆盖前者，解析面板可能拿到错误 topic 的数据。

**方案**：
```cpp
// 之前
QMap<QString, QString> mRawJsonCache;           // sn → json
QString latestRawJson(const QString& sn) const;

// 之后
QMap<QString, QMap<QString, QString>> mRawJsonCache; // sn → topic → json
QString latestRawJson(const QString& sn, const QString& topic = QString()) const;
```

- 不传 topic 时回退到返回任意可用 topic 的 JSON（兼容旧调用）
- `TopicParsePanel::refresh()` 传入选中的 topic 精确获取数据

### 3. topic_mappings.json 扩展

```json
{
  "topics": {
    "thing/product/{sn}/osd": { ... },
    "thing/product/{sn}/state": { /* 复用 osd 的 fields/groups */ },
    "sys/product/{sn}/status": { /* 基于 dock-status.md */ }
  }
}
```

**state topic**：DJI 协议中 `/state` 与 `/osd` 使用相同的属性名称（dock-osd.md 定义），区别仅在于推送机制（state 为事件驱动，osd 为 0.5Hz 定时）。字段映射完全复用。

**status topic**：`sys/product/{gateway_sn}/status` 是网关设备上线时的一次性状态上报，字段定义来自 dock-status.md，包含 gateway 元信息 + 子设备列表。

## 数据流

```
MQTT消息 (topic: thing/product/dock_001/state, payload: {...})
  │
  ▼
DeviceManager::parseAndRoute()
  │
  ├─ mRawJsonCache["dock_001"]["thing/product/dock_001/state"] = formattedJson
  │
  └─ emit deviceOsdUpdated("dock_001", "thing/product/dock_001/state", formattedJson)
        │
        ▼
MainWindow::onOsdUpdated()  →  RawJsonPanel 更新
        │
        ▼
TopicParsePanel::refresh()
  │
  ├─ mDevMgr->latestRawJson("dock_001", "thing/product/dock_001/state")
  ├─ mMapping->mappingForTopic("thing/product/dock_001/state")
  │   → 模式匹配 thing/product/{sn}/state → TopicMappingConfig
  └─ renderGroups(data)  →  中文分组渲染
```
```

## openspec/changes/device-property-push-match/tasks.md

- Source: openspec/changes/device-property-push-match/tasks.md
- Lines: 1-27
- SHA256: d2270a8ddaad3e5fd33af3a83273530c33bca067d774c7766a34bd0e9d2348a9

```md
# 任务清单

## 任务

- [ ] **Task 1: 重命名 OsdParsePanel → TopicParsePanel**
  - 重命名 `src/ui/OsdParsePanel.h` → `src/ui/TopicParsePanel.h`（更新 include guard + 类名）
  - 重命名 `src/ui/OsdParsePanel.cpp` → `src/ui/TopicParsePanel.cpp`（更新 include + 类名引用）
  - 更新 `src/ui/MainWindow.h` 中成员类型 + include
  - 更新 `src/ui/MainWindow.cpp` 中所有引用
  - 更新 `CMakeLists.txt` 源文件列表
  - 编译验证通过

- [ ] **Task 2: DeviceManager mRawJsonCache 改为 per-topic 存储**
  - 修改 `mRawJsonCache` 类型为 `QMap<QString, QMap<QString, QString>>`
  - 修改 `latestRawJson()` 签名，增加 topic 参数（默认空，空时返回任意可用 topic 数据兼容旧调用）
  - 修改 `parseAndRoute()` 中写入缓存的 key 为 sn+topic
  - 编译验证通过

- [ ] **Task 3: TopicParsePanel 传入 topic 获取精确数据**
  - 修改 `TopicParsePanel::refresh()` 调用 `latestRawJson` 时传入 `mTopic`
  - 编译验证通过

- [ ] **Task 4: topic_mappings.json 新增 state 和 status 映射**
  - 新增 `thing/product/{sn}/state` 条目（复用 osd 的 fields 和 groups）
  - 新增 `sys/product/{sn}/status` 条目（基于 dock-status.md 字段定义）
  - 将更新后的 `topic_mappings.json` 复制到 `deploy/` 目录
  - 编译验证通过
```

