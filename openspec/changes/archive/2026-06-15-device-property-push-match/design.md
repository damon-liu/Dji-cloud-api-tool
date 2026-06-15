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
