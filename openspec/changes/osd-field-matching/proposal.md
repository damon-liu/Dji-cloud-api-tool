# Proposal: OSD 字段匹配

## 问题背景

`config/dock-osd.md` 包含来自 DJI 上云 API 官方文档的 Dock OSD 属性定义（150行 Markdown 表格），涵盖约 80+ 个属性字段。`config/topic_mappings.json` 是 OSD JSON 解析面板的中文映射配置文件。

当前 `topic_mappings.json` 中存在大量缺失的 Dock 字段（约 50+ 个），例如：
- `air_conditioner.air_conditioner_state` — 机场空调状态（PRD 点名）
- `home_position_is_valid` — home 点有效性
- `rtcm_info.*` — RTK 标定源全部子字段
- `dongle_infos.*` — 4G Dongle 信息全部子字段
- `wireless_link.*` — 图传链路全部子字段
- 以及其他 40+ 个字段

同时，`topic_mappings.json` 中已有的飞行器专属字段（如 `horizontal_speed`、`attitude_head`、`gear` 等）需要保留。

## 目标

编写一个 Python 自动化脚本，解析 `dock-osd.md` 表格，与 `topic_mappings.json` 对比，自动补充缺失字段。

## 范围

### 涉及
- 新建 `scripts/sync_topic_mappings.py` — 字段同步脚本
- 更新 `config/topic_mappings.json` — 补充缺失的 Dock OSD 字段
- 输出报告：新增了哪些字段、未匹配的字段保留情况

### 不涉及
- 不修改 C++ 代码（`TopicMapping.h` 已支持任意字段加载）
- 不处理 dock-osd.md 以外的 topic（如飞行器 events topic）
- 不改变 `topic_mappings.json` 的数据结构
- 不做 DJI 官网 HTML 实时抓取（那是后续迭代）

## 验收标准

1. 脚本能正确解析 `dock-osd.md` 中 `»` 标记的嵌套结构
2. 脚本能提取 Column（字段 key）、Name（中文名）、constraint（枚举值/单位）
3. 执行后 `topic_mappings.json` 包含所有 dock-osd.md 中的字段
4. `air_conditioner.air_conditioner_state` 正确匹配并添加
5. 已有字段（含飞行器独有字段）原样保留不丢失
6. 脚本可重复执行（幂等）
