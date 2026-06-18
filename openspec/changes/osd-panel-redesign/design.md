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
