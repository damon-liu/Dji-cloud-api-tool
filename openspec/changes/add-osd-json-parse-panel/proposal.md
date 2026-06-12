## Why

当前系统只能展示原始 JSON 和设备信息中硬编码的少量 OSD 字段，用户无法直观阅读 DJI 上报的全部遥测数据。DJI OSD JSON 包含 30+ 个字段（如 `electric_supply_voltage`、`environment_temperature` 等），需要一套 key→中文翻译 + 分组展示机制，让用户无需查阅 API 文档即可快速理解设备状态。

## What Changes

- 新增 `OsdParsePanel` 面板，放置在设备信息下方，定时从右侧原始 JSON 获取最新数据并翻译展示
- 新增 `topic_mappings.json` 外部配置文件，按 topic 模式维护 JSON key → 中文名称、单位、枚举值翻译
- 新增 `TopicMapping` 类，负责加载映射配置并提供查询接口
- 面板支持分组表格展示、可配置刷新间隔、暂停/恢复、值变化高亮
- v1.0 先实现 `thing/product/{sn}/osd` topic 的映射（覆盖机场 Dock + 飞机 Aircraft 两类设备）

## Capabilities

### New Capabilities
- `osd-json-parse`: 从原始 JSON 定时提取最新数据，按 key→中文映射翻译后以分组表格形式展示，支持刷新间隔配置和暂停/恢复

### Modified Capabilities
<!-- 无现有 spec 被修改，本功能是独立新增面板 -->

## Impact

- 新增文件: `src/ui/OsdParsePanel.h`, `src/core/TopicMapping.h`, `config/topic_mappings.json`
- 修改文件: `src/ui/MainWindow.h/cpp`（布局调整：OSD 面板下方新增解析面板）、`CMakeLists.txt`（添加新源文件）
- 无 API 变更、无向后兼容问题
