# Comet Design Handoff

- Change: osd-field-matching
- Phase: design
- Mode: compact
- Context hash: 7bfb86fec11b8a4f92c411bbb9cb854a577fa1016ee1a3958f2d953489cb26f5

Generated-by: comet-handoff.sh

OpenSpec remains the canonical capability spec. This handoff is a deterministic, source-traceable context pack, not an agent-authored summary.

## openspec/changes/osd-field-matching/proposal.md

- Source: openspec/changes/osd-field-matching/proposal.md
- Lines: 1-41
- SHA256: 5994428330579ae0b3d485e18bb558265e208b92030cf94540b33c9d01cd8328

```md
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
```

## openspec/changes/osd-field-matching/design.md

- Source: openspec/changes/osd-field-matching/design.md
- Lines: 1-60
- SHA256: 5980869603a7c83d5b3506f043a5b2da74805fd62383444b7939c49f2707f03c

```md
# Design: OSD 字段匹配

## 方案选型

采用 **Python 脚本 + 手动执行** 模式：
- 不需要编译，直接在项目根目录运行
- Python 标准库即可完成（json, re, pathlib）
- 脚本可重复执行，幂等安全

## 架构

```
scripts/sync_topic_mappings.py
  ├── parse_dock_osd_md(path) → List[OsdField]
  │     · 正则解析 Markdown 表格行
  │     · 通过 » 计数确定嵌套深度
  │     · 提取 Column/Name/constraint
  │
  ├── load_topic_mappings(path) → dict
  │
  ├── merge_fields(dock_fields, existing_fields) → (new_fields, report)
  │     · 对比：dock 字段 vs 现有字段
  │     · 新增缺失字段
  │     · 保留独有字段
  │     · 更新 groups
  │
  └── save_topic_mappings(path, data)
```

## 数据流

```
config/dock-osd.md ──parse──▶  OsdField[]  ──merge──▶  topic_mappings.json
                                    │                      │
                  config/topic_mappings.json ──load────────┘
```

## 关键决策

### 1. 嵌套字段命名规则
- 使用 `.` 分隔：`air_conditioner.air_conditioner_state`
- 与现有 `topic_mappings.json` 中 `drone_charge_state.state` 风格一致
- 数组元素用 `[]`：如 `dongle_infos[].imei`

### 2. constraint 解析规则
- constraint 列为 JSON 字符串：`{"0":"关闭","1":"开启"}`
- 提取为 `values` 映射
- 提取 `unit_name` 为 `unit` 字段

### 3. 幂等性保证
- 脚本执行后，再次执行不会产生重复字段
- 已存在的字段保持原值不变（不覆盖人工修改）

### 4. 新增字段的 group 处理
- 自动归入 `other_dock` 分组
- 用户可以后续手动调整分组

### 5. 字段名匹配规则
- 精确匹配：`air_conditioner_state`（dock-osd.md sub-field） vs `air_conditioner.air_conditioner_state`（JSON key）
- 已存在的 `air_conditioner_mode` 不会被覆盖（因为 key 不同）
```

## openspec/changes/osd-field-matching/tasks.md

- Source: openspec/changes/osd-field-matching/tasks.md
- Lines: 1-31
- SHA256: 15b9b75e9a2fde48e4fe7672539894aaca863ed0c6c5d30798ec11333494072e

```md
# Tasks: OSD 字段匹配

## 任务列表

- [ ] **Task 1: 创建 Markdown 表格解析模块**
  - 编写 `parse_dock_osd_md()` 函数
  - 正确解析 `| Column | Name | Type | constraint | ...` 表头
  - 通过 `»` 前缀计数确定嵌套层级
  - 生成带嵌套路径的字段列表（如 `air_conditioner.air_conditioner_state`）
  - 处理 constraint 列的 JSON 提取（枚举值和单位）

- [ ] **Task 2: 创建字段合并逻辑**
  - 编写 `merge_fields()` 函数
  - 加载现有 `topic_mappings.json`
  - 对比找出缺失字段
  - 保留独有字段（飞行器专属字段）
  - 生成合理的 `values`（枚举）和 `unit`（单位）
  - 自动创建 `other_dock` group 收纳新字段

- [ ] **Task 3: 创建主脚本入口**
  - 编写 `main()` 函数串联解析+合并+输出
  - 支持命令行参数：`--dry-run` 仅报告不写入
  - 输出变更报告：新增字段列表、保留字段说明
  - 幂等保证：重复执行不重复添加

- [ ] **Task 4: 执行脚本并验证结果**
  - 运行 `python scripts/sync_topic_mappings.py`
  - 验证 `air_conditioner.air_conditioner_state` 已添加
  - 验证现有飞行器字段未丢失
  - 验证 JSON 格式有效可被 C++ 加载
  - 编译项目确认无破坏
```

## openspec/changes/osd-field-matching/specs/osd-field-matching/spec.md

- Source: openspec/changes/osd-field-matching/specs/osd-field-matching/spec.md
- Lines: 1-58
- SHA256: 99f0436d72f57149bf0a1041ae2461e9df075b73d4b54b50715ed1551d3b3872

```md
# OSD Field Matching

## Overview

自动解析 `config/dock-osd.md` 中 DJI 官方属性表格，对比 `config/topic_mappings.json`，自动补充缺失的 OSD 字段映射。

## ADDED Requirements

### Requirement: Markdown Table Parser
**ID:** OFM-001
**Priority:** Must
**Description:** 脚本 SHALL 正确解析 `config/dock-osd.md` 中的 Markdown 表格，通过 `»` 前缀识别嵌套层级，提取 Column、Name、constraint 三列。

#### Scenario: Parse flat field
- **Given** dock-osd.md 包含行 `| home_position_is_valid | home点有效性 | enum_int | {"0":"..."} |...`
- **When** 脚本解析该行
- **Then** 生成字段 `home_position_is_valid`，中文名 "home点有效性"，values 包含 4 个枚举项

#### Scenario: Parse nested field
- **Given** dock-osd.md 包含行 `| air_conditioner | 机场空调工作状态信息 | struct | |`
- **And** 下一行 `| »air_conditioner_state | 机场空调状态 | enum_int | {"0":"空闲模式"...} |`
- **When** 脚本解析该区域
- **Then** 生成字段 `air_conditioner.air_conditioner_state`，中文名 "机场空调状态"，values 包含 9 个枚举项

### Requirement: Field Merge Logic
**ID:** OFM-002
**Priority:** Must
**Description:** 脚本 SHALL 对比解析出的 Dock 字段与 `topic_mappings.json` 现有字段，仅添加缺失字段，保留所有现有字段。

#### Scenario: Add missing field
- **Given** dock-osd.md 有 `air_conditioner.air_conditioner_state`，topic_mappings.json 中没有
- **When** 执行合并
- **Then** `air_conditioner.air_conditioner_state` 添加到 topic_mappings.json 的 fields 中

#### Scenario: Preserve existing field
- **Given** topic_mappings.json 有 `horizontal_speed`（飞行器字段），dock-osd.md 中没有
- **When** 执行合并
- **Then** `horizontal_speed` 保留不变

#### Scenario: Idempotent execution
- **Given** 脚本已执行过一次，所有缺失字段已添加
- **When** 再次执行脚本
- **Then** topic_mappings.json 不产生任何变化

### Requirement: Unit and Enum Extraction
**ID:** OFM-003
**Priority:** Must
**Description:** 脚本 SHALL 从 constraint 列的 JSON 中提取 `unit_name` 作为 unit，提取枚举映射作为 values。

#### Scenario: Extract unit from constraint
- **Given** constraint 为 `{"unit_name":"米 / m"}`
- **When** 解析该字段
- **Then** unit 设为 "m"

#### Scenario: Extract enum values
- **Given** constraint 为 `{"0":"关闭","1":"开启"}`
- **When** 解析该字段
- **Then** values 为 `{"0": "关闭", "1": "开启"}`
```

