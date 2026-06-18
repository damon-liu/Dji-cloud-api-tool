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
