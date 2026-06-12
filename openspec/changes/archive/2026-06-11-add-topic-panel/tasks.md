## 1. 数据层 — Topic 启用/禁用状态

- [ ] 1.1 `TopicManager` 新增 `mDisabledTopics` 集合 (`QMap<QString, QSet<QString>>`)，提供 `setTopicEnabled(sn, topic, enabled)`、`isTopicEnabled(sn, topic)`、`enabledTopicsForDevice(sn)` 方法
- [ ] 1.2 修改 `TopicManager::allTopics()` 和 `topicsForDevice()`，区分全部 topic 和仅启用的 topic；`removeDevice()` 时同步清理 `mDisabledTopics`
- [ ] 1.3 `ConfigStore` 新增 `disabledTopicsForDevice(sn)` 和 `setDisabledTopicsForDevice(sn, topics)` 方法；`mDeviceTopics` 类型从 `QMap<QString, QStringList>` 改为 `QMap<QString, QSet<QString>>`
- [ ] 1.4 `ConfigStore::load()` 解析新 JSON 格式 `disabled_topics` 字段，旧格式（纯字符串数组）向后兼容
- [ ] 1.5 `ConfigStore::save()` 输出 `disabled_topics` 数组，保持 `topics` 字段为全部 topic（含禁用的）
- [ ] 1.6 `DeviceManager` 新增 `setTopicEnabled(sn, topic, enabled)` 和 `isTopicEnabled(sn, topic)` 方法，同步到 ConfigStore 并持久化

## 2. MQTT 层 — 断开连接修复与禁用过滤

- [ ] 2.1 `MqttClientManager` 新增 `mIntentionalDisconnect` 标志位；`disconnectFromBroker()` 中设为 true
- [ ] 2.2 `MqttClientManager::onDisconnected()` 检查 `mIntentionalDisconnect`，若为 true 则跳过 `startReconnect()` 并重置标志位
- [ ] 2.3 `MqttClientManager::connectToBroker()` 重置 `mIntentionalDisconnect = false`，确保后续意外断线可重连
- [ ] 2.4 `DeviceManager::onTopicsChanged()` 中仅将启用状态的 topic 传入 `mMqttManager->replaceSubscriptions()`；topic 启用/禁用切换时触发实时订阅/取消订阅

## 3. UI 层 — TopicListWidget 组件

- [ ] 3.1 新建 `src/ui/TopicListWidget.h` 和 `src/ui/TopicListWidget.cpp`，包含 `QListWidget` + 标题 + 右侧按钮列（＋ 添加、◎ 切换启用/禁用、✕ 删除）
- [ ] 3.2 `TopicListWidget` 提供 `setTopics(sn, topics, disabledTopics)` 刷新列表，每条 topic 根据启用/禁用状态显示 `●`/`○` 前缀（禁用项灰色）
- [ ] 3.3 `TopicListWidget` 发出 `topicAdded(sn, topic)`、`topicToggled(sn, topic)`、`topicRemoved(sn, topic)` 信号
- [ ] 3.4 实现添加 topic 对话框（复用 `TopicEditDialog` 的添加逻辑或内联 `QInputDialog`）
- [ ] 3.5 实现删除 topic 确认对话框
- [ ] 3.6 无设备选中时显示"（请选择设备）"，无 topic 时显示"（无 Topic）"
- [ ] 3.7 更新 `CMakeLists.txt` 添加 `TopicListWidget.cpp` 到构建

## 4. UI 层 — MainWindow 布局重构

- [ ] 4.1 `setupLayout()` 左侧面板增加 `TopicListWidget`（置于设备树下方），设置最大高度 ~200px 并启用滚动
- [ ] 4.2 移除旧的 `mEditTopicBtn`（✎）及其样式和信号连接
- [ ] 4.3 `onDeviceSelected()` 中调用 `mTopicListWidget->setTopics()` 同步当前设备 topic；移除旧的 `mEditTopicBtn->setEnabled(true)` 等逻辑
- [ ] 4.4 连接 `TopicListWidget` 信号到 `DeviceManager` 对应方法（addTopic / setTopicEnabled / removeTopic）
- [ ] 4.5 连接 `DeviceManager` 的设备变更信号，刷新 topic 列表

## 5. UI 层 — 添加设备逻辑变更

- [ ] 5.1 修改 `MainWindow::onAddDevice()`：选中 Dock 时跳过类型选择，固定添加 Aircraft 子设备（`parentSn` = Dock SN）
- [ ] 5.2 选中 Aircraft 时 `mAddDeviceBtn` 禁用；选中 Dock 或未选中时启用
- [ ] 5.3 `onDeviceSelected()` 中根据选中设备类型更新 `mAddDeviceBtn` 状态

## 6. UI 层 — 工具栏按钮调整

- [ ] 6.1 `setupToolBar()` 中将 `mConnectAct` 和 `mDisconnectAct` 移到 spacer 右侧（布局变为：配置 — spacer — Broker 标签 — 连接 — 断开）
- [ ] 6.2 调整按钮样式以适配右侧位置

## 7. 集成验证

- [ ] 7.1 验证断开连接按钮：手动断开后不自动重连，再次连接后意外断线仍可重连
- [ ] 7.2 验证 topic 禁用：禁用后 MQTT 订阅列表更新，重新连接后禁用 topic 仍不订阅
- [ ] 7.3 验证配置持久化：重启应用后 topic 启用/禁用状态恢复
- [ ] 7.4 验证旧配置兼容：使用旧格式 `config.json`（无 `disabled_topics`）启动，所有 topic 默认启用
- [ ] 7.5 验证 Dock 添加子设备：子设备显示在 Dock 下，有默认 OSD topic，Aircraft 无法添加子设备
