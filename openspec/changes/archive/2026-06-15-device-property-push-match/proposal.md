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
