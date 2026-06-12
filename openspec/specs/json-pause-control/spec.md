# JSON Pause Control

## Purpose

原始 JSON 面板提供暂停/继续控制，暂停时冻结显示但后台 MQTT 订阅持续进行，恢复时一次性显示暂停期间累积的数据。

## Requirements

### Requirement: JSON panel pause control

原始 JSON 面板 SHALL 在复制按钮旁提供暂停/继续按钮。点击暂停后面板 SHALL 停止刷新显示，但 MQTT 数据订阅 SHALL 持续进行。点击继续后 SHALL 恢复实时刷新。

#### Scenario: Pause JSON display
- **WHEN** 用户点击「⏸ 暂停」按钮
- **THEN** 原始 JSON 面板冻结当前显示不再刷新，按钮变为「▶ 继续」，后台 MQTT 订阅不受影响

#### Scenario: Resume JSON display
- **WHEN** 用户在暂停状态下点击「▶ 继续」按钮
- **THEN** 原始 JSON 面板恢复实时刷新，一次性显示暂停期间累积的数据

#### Scenario: Pause buffer overflow protection
- **WHEN** 暂停状态下累积超过 1000 条新数据
- **THEN** 系统从缓冲区头部删除最旧记录，保持缓冲不超过 1000 条
