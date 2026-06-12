## ADDED Requirements

### Requirement: Toggle topic enabled/disabled state

系统 SHALL 允许用户对当前选中设备的单个 topic 进行启用/禁用切换。禁用 topic SHALL 在列表中显示为灰色 `○` 前缀，启用 topic SHALL 显示为 `●` 前缀。

#### Scenario: Disable an enabled topic
- **WHEN** 用户在 topic 列表中选中一个启用状态的 topic 并点击「禁用」按钮
- **THEN** 该 topic 状态变为禁用，列表中对应条目前缀变为 `○` 并显示灰色，MQTT 取消订阅该 topic

#### Scenario: Enable a disabled topic
- **WHEN** 用户在 topic 列表中选中一个禁用状态的 topic 并点击「启用」按钮
- **THEN** 该 topic 状态变为启用，列表中对应条目前缀变为 `●`，MQTT 重新订阅该 topic（若 Broker 已连接）

#### Scenario: Toggle with no topic selected
- **WHEN** 用户未选中任何 topic 时
- **THEN** 启用/禁用按钮处于禁用状态

### Requirement: MQTT subscription excludes disabled topics

系统 SHALL 在订阅时过滤掉所有已禁用的 topic，仅向 Broker 订阅启用状态的 topic。

#### Scenario: Broker connected with mixed topic states
- **WHEN** Broker 已连接且设备同时有启用和禁用的 topic
- **THEN** 只有启用状态的 topic 被发送到 Broker 进行订阅，禁用 topic 不发送订阅请求

#### Scenario: Topic disabled while connected
- **WHEN** 用户禁用一个处于启用状态且已订阅的 topic
- **THEN** 系统立即取消订阅该 topic（调用 unsubscribe）

#### Scenario: Topic enabled while connected
- **WHEN** 用户启用一个处于禁用状态的 topic
- **THEN** 系统立即订阅该 topic（调用 subscribe）

### Requirement: Persist topic enabled/disabled state

Topic 的启用/禁用状态 SHALL 持久化到 `config.json` 中，应用重启后 SHALL 恢复之前的启用/禁用状态。

#### Scenario: Save disabled topics to config
- **WHEN** 用户禁用某个 topic 后配置被保存
- **THEN** `config.json` 中该设备条目下 `disabled_topics` 数组包含该 topic 字符串

#### Scenario: Load config with disabled topics
- **WHEN** 应用启动加载包含 `disabled_topics` 字段的配置文件
- **THEN** 对应 topic 在列表中显示为禁用状态（`○` 灰色），且不参与 MQTT 订阅

#### Scenario: Load legacy config without disabled_topics
- **WHEN** 应用启动加载旧格式配置文件（topic 为纯字符串数组，无 `disabled_topics` 字段）
- **THEN** 所有 topic 默认启用，正常参与 MQTT 订阅
