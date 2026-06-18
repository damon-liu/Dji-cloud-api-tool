# Brainstorm Summary
- Change: osd-panel-redesign
- Date: 2026-06-18

## 确认的技术方案
- 两个 QGroupBox 子面板（mDockPanel / mAircraftPanel）在同一 QHBoxLayout 并排
- 各子面板内部 QGridLayout 每行4列（标签|值|标签|值）
- 删除设备信息 GroupBox 及相关 QLabel
- DockOsd 加 dock_inside_temp/rainfall；AircraftOsd 加 mode_code/battery_temp/height/gps_number/wind_speed
- showOsd(): dock→两侧显示，独立aircraft→仅右面板

## 关键取舍与风险
- 缺失JSON字段key需从实际数据验证，先用合理key名
- 无

## 测试策略
- 手动验证：选中dock显示两侧面板；选中aircraft仅右侧；字段值正确

## Spec Patch
- 无
