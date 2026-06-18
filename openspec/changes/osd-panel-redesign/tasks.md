# Tasks: OSD 面板改造 (PRD 1.9)

### 1. OsdData.h 补充字段解析
- [x] DockOsd 新增: dock_inside_temperature, rainfall 及解析
- [x] AircraftOsd 新增: mode_code, battery_temperature, height, gps_number, wind_speed 及解析

### 2. OsdPanel 重写面板布局
- [x] 删除设备信息 GroupBox 及相关 QLabel 成员
- [x] 新增 mDockPanel / mAircraftPanel (QGroupBox + QGridLayout)
- [x] 两个面板放入 QHBoxLayout 并排
- [x] 新增字段 QLabel 成员变量
- [x] 每行两字段：标签 | 值 | 标签 | 值

### 3. 重写显示逻辑
- [x] showOsd(): dock → 左侧机场+右侧飞机；独立 aircraft → 仅右侧
- [x] showDockOsd(): 填机场面板字段
- [x] showAircraftOsd(): 填飞机面板字段
- [x] clear(): 两个面板均隐藏/清空

### 4. 编译验证 + 提交
- [x] cmake --build build_mingw 通过
- [x] git commit
