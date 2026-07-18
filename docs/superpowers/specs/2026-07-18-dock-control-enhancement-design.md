# 机场控制面板增强设计

**日期**: 2026-07-18
**版本**: v1.0.2

---

## 概述

增强机场控制面板（DockControlPanel），改进在线状态显示、新增飞行控制功能、支持机场切换。

## 设计内容

### 1. 在线状态样式改造

**文件**: `src/ui/DockControlPanel.cpp`

将 `setDevice()` 中的纯文本在线/离线显示改为与设备树一致的 emoji + 颜色样式：

- 在线：`🟢 在线`，样式 `color: #1e8e3e; font-weight: bold;`
- 离线：`🔴 离线`，样式 `color: #d93025; font-weight: bold;`

### 2. 机场设备选择器

**文件**: `src/ui/DockControlPanel.h`, `src/ui/DockControlPanel.cpp`

将顶行 `mDeviceLabel`（QLabel）替换为可搜索下拉框（editable QComboBox）：

| 特性 | 说明 |
|------|------|
| 下拉列表 | 只显示**在线**机场设备 |
| 默认选中 | 设备树当前选中的机场（或关联飞机对应的父机场） |
| 搜索过滤 | QComboBox editable，输入文字自动过滤 |
| 状态指示 | 下拉项用 🟢 前缀 |
| 切换联动 | 切换机场后清空调试模式状态，重新绑定 |

**新增方法**：
- `setAvailableDocks(const QVector<DeviceInfo>& docks)` — MainWindow 传入在线机场列表
- `currentGatewaySn() const` — 获取当前选中机场 SN

**信号**：无新增（内部切换直接在面板内完成，commandRequested 使用新选中的 gatewaySn）

### 3. 飞行控制卡片

**文件**: `src/ui/DockControlPanel.h`, `src/ui/DockControlPanel.cpp`

在卡片行末尾新增 **"飞行控制"** QGroupBox，包含两个按钮：

- **一键起飞** (`mTakeoffBtn`)
- **一键返航** (`mReturnBtn`)

启用条件：`mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending`
（不依赖调试模式，与其他卡片不同）

### 4. 一键起飞弹窗

**文件**: `src/ui/TakeoffDialog.h` (新建)

独立的 QDialog，标题 "一键起飞 - 飞行参数设置"：

| 字段 | 控件 | 默认值 | 范围 |
|------|------|--------|------|
| 目标纬度 | QDoubleSpinBox | 机场当前纬度 | -90.0 ~ 90.0，6 位小数 |
| 目标经度 | QDoubleSpinBox | 机场当前经度 | -180.0 ~ 180.0，6 位小数 |
| 飞行高度 | QDoubleSpinBox | 100.0m | 20.0 ~ 500.0m |
| 飞行速度 | QDoubleSpinBox | 0（使用默认）| 0 ~ 15.0 m/s，0 表示不设置 |
| ☑ 环境确认 | QCheckBox | 未勾选 | "我确认当前环境适合起飞，已完成飞行前检查" |

- 确定按钮在环境确认未勾选时 disabled
- 机场当前位置由 `DockOds` 的 `latitude`/`longitude` 获取
- 点击确定后返回 `TakeoffParams{latitude, longitude, height, speed}` 结构体

### 5. 一键返航

确认对话框（QMessageBox::question）："确定要让飞机返航吗？" → 确认后发送 `return_home` 指令。

### 6. 后端改动

**文件**: `src/core/DockCommand.h`, `src/core/DockCommand.cpp`

**DockCommandType 枚举**新增：
- `Takeoff` → method: `takeoff_to_point`
- `Return` → method: `return_home`

**DockCommandBuilder::build()** 签名改动：
```cpp
static DockCommandRequest build(const QString& gatewaySn, DockCommandType type,
                                const QJsonObject& data = {});
```
支持传入 `data` 参数（起飞时需要 lat/lon/height/speed）。

**DockCommandBuilder::requiresDebugMode()** 更新：
- `Takeoff` 和 `Return` 返回 `false`

### 7. MainWindow 联动

**文件**: `src/ui/MainWindow.cpp`

- 在 `connectSignals()` 中注册设备变化时调用 `setAvailableDocks()` 更新机场列表
- 在 `onDeviceSelected()` 中传入 dockOds 位置数据用于起飞弹窗默认值

## 数据流

```
MainWindow                      DockControlPanel
─────────                       ────────────────
setAvailableDocks(docks)  ──→   更新 QComboBox 选项
onDeviceSelected(sn)      ──→   切换选中机场（如有变化）
                                 ↓
用户点击 "一键起飞"      ──→   打开 TakeoffDialog
                                 ↓ 填入默认 lat/lon
                                 ↓ 用户确认
                                 ↓
                        emit commandRequested(gatewaySn, Takeoff, data)
                                 ↓
                        MainWindow → DeviceManager::executeDockCommand()
                                 ↓
                        MQTT publish takeoff_to_point
```

## 涉及文件

| 文件 | 改动类型 |
|------|----------|
| `src/ui/DockControlPanel.h` | 修改：新增成员、方法 |
| `src/ui/DockControlPanel.cpp` | 修改：setupUi、按钮逻辑、选择器 |
| `src/ui/TakeoffDialog.h` | **新建**：起飞参数弹窗 |
| `src/core/DockCommand.h` | 修改：枚举 + build 签名 |
| `src/core/DockCommand.cpp` | 修改：method/displayName/build 实现 |
| `src/ui/MainWindow.cpp` | 修改：联动机场列表和位置数据 |

## 测试要点

- [ ] 在线机场 🟢 绿色、离线机场 🔴 红色
- [ ] QComboBox 过滤只显示在线机场
- [ ] 输入文字过滤机场名称
- [ ] 飞行控制按钮不依赖调试模式
- [ ] 一键起飞弹窗默认值为机场当前位置
- [ ] 环境确认未勾选时确定按钮 disabled
- [ ] 飞行速度为 0 时不发送 speed 字段
- [ ] 一键返航确认对话框正常弹出
- [ ] 切换机场后调试模式状态重置
