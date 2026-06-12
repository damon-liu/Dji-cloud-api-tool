## Why

当前界面将 topic 管理隐藏在对话框中，用户无法直观查看设备的订阅主题；MQTT 断开按钮因自动重连逻辑冲突而失效。本次变更将 topic 视图提升为左侧面板的独立区域，并新增 topic 级别的启用/禁用控制，同时修复断开连接 Bug。

## What Changes

- **新增 Topic 列表面板**：在设备树下方独立的 topic 列表区域，展示当前选中设备的所有订阅 topic，包含 topic 的启用/禁用状态指示
- **Topic 操作按钮迁移**：新增 topic 专用的「添加」「禁用/启用」「删除」按钮，置于 topic 区域右侧；移除旧的设备「✎ 编辑 Topic」按钮
- **Topic 启用/禁用**：支持对单个 topic 进行启用/禁用切换，禁用后该 topic 不再被 MQTT 订阅，状态持久化到配置文件
- **机场添加手飞无人机**：选中的 Dock 设备可通过「添加设备」新增手飞无人机作为子设备（含设备名称和 SN），手飞无人机不可再添加子设备
- **修复断开连接 Bug**：用户手动断开 MQTT 连接后不再触发自动重连（`MqttClientManager` 增加主动断开标记）
- **连接/断开按钮移至右侧**：工具栏中连接和断开操作按钮移至界面右侧

## Capabilities

### New Capabilities
- `topic-list-panel`: 设备树下方独立的 topic 列表区域，展示选中设备的订阅 topic 及其启用/禁用状态，提供添加、禁用、删除操作
- `topic-enable-disable`: 单个 topic 级别的启用/禁用切换，禁用后不参与 MQTT 订阅，状态持久化
- `dock-add-handheld`: 机场设备可添加手飞无人机作为子设备（含设备名称和 SN），手飞无人机限制不可添加子设备

### Modified Capabilities
<!-- 当前 openspec/specs/ 中无已有 spec，无修改型能力 -->

## Impact

- **数据层**: `ConfigStore` — topic JSON 格式从纯字符串数组变更为带 `enabled` 字段的结构（向后兼容旧格式）；`TopicManager` — 新增禁用 topic 集合管理
- **MQTT 层**: `MqttClientManager` — 订阅时过滤禁用 topic；修复手动断开后自动重连 Bug
- **UI 层**: `MainWindow` — 左侧面板布局重构（新增 TopicListWidget，移除 ✎ 按钮，连接/断开按钮右移）；新增 `TopicListWidget` 组件
- **配置文件**: `config.json` 中 topics 字段格式变更，需兼容旧格式读取
