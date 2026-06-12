## Context

当前系统通过 `OsdPanel` 展示设备信息，但仅硬编码解析了少量字段（电量、电压、速度等）。DJI Cloud API 的 OSD 消息包含 30+ 字段，大部分以原始 JSON 形式存在但不可读。PRD 1.4 要求在设备信息下方新增一个 JSON 解析面板，将 key 翻译为中文并分组展示。

现有架构：Qt 6 + C++17，单线程事件驱动。`DeviceManager` 已缓存 `mRawJsonCache[sn]`（最新 JSON）和 `mJsonHistory[sn][topic]`（历史列表），`RawJsonPanel` 已实现暂停/恢复机制。新面板可复用这些基础设施。

## Goals / Non-Goals

**Goals:**
- 提供 `topic_mappings.json` 外部配置文件，定义 JSON key → 中文名称、单位、枚举值翻译
- 实现 `OsdParsePanel` 面板，以分组表格形式展示解析后的数据
- 支持可配置刷新间隔（默认 2s，匹配 OSD 0.5Hz 上报频率）
- 支持暂停/恢复刷新（与 RawJsonPanel 暂停机制独立）
- 值变化时短暂高亮（复用 OsdPanel 的 setFieldValue 模式）
- v1.0 覆盖 `thing/product/{sn}/osd` topic（机场 + 飞机）

**Non-Goals:**
- state/events topic 的映射（后续版本）
- 用户通过 UI 编辑映射配置（当前手动编辑 JSON 文件）
- 映射热重载（需重启应用生效）

## Decisions

### 1. 映射配置：外部 JSON 文件 vs 硬编码

**选择**: 外部 `config/topic_mappings.json`

**理由**: PRD 明确说"需要维护每个 topic 原始 JSON 中 key 对应的中文"，DJI 可能会新增字段。外部文件让用户无需重新编译即可修改映射。类似 `config.json` 的管理方式，与项目现有模式一致。

### 2. 面板位置：OSD 下方 vs JSON 下方 vs 独立

**选择**: 在右侧水平分割器的左半区，OsdPanel 下方垂直堆叠

**理由**: 改动最小，PRD 原文"设备信息下面新增"指向明确。OSD 面板展示设备摘要，下方的解析面板展示详细遥测，逻辑上是从概括到详细的递进关系。

### 3. 展示格式：分组表格 vs 扁平列表 vs JSON 树

**选择**: 分组表格（两列排列，按类别分区）

**理由**: 分组降低信息密度，用户按需查看。两列布局充分利用面板宽度。分组定义内置在 `topic_mappings.json` 中，不同 topic 可自定义分组。

### 4. 嵌套 Key 表示：点号分隔

**选择**: 用 `.` 分隔嵌套路径（如 `battery.capacity_percent`），数组索引用 `[n]`（如 `batteries[0].voltage`）

**理由**: 简洁直观，与 JSON Path 风格一致。`TopicMapping` 类据此展平嵌套 JSON 对象进行匹配。

### 5. 枚举值翻译

**选择**: 在 `topic_mappings.json` 的字段定义中支持 `values` 映射表

**理由**: DJI 大量字段使用枚举（如 `mode_code`: 0=待机, 4=自动起飞…），值翻译与 key 翻译同样重要。放在同一配置文件中保持一致性。

## Risks / Trade-offs

- **文件缺失风险**: 若 `topic_mappings.json` 丢失，面板降级显示原始英文 key → 首次启动时自动生成默认映射文件（包含 osd topic 的完整映射）
- **未映射字段**: DJI 新增字段在映射表中不存在 → 在面板底部灰色显示未映射字段（原始 key + 值），提醒用户更新映射文件
- **性能**: 每条 JSON 都需要遍历映射表和递归展平 → OSD 上报频率仅 0.5Hz，JSON 体积小（<2KB），QTimer 可在主线程安全运行，无性能风险
- **数值精度**: JSON 中电压单位是 mV，映射文件指定 `unit: "V"`，面板需做单位转换 → 当前版本不做自动转换，unit 仅作显示后缀；值直接取自 JSON 原文

## Open Questions

- 刷新间隔的可用选项：建议 [1s, 2s, 5s, 10s]，默认 2s（与 OSD 0.5Hz 上报频率匹配）
- 是否需要导出翻译后的数据？→ 不在 v1.0 范围内
