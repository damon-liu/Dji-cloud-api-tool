# JSON Topic Filter

## Purpose

原始 JSON 面板根据用户在 Topic 列表中选中的 topic 过滤显示内容。选中 topic 时仅显示该 topic 的数据；未选中时显示所有。

## Requirements

### Requirement: JSON panel filters by selected topic

系统 SHALL 在原始 JSON 面板中根据用户在 Topic 列表中选中的 topic 过滤显示内容。当用户选中某个 topic 时，面板 SHALL 仅显示该 topic 的历史订阅数据；当用户未选中任何 topic 时，面板 SHALL 显示当前设备所有 topic 的数据。

#### Scenario: User selects a topic in topic list
- **WHEN** 用户在 Topic 列表中选择一个 topic
- **THEN** 原始 JSON 面板刷新，仅显示该 topic 的历史订阅数据

#### Scenario: No topic selected
- **WHEN** 用户未选中任何 topic（或 topic 列表无选中项）
- **THEN** 原始 JSON 面板显示当前设备所有 topic 的历史数据

#### Scenario: Switch between topics
- **WHEN** 用户从 topic A 切换到 topic B
- **THEN** 原始 JSON 面板清除当前内容，加载 topic B 的历史数据

### Requirement: JSON history organized by topic

系统 SHALL 将 JSON 历史数据按 MQTT topic 存储，而非仅按设备 SN 聚合。`DeviceManager` SHALL 在收到消息时记录消息来源 topic。

#### Scenario: Message stored with topic association
- **WHEN** MQTT 消息到达指定 topic
- **THEN** 系统将 JSON 数据存入该 topic 对应的历史缓冲区
