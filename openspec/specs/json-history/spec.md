# JSON History

## Purpose

原始 JSON 面板累积显示所有历史 MQTT 上报数据，而非仅显示最新一条。最新数据追加到面板末尾，自动滚动到底部。

## Requirements

### Requirement: JSON panel accumulates historical data

系统 SHALL 在原始 JSON 面板中累积显示设备的所有历史 MQTT 消息，最新消息追加到面板末尾。系统 SHALL 在追加新消息后自动滚动到底部。

#### Scenario: New MQTT message arrives
- **WHEN** 设备收到新的 MQTT 消息
- **THEN** 原始 JSON 面板在现有内容末尾追加新消息（以分隔线隔开），并自动滚动到底部

#### Scenario: Switch to different device
- **WHEN** 用户切换到不同设备
- **THEN** 原始 JSON 面板显示该设备的历史累积数据（若存在），若该设备无历史数据则显示空

#### Scenario: Clear history
- **WHEN** 用户点击清空按钮
- **THEN** 原始 JSON 面板清空当前设备的所有历史数据

#### Scenario: Pause stops display updates
- **WHEN** 用户点击暂停按钮
- **THEN** 原始 JSON 面板冻结当前显示，不再追加新数据，但后台持续订阅

### Requirement: JSON history capacity limit

系统 SHALL 限制每设备 JSON 历史记录最大 500 条。超出限制时 SHALL 从头部删除最旧记录。

#### Scenario: History exceeds capacity
- **WHEN** 某设备的 JSON 历史记录超过 500 条
- **THEN** 系统自动删除最早的记录，保持总数不超过 500 条
