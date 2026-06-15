---
comet_change: osd-field-matching
role: technical-design
canonical_spec: openspec
---

# OSD 字段匹配 — 技术设计

## 概述

编写 Python 脚本 `scripts/sync_topic_mappings.py`，解析 `config/dock-osd.md`（DJI 上云 API Dock 属性表格），与 `config/topic_mappings.json` 对比，自动补充缺失字段映射。

## 架构

```
scripts/sync_topic_mappings.py          # 单文件，Python 标准库
├── Row                               # dataclass: 解析后的单行
├── OsdField                          # dataclass: 带嵌套路径的字段
├── parse_dock_osd_md(path)           # Markdown 表格 → List[OsdField]
│     · 正则：^\| .+ \|$              #   匹配表格行
│     · 跳过：表头行、分隔行          #
│     · » 计数 → 深度栈               #   构建嵌套路径
│     · constraint 列 → JSON 解析     #   提取 values + unit
│
├── merge_fields(dock_fields,         # 对比合并
│                existing_mappings)    #
│     · 构建现有字段 key set          #
│     · 遍历 dock 字段                #
│     ·   已在 set → 跳过             #
│     ·   不在 set → 添加到 fields    #
│     · 保留独有字段不动              #
│     · 按分组规则生成 groups         #
│
└── main()                            # CLI 入口
      · argparse: --dry-run           #
      · 默认模式: 直接写入            #
```

## 数据流

```
config/dock-osd.md
    │ parse_dock_osd_md()
    ▼
OsdField[] (约 80+ 个)
    │ merge_fields()
    ├── config/topic_mappings.json ──► 现有字段 set
    ▼
updated_topic_mappings
    │ json.dump()
    ▼
config/topic_mappings.json (覆盖写入)
```

## 关键设计决策

### 1. 嵌套字段命名

通过 `»` 前缀计数确定深度，构建点号分隔路径：

```
dock-osd.md                     →  JSON key
──────────────────────────────────────────────────
air_conditioner                 →  air_conditioner (struct, 不单独映射)
»air_conditioner_state          →  air_conditioner.air_conditioner_state
»switch_time                    →  air_conditioner.switch_time

dongle_infos                    →  dongle_infos (array, 不单独映射)
»imei                           →  dongle_infos[].imei
»dongle_type                    →  dongle_infos[].dongle_type
»esim_infos                     →  dongle_infos[].esim_infos
»»telecom_operator              →  dongle_infos[].esim_infos[].telecom_operator
```

规则：父节点为 array 时子节点用 `[]`，父节点为 struct 时子节点用 `.`。

### 2. constraint JSON 解析

```python
# 输入: '{"0":"关闭","1":"开启"}'
# 输出: values={"0":"关闭","1":"开启"}, unit=""

# 输入: '{"max":100,"min":0,"unit_name":"%"}'
# 输出: values={}, unit="%"

# 输入: '' (空)
# 输出: values={}, unit=""
```

`unit_name` 提取规则：取 `/` 后的简短形式。例如 `"摄氏度 / °C"` → `"°C"`，`"米 / m"` → `"m"`。无 `/` 时取原值。

### 3. 幂等性

- 脚本每次执行前先加载现有 JSON
- 对比时只添加缺失字段，不修改已有字段
- JSON 输出使用 `sort_keys=True` 保证稳定性

### 4. 分组规则

按语义将新字段归入预设分组。脚本内置一个 `GROUP_RULES` 字典，将 key 前缀映射到分组定义：

```python
GROUP_RULES = [
    ("air_conditioner.",           "air_conditioner", "❄️ 空调"),
    ("rtcm_info.",                 "rtcm",            "📡 RTK标定"),
    ("dongle_infos[].",            "dongle",          "📶 4G Dongle"),
    ("wireless_link.",             "wireless",        "📶 图传链路"),
    ("wireless_link_topo.",        "wireless",        "📶 图传链路"),
    ("live_capacity.",             "live",            "🎥 直播"),
    ("live_status[].",             "live",            "🎥 直播"),
    ("maintain_status.",           "maintain",        "🔧 保养"),
    ("sub_device.",                "sub_device",      "📱 子设备"),
    ("media_file_detail.",         "media",           "📁 媒体"),
    ("drone_battery_maintenance_info.heat_state",  "battery_ext", "🔋 电池扩展"),
    ("drone_battery_maintenance_info.batteries[].", "battery_ext", "🔋 电池扩展"),
    ("position_state.is_calibration", "position_ext", "📍 定位扩展"),
    ("home_position_is_valid",     "position_ext",    "📍 定位扩展"),
    ("heading",                    "position_ext",    "📍 定位扩展"),
    ("self_converge_coordinate.",  "position_ext",    "📍 定位扩展"),
    ("drc_state",                  "position_ext",    "📍 定位扩展"),
]
```

未匹配到任何规则的字段归入 `device_ext` 分组。

### 5. 错误处理

- Markdown 文件不存在 → 打印错误并退出码 1
- JSON 文件不存在 → 视为空映射，从零构建
- constraint 列 JSON 解析失败 → 警告并跳过该列
- 输出 JSON 写入失败 → 打印错误并退出码 1（不覆盖原文件）

## 测试策略

| 测试点 | 方法 |
|--------|------|
| 解析正确性 | 对比脚本输出的字段数 ≈ dock-osd.md 表格行数 |
| 嵌套路径 | 抽查 `air_conditioner.air_conditioner_state` 等 |
| 幂等性 | 连续执行两次，diff 无变化 |
| JSON 有效性 | `python -m json.tool` 校验；C++ 加载测试 |
| 独有字段保留 | grep `horizontal_speed`、`gear` 确认仍在 |
| 空 constraint | 确认 unit="" 非 null |

## 文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `scripts/sync_topic_mappings.py` | 新建 | 同步脚本 |
| `config/topic_mappings.json` | 更新 | 补充缺失字段 |
