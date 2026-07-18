# 功能中心与机场控制窗口实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 工具栏新增「🧰 功能中心」菜单入口，机场控制改为非模态独立窗口，指令终态弹窗提醒 + 完整下发记录（时间/名称/结果/Topic/下发/响应）。

**Architecture:** `DockCommandResult` 扩展 `requestJson`/`replyJson` 透传字段（执行器填入）；`DockControlPanel` 增加日志流记录区与终态弹窗；新增纯头文件 `DockControlDialog` 薄壳；`MainWindow` 移除底部折叠面板，改由工具栏菜单打开对话框，信号接线不变。

**Tech Stack:** Qt 6 Widgets、C++17、CMake + MinGW

**Spec:** `docs/superpowers/specs/2026-07-18-function-center-dock-control-window-design.md`

**全局注意：**
- 编译 `cmake --build build_mingw`，禁止 `--clean-first`；链接 Permission denied 时先结束 DjiCloudApi.exe
- 工作区有未提交的无关改动 `src/ui/ConfigDialog.cpp`（密码按钮，等用户验证）——commit 显式指定文件，严禁 `git add -A`
- 工作区已有**未提交的卡片式布局改动**（`src/ui/DockControlPanel.cpp` 的 setupUi 重写，已编译验证）——Task 1 先单独提交它

---

### Task 1: 提交已完成的卡片式布局改动

**Files:**
- 已修改待提交: `src/ui/DockControlPanel.cpp`（setupUi 卡片式布局，前一计划已完成编译验证）

- [ ] **Step 1: 提交**

```bash
git add src/ui/DockControlPanel.cpp
git commit -m "refactor: 机场控制面板改为卡片式横向布局"
```

---

### Task 2: DockCommandResult 扩展与执行器透传

**Files:**
- Modify: `src/core/DockCommand.h:44-52`（DockCommandResult 结构体）
- Modify: `src/core/DockCommandExecutor.h`（2 个成员）
- Modify: `src/core/DockCommandExecutor.cpp`（execute / onMqttMessage / emitState）

- [ ] **Step 1: `DockCommand.h` 中 `DockCommandResult` 增加两个字段**

```cpp
struct DockCommandResult {
    DockCommandType type = DockCommandType::DebugModeOpen;
    DockCommandState state = DockCommandState::Failed;
    QString gatewaySn;
    QString tid;
    QString method;
    int resultCode = -1;
    QString message;
    QString requestJson;   // 下发报文（缩进格式化）
    QString replyJson;     // 响应报文（缩进格式化），无回复时为空
};
```

- [ ] **Step 2: `DockCommandExecutor.h` 成员区增加**

在 `QString            mReplyTopic;` 之后追加：

```cpp
    QString            mRequestJson;
    QString            mReplyJson;
```

- [ ] **Step 3: `DockCommandExecutor.cpp` 三处修改**

include 区补充：

```cpp
#include <QJsonParseError>
```

`execute()` 中，`mHasPending = true;` 之后、`emitState(Publishing...)` 之前追加：

```cpp
    mRequestJson = QString::fromUtf8(
        QJsonDocument(mPending.payload).toJson(QJsonDocument::Indented));
    mReplyJson.clear();
```

`onMqttMessage()` 中，`mTimeoutTimer->stop();` 之前追加（tid 匹配之后）：

```cpp
    QJsonParseError parseError;
    const QJsonDocument replyDoc = QJsonDocument::fromJson(payload, &parseError);
    mReplyJson = (parseError.error == QJsonParseError::NoError)
        ? QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented))
        : QString::fromUtf8(payload);  // 非法 JSON 原样展示
```

`emitState()` 中，`result.message    = message;` 之后追加：

```cpp
    result.requestJson = mRequestJson;
    result.replyJson   = mReplyJson;
```

- [ ] **Step 4: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译成功

- [ ] **Step 5: 提交**

```bash
git add src/core/DockCommand.h src/core/DockCommandExecutor.h src/core/DockCommandExecutor.cpp
git commit -m "feat: 机场控制指令结果透传下发与响应报文"
```

---

### Task 3: DockControlPanel 增加下发记录区与终态弹窗

**Files:**
- Modify: `src/ui/DockControlPanel.h`（前置声明、成员、私有方法）
- Modify: `src/ui/DockControlPanel.cpp`（setupUi 追加记录区、onCommandStateChanged、新方法 appendHistory）

- [ ] **Step 1: `DockControlPanel.h` 修改**

前置声明区 `class QPushButton;` 之后追加：

```cpp
class QPlainTextEdit;
```

私有方法区 `void setStatus(const QString& text, bool error = false);` 之后追加：

```cpp
    void appendHistory(const DockCommandResult& result);
```

成员区 `QPushButton* mChargeCloseBtn = nullptr;` 之后追加：

```cpp
    QPlainTextEdit* mHistoryEdit = nullptr;
```

- [ ] **Step 2: `DockControlPanel.cpp` include 区补充**

```cpp
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTime>
```

- [ ] **Step 3: `setupUi()` 末尾追加记录区**

将 `mainLayout->addLayout(cardRow, 1);` 改为：

```cpp
    mainLayout->addLayout(cardRow);

    // ——— 下发记录：日志流文本块，最新在最上 ———
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);
```

- [ ] **Step 4: `onCommandStateChanged()` 终态分支增加记录与弹窗**

将终态处理段（`mPending = false;` 起到函数尾）替换为：

```cpp
    mPending = false;
    if (result.state == DockCommandState::Succeeded) {
        if (result.type == DockCommandType::DebugModeOpen)
            mDebugModeState = DebugModeState::Enabled;
        else if (result.type == DockCommandType::DebugModeClose)
            mDebugModeState = DebugModeState::Disabled;
        setStatus(QString::fromUtf8("%1成功：%2").arg(action, result.message));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    appendHistory(result);
    updateButtonStates();

    // 终态弹窗提醒（放在最后，模态弹窗不阻塞状态刷新）
    if (result.state == DockCommandState::Succeeded)
        QMessageBox::information(this, QString::fromUtf8("控制成功"),
            QString::fromUtf8("“%1”执行成功（%2）").arg(action, result.message));
    else
        QMessageBox::warning(this, QString::fromUtf8("控制失败"),
            QString::fromUtf8("“%1”执行失败：%2").arg(action, result.message));
}
```

- [ ] **Step 5: 文件末尾新增 `appendHistory()` 实现**

```cpp
void DockControlPanel::appendHistory(const DockCommandResult& result) {
    const QString action = DockCommandBuilder::displayName(result.type);
    QString verdict;
    switch (result.state) {
    case DockCommandState::Succeeded:
        verdict = QString::fromUtf8("✅ 成功 (result=0)");
        break;
    case DockCommandState::TimedOut:
        verdict = QString::fromUtf8("❌ 超时");
        break;
    default:
        verdict = QString::fromUtf8("❌ 失败（%1）").arg(result.message);
        break;
    }

    QString block;
    block += QStringLiteral("[%1] %2  %3\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action, verdict);
    block += QString::fromUtf8("Topic: thing/product/%1/services\n").arg(result.gatewaySn);
    block += QString::fromUtf8("下发:\n%1\n").arg(result.requestJson.trimmed());
    block += QString::fromUtf8("响应:\n%1\n").arg(result.replyJson.isEmpty()
        ? QString::fromUtf8("（无响应）") : result.replyJson.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    // 最新记录插入顶部
    mHistoryEdit->moveCursor(QTextCursor::Start);
    mHistoryEdit->insertPlainText(block);
    mHistoryEdit->moveCursor(QTextCursor::Start);
}
```

- [ ] **Step 6: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译成功

- [ ] **Step 7: 提交**

```bash
git add src/ui/DockControlPanel.h src/ui/DockControlPanel.cpp
git commit -m "feat: 机场控制面板增加下发记录区与终态弹窗提醒"
```

---

### Task 4: DockControlDialog 与 MainWindow 功能中心入口

**Files:**
- Create: `src/ui/DockControlDialog.h`（纯头文件）
- Modify: `src/ui/MainWindow.h`（include、成员）
- Modify: `src/ui/MainWindow.cpp`（setupToolBar、setupLayout）
- Modify: `CMakeLists.txt`（HEADERS）

- [ ] **Step 1: 创建 `src/ui/DockControlDialog.h`**

```cpp
#ifndef DOCKCONTROLDIALOG_H
#define DOCKCONTROLDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include "DockControlPanel.h"

// 机场控制独立窗口：非模态薄壳，内嵌 DockControlPanel。
// 关闭仅隐藏（QDialog 默认行为），再次打开为同一实例。
class DockControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit DockControlDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , mPanel(new DockControlPanel(this))
    {
        setWindowTitle(QString::fromUtf8("机场控制"));
        resize(720, 520);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->addWidget(mPanel);
    }

    DockControlPanel* panel() const { return mPanel; }

private:
    DockControlPanel* mPanel;
};

#endif // DOCKCONTROLDIALOG_H
```

- [ ] **Step 2: `MainWindow.h` 修改**

`#include "DockControlPanel.h"` 改为：

```cpp
#include "DockControlDialog.h"
```

成员区：删除 `QPushButton*       mToggleDockCtrlBtn;`，并把

```cpp
    DockControlPanel*  mDockControlPanel;
```

改为：

```cpp
    DockControlPanel*  mDockControlPanel = nullptr;   // 指向对话框内的面板
    DockControlDialog* mDockCtrlDialog = nullptr;
```

- [ ] **Step 3: `MainWindow.cpp` `setupToolBar()` 新增功能中心按钮**

在配置按钮 connect 块之后、`// 帮助按钮（配置按钮右侧）` 之前插入：

```cpp
    // 功能中心按钮（配置与帮助之间）
    auto* featureBtn = new QToolButton(this);
    featureBtn->setText("🧰 功能中心");
    featureBtn->setObjectName("helpBtn");   // 复用帮助按钮样式
    featureBtn->setPopupMode(QToolButton::InstantPopup);
    featureBtn->setCursor(Qt::PointingHandCursor);
    {
        auto* menu = new QMenu(featureBtn);
        menu->addAction("🎮 机场控制", this, [this]() {
            if (!mDockCtrlDialog) return;
            mDockCtrlDialog->show();
            mDockCtrlDialog->raise();
            mDockCtrlDialog->activateWindow();
        });
        featureBtn->setMenu(menu);
    }
    toolbar->addWidget(featureBtn);
```

- [ ] **Step 4: `MainWindow.cpp` `setupLayout()` 移除底部面板、改建对话框**

删除以下代码块：

```cpp
    // 机场控制（折叠）
    mDockControlPanel = new DockControlPanel(this);
    mDockControlPanel->setVisible(false);
    mDockControlPanel->setMinimumHeight(120);
    verticalSplitter->addWidget(mDockControlPanel);
    verticalSplitter->setStretchFactor(2, 0);
```

将切换按钮区（`mToggleDockCtrlBtn` 创建 + `toggleRow`）替换回单按钮：

```cpp
    mTogglePublishBtn = new QPushButton("▶ Topic 下发", this);
    mTogglePublishBtn->setObjectName("publishToggle");
    mTogglePublishBtn->setCheckable(true);
    mTogglePublishBtn->setCursor(Qt::PointingHandCursor);
    connect(mTogglePublishBtn, &QPushButton::toggled, this, [this](bool checked) {
        mPublishPanel->setVisible(checked);
        mTogglePublishBtn->setText(checked ? "◢ Topic 下发" : "▶ Topic 下发");
    });

    rightLayout->addWidget(mTogglePublishBtn);
```

函数末尾初始状态段改为：

```cpp
    // 加载 publish 模板 + 初始连接状态
    mPublishPanel->loadTemplates(QApplication::applicationDirPath() + "/config/topic-send-construct/topic-send-construct.md");
    mPublishPanel->setConnected(mDevMgr->isConnected());

    // 机场控制独立窗口（功能中心菜单打开）
    mDockCtrlDialog = new DockControlDialog(this);
    mDockControlPanel = mDockCtrlDialog->panel();
    mDockControlPanel->setConnected(mDevMgr->isConnected());
```

说明：`connectSignals()` 与 `onDeviceSelected()` 中对 `mDockControlPanel` 的既有接线不动，面板换了宿主依然有效。

- [ ] **Step 5: `CMakeLists.txt` HEADERS 增加**

`src/ui/DockControlPanel.h` 之后追加：

```cmake
    src/ui/DockControlDialog.h
```

- [ ] **Step 6: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译成功

- [ ] **Step 7: 提交**

```bash
git add src/ui/DockControlDialog.h src/ui/MainWindow.h src/ui/MainWindow.cpp CMakeLists.txt
git commit -m "feat: 工具栏新增功能中心入口，机场控制改为非模态独立窗口"
```

---

### Task 5: 手动 GUI 验证

**Files:** 无代码改动

- [ ] **Step 1: 运行程序**

Run: `cd build_mingw && ./DjiCloudApi.exe &`（后台）

- [ ] **Step 2: 逐项验证（请用户操作确认）**

1. 工具栏「🧰 功能中心」位于配置与帮助之间，菜单弹出样式与帮助一致
2. 点「🎮 机场控制」弹出非模态窗口（720×520）；主窗口仍可操作；重复点击置前；关闭后重新打开状态保留
3. 底部只剩「▶ Topic 下发」，无「机场控制」切换按钮
4. 窗口内：顶行信息 + 4 卡片 + 下发记录区（深色控制台样式，占据下半部）
5. 主窗口切换设备 → 窗口标题信息跟随；断开连接 → 按钮禁用
6. 下发指令 → 终态弹窗提醒（成功 information / 失败与超时 warning）→ 记录区顶部插入完整记录块（时间/名称/结果/Topic/下发/响应；超时显示"（无响应）"）

Expected: 全部通过（无真机时以超时路径验证第 6 项）
