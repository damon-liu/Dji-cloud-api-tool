# 机场常用控制按钮 — 设计文档

日期：2026-07-18
状态：已确认（用户授权全部采用推荐方案）

## 需求

实现 RELEASE-v1.0.1.md 预告的 v1.0.2「常用控制按钮」：在界面集成一键控制按钮，无需手动编辑 JSON 即可执行常见机场操作。

**本迭代范围**：

| 控制项 | 指令 method |
|--------|-------------|
| 进入/退出远程调试 | `debug_mode_open` / `debug_mode_close` |
| 飞机开机/关机 | `drone_open` / `drone_close` |
| 舱盖打开/关闭 | `cover_open` / `cover_close` |
| 充电开启/关闭 | `charge_open` / `charge_close` |

**明确延后**：「🚀 一键起飞」——走 `takeoff_to_point` API，需要目标经纬度、安全起飞高度、返航高度等参数，且需先抢占飞行控制权，不适合无参数一键发送。留待后续版本单独设计参数对话框。

## 现状基础（复用未跟踪文件）

以下文件已存在且代码完整，直接复用：

- `src/core/DockCommand.h/.cpp` — 指令类型枚举、`DockCommandBuilder`（构造 `thing/product/{sn}/services` 报文：tid/bid/timestamp/gateway/method/data；解析 reply 的 tid + data.result）
- `src/ui/DockControlPanel.h/.cpp` — 控制面板 UI：调试模式组 + 常用控制网格；二次确认弹窗；状态机（须先进入调试模式、pending 期间禁用、离线/未连接禁用）；信号 `commandRequested(gatewaySn, type)`；槽 `onCommandStateChanged(DockCommandResult)`

**缺失的集成**（本次实现内容）：CMake 未登记、无指令执行器、MainWindow 未接入。

## 架构设计

```
DockControlPanel ──commandRequested──▶ DeviceManager::executeDockCommand
                                              │
                                              ▼
                                    DockCommandExecutor::execute
                                       │ 1. 拒绝并发（单 pending）
                                       │ 2. DockCommandBuilder::build
                                       │ 3. 确保订阅 {sn}/services_reply
                                       │ 4. publish + 10s 超时定时器
                                       ▼
MqttClientManager ──messageReceived──▶ DeviceManager::onMqttMessage
                                       │ 先转发 executor 再 parseAndRoute
                                       ▼
                          DockCommandExecutor::onMqttMessage
                             │ 匹配 reply topic + tid → result
                             ▼
      commandStateChanged(DockCommandResult) ──▶ DeviceManager 转发
                             ──dockCommandStateChanged──▶ DockControlPanel
```

### 1. 新增 `src/core/DockCommandExecutor.h/.cpp`

- 构造：`DockCommandExecutor(MqttClientManager* mqtt, QObject* parent)`
- `bool execute(const QString& gatewaySn, DockCommandType type)`
  - 有 pending 时直接返回 false
  - `DockCommandBuilder::build()` 生成请求
  - `mqtt->subscribeTopics({"thing/product/<sn>/services_reply"})`（内部去重，防止用户禁用了该 topic 导致收不到回复）
  - 发出 `Publishing` 状态 → `mqtt->publish()` → 发出 `WaitingReply` 状态
  - 启动 10 秒单次超时定时器
- `void onMqttMessage(const QString& topic, const QByteArray& payload)`
  - 无 pending 或 topic 不匹配 pending 的 reply topic → 忽略
  - `DockCommandBuilder::parseReply()`：invalid 或 tid 不匹配 → 忽略
  - `result == 0` → `Succeeded`；否则 → `Failed`（message 含错误码）
  - 清除 pending、停止定时器
- 超时 → `TimedOut`（message 提示检查 services_reply 订阅与设备状态）
- 信号：`commandStateChanged(const DockCommandResult&)`

### 2. `DeviceManager` 接入

- 新增成员 `DockCommandExecutor* mDockCmdExecutor`（构造函数创建）
- 新增槽 `executeDockCommand(const QString& gatewaySn, DockCommandType type)` 转调 executor
- `onMqttMessage()` 中先 `mDockCmdExecutor->onMqttMessage(topic, payload)` 再 `parseAndRoute(topic, payload)`
- 新增信号 `dockCommandStateChanged(const DockCommandResult&)`，构造函数中与 executor 信号直连转发

### 3. `MainWindow` 接入

- 新增成员 `DockControlPanel* mDockControlPanel`、`QPushButton* mToggleDockCtrlBtn`
- `setupLayout()`：面板加入右侧 `verticalSplitter`（`mPublishPanel` 之后），默认隐藏、`setMinimumHeight(120)`；底部把「▶ Topic 下发」按钮改为放入 QHBoxLayout，并排新增「▶ 机场控制」切换按钮（同 `publishToggle` objectName 样式，checkable）
- `onDeviceSelected()`：Dock → `setDevice(dev->name, sn, dev->online)`；Aircraft 且有 parentSn → 用父机场的 name/sn/online；否则 `clearDevice()`。取消选中分支同样 `clearDevice()`
- `connectSignals()`：
  - `mDockControlPanel::commandRequested` → `mDevMgr::executeDockCommand`
  - `mDevMgr::dockCommandStateChanged` → `mDockControlPanel::onCommandStateChanged`
  - `brokerConnected` lambda 内加 `mDockControlPanel->setConnected(true)`；`brokerDisconnected` 加 `setConnected(false)`
  - `deviceOnlineChanged` lambda 内：若变化设备与面板当前 gatewaySn 相关则刷新 `setDevice`（复用 onDeviceSelected 的当前选中刷新即可）

### 4. `CMakeLists.txt`

SOURCES 增加 `src/core/DockCommand.cpp`、`src/core/DockCommandExecutor.cpp`、`src/ui/DockControlPanel.cpp`；HEADERS 增加对应 3 个 .h。

## 错误处理

| 场景 | 行为 |
|------|------|
| MQTT 未连接 | 面板按钮禁用（现有 setConnected 逻辑） |
| 机场离线 | 面板按钮禁用（现有 setDevice 逻辑） |
| 并发指令 | executor 拒绝，面板 mPending 也已拦截 |
| reply result != 0 | Failed 状态 + 错误码展示 |
| 10s 无回复 | TimedOut 状态 + 提示 |
| tid 不匹配（他端指令的回复） | 忽略 |

## 测试

无自动化测试基础设施，手动验证：

1. `cmake --build build_mingw` 编译通过
2. 未连接 broker：面板所有按钮禁用，状态提示"MQTT 未连接"
3. 连接后选中机场/子飞机：面板显示机场信息；未进入调试模式时仅调试按钮可用
4. 下发"进入远程调试"：确认弹窗 → 状态变化（Publishing → WaitingReply → 结果）
5. 无真机场景：等待 10s 观察超时提示
6. 「▶ 机场控制」按钮展开/收起面板正常
