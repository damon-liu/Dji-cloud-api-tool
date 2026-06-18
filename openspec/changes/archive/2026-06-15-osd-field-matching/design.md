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
