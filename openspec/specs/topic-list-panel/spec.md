# Topic List Panel

## Purpose

提供设备树下方的内联 topic 列表区域，允许用户在选中设备时直观查看、添加、删除该设备的所有 MQTT 订阅 topic，替代原有的弹窗式 Topic 编辑对话框。

## Requirements

### Requirement: Topic list panel display

系统 SHALL 在设备树下方提供一个独立的 topic 列表区域，与设备树处于同一列（左侧面板）。该区域 SHALL 仅在用户选中设备时显示该设备的所有订阅 topic。设备树（QTreeWidget）与 Topic 面板内的 QListWidget SHALL 采用相同的宽度约束（最小 170px，最大 220px），确保左右边缘对齐。

#### Scenario: User selects a device
- **WHEN** 用户在设备树中点击选择一个设备
- **THEN** topic 列表区域显示该设备的所有订阅 topic，每条 topic 显示其启用/禁用状态（● 启用 / ○ 禁用）

#### Scenario: No device selected
- **WHEN** 没有设备被选中
- **THEN** topic 列表区域显示空状态提示"（请选择设备）"

#### Scenario: Device has no topics
- **WHEN** 选中设备没有任何 topic
- **THEN** topic 列表显示空提示"（无 Topic）"

#### Scenario: Panel width alignment
- **WHEN** 左侧面板渲染完成
- **THEN** DeviceTree 与 Topic 面板中的 QListWidget 左右边缘对齐，宽度一致

### Requirement: Topic add button

Topic 列表区域右侧 SHALL 提供「添加」按钮（＋），点击后弹出输入框允许用户输入新的 MQTT topic 字符串并添加到当前选中设备。

#### Scenario: Add a new topic
- **WHEN** 用户点击 topic 区域的「＋」按钮并输入有效 topic 字符串
- **THEN** 系统将该 topic 添加到当前选中设备，topic 列表刷新，MQTT 自动订阅该 topic（若设备已连接且 topic 为启用状态）

#### Scenario: Add empty topic
- **WHEN** 用户点击「＋」按钮但输入为空
- **THEN** 系统不执行任何操作（或弹出提示）

#### Scenario: Add duplicate topic
- **WHEN** 用户输入的 topic 已存在于当前设备
- **THEN** 系统不重复添加，topic 列表不变

### Requirement: Topic delete button

Topic 列表区域右侧 SHALL 提供「删除」按钮（✕），点击后删除当前选中的 topic。

#### Scenario: Delete a topic
- **WHEN** 用户在 topic 列表中选中一个 topic 并点击「删除」按钮
- **THEN** 系统弹出确认对话框，确认后从设备移除该 topic，MQTT 取消订阅，持久化配置

#### Scenario: Delete with no topic selected
- **WHEN** 用户未在 topic 列表中选中任何 topic 时点击「删除」按钮
- **THEN** 删除按钮处于禁用状态

### Requirement: Old edit button removal

系统 SHALL 移除设备树右侧的「✎ 编辑 Topic」按钮，其功能由 topic 列表区域替代。

#### Scenario: Sidebar buttons after removal
- **WHEN** 应用启动
- **THEN** 设备树右侧仅显示「＋ 添加设备」和「✕ 删除设备」按钮

### Requirement: Topic list auto-refresh

当选中设备的 topic 集合发生变化时，topic 列表 SHALL 自动刷新。刷新时 SHALL 保持用户手动选中的 topic 项不变；仅当切换设备（选中不同 SN）时才默认选中第一个 topic。

#### Scenario: Topic added by external action
- **WHEN** 当前选中设备的 topic 通过任何途径被添加或删除
- **THEN** topic 列表自动更新显示最新状态

#### Scenario: User selection preserved during refresh
- **WHEN** 用户已手动选中某个 topic，且 topic 列表因数据更新而刷新
- **THEN** 刷新后该 topic 保持选中状态（若该 topic 仍存在于列表中）

#### Scenario: First item selected on device switch
- **WHEN** 用户切换到不同设备
- **THEN** topic 列表默认选中第一个 topic（若存在）
