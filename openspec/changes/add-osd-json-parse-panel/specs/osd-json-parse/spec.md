## ADDED Requirements

### Requirement: 外部映射配置文件
系统 SHALL 在 `config/topic_mappings.json` 中维护 JSON key → 中文名称的映射配置。配置文件 SHALL 按 topic 模式组织，每个 topic SHALL 包含字段定义（中文名、单位、可选的枚举值翻译）和分组定义（字段归类）。

#### Scenario: 首次启动自动生成默认映射文件
- **WHEN** 应用首次启动且 `config/topic_mappings.json` 不存在
- **THEN** 系统 SHALL 自动生成包含 `thing/product/{sn}/osd` topic 完整映射的默认配置文件

#### Scenario: 映射文件损坏
- **WHEN** 映射文件存在但 JSON 格式无效
- **THEN** 系统 SHALL 输出警告日志并使用内置的最小映射表降级运行

### Requirement: OSD 数据定时解析与翻译
OsdParsePanel SHALL 使用 QTimer 定时从 DeviceManager 获取选中设备的当前选中 topic 的最新原始 JSON 数据，并根据 topic 对应的映射配置将 key 翻译为中文后展示。

#### Scenario: 正常刷新
- **WHEN** 用户选中一个设备和一个 topic 且 QTimer 触发
- **THEN** 系统 SHALL 从 `DeviceManager::latestRawJson()` 获取最新 JSON，解析并翻译后在面板中展示

#### Scenario: 未选中设备或 topic
- **WHEN** 用户未选中任何设备或 topic
- **THEN** OsdParsePanel SHALL 清空面板内容

#### Scenario: 无映射配置
- **WHEN** 当前 topic 在映射文件中无对应配置
- **THEN** 面板 SHALL 显示提示信息"该 topic 暂无映射配置"

### Requirement: 分组表格展示
解析后的数据 SHALL 按映射文件中定义的分组进行分类展示，每组包含标题行和两列表格（中文名 | 值）。映射文件中已定义但 JSON 中不存在的字段 SHALL 显示为 "-"。

#### Scenario: 已映射字段展示
- **WHEN** JSON 中的字段 key 在映射表中存在
- **THEN** 系统 SHALL 显示中文名称、值（附加单位后缀），并归类到对应分组

#### Scenario: 未映射字段展示
- **WHEN** JSON 中存在映射表未定义的字段
- **THEN** 系统 SHALL 在面板底部以灰色样式显示该字段的原始英文 key 和值

#### Scenario: 枚举值翻译
- **WHEN** 字段在映射表中定义了 `values` 枚举映射且 JSON 中的值匹配某枚举项
- **THEN** 系统 SHALL 显示翻译后的中文枚举值而非原始数值

### Requirement: 刷新间隔配置
用户 SHALL 能够从预设选项（1秒、2秒、5秒、10秒）中选择 OsdParsePanel 的刷新间隔，默认值为 2 秒。

#### Scenario: 修改刷新间隔
- **WHEN** 用户在下拉框中选择新的刷新间隔
- **THEN** QTimer SHALL 以新的间隔重新启动

### Requirement: 暂停与恢复刷新
OsdParsePanel SHALL 提供暂停按钮，暂停时面板内容 SHALL 保持冻结，但后台数据订阅不受影响。恢复时 SHALL 立即用最新数据刷新。

#### Scenario: 暂停刷新
- **WHEN** 用户点击"暂停"按钮
- **THEN** QTimer SHALL 停止触发，面板内容保持不变，按钮文字变为"继续"

#### Scenario: 恢复刷新
- **WHEN** 用户在暂停状态点击"继续"按钮
- **THEN** QTimer SHALL 恢复并按当前间隔重新开始，面板立即用最新数据刷新一次

### Requirement: 值变化高亮
当解析后的字段值相比上一次发生变化时，该字段的值 SHALL 以蓝色短暂高亮（约 1.2 秒），不变的值 SHALL 静默更新。

#### Scenario: 值变化
- **WHEN** 某字段的值与上次刷新时不同
- **THEN** 该字段的显示值 SHALL 以蓝色粗体样式高亮，1.2 秒后恢复默认样式

#### Scenario: 值不变
- **WHEN** 某字段的值与上次刷新时相同
- **THEN** 该字段的显示值 SHALL 保持默认样式不变
