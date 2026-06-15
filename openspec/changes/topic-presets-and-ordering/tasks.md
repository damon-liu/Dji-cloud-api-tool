# Tasks: 完善订阅和发送 Topic

## 实现任务

- [x] 1. **DeviceManager 默认 topic 追加**
  在 `addDevice()` 中，检测设备类型为 `Dock` 时，追加 7 个预设 topic 并默认禁用。
  - 文件: `src/core/DeviceManager.cpp`
  - 新增常量 `DEFAULT_DOCK_TOPICS`（7 个 topic 模式）
  - 调用 `mTopicManager->setDisabledTopicsForDevice()` 将新 topic 设为禁用

- [x] 2. **TopicListWidget 上移/下移按钮**
  新增 ▲ 和 ▼ 按钮，实现 topic 顺序调整功能。
  - 文件: `src/ui/TopicListWidget.h`, `src/ui/TopicListWidget.cpp`
  - 新增 signal: `topicOrderChanged(const QString& sn, const QStringList& orderedTopics)`
  - 按钮仅在选中有效 topic 时启用
  - 保持启用/禁用状态标记

- [x] 3. **MainWindow 连接排序信号**
  连接 `topicOrderChanged` 信号到 DeviceManager，更新 topic 顺序并持久化。
  - 文件: `src/ui/MainWindow.cpp`

- [x] 4. **PublishPanel 下发预设 topic**
  新增 5 个下发专用 topic 预设，合并到 ComboBox 中（不去订阅）。
  - 文件: `src/ui/PublishPanel.h`
  - 新增 `setDeviceSn(sn)` 方法用于替换 {sn}
  - ComboBox 显示: 订阅 topic + 分隔线 + 下发预设 topic

- [x] 5. **编译验证与端到端测试**
  - 编译项目
  - 创建新机场设备，验证 7 个默认 topic 已添加且处于禁用状态
  - 验证上移/下移功能正常
  - 验证下发面板额外显示 5 个预设 topic
  - 验证无人机不受影响
