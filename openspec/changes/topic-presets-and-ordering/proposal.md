# Proposal: 完善订阅和发送 Topic

## 问题背景

当前系统在创建机场设备时仅默认添加 `thing/product/{sn}/osd` 一个 topic。DJI Cloud API 协议包含多种标准 topic（状态、事件、请求/响应等），用户需要手动逐个添加，操作繁琐且容易遗漏。同时下发面板仅展示已订阅的 topic，缺少下发专用的 topic 预设。

## 目标

1. **创建机场设备时自动追加 7 个默认 topic**（默认禁用状态），覆盖 DJI 协议常用的订阅 topic
2. **下发面板额外提供 5 个下发专用 topic**，独立于订阅列表，便于用户选择后发送指令
3. **Topic 列表支持上移/下移排序**，用户可手动调整 topic 顺序并持久化

## 范围

### 涉及

- `DeviceManager::addDevice()` — 创建机场时追加默认 topic
- `TopicListWidget` — 新增上移/下移按钮
- `PublishPanel` — 新增独立的下发 topic 预设列表
- `TopicManager` / `ConfigStore` — topic 顺序持久化

### 不涉及

- 无人机设备（仅机场受影响）
- 已有设备的 topic 列表（不自动补全）
- MQTT 连接/断开逻辑
- TopicEditDialog

## 非目标

- 不实现拖拽排序
- 下发 panel 的 5 个 topic 不出现在订阅列表中
- 不修改无人机创建设备流程
