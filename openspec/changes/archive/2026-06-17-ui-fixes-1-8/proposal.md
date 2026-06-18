# Proposal: 界面问题修复 (PRD 1.8)

## 问题背景

当前 DJI Cloud API 监控客户端存在三个界面行为问题，影响使用体验：

1. **OsdPanel 机场数据缺乏定时刷新**：设备信息面板完全依赖 MQTT 事件驱动更新，当数据长时不更新时界面僵死，用户无法感知当前缓存状态。而 TopicParsePanel 已有定时刷新机制但默认间隔为 2s。
2. **断连后界面不冻结**：MQTT 断开连接后，OsdPanel 和 TopicParsePanel 仍按各自定时器持续尝试刷新（虽然读不到新数据），给用户"仍在工作"的错觉。
3. **Topic 逐个启用/禁用效率低**：TopicListWidget 仅支持单个 topic 的启用/禁用切换，当设备订阅多个 topic 时需要逐一点击。

## 目标

1. OsdPanel 新增可配置间隔的定时刷新（默认 1s），与事件驱动更新并行工作
2. MQTT 断连时自动暂停 OsdPanel 和 TopicParsePanel 的数据刷新，重连后自动恢复
3. TopicListWidget 新增一键全部启用/禁用当前设备所有 topic 的按钮

## 范围

### 涉及模块

| 模块 | 改动类型 |
|------|----------|
| `OsdPanel` | 新增 QTimer + 刷新间隔 ComboBox；新增 pause/resume 接口 |
| `TopicParsePanel` | 新增公开 pause/resume 接口（供 MainWindow 调用） |
| `MainWindow` | 连接 brokerDisconnected/brokerConnected → 面板暂停/恢复 |
| `TopicListWidget` | 新增「全部启用/全部禁用」按钮 + 信号 |
| `DeviceManager` | 新增 `setAllTopicsEnabled(sn, enabled)` 批量接口 |
| `TopicManager` | 新增 `setAllTopicsEnabled(sn, enabled)` 实现 |

### 非目标

- 不修改 RawJsonPanel 的手动暂停逻辑
- 不改变 TopicParsePanel 已有的手动暂停/刷新间隔机制
- 不涉及 PublishPanel（v1.1 范围）
- 不修改 MQTT 重连策略
