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
