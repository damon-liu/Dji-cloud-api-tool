## MODIFIED Requirements

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
