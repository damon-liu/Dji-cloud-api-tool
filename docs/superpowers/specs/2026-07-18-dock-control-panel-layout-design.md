# 机场控制面板布局重设计（方案 A：卡片式横向分组）— 设计文档

日期：2026-07-18
状态：已确认（用户通过可视化 mockup 选定方案 A）

## 需求

`DockControlPanel` 现有垂直堆叠布局（设备标签 → 调试模式组 → 常用控制组 → 状态标签）不适配其所在的宽扁形折叠区域（主窗口右下角，最小高度 120px）。重新设计为横向布局。

**约束：内部控制逻辑不改**——`setDevice` / `clearDevice` / `setConnected` / `requestCommand` / `onCommandStateChanged` / `updateButtonStates` / `setStatus` 及信号槽全部保持原样，仅重写 `setupUi()` 的布局代码。头文件成员不变。

## 备选方案（浏览器 mockup 对比）

| 方案 | 说明 | 结论 |
|------|------|------|
| A. 卡片式横向分组 | 顶行信息 + 4 卡片组横排 | ✅ 用户选定 |
| B. 单行工具栏式 | 全部按钮一行、竖线分隔 | ❌ 分组感弱 |
| C. 左信息区 + 右按钮网格 | 左信息栏 + 2×4 网格 | ❌ 割裂感强 |

## 布局设计（方案 A）

```
┌────────────────────────────────────────────────────────────────────┐
│ 当前机场：机场-001（dock_001）· 在线        远程调试已开启，可执行常用控制 │  ← 顶行
│ ┌─远程调试────────┐ ┌─飞机电源──┐ ┌─机场舱盖──┐ ┌─飞机充电──┐        │
│ │ 状态：已开启    │ │           │ │           │ │           │        │  ← 卡片行
│ │ [进入] [退出]   │ │[开机][关机]│ │[打开][关闭]│ │[开启][关闭]│        │
│ └─────────────────┘ └───────────┘ └───────────┘ └───────────┘        │
└────────────────────────────────────────────────────────────────────┘
```

**结构：**

1. **顶行（QHBoxLayout）**：`mDeviceLabel`（左，加粗）+ stretch + `mStatusLabel`（右，执行状态）
2. **卡片行（QHBoxLayout，spacing 10）**：4 个 `QGroupBox`：
   - **远程调试**（stretch 3）：`mDebugModeLabel`（顶部）+ stretch + 按钮行 [`mDebugOpenBtn`「进入」 | `mDebugCloseBtn`「退出」]
   - **飞机电源**（stretch 2）：stretch + 按钮行 [`mDroneOpenBtn`「开机」 | `mDroneCloseBtn`「关机」]
   - **机场舱盖**（stretch 2）：stretch + 按钮行 [`mCoverOpenBtn`「打开」 | `mCoverCloseBtn`「关闭」]
   - **飞机充电**（stretch 2）：stretch + 按钮行 [`mChargeOpenBtn`「开启」 | `mChargeCloseBtn`「关闭」]

**细节：**

- 调试按钮文本由「进入远程调试/退出远程调试」缩短为「进入/退出」（卡片标题已含语义）；确认弹窗文案来自 `DockCommandBuilder::displayName`，不受影响
- 三个功能卡片用局部 lambda `makeCard(title, openText, closeText, openBtn&, closeBtn&)` 创建，消除重复
- 按钮统一 `setCursor(PointingHandCursor)`、`setMinimumHeight(30)`（沿用现有循环）
- `mStatusLabel` 置于顶行右侧，改 `setWordWrap(false)`（单行显示，横向空间充足）
- GroupBox 样式沿用 MainWindow 全局样式表，无需自定义
- 移除原 `mainLayout->addStretch()`（面板高度由分割器控制，卡片行填满剩余空间）

## 改动范围

仅 `src/ui/DockControlPanel.cpp` 的 `setupUi()` 函数。头文件、其余函数、信号槽连接全部不动。

## 测试

1. `cmake --build build_mingw` 编译通过
2. 手动验证：展开「机场控制」面板 → 顶行信息 + 4 卡片横排；120px 高度下无挤压；按钮禁用/启用逻辑、状态文字更新与重设计前一致
