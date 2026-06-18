---
comet_change: osd-panel-redesign
role: technical-design
canonical_spec: openspec
---

# OSD 面板改造 (PRD 1.9) — 技术设计

## 布局架构

```
QVBoxLayout (mMainLayout)
├── 标题栏 (titleLabel + mIntervalCombo)
├── QFrame::HLine
└── QHBoxLayout (mPanelsRow)
    ├── QGroupBox "机场信息" (mDockPanel) ─ QGridLayout 两列
    └── QGroupBox "飞机信息" (mAircraftPanel) ─ QGridLayout 两列
```

## 数据字段

### DockOsd 新增
| 字段 | JSON key | 显示 |
|------|----------|------|
| dock_inside_temp | dock_inside_temperature | ℃ |
| rainfall | rainfall | mm |

### AircraftOsd 新增
| 字段 | JSON key | 显示 |
|------|----------|------|
| mode_code | mode_code | int (映射文本) |
| battery_temp | battery_temperature | ℃ |
| height | height | m |
| gps_number | gps_number | 颗 |
| wind_speed | wind_speed | m/s |

## 显示逻辑
- dock: mDockPanel.show() + mAircraftPanel.show()(子飞机)/hide()
- independent aircraft: mDockPanel.hide() + mAircraftPanel.show()
- showDockOsd/showAircraftOsd 各自填充对应面板
