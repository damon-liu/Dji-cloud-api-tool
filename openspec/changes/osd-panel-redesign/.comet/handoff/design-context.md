# Comet Design Handoff

- Change: osd-panel-redesign
- Phase: design
- Mode: compact
- Context hash: 3c8ccef61cc4bb5800ecd958464a434e3e659d3e8ed6e4bf4b8a4e98d51fb3c7

Generated-by: comet-handoff.sh

OpenSpec remains the canonical capability spec. This handoff is a deterministic, source-traceable context pack, not an agent-authored summary.

## openspec/changes/osd-panel-redesign/proposal.md

- Source: openspec/changes/osd-panel-redesign/proposal.md
- Lines: 1-30
- SHA256: 502d80f4dfcb8ed967e06f50a689579e01e85839d8768686ae50192e5559e290

```md
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
```

## openspec/changes/osd-panel-redesign/design.md

- Source: openspec/changes/osd-panel-redesign/design.md
- Lines: 1-70
- SHA256: f6a7902cfe9f29e5c1ea4d09eb053ae2419c142fed4fcfef4f95e697272f0392

```md
# Design: OSD 面板改造 (PRD 1.9)

## 面板布局

```
┌──────────────────────────────────────────────────────────┐
│  📡 OSD 面板                             [刷新间隔: 1s▾] │
│ ─────────────────────────────────────────────────────── │
│  ┌─────────────────────┐  ┌─────────────────────┐       │
│  │   🏢 机场信息       │  │   ✈ 飞机信息        │       │
│  │   QGridLayout       │  │   QGridLayout       │       │
│  │   (label : value)   │  │   (label : value)   │       │
│  └─────────────────────┘  └─────────────────────┘       │
└──────────────────────────────────────────────────────────┘
```

- 顶层: `QVBoxLayout` — 标题栏 + 分隔线 + `mPanelsRow`（QHBoxLayout）
- `mPanelsRow` 包含 `mDockPanel`（左）和 `mAircraftPanel`（右）
- 选中 dock: 两侧都显示（dock + 子飞机），右侧面板标题改为对应飞机名称
- 选中独立 aircraft: 隐藏左侧，右侧铺满或居中

## 子面板内部

均使用 `QGroupBox` + `QGridLayout`，每行 4 列（标签 | 值 | 标签 | 值）：

```
┌─ 机场信息 ───────────────────┐
│ 经纬度  22.1°, 113.2°       │
│ 舱盖    关闭                 │
│ 飞行器  在舱内               │
│ 舱内温度 28.5℃               │
│ 环境温度  32.0℃              │
│ 风速    2.1 m/s              │
│ 降雨量  0 mm                 │
└──────────────────────────────┘
```

- 每个字段对: `[QLabel(中文名)] [QLabel(值)]` 占 2 列
- 使用 `QGridLayout::addWidget(label, row, col)` 手动排布

## OsdData.h 字段补充

### DockOsd 新增

| 字段 | JSON key | 类型 |
|------|----------|------|
| 舱内温度 | `dock_inside_temperature` | double |
| 降雨量 | `rainfall` | double |

### AircraftOsd 新增

| 字段 | JSON key | 类型 |
|------|----------|------|
| 飞行器状态 | `mode_code` | int |
| 电池温度 | `battery_temperature` | double |
| 高度(起飞点) | `height` (相对高度) | double |
| GPS搜星数 | `gps_number` | int |
| 风速 | `wind_speed`(dock层) 或从OSD解析 | double |

## 显示逻辑

```cpp
showOsd(device, aircraftOsd, dockOsd, rawJson):
  if device→type == Dock:
    mDockPanel→show();  showDockOsd(dockOsd)
    if has child aircraft: mAircraftPanel→show(); showAircraftOsd(childOsd)
    else: mAircraftPanel→hide()
  else if device→type == Aircraft && !device→isChild:
    mDockPanel→hide();  mAircraftPanel→show(); showAircraftOsd(aircraftOsd)
```
```

## openspec/changes/osd-panel-redesign/tasks.md

- Source: openspec/changes/osd-panel-redesign/tasks.md
- Lines: 1-22
- SHA256: dc442a1d083e3cde5a76376e42eb438e56c523b11252522b4180e2618e796211

```md
# Tasks: OSD 面板改造 (PRD 1.9)

### 1. OsdData.h 补充字段解析
- [ ] DockOsd 新增: dock_inside_temperature, rainfall 及解析
- [ ] AircraftOsd 新增: mode_code, battery_temperature, height, gps_number, wind_speed 及解析

### 2. OsdPanel 重写面板布局
- [ ] 删除设备信息 GroupBox 及相关 QLabel 成员
- [ ] 新增 mDockPanel / mAircraftPanel (QGroupBox + QGridLayout)
- [ ] 两个面板放入 QHBoxLayout 并排
- [ ] 新增字段 QLabel 成员变量
- [ ] 每行两字段：标签 | 值 | 标签 | 值

### 3. 重写显示逻辑
- [ ] showOsd(): dock → 左侧机场+右侧飞机；独立 aircraft → 仅右侧
- [ ] showDockOsd(): 填机场面板字段
- [ ] showAircraftOsd(): 填飞机面板字段
- [ ] clear(): 两个面板均隐藏/清空

### 4. 编译验证 + 提交
- [ ] cmake --build build_mingw 通过
- [ ] git commit
```

