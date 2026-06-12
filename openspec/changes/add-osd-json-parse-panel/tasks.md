## 1. 映射配置

- [ ] 1.1 创建 `config/topic_mappings.json`，包含 `thing/product/{sn}/osd` topic 的完整映射（dock 30+ 字段 + aircraft 30+ 字段），含分组定义、中文名、单位、枚举值翻译
- [ ] 1.2 实现 `src/core/TopicMapping.h` — 加载/解析 JSON 映射文件，提供 `mappingForTopic(topic)` 查询接口，返回字段映射表和分组列表
- [ ] 1.3 处理异常：首次启动自动生成默认映射文件、JSON 格式错误时降级到内置最小映射表

## 2. JSON 解析面板

- [ ] 2.1 实现 `src/ui/OsdParsePanel.h` — 分组表格面板，包含标题栏（显示当前 topic）、刷新间隔选择器（1s/2s/5s/10s）、暂停/继续按钮
- [ ] 2.2 实现定时刷新逻辑：QTimer 驱动，从 `DeviceManager::latestRawJson()` 获取最新 JSON，调用 `TopicMapping` 翻译后渲染
- [ ] 2.3 实现分组表格渲染：按映射中定义的 groups 生成分组标题行 + 两列 QFormLayout（中文名 | 值+单位），未映射字段灰色显示在底部
- [ ] 2.4 实现值变化高亮：对比新旧值，变化字段蓝闪 1.2 秒（复用 OsdPanel 的 QTimer::singleShot 高亮模式）
- [ ] 2.5 实现暂停/恢复：暂停时停止 QTimer 并冻结面板，恢复时立即刷新一次数据

## 3. 主窗口集成

- [ ] 3.1 修改 `MainWindow::setupLayout()` — 右侧分割器左半区改为 QVBoxLayout，OsdPanel 在上、OsdParsePanel 在下
- [ ] 3.2 修改 `MainWindow::connectSignals()` — 连接 topicSelectionChanged → OsdParsePanel::setTopic()，设备选择/OFF切换 → 面板清空/刷新
- [ ] 3.3 更新 `CMakeLists.txt` 添加新源文件和 `config/topic_mappings.json` 的资源部署

## 4. 编译验证

- [ ] 4.1 使用 MinGW 编译项目，修复所有编译错误和警告
- [ ] 4.2 启动应用验证：添加设备 → 连接 Broker → 确认 JSON 解析面板正常展示翻译后的数据
- [ ] 4.3 测试边界情况：无映射 topic、空 JSON、未选中设备、快速切换 topic
