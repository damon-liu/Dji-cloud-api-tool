#!/usr/bin/env python3
"""同步 dock-osd.md 到 topic_mappings.json，自动补充缺失字段映射。

用法:
  py scripts/sync_topic_mappings.py          # 执行同步
  py scripts/sync_topic_mappings.py --dry-run # 仅报告差异
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Windows 控制台 UTF-8 支持
if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

# ============================================================
# 常量
# ============================================================

PROJECT_ROOT = Path(__file__).resolve().parent.parent
MD_PATH = PROJECT_ROOT / "config" / "dock-osd.md"
JSON_PATH = PROJECT_ROOT / "config" / "topic_mappings.json"
TOPIC_KEY = "thing/product/{sn}/osd"

# ============================================================
# 分组规则: (key_prefix, group_id, group_label)
# 按顺序匹配，命中第一个规则后停止
# ============================================================
GROUP_RULES: List[Tuple[str, str, str]] = [
    ("air_conditioner.",           "air_conditioner", "❄️ 空调"),
    ("rtcm_info.",                 "rtcm",            "\U0001f4e1 RTK标定"),
    ("dongle_infos[].",            "dongle",          "\U0001f4f6 4G Dongle"),
    ("wireless_link.",             "wireless",        "\U0001f4f6 图传链路"),
    ("wireless_link_topo.",        "wireless",        "\U0001f4f6 图传链路"),
    ("live_capacity.",             "live",            "\U0001f3a5 直播"),
    ("live_status[].",             "live",            "\U0001f3a5 直播"),
    ("maintain_status.",           "maintain",        "\U0001f527 保养"),
    ("sub_device.",                "sub_device",      "\U0001f4f1 子设备"),
    ("media_file_detail.",         "media",           "\U0001f4c1 媒体"),
    ("drone_battery_maintenance_info.heat_state",  "battery_ext", "\U0001f50b 电池扩展"),
    ("drone_battery_maintenance_info.batteries[].", "battery_ext", "\U0001f50b 电池扩展"),
    ("position_state.is_calibration", "position_ext", "\U0001f4cd 定位扩展"),
    ("home_position_is_valid",     "position_ext",    "\U0001f4cd 定位扩展"),
    ("heading",                    "position_ext",    "\U0001f4cd 定位扩展"),
    ("self_converge_coordinate.",  "position_ext",    "\U0001f4cd 定位扩展"),
    ("drc_state",                  "position_ext",    "\U0001f4cd 定位扩展"),
]

FALLBACK_GROUP_ID = "device_ext"
FALLBACK_GROUP_LABEL = "\U0001f527 设备扩展"


# ============================================================
# 数据结构
# ============================================================

@dataclass
class OsdField:
    """从 dock-osd.md 解析出的单个字段，包含嵌套路径。"""
    key: str                # 完整路径，如 "air_conditioner.air_conditioner_state"
    zh: str                 # 中文名称
    type: str               # 数据类型: struct, array, enum_int, int, float, text, bool, date
    values: Dict[str, str]  # 枚举值映射 (仅 enum_int / bool)
    unit: str               # 单位 (提取简短形式)
    description: str        # 字段说明


# ============================================================
# Markdown 解析
# ============================================================

def _count_prefix(s: str) -> int:
    """统计字符串开头的 » 数量，返回层级深度 (0 = 顶层)。"""
    content = s.strip()
    count = 0
    for ch in content:
        if ch == '\xbb':  # »
            count += 1
        else:
            break
    return count


def _strip_prefix(s: str) -> str:
    """去除开头的 » 及空格，返回干净字段名。"""
    return re.sub(r'^[»\s]+', '', s.strip())


def _parse_constraint(constraint_raw: str) -> Tuple[Dict[str, str], str]:
    """解析 constraint 列 JSON，返回 (values_dict, unit)。

    Examples:
        '{"0":"关闭","1":"开启"}'      -> ({"0":"关闭","1":"开启"}, "")
        '{"max":100,"min":0,"unit_name":"%"}'
                                      -> ({}, "%")
        '{"unit_name":"摄氏度 / °C"}'  -> ({}, "°C")
        ''                            -> ({}, "")
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

    table_row_re = re.compile(r'^\| .+ \|$')
    separator_re = re.compile(r'^\|[\s\-:]+\|')
    header_re = re.compile(r'^\| Column')

    with open(md_file, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip("\n").rstrip("\r")

            if not table_row_re.match(line):
                continue
            if separator_re.match(line):
                continue
            if header_re.match(line):  # 跳过表头行
                continue

            # 分割单元格
            cols = [c.strip() for c in line.split("|")[1:-1]]
            if len(cols) < 5:
                continue

            key_raw = cols[0]
            name_cn = cols[1]
            type_str = cols[2]
            constraint_raw = cols[3]
            desc = cols[4] if len(cols) > 4 else ""

            # 计算深度
            depth = _count_prefix(key_raw)
            field_name = _strip_prefix(key_raw)
            is_array_type = (type_str == "array")

            # 调整深度栈：pop 直到栈长度 == depth
            while len(depth_stack) > depth:
                depth_stack.pop()

            if depth == 0:
                # 新顶层节点 —— 清栈
                depth_stack.clear()
                if is_array_type or type_str == "struct":
                    # 父节点，推入栈，不生成 OsdField
                    depth_stack.append((field_name, is_array_type))
                else:
                    # 叶子字段
                    values, unit = _parse_constraint(constraint_raw)
                    fields.append(OsdField(
                        key=field_name,
                        zh=name_cn,
                        type=type_str,
                        values=values,
                        unit=unit,
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


# ============================================================
# 字段合并
# ============================================================

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


def merge_fields(
    dock_fields: List[OsdField],
    existing_mappings: dict,
    topic_key: str = TOPIC_KEY,
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
        topic = {
            "description": "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)",
            "fields": {},
            "groups": [],
        }

    existing_fields: Dict[str, dict] = dict(topic.get("fields", {}))
    existing_groups: List[dict] = list(topic.get("groups", []))
    existing_keys: set = set(existing_fields.keys())
    existing_group_ids: set = {g["id"] for g in existing_groups}

    added_count = 0
    skipped_count = 0

    # 遍历 dock 字段，添加缺失的
    for f in dock_fields:
        if f.key in existing_keys:
            skipped_count += 1
            continue

        field_def: dict = {"zh": f.zh, "unit": f.unit}
        if f.values:
            field_def["values"] = f.values

        existing_fields[f.key] = field_def
        existing_keys.add(f.key)
        added_count += 1

    # 构建排序后的 fields
    sorted_fields = dict(sorted(existing_fields.items()))

    # 为新字段生成分组：只创建尚不存在的 group_id
    all_zones = _build_zones(dock_fields)
    for gid in all_zones:
        if gid not in existing_group_ids:
            label = FALLBACK_GROUP_LABEL
            for _, rgid, rlabel in GROUP_RULES:
                if rgid == gid:
                    label = rlabel
                    break
            existing_groups.append({"id": gid, "label": label, "keys": all_zones[gid]})

    # 确保 FALLBACK_GROUP 也列出了未匹配字段
    if FALLBACK_GROUP_ID not in existing_group_ids:
        if FALLBACK_GROUP_ID in all_zones:
            existing_groups.append({
                "id": FALLBACK_GROUP_ID,
                "label": FALLBACK_GROUP_LABEL,
                "keys": all_zones[FALLBACK_GROUP_ID],
            })

    print(f"  新增字段: {added_count}")
    print(f"  已存在(跳过): {skipped_count}")
    print(f"  总字段数: {len(sorted_fields)}")
    print(f"  总分组数: {len(existing_groups)}")

    # 构建输出结构
    result = dict(existing_mappings) if existing_mappings else {}
    result.setdefault("topics", {})
    result["topics"][topic_key] = {
        "description": topic.get("description", "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)"),
        "fields": sorted_fields,
        "groups": existing_groups,
    }
    return result


# ============================================================
# 主入口
# ============================================================

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

    # 1. 解析 Markdown
    print(f"\U0001f4d6 解析: {MD_PATH}")
    dock_fields = parse_dock_osd_md(str(MD_PATH))
    print(f"  解析到 {len(dock_fields)} 个字段")

    # 2. 加载现有 JSON
    print(f"\U0001f4c2 加载: {JSON_PATH}")
    if JSON_PATH.exists():
        with open(JSON_PATH, "r", encoding="utf-8") as f:
            existing = json.load(f)
    else:
        print("  ⚠ JSON 文件不存在，将从头构建")
        existing = {}

    # 3. 合并
    print("\U0001f504 合并中...")
    updated = merge_fields(dock_fields, existing)

    # 4. 输出
    output_json = json.dumps(updated, ensure_ascii=False, indent=2, sort_keys=True)

    if args.dry_run:
        print(f"\n\U0001f4cb --dry-run 模式，不写入文件")
        print(f"  输出 JSON 长度: {len(output_json)} 字符")
        return

    print(f"\U0001f4be 写入: {JSON_PATH}")
    # 直接写入（项目配置文件，非关键竞态场景）
    try:
        with open(JSON_PATH, "w", encoding="utf-8") as f:
            f.write(output_json)
            f.write("\n")
        print("  ✓ 写入成功")
    except Exception as e:
        print(f"  ✗ 写入失败: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
