---
change: osd-field-matching
design-doc: docs/superpowers/specs/2026-06-15-osd-field-matching-design.md
base-ref: d8ff9499b63f2edfca8f7510114901eb79f79000
---

# OSD 字段匹配 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 编写 Python 脚本 `scripts/sync_topic_mappings.py`，解析 `config/dock-osd.md`（DJI Dock OSD 属性表格，约 150 行），与 `config/topic_mappings.json` 对比，自动补充缺失字段映射。

**Architecture:** 单文件 Python 脚本（仅标准库依赖），包含三个核心模块：Markdown 表格解析（`parse_dock_osd_md`）、字段合并（`merge_fields`）、CLI 入口（`main` + argparse）。通过 `»` 前缀计数构建点号分隔嵌套路径，`constraint` 列 JSON 解析提取枚举值和单位。幂等执行：重复运行无变化。

**Tech Stack:** Python 3.x（标准库：argparse, json, re, dataclasses, sys, pathlib）

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `scripts/sync_topic_mappings.py` | 新建 | 单文件脚本：解析 + 合并 + 输出 |
| `config/topic_mappings.json` | 更新（脚本写入） | 补充缺失的 Dock OSD 字段 |

所有逻辑集中在 `scripts/sync_topic_mappings.py` 一个文件中，按 dataclass → 解析函数 → 合并函数 → CLI 入口的顺序组织。

---

## 字段命名约定

| markdown 层级 | JSON key 模式 | 示例 |
|---|---|---|
| 顶层 | `field_name` | `home_position_is_valid` |
| struct 子字段 | `parent.child` | `air_conditioner.air_conditioner_state` |
| array 子元素 | `parent[].child` | `dongle_infos[].imei` |
| 嵌套 array | `parent[].nested[].child` | `dongle_infos[].esim_infos[].telecom_operator` |
| struct 内 array | `parent.nested[].child` | `live_capacity.device_list[].sn` |

深度栈维护一个列表，每项记录 `(prefix, is_array)`：
- 遇到 `»` 子行时，若父节点 type 为 `array`，子路径追加 `[]`；若父节点 type 为 `struct`，子路径用 `.` 连接。
- 遇到新的顶层行（无 `»` 前缀）时清空深度栈。

---

### Task 1: 创建 Markdown 表格解析模块

**Files:**
- Create: `scripts/sync_topic_mappings.py`

实现 `parse_dock_osd_md(path)` 函数，从 `config/dock-osd.md` 解析表格行，输出 `List[OsdField]`。

- [ ] **Step 1: 创建脚本骨架和数据结构**

写入 `scripts/sync_topic_mappings.py`：

```python
#!/usr/bin/env python3
"""同步 dock-osd.md 到 topic_mappings.json，自动补充缺失字段映射。"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class OsdField:
    """从 dock-osd.md 解析出的单个字段，包含嵌套路径。"""
    key: str                # 完整路径，如 "air_conditioner.air_conditioner_state"
    zh: str                 # 中文名称
    type: str               # 数据类型: struct, array, enum_int, int, float, text, bool, date
    values: Dict[str, str]  # 枚举值映射 (仅 enum_int / bool)
    unit: str               # 单位 (提取简短形式)
    description: str        # 字段说明


# 表头列索引（0-based）
COL_KEY = 0
COL_NAME = 1
COL_TYPE = 2
COL_CONSTRAINT = 3
COL_DESC = 4

# 匹配 table row: 以 | 开头、以 | 结尾
TABLE_ROW_RE = re.compile(r'^\| .+ \|$')
# 跳过的行：表头分隔线（如 | --- | --- |）
SEPARATOR_RE = re.compile(r'^\|[\s\-:]+\|')


def _count_prefix(s: str) -> int:
    """统计单元格内容开头的 » 数量，返回层级深度 (0 = 顶层)。"""
    content = s.strip()
    count = 0
    for ch in content:
        if ch == '»':
            count += 1
        else:
            break
    return count


def _strip_prefix(s: str) -> str:
    """去除开头的 » 及后续空格，返回干净字段名。"""
    content = s.strip()
    return re.sub(r'^[»\s]+', '', content)
```

- [ ] **Step 2: 实现 constraint 列 JSON 解析函数**

追加到 `scripts/sync_topic_mappings.py`（在 `_strip_prefix` 函数之后）：

```python
def _parse_constraint(constraint_raw: str) -> Tuple[Dict[str, str], str]:
    """解析 constraint 列 JSON，返回 (values_dict, unit)。

    Examples:
        '{"0":"关闭","1":"开启"}'      → ({"0":"关闭","1":"开启"}, "")
        '{"max":100,"min":0,"unit_name":"%"}'
                                      → ({}, "%")
        '{"unit_name":"摄氏度 / °C"}'  → ({}, "°C")
        ''                            → ({}, "")
    """
    values: Dict[str, str] = {}
    unit = ""

    raw = constraint_raw.strip()
    if not raw:
        return values, unit

    try:
        obj = json.loads(raw)
    except json.JSONDecodeError:
        print(f"  ⚠ 警告: constraint JSON 解析失败: {raw[:80]}", file=sys.stderr)
        return values, unit

    if not isinstance(obj, dict):
        return values, unit

    # 提取 unit: 取 unit_name 中 / 后的简短形式
    unit_name = obj.get("unit_name")
    if isinstance(unit_name, str) and unit_name:
        if " / " in unit_name:
            unit = unit_name.split(" / ", 1)[1]
        else:
            unit = unit_name

    # 提取枚举值: 过滤掉 meta 键 (max/min/step/unit_name/item_type/size/length/desc)
    meta_keys = {"max", "min", "step", "unit_name", "item_type", "size", "length", "desc"}
    for k, v in obj.items():
        if k not in meta_keys and isinstance(v, str):
            values[k] = v

    return values, unit
```

- [ ] **Step 3: 实现 `parse_dock_osd_md()` 主解析函数**

追加到 `scripts/sync_topic_mappings.py`：

```python
def parse_dock_osd_md(md_path: str) -> List[OsdField]:
    """解析 dock-osd.md 表格，返回所有叶子字段列表。

    规则:
    - 通过 » 计数确定嵌套深度
    - struct/array 父节点不生成字段（没有实际数据值）
    - 叶子字段生成完整嵌套路径
    - 跳过表头行和分隔行
    """
    md_file = Path(md_path)
    if not md_file.exists():
        print(f"错误: 文件不存在: {md_path}", file=sys.stderr)
        sys.exit(1)

    fields: List[OsdField] = []
    # 深度栈: [(prefix, is_array)]
    depth_stack: List[Tuple[str, bool]] = []

    with open(md_file, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip("\n").rstrip("\r")

            if not TABLE_ROW_RE.match(line):
                continue
            if SEPARATOR_RE.match(line):
                continue

            # 分割单元格
            cols = [c.strip() for c in line.split("|")[1:-1]]
            if len(cols) < 5:
                continue

            key_raw = cols[COL_KEY]
            name_cn = cols[COL_NAME]
            type_str = cols[COL_TYPE]
            constraint_raw = cols[COL_CONSTRAINT]
            desc = cols[COL_DESC] if len(cols) > COL_DESC else ""

            # 计算深度
            depth = _count_prefix(key_raw)
            field_name = _strip_prefix(key_raw)

            # 调整深度栈：pop 直到栈长度 == depth
            while len(depth_stack) > depth:
                depth_stack.pop()

            # 对于非顶层行，需要用父 prefix 构建路径
            # 顶层 (depth=0): 深度栈为空
            is_array_type = (type_str == "array")

            if depth == 0:
                # 新顶层节点 —— 清栈
                depth_stack.clear()
                if is_array_type or type_str == "struct":
                    # 父节点，推入栈，不生成 OsdField
                    depth_stack.append((field_name, is_array_type))
                else:
                    # 叶子字段
                    fields.append(OsdField(
                        key=field_name,
                        zh=name_cn,
                        type=type_str,
                        values=_parse_constraint(constraint_raw)[0],
                        unit=_parse_constraint(constraint_raw)[1],
                        description=desc,
                    ))
            else:
                # 子字段：根据父节点类型拼接路径
                parent_prefix, parent_is_array = depth_stack[-1]
                if parent_is_array:
                    full_key = f"{parent_prefix}[].{field_name}"
                else:
                    full_key = f"{parent_prefix}.{field_name}"

                if is_array_type or type_str == "struct":
                    # 中间节点，推入栈
                    depth_stack.append((full_key, is_array_type))
                else:
                    # 叶子字段
                    values, unit = _parse_constraint(constraint_raw)
                    fields.append(OsdField(
                        key=full_key,
                        zh=name_cn,
                        type=type_str,
                        values=values,
                        unit=unit,
                        description=desc,
                    ))

    return fields
```

- [ ] **Step 4: 编写解析功能自测**

在 `scripts/sync_topic_mappings.py` 末尾追加临时 `if __name__ == "__main__"` 块：

```python
if __name__ == "__main__":
    # 临时自测：解析 dock-osd.md
    PROJECT_ROOT = Path(__file__).resolve().parent.parent
    md_path = PROJECT_ROOT / "config" / "dock-osd.md"

    fields = parse_dock_osd_md(str(md_path))
    print(f"解析到 {len(fields)} 个字段")
    print()

    # 抽查关键字段
    checks = [
        "home_position_is_valid",
        "heading",
        "air_conditioner.air_conditioner_state",
        "air_conditioner.switch_time",
        "dongle_infos[].imei",
        "dongle_infos[].esim_infos[].telecom_operator",
        "wireless_link.4g_link_state",
        "live_status[].video_id",
        "drone_battery_maintenance_info.heat_state",
        "maintain_status.maintain_status_array[].state",
        "position_state.is_calibration",
        "self_converge_coordinate.latitude",
    ]
    field_map = {f.key: f for f in fields}
    for chk in checks:
        if chk in field_map:
            f = field_map[chk]
            print(f"  ✓ {chk}: zh={f.zh}, unit={f.unit!r}, values={f.values}")
        else:
            print(f"  ✗ {chk}: 未找到!")
```

- [ ] **Step 5: 运行解析自测验证**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python scripts/sync_topic_mappings.py
```

预期输出：
- 解析到约 80+ 个字段
- 所有抽查字段显示 `✓`
- `air_conditioner.air_conditioner_state` 的 unit 为空、values 包含 9 个枚举
- `heading` 的 unit 为 `"°"`
- `wireless_link.4g_link_state` 的 values 为 `{"0":"断开","1":"连接"}`

- [ ] **Step 6: 移除临时自测代码**

在进入 Task 2 前，删除 `if __name__ == "__main__":` 下所有临时自测代码（保留该 guard 行，后续 Task 3 会重新填充）。

- [ ] **Step 7: 提交**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
git add scripts/sync_topic_mappings.py
git commit -m "feat: 添加 Markdown 表格解析模块 — parse_dock_osd_md()"
```

---

### Task 2: 创建字段合并逻辑

**Files:**
- Modify: `scripts/sync_topic_mappings.py`

实现 `merge_fields()` 函数，加载现有 `topic_mappings.json`，与解析出的 `OsdField` 列表对比，只添加缺失字段，保留独有字段，按分组规则生成 groups。

- [ ] **Step 1: 定义分组规则和辅助函数**

在 `parse_dock_osd_md()` 函数之后追加：

```python
# ============================================================
# 分组规则: (key_prefix, group_id, group_label)
# 按顺序匹配，命中第一个规则后停止
# ============================================================
GROUP_RULES: List[Tuple[str, str, str]] = [
    ("air_conditioner.",           "air_conditioner", "❄ 空调"),
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

# 未匹配到的字段归入此分组
FALLBACK_GROUP_ID = "device_ext"
FALLBACK_GROUP_LABEL = "🔧 设备扩展"


def _build_zones(fields: List[OsdField]) -> Dict[str, List[str]]:
    """按 group_id 对字段 key 分组。

    返回: {group_id: [key1, key2, ...]}
    """
    groups: Dict[str, List[str]] = {}
    for f in fields:
        group_id = FALLBACK_GROUP_ID
        for prefix, gid, _ in GROUP_RULES:
            if f.key.startswith(prefix):
                group_id = gid
                break
        groups.setdefault(group_id, []).append(f.key)
    return groups
```

- [ ] **Step 2: 实现 `merge_fields()` 函数**

追加到 `_build_zones` 函数之后：

```python
def merge_fields(
    dock_fields: List[OsdField],
    existing_mappings: dict,
    topic_key: str = "thing/product/{sn}/osd",
) -> dict:
    """合并 dock 解析字段到现有 topic_mappings。

    - 现有字段保持不变（幂等）
    - 只添加缺失字段
    - 保留独有字段（如飞行器专属字段 horizontal_speed、gear 等）
    - 自动为新字段创建分组

    返回更新后的 topic_mappings（dict 格式，适合 json.dump）。
    """
    topic = existing_mappings.get("topics", {}).get(topic_key)
    if topic is None:
        # topic_mappings 为空或缺失此 topic，从零构建
        topic = {"description": "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)", "fields": {}, "groups": []}

    existing_fields: Dict[str, dict] = topic.get("fields", {})
    existing_groups: List[dict] = topic.get("groups", [])

    # 构建现有 key 集合
    existing_keys: set = set(existing_fields.keys())

    # 构建现有 group_id 集合
    existing_group_ids: set = {g["id"] for g in existing_groups}

    added_count = 0
    skipped_count = 0

    # 遍历 dock 字段，添加缺失的
    for f in dock_fields:
        if f.key in existing_keys:
            skipped_count += 1
            continue

        # 构建字段定义
        field_def: dict = {"zh": f.zh, "unit": f.unit}
        if f.values:
            field_def["values"] = f.values

        existing_fields[f.key] = field_def
        existing_keys.add(f.key)
        added_count += 1

    # 为新字段生成分组（只添加新 group_id 的分组）
    new_field_groups = _build_zones([f for f in dock_fields if f.key not in _get_pre_merge_keys(topic)])

    # 实际上 _get_pre_merge_keys 用起来太复杂，简化：用 merge 后的 fields 去分组
    # 但只创建尚不存在的 group_id
    all_zones = _build_zones(dock_fields)
    for gid in all_zones:
        if gid not in existing_group_ids:
            # 找到对应 label
            label = FALLBACK_GROUP_LABEL
            for _, rgid, rlabel in GROUP_RULES:
                if rgid == gid:
                    label = rlabel
                    break
            existing_groups.append({"id": gid, "label": label, "keys": all_zones[gid]})

    # 确保 FALLBACK_GROUP 也列出了未匹配字段
    if FALLBACK_GROUP_ID not in existing_group_ids:
        if FALLBACK_GROUP_ID in all_zones:
            existing_groups.append({"id": FALLBACK_GROUP_ID, "label": FALLBACK_GROUP_LABEL, "keys": all_zones[FALLBACK_GROUP_ID]})

    print(f"  新增字段: {added_count}")
    print(f"  已存在(跳过): {skipped_count}")
    print(f"  总字段数: {len(existing_fields)}")
    print(f"  总分组数: {len(existing_groups)}")

    # 构建输出结构
    result = existing_mappings.copy() if existing_mappings else {}
    result.setdefault("topics", {})
    result["topics"][topic_key] = {
        "description": topic.get("description", "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)"),
        "fields": existing_fields,
        "groups": existing_groups,
    }
    return result
```

- [ ] **Step 3: 重构 `merge_fields` 解决 `_get_pre_merge_keys` 未定义问题**

上一步中引用了不存在的 `_get_pre_merge_keys`。需要修正 `merge_fields` 中新增字段计数的逻辑。将 `merge_fields` 函数中那两行替换为正确的实现：

编辑 `merge_fields` 函数体，删除引用 `_get_pre_merge_keys` 的行，改为在遍历前记录已有 key 集合：

定位到 `merge_fields` 函数中的这段代码（约在函数中间）：

```python
    # 为新字段生成分组（只添加新 group_id 的分组）
    new_field_groups = _build_zones([f for f in dock_fields if f.key not in _get_pre_merge_keys(topic)])

    # 实际上 _get_pre_merge_keys 用起来太复杂，简化：用 merge 后的 fields 去分组
    # 但只创建尚不存在的 group_id
    all_zones = _build_zones(dock_fields)
```

替换为：

```python
    # 为新字段生成分组：只创建尚不存在的 group_id
    all_zones = _build_zones(dock_fields)
```

即删除 `new_field_groups` 行及其注释，保留 `all_zones` 行。

- [ ] **Step 4: 提交**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
git add scripts/sync_topic_mappings.py
git commit -m "feat: 添加字段合并逻辑 — merge_fields()"
```

---

### Task 3: 创建主脚本入口和 CLI

**Files:**
- Modify: `scripts/sync_topic_mappings.py`

实现 `main()` 函数，串联解析 → 合并 → 输出流程，支持 `--dry-run`。

- [ ] **Step 1: 实现 `main()` 函数**

追加到 `scripts/sync_topic_mappings.py` 末尾（在 `merge_fields` 函数之后）：

```python
def main() -> None:
    """CLI 入口: 同步 dock-osd.md → topic_mappings.json"""
    parser = argparse.ArgumentParser(
        description="从 config/dock-osd.md 解析字段并同步到 config/topic_mappings.json"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="仅报告变更，不写入文件",
    )
    args = parser.parse_args()

    PROJECT_ROOT = Path(__file__).resolve().parent.parent
    md_path = PROJECT_ROOT / "config" / "dock-osd.md"
    json_path = PROJECT_ROOT / "config" / "topic_mappings.json"

    # 1. 解析 Markdown
    print(f"📖 解析: {md_path}")
    dock_fields = parse_dock_osd_md(str(md_path))
    print(f"  解析到 {len(dock_fields)} 个字段")

    # 2. 加载现有 JSON
    print(f"📂 加载: {json_path}")
    if json_path.exists():
        with open(json_path, "r", encoding="utf-8") as f:
            existing = json.load(f)
    else:
        print("  ⚠ JSON 文件不存在，将从头构建")
        existing = {}

    # 3. 合并
    print("🔄 合并中...")
    updated = merge_fields(dock_fields, existing)

    # 4. 输出
    if args.dry_run:
        print("\n📋 --dry-run 模式，不写入文件")
        print(f"  输出 JSON 长度: {len(json.dumps(updated, ensure_ascii=False, indent=2, sort_keys=True))} 字符")
        return

    print(f"💾 写入: {json_path}")
    # 安全写入：先写临时文件，再替换
    tmp_path = json_path.with_suffix(".tmp")
    try:
        with open(tmp_path, "w", encoding="utf-8") as f:
            json.dump(updated, f, ensure_ascii=False, indent=2, sort_keys=True)
            f.write("\n")
        tmp_path.replace(json_path)
        print("  ✓ 写入成功")
    except Exception as e:
        print(f"  ✗ 写入失败: {e}", file=sys.stderr)
        if tmp_path.exists():
            tmp_path.unlink()
        sys.exit(1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 运行 `--dry-run` 验证**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python scripts/sync_topic_mappings.py --dry-run
```

预期输出：
- 解析到约 80+ 个字段
- 新增字段数 > 0
- 不修改 `topic_mappings.json` 文件

- [ ] **Step 3: 提交**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
git add scripts/sync_topic_mappings.py
git commit -m "feat: 添加 CLI 入口 — main() + --dry-run"
```

---

### Task 4: 执行脚本并验证结果

**Files:**
- Modify (by script): `config/topic_mappings.json`

运行脚本，逐项验证设计文档中列出的测试点。

- [ ] **Step 1: 备份原始 JSON 并执行脚本**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
cp config/topic_mappings.json config/topic_mappings.json.bak
python scripts/sync_topic_mappings.py
```

- [ ] **Step 2: 验证解析正确性 — 字段数**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python -c "
import json
with open('config/topic_mappings.json','r',encoding='utf-8') as f:
    data = json.load(f)
fs = data['topics']['thing/product/{sn}/osd']['fields']
print(f'总字段数: {len(fs)}')
"
```

预期：字段数约 130+（原有 ~65 + 新增 ~65+）。

- [ ] **Step 3: 验证新增字段存在**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python -c "
import json
with open('config/topic_mappings.json','r',encoding='utf-8') as f:
    data = json.load(f)
fs = data['topics']['thing/product/{sn}/osd']['fields']

checks = [
    'air_conditioner.air_conditioner_state',
    'air_conditioner.switch_time',
    'rtcm_info.mount_point',
    'rtcm_info.source_type',
    'dongle_infos[].imei',
    'dongle_infos[].esim_infos[].telecom_operator',
    'wireless_link.4g_link_state',
    'wireless_link.sdr_link_state',
    'live_status[].video_id',
    'live_status[].status',
    'live_capacity.available_video_number',
    'drone_battery_maintenance_info.heat_state',
    'drone_battery_maintenance_info.batteries[].voltage',
    'maintain_status.maintain_status_array[].state',
    'position_state.is_calibration',
    'home_position_is_valid',
    'heading',
    'drc_state',
    'self_converge_coordinate.latitude',
    'sub_device.device_sn',
    'media_file_detail.remain_upload',
]

for c in checks:
    if c in fs:
        f = fs[c]
        print(f'  ✓ {c}: zh={f[\"zh\"]}, unit={f.get(\"unit\",\"\")!r}')
    else:
        print(f'  ✗ {c}: 缺失!')
"
```

预期：所有检查项显示 `✓`。

- [ ] **Step 4: 验证独有字段（飞行器专属字段）未丢失**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python -c "
import json
with open('config/topic_mappings.json','r',encoding='utf-8') as f:
    data = json.load(f)
fs = data['topics']['thing/product/{sn}/osd']['fields']

unique_fields = [
    'horizontal_speed', 'vertical_speed', 'elevation',
    'attitude_head', 'attitude_pitch', 'attitude_roll',
    'gear', 'home_distance', 'total_flight_distance', 'total_flight_time',
    'height_limit', 'wind_direction', 'track_id',
    'electric_supply_voltage', 'putter_state',
    'battery.capacity_percent', 'battery.remain_flight_time',
    'battery.return_home_power', 'battery.landing_power',
    'battery.batteries[0].voltage', 'battery.batteries[0].temperature',
    'battery.batteries[1].voltage', 'battery.batteries[1].temperature',
]
for fname in unique_fields:
    if fname in fs:
        print(f'  ✓ {fname} 保留')
    else:
        print(f'  ✗ {fname} 丢失!')
"
```

预期：所有检查项显示 `✓ ... 保留`。

- [ ] **Step 5: 验证幂等性 — 连续执行两次 diff 无变化**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python scripts/sync_topic_mappings.py
cp config/topic_mappings.json config/topic_mappings_pass2.json
python scripts/sync_topic_mappings.py
diff config/topic_mappings.json config/topic_mappings_pass2.json
rm config/topic_mappings_pass2.json
```

预期：`diff` 无输出（文件完全相同）。

- [ ] **Step 6: 验证 JSON 格式有效性**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
python -m json.tool config/topic_mappings.json > nul && echo "JSON 格式有效"
```

预期：输出 `JSON 格式有效`。

- [ ] **Step 7: 验证 C++ 项目加载不破坏**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
cmake --build build_mingw
```

预期：编译成功，无错误。

- [ ] **Step 8: 清理备份并提交**

```bash
cd D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool
rm config/topic_mappings.json.bak
git add scripts/sync_topic_mappings.py config/topic_mappings.json
git commit -m "feat: 执行 sync_topic_mappings.py 补充缺失 Dock OSD 字段映射"
```

---

## 自审清单

### 1. 设计文档覆盖检查

| 设计要点 | 对应任务 |
|----------|----------|
| `parse_dock_osd_md()` — 正则匹配表格行 | Task 1 Step 3 |
| `»` 深度栈构建嵌套路径 | Task 1 Step 3 |
| struct 子字段用 `.`，array 子字段用 `[]` | Task 1 Step 3 |
| constraint JSON 解析 (values + unit) | Task 1 Step 2 |
| `unit_name` 取 `/` 后简短形式 | Task 1 Step 2 |
| `merge_fields()` — 只添加缺失，不修改已有 | Task 2 Step 2 |
| `GROUP_RULES` 分组规则 | Task 2 Step 1 |
| 未匹配字段归入 `device_ext` | Task 2 Step 1 |
| `--dry-run` CLI 参数 | Task 3 Step 1 |
| 幂等性 — sort_keys=True | Task 2 Step 2 |
| 安全写入 (tmp + replace) | Task 3 Step 1 |
| 解析 ~80+ 字段 | Task 1 Step 5 |
| 水平速度、gear 等独有字段保留 | Task 4 Step 4 |
### 2. 占位符扫描

无 TBD、TODO、待实现标记。所有步骤均有具体代码或命令。

### 3. 类型一致性

- `OsdField` dataclass 在 Task 1 Step 1 定义，Task 2/3 复用，字段名一致
- `parse_dock_osd_md()` 返回 `List[OsdField]`，`merge_fields()` 接收此类型
- `GROUP_RULES` 在 Task 2 Step 1 定义，`_build_zones` 和 `merge_fields` 均使用 `(prefix, gid, label)` 元组
- `FALLBACK_GROUP_ID` / `FALLBACK_GROUP_LABEL` 全局常量，定义和使用一致
