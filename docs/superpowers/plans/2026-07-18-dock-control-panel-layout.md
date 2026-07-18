# DockControlPanel 布局重设计实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将机场控制面板从垂直堆叠布局改为「顶行信息 + 4 卡片组横排」布局（方案 A），适配宽扁形折叠区域。

**Architecture:** 仅重写 `DockControlPanel::setupUi()`：顶行 QHBoxLayout（设备标签 + stretch + 状态标签），卡片行 QHBoxLayout 放 4 个 QGroupBox；三个功能卡片用局部 lambda 创建。所有成员指针、信号槽、状态机逻辑不变。

**Tech Stack:** Qt 6 Widgets

**Spec:** `docs/superpowers/specs/2026-07-18-dock-control-panel-layout-design.md`

**全局注意：**
- 编译 `cmake --build build_mingw`，**禁止 `--clean-first`**；链接 Permission denied 时先关闭运行中的 DjiCloudApi.exe
- 工作区有未提交的无关改动（`src/ui/ConfigDialog.cpp`）——commit 显式指定文件，严禁 `git add -A`

---

### Task 1: 重写 setupUi() 布局

**Files:**
- Modify: `src/ui/DockControlPanel.cpp:17-95`（仅 `setupUi()` 函数）

- [ ] **Step 1: 替换 setupUi() 实现**

将 `src/ui/DockControlPanel.cpp` 中 `setupUi()` 全函数（现第 17-95 行）替换为：

```cpp
void DockControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ——— 顶行：设备信息（左） + 执行状态（右） ———
    auto* topRow = new QHBoxLayout;
    mDeviceLabel = new QLabel(QString::fromUtf8("请选择机场设备"), this);
    mDeviceLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #333;"));
    topRow->addWidget(mDeviceLabel);
    topRow->addStretch();
    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(false);
    setStatus(QString::fromUtf8("连接机场后可使用快捷控制"));
    topRow->addWidget(mStatusLabel);
    mainLayout->addLayout(topRow);

    // ——— 卡片行：远程调试 | 飞机电源 | 机场舱盖 | 飞机充电 ———
    auto* cardRow = new QHBoxLayout;
    cardRow->setSpacing(10);

    // 远程调试卡片（含状态标签）
    auto* debugGroup = new QGroupBox(QString::fromUtf8("远程调试"), this);
    auto* debugLayout = new QVBoxLayout(debugGroup);
    mDebugModeLabel = new QLabel(QString::fromUtf8("状态：未知"), debugGroup);
    debugLayout->addWidget(mDebugModeLabel);
    debugLayout->addStretch();
    auto* debugBtnRow = new QHBoxLayout;
    mDebugOpenBtn  = new QPushButton(QString::fromUtf8("进入"), debugGroup);
    mDebugCloseBtn = new QPushButton(QString::fromUtf8("退出"), debugGroup);
    debugBtnRow->addWidget(mDebugOpenBtn);
    debugBtnRow->addWidget(mDebugCloseBtn);
    debugLayout->addLayout(debugBtnRow);
    cardRow->addWidget(debugGroup, 3);

    // 功能卡片：标题 + 底部按钮行
    auto makeCard = [this](const QString& title, const QString& openText,
                           const QString& closeText,
                           QPushButton*& openBtn, QPushButton*& closeBtn) {
        auto* group = new QGroupBox(title, this);
        auto* v = new QVBoxLayout(group);
        v->addStretch();
        auto* row = new QHBoxLayout;
        openBtn  = new QPushButton(openText, group);
        closeBtn = new QPushButton(closeText, group);
        row->addWidget(openBtn);
        row->addWidget(closeBtn);
        v->addLayout(row);
        return group;
    };

    cardRow->addWidget(makeCard(QString::fromUtf8("飞机电源"), QString::fromUtf8("开机"),
                                QString::fromUtf8("关机"), mDroneOpenBtn, mDroneCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("机场舱盖"), QString::fromUtf8("打开"),
                                QString::fromUtf8("关闭"), mCoverOpenBtn, mCoverCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("飞机充电"), QString::fromUtf8("开启"),
                                QString::fromUtf8("关闭"), mChargeOpenBtn, mChargeCloseBtn), 2);

    mainLayout->addLayout(cardRow, 1);

    const QList<QPushButton*> buttons = {
        mDebugOpenBtn, mDebugCloseBtn, mDroneOpenBtn, mDroneCloseBtn,
        mCoverOpenBtn, mCoverCloseBtn, mChargeOpenBtn, mChargeCloseBtn
    };
    for (auto* button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(30);
    }

    connect(mDebugOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DebugModeOpen); });
    connect(mDebugCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DebugModeClose); });
    connect(mDroneOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DroneOpen); });
    connect(mDroneCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DroneClose); });
    connect(mCoverOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::CoverOpen); });
    connect(mCoverCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::CoverClose); });
    connect(mChargeOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeOpen); });
    connect(mChargeCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeClose); });
}
```

要点：
- 原 `mStatusLabel` 在构造时通过 `setStatus()` 初始化文案与样式（原代码是先建 label 再 `setStatus(mStatusLabel->text())`，等价简化）
- 原 `mainLayout->addStretch()` 删除，卡片行以 stretch 1 填满剩余高度
- 原 `QMessageBox` include 等其他代码不动（`requestCommand` 仍用它）

- [ ] **Step 2: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译链接成功

- [ ] **Step 3: 启动程序手动验证**

Run: `cd build_mingw && ./DjiCloudApi.exe &`（后台）

请用户验证：
1. 展开「机场控制」：顶行左侧设备信息、右侧状态文字；下方 4 卡片横排（远程调试稍宽）
2. 120px 最小高度下无挤压、无滚动
3. 按钮禁用/启用、调试模式门禁、状态文字更新与之前功能一致

- [ ] **Step 4: 提交**

```bash
git add src/ui/DockControlPanel.cpp
git commit -m "refactor: 机场控制面板改为卡片式横向布局适配折叠区域"
```
