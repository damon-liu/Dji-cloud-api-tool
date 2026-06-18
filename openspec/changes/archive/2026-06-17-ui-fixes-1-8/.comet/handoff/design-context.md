# Comet Design Handoff

- Change: ui-fixes-1-8
- Phase: design
- Mode: compact
- Context hash: bc1922287f2f47cf684c7ef668cec45b2c1dddd975e90e6f2bf4f555a3d72b9f

Generated-by: comet-handoff.sh

OpenSpec remains the canonical capability spec. This handoff is a deterministic, source-traceable context pack, not an agent-authored summary.

## openspec/changes/ui-fixes-1-8/proposal.md

- Source: openspec/changes/ui-fixes-1-8/proposal.md
- Lines: 1-35
- SHA256: d916f857dab191dc8ba94d42b41f0dca1e39d60e427253480062c285368ba922

```md
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
```

## openspec/changes/ui-fixes-1-8/design.md

- Source: openspec/changes/ui-fixes-1-8/design.md
- Lines: 1-44
- SHA256: 2abd3be7b52e656e8d33b00cb0748c7702007c54021ccb49c2fb84cbf12e824c

```md
# Design: 界面问题修复 (PRD 1.8)

## 架构决策

### 1. OsdPanel 定时刷新

**方案：事件驱动 + 定时器并行**

OsdPanel 新增 `QTimer`（默认 1000ms），定时从 `DeviceManager` 缓存读取最新 OSD 数据并刷新 UI。与现有 `MainWindow::onOsdUpdated` 信号驱动的刷新并行工作：

- MQTT 消息到达 → `deviceOsdUpdated` 信号 → 立即刷新（保持现有路径）
- 定时器到期 → 从缓存读取最新 OSD → 刷新（兜底，确保 UI 始终显示缓存中最新的状态）

新增 `QComboBox` 提供间隔选项：1s / 2s / 5s / 10s，默认 1s。

### 2. 断连自动暂停/恢复

**方案：状态传递而非耦合**

```
MainWindow 中监听 brokerDisconnected / brokerConnected：
  brokerDisconnected → OsdPanel::pause()  + TopicParsePanel::pause()
  brokerConnected    → OsdPanel::resume() + TopicParsePanel::resume()
```

- OsdPanel::pause() / resume()：控制新增的定时器启停
- TopicParsePanel::pause() / resume()：新增公开方法，停止/启动内部 mRefreshTimer
- 手动暂停状态优先：若用户在断连前手动暂停了面板，重连后保持暂停

### 3. 一键全量启用/禁用 Topic

**方案：TopicListWidget 新增按钮 + DeviceManager 批量接口**

```
TopicListWidget 标题栏新增 "⊘" 按钮
   ↓ 点击
   → 判断当前设备 topic 状态（全部启用则执行全部禁用，否则全部启用）
   → emit topicAllToggled(deviceSn, enabled)
   → MainWindow 连接 → DeviceManager::setAllTopicsEnabled(deviceSn, enabled)
   → TopicManager::setAllTopicsEnabled 逐个调用 setTopicEnabled
   → 发射 topicsChanged → MQTT 重新订阅
```

`DeviceManager::setAllTopicsEnabled` 代理到 `TopicManager`，后者遍历设备所有 topic 调用已有的 `setTopicEnabled`。禁用状态通过 `ConfigStore` 的 `mDisabledTopics` 持久化。
```

## openspec/changes/ui-fixes-1-8/tasks.md

- Source: openspec/changes/ui-fixes-1-8/tasks.md
- Lines: 1-28
- SHA256: 527830696ba213c89d41eaa37548aea97f11fbe6f29d9abf2739a15167cad71e

```md
# Tasks: 界面问题修复 (PRD 1.8)

## 任务清单

### 1. OsdPanel 添加定时刷新 + 间隔配置
- [ ] 在 OsdPanel 添加 `QTimer` 成员（默认 1000ms）
- [ ] 添加刷新间隔 `QComboBox`（1s/2s/5s/10s，默认 1s）
- [ ] 实现定时 `refresh()` 槽函数：从 DeviceManager 缓存读取 OSD 刷新 UI
- [ ] 添加 `pause()` / `resume()` 公开接口
- [ ] 在 MainWindow::setupLayout 中传入 DeviceManager 指针给 OsdPanel

### 2. TopicParsePanel 公开暂停接口
- [ ] 添加 `pause()` / `resume()` 公开槽函数
- [ ] 记录手动暂停状态，resume 时仅在非手动暂停时恢复

### 3. MainWindow 连接断连信号到面板暂停
- [ ] brokerDisconnected 处理中调用 OsdPanel::pause() + TopicParsePanel::pause()
- [ ] brokerConnected 处理中调用 OsdPanel::resume() + TopicParsePanel::resume()
- [ ] 处理手动暂停优先逻辑：断连前若手动暂停则重连后保持暂停

### 4. TopicManager 添加批量启用/禁用接口
- [ ] 新增 `setAllTopicsEnabled(deviceSn, enabled)` 方法
- [ ] DeviceManager 添加同名代理方法

### 5. TopicListWidget 添加全部启用/禁用按钮
- [ ] 标题栏新增全部切换按钮（视觉区分于单个切换 ◎ 按钮）
- [ ] 实现按钮点击逻辑：判断状态、发射 `topicAllToggled` 信号
- [ ] MainWindow 连接信号到 DeviceManager::setAllTopicsEnabled
```

