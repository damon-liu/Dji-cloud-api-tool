## ADDED Requirements

### Requirement: Dock can add handheld drone as child device

当用户在设备树中选中一个 Dock（机场）设备时，系统 SHALL 允许通过「添加设备」按钮为该 Dock 新增手飞无人机作为子设备。新增时 SHALL 要求用户输入设备名称和 SN。

#### Scenario: Add handheld drone to selected dock
- **WHEN** 用户在设备树中选中一个 Dock 设备并点击「添加设备」按钮
- **THEN** 系统弹出输入框，依次询问手飞无人机的设备名称和 SN，确认后将 Aircraft 设备添加为 Dock 的子设备（`parentSn` 设为 Dock 的 SN），设备树中显示为 Dock 的子节点

#### Scenario: Default topic for handheld drone
- **WHEN** 手飞无人机作为 Dock 子设备添加成功
- **THEN** 系统自动为其生成默认 OSD topic（`thing/product/{sn}/osd`）

#### Scenario: Child device displayed under parent dock
- **WHEN** 手飞无人机添加到 Dock 后
- **THEN** 设备树中该无人机显示为 Dock 的子节点（缩进，图标为 ✈），Dock 节点自动展开

### Requirement: Handheld drone cannot have child devices

手飞无人机（`DeviceType::Aircraft`）SHALL NOT 被允许添加子设备。当选中 Aircraft 设备时，添加设备按钮 SHALL 处于禁用状态。

#### Scenario: Add device button disabled for aircraft
- **WHEN** 用户在设备树中选中一个 Aircraft 类型设备
- **THEN** 「添加设备」按钮处于禁用状态（灰色），不可点击

#### Scenario: Add device button enabled for dock
- **WHEN** 用户在设备树中选中一个 Dock 类型设备
- **THEN** 「添加设备」按钮处于可用状态

#### Scenario: Add device button enabled when nothing selected
- **WHEN** 用户未选中任何设备
- **THEN** 「添加设备」按钮处于可用状态，允许添加顶级设备（Dock 或独立 Aircraft）

### Requirement: Disconnect button fix

系统 SHALL 在用户手动点击「断开」按钮后停止 MQTT 连接且不触发自动重连。自动重连机制 SHALL 仅在非手动断开（网络异常、Broker 主动断开等）时生效。

#### Scenario: Manual disconnect stops reconnect
- **WHEN** 用户点击「◎ 断开」按钮
- **THEN** MQTT 连接断开，状态栏显示"未连接"，系统不尝试自动重新连接

#### Scenario: Auto reconnect on unexpected disconnect
- **WHEN** MQTT 连接因网络异常或 Broker 主动断开而中断
- **THEN** 系统自动启动指数退避重连机制（基数 1s，上限 30s）

#### Scenario: Reconnect after manual disconnect then connect
- **WHEN** 用户手动断开后再次点击「● 连接」
- **THEN** 系统正常连接 Broker，后续若发生意外断线仍触发自动重连
