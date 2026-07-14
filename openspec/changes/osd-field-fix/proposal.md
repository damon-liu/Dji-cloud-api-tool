# Proposal: OSD 字段修复 (PRD 1.9 字段修复)

## 问题

根据真实 DJI 机库上报 JSON 数据，OsdData.h 中多个字段类型/key 与 API 不匹配：
- `cover_state` 是 int (0/1)，代码用 QString
- `temperature` key 是 "temperature"，代码错误使用 "dock_inside_temperature"
- `position_state.gps_number` 未从嵌套对象中解析

## 根因

字段类型假设错误，未对照 DJI API 实际返回格式。

## 修复目标

修正 DockOsd 中字段类型和 JSON key 名称，使解析与实际数据一致。
