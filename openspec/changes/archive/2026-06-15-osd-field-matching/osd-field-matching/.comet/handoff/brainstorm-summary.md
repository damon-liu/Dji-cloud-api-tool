# Brainstorm Summary

- Change: osd-field-matching
- Date: 2026-06-15

## 确认的技术方案

Python 单脚本 `scripts/sync_topic_mappings.py`，3 模块架构：
- `parse_dock_osd_md()` — 正则解析 Markdown 表格，`»` 计深度构建嵌套路径
- `merge_fields()` — 对比 dock 字段与现有 JSON，只添加不覆盖
- `main()` — CLI 入口，支持 `--dry-run`

## 关键取舍与风险

- 嵌套命名：`.` 分隔，数组用 `[]`（与 DJI 文档一致）
- 单位提取：仅从 constraint JSON `unit_name` 中取
- 分组：按语义创建约 12 个新 group
- 不覆盖已有字段的人工修改值
- 风险：Markdown 格式变更 → 宽松正则 + 错误行号报告

## 测试策略

- 手动对比脚本输出报告与原文
- 验证输出 JSON 可被 `TopicMapping::load()` 加载
- 编译确认 C++ 侧无破坏
- 二次执行确认幂等

## Spec Patch

无
