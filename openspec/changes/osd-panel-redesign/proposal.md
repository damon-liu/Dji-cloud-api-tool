# Proposal: OSD 面板改造 (PRD 1.9)

## 问题背景

当前 OsdPanel 包含三块区域：
- "设备信息"：名称/SN/类型/状态/更新时间
- "机场数据"：经纬度/舱盖/推杆/风速/备降点
- "飞行数据"：飞机专属遥测字段

三个区域使用 QFormLayout 单列纵向排列，信息密度低。"设备信息"与设备树重复无意义。

## 目标

1. 删除"设备信息"区域（名称/SN/类型等已由设备列表承载）
2. 机场信息面板（dock OSD）移至原设备信息位置，展示核心机场遥测字段，两列布局
3. 原机场数据位置替换为飞机信息面板（aircraft OSD），两列布局
4. 两个子面板在同一行并排显示

## 范围

| 涉及 | 改动 |
|------|------|
| `OsdPanel.h/cpp` | 重写 setupUi + showOsd/showDockOsd/showAircraftOsd |
| `OsdData.h` | DockOsd/AircraftOsd 补充缺失字段解析 |

## 非目标

- 不修改 MQTT 数据流/路由
- 不改变定时刷新机制
- 不影响 JSON 解析/Topic 列表面板
