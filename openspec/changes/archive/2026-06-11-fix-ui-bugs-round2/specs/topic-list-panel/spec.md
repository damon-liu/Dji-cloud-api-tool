## MODIFIED Requirements

### Requirement: Topic add button

Topic 列表区域右侧 SHALL 提供「添加」按钮（＋），点击后弹出输入框允许用户输入新的 MQTT topic 字符串并添加到当前选中设备。「＋」按钮 SHALL 与设备列表中的「＋」按钮样式和尺寸（28×28px）保持一致。

### Requirement: Topic delete button

Topic 列表区域右侧 SHALL 提供「删除」按钮（✕），点击后删除当前选中的 topic。「✕」按钮 SHALL 与设备列表中的「✕」按钮样式和尺寸（28×28px）保持一致。

#### Scenario: Delete a topic
- **WHEN** 用户在 topic 列表中选中一个 topic 并点击「删除」按钮
- **THEN** 系统弹出确认对话框，确认后从设备移除该 topic，MQTT 取消订阅，持久化配置

#### Scenario: Delete with no topic selected
- **WHEN** 用户未在 topic 列表中选中任何 topic 时点击「删除」按钮
- **THEN** 删除按钮处于禁用状态
