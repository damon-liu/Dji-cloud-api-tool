# 功能中心与机场控制窗口 — 设计文档

日期：2026-07-18
状态：已确认（本文档取代 2026-07-18-dock-control-panel-layout-design.md 中"面板置于底部折叠区"的方案；卡片式布局本身沿用）

## 需求（用户逐条确认）

1. 工具栏新增「🧰 功能中心」按钮（位于「⚙ 配置」与「💡 帮助」之间），弹出菜单，首项「🎮 机场控制」；后期可扩展其他功能菜单项
2. 点击「机场控制」弹出**非模态独立窗口**——控制的同时主窗口 OSD 可正常查看；窗口跟随主窗口设备选中变化；关闭仅隐藏，再次打开为同一窗口并置前
3. 移除底部「▶ 机场控制」折叠面板与切换按钮（「▶ Topic 下发」保留）
4. **结果提醒**：指令到达终态必须弹窗提醒——成功 `QMessageBox::information`，失败/超时 `QMessageBox::warning`，内容含控制名称与结果码/原因；中间态（发布中/等待响应）仅更新状态标签
5. **下发记录**：窗口内日志流文本块区域，每条指令一个完整记录块（时间、控制名称、结果、Topic、下发参数、响应参数），最新在最上，仅本次运行内存保存

## 窗口结构

```
┌─ 机场控制 ──────────────────────────────────── ✕ ─┐
│ 当前机场：机场-001（dock_001）· 在线   状态：等待机场响应 │
│ ┌─远程调试────┐ ┌─飞机电源─┐ ┌─机场舱盖─┐ ┌─飞机充电─┐ │
│ │ 状态：已开启│ │          │ │          │ │          │ │
│ │ [进入][退出]│ │[开机][关机]│ │[打开][关闭]│ │[开启][关闭]│ │
│ └─────────────┘ └──────────┘ └──────────┘ └──────────┘ │
│ ┌─ 下发记录 ────────────────────────────────────────┐ │
│ │ [14:32:05] 打开舱盖  ✅ 成功 (result=0)            │ │
│ │ Topic: thing/product/dock_001/services            │ │
│ │ 下发: { "tid": "...", "method": "cover_open", …}  │ │
│ │ 响应: { "tid": "...", "data": { "result": 0 } }   │ │
│ │ ────────────────────────────────────              │ │
│ │ [14:31:48] 进入远程调试  ❌ 超时                    │ │
│ │ …  响应: （无响应）                                 │ │
│ └───────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

默认尺寸 720×520，可调。记录区为深色控制台样式（沿用全局 QPlainTextEdit 样式，与原始 JSON 面板一致）。

## 组件设计

### 1. `DockCommandResult` 扩展（`src/core/DockCommand.h`）

新增两个字段（纯透传，不改流程）：

- `QString requestJson` — 下发报文（缩进格式化），发布时由执行器填入
- `QString replyJson` — 响应报文（缩进格式化），收到回复时填入；超时/失败无回复则为空

### 2. `DockCommandExecutor`（`src/core/DockCommandExecutor.h/.cpp`）

- 新增成员 `QString mRequestJson`、`QString mReplyJson`：`execute()` 时生成请求 JSON 并清空回复；`onMqttMessage()` 匹配后把原始回复 payload 格式化存入
- `emitState()` 把两者填入 `DockCommandResult`

### 3. `DockControlPanel`（`src/ui/DockControlPanel.h/.cpp`）

- 布局：顶行 + 卡片行（已实现的方案 A）下方新增「下发记录」`QGroupBox`，内含 `QPlainTextEdit* mHistoryEdit`（只读），卡片行不再拉伸、记录区占剩余空间（stretch 1）
- `onCommandStateChanged()` 终态时：
  - 记录块**插入顶部**：`[HH:mm:ss] 名称 结果\nTopic: …\n下发:\n…\n响应:\n…\n────\n`
  - 弹窗：成功 `information` / 失败与超时 `warning`（父窗口为面板自身）
- 其余逻辑（状态机、按钮门禁、确认弹窗）不变

### 4. `DockControlDialog`（新增 `src/ui/DockControlDialog.h`，纯头文件）

薄壳 `QDialog`：标题"机场控制"、默认 720×520、内嵌 `DockControlPanel`（`panel()` 访问器）。非模态；关闭即隐藏（QDialog 默认行为）。

### 5. `MainWindow`

- **工具栏**：「⚙ 配置」与「💡 帮助」之间新增 `QToolButton`「🧰 功能中心」，`InstantPopup` 菜单（复用 helpBtn 的样式模式，objectName 沿用 `helpBtn` 样式或新增同款），菜单项「🎮 机场控制」→ `mDockCtrlDialog->show(); raise(); activateWindow();`
- **移除**：底部 `mDockControlPanel` 加入 verticalSplitter 的代码、`mToggleDockCtrlBtn` 及其切换行（恢复「▶ Topic 下发」单按钮直接 addWidget）
- **新增成员**：`DockControlDialog* mDockCtrlDialog`（`setupLayout()` 中创建，隐藏）；`mDockControlPanel` 改为指向对话框内的面板（`mDockCtrlDialog->panel()`），原有全部信号接线（设备选中、连接状态、指令结果）不变
- CMakeLists HEADERS 增加 `src/ui/DockControlDialog.h`

## 错误处理

沿用现有机制；新增路径：响应 payload 非法 JSON 时记录区原样显示原始文本。

## 测试

1. `cmake --build build_mingw` 编译通过
2. 手动验证：
   - 工具栏出现「🧰 功能中心」（配置与帮助之间），菜单弹出、点击「机场控制」弹出非模态窗口；重复点击置前
   - 底部不再有「▶ 机场控制」，「▶ Topic 下发」正常
   - 窗口内布局：顶行 + 4 卡片 + 下发记录区
   - 主窗口切换设备，窗口信息跟随变化；断开连接按钮禁用
   - 下发指令：确认弹窗 → 状态变化 → 终态弹提醒框 → 记录区顶部插入完整记录（超时响应显示"（无响应）"）
