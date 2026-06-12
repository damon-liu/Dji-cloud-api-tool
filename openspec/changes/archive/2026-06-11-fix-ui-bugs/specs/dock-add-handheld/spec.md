## MODIFIED Requirements

### Requirement: Handheld drone cannot have child devices

手飞无人机（`DeviceType::Aircraft`）SHALL NOT 被允许添加子设备。当选中 Aircraft 设备时，添加设备按钮 SHALL 处于禁用状态。系统 SHALL 支持用户通过点击设备树空白区域取消当前设备选中。

#### Scenario: Add device button disabled for aircraft
- **WHEN** 用户在设备树中选中一个 Aircraft 类型设备
- **THEN** 「添加设备」按钮处于禁用状态（灰色），不可点击

#### Scenario: Add device button enabled for dock
- **WHEN** 用户在设备树中选中一个 Dock 类型设备
- **THEN** 「添加设备」按钮处于可用状态

#### Scenario: Add device button enabled when nothing selected
- **WHEN** 用户未选中任何设备
- **THEN** 「添加设备」按钮处于可用状态，允许添加顶级设备（Dock 或独立 Aircraft）

#### Scenario: Deselect device by clicking empty area
- **WHEN** 用户在设备树空白区域点击
- **THEN** 当前选中设备被取消选中，「添加设备」按钮恢复为「可添加顶级设备」状态

#### Scenario: Deselect clears detail panel
- **WHEN** 用户取消选中设备
- **THEN** OSD 面板、原始 JSON 面板、Topic 列表面板均清空或显示空状态

### Requirement: Dock can add handheld drone as child device

当用户在设备树中选中一个 Dock（机场）设备时，系统 SHALL 允许通过「添加设备」按钮为该 Dock 新增手飞无人机作为子设备。新增时 SHALL 要求用户输入设备名称和 SN。用户 SHALL 可通过点击设备树空白区域取消选中 Dock，之后「添加设备」按钮恢复为添加顶级设备。

#### Scenario: Add handheld drone to selected dock
- **WHEN** 用户在设备树中选中一个 Dock 设备并点击「添加设备」按钮
- **THEN** 系统弹出输入框，依次询问手飞无人机的设备名称和 SN，确认后将 Aircraft 设备添加为 Dock 的子设备（`parentSn` 设为 Dock 的 SN），设备树中显示为 Dock 的子节点

#### Scenario: Default topic for handheld drone
- **WHEN** 手飞无人机作为 Dock 子设备添加成功
- **THEN** 系统自动为其生成默认 OSD topic（`thing/product/{sn}/osd`）

#### Scenario: Child device displayed under parent dock
- **WHEN** 手飞无人机添加到 Dock 后
- **THEN** 设备树中该无人机显示为 Dock 的子节点（缩进，图标为 ✈），Dock 节点自动展开

#### Scenario: Cancel dock selection to add top-level device
- **WHEN** 用户已选中 Dock 但想添加顶级设备
- **THEN** 用户可点击设备树空白区域取消选中，然后点击「添加设备」进行顶级设备添加
