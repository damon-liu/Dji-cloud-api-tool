# Tasks: 界面问题修复 (PRD 1.8)

## 任务清单

### 1. OsdPanel 添加定时刷新 + 间隔配置
- [x] 在 OsdPanel 添加 `QTimer` 成员（默认 1000ms）
- [x] 添加刷新间隔 `QComboBox`（1s/2s/5s/10s，默认 1s）
- [x] 实现定时 `refresh()` 槽函数：从 DeviceManager 缓存读取 OSD 刷新 UI
- [x] 添加 `pause()` / `resume()` 公开接口
- [x] 在 MainWindow::setupLayout 中传入 DeviceManager 指针给 OsdPanel

### 2. TopicParsePanel 公开暂停接口
- [x] 添加 `pause()` / `resume()` 公开槽函数
- [x] 记录手动暂停状态，resume 时仅在非手动暂停时恢复

### 3. MainWindow 连接断连信号到面板暂停
- [x] brokerDisconnected 处理中调用 OsdPanel::pause() + TopicParsePanel::pause()
- [x] brokerConnected 处理中调用 OsdPanel::resume() + TopicParsePanel::resume()
- [x] 处理手动暂停优先逻辑：断连前若手动暂停则重连后保持暂停

### 4. TopicManager 添加批量启用/禁用接口
- [x] 新增 `setAllTopicsEnabled(deviceSn, enabled)` 方法
- [x] DeviceManager 添加同名代理方法

### 5. TopicListWidget 添加全部启用/禁用按钮
- [x] 标题栏新增全部切换按钮（视觉区分于单个切换 ◎ 按钮）
- [x] 实现按钮点击逻辑：判断状态、发射 `topicAllToggled` 信号
- [x] MainWindow 连接信号到 DeviceManager::setAllTopicsEnabled
