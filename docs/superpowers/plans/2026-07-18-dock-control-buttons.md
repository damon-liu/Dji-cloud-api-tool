# 机场常用控制按钮实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 集成已有的 DockCommand/DockControlPanel 代码，新增指令执行器，实现机场常用控制按钮（调试模式、开关机、舱盖、充电）的完整下发-回复闭环。

**Architecture:** 复用未跟踪的 `DockCommand.h/.cpp`（指令构造/解析）与 `DockControlPanel.h/.cpp`（面板 UI），新写 `DockCommandExecutor`（发布 → 订阅 reply → tid 匹配 → 10s 超时），由 `DeviceManager` 持有并转发消息，`MainWindow` 以折叠面板形式接入（与 Topic 下发面板同款交互）。

**Tech Stack:** Qt 6 (Core/Widgets/Mqtt)、C++17、CMake + MinGW

**Spec:** `docs/superpowers/specs/2026-07-18-dock-control-buttons-design.md`

**全局注意：**
- 编译命令 `cmake --build build_mingw`，**禁止 `--clean-first`**（保留 config.json 凭证）
- 链接报 `Permission denied` 说明 `DjiCloudApi.exe` 正在运行，先关闭程序
- 工作区有一个**未提交的无关改动**（`src/ui/ConfigDialog.cpp` 密码按钮，等用户验证）——所有 commit 必须**显式指定文件路径**，严禁 `git add -A` / `git add .`
- 中文提交信息

---

### Task 1: 新建 DockCommandExecutor 并登记 CMake

**Files:**
- Create: `src/core/DockCommandExecutor.h`
- Create: `src/core/DockCommandExecutor.cpp`
- Modify: `CMakeLists.txt:34`（SOURCES 列表）、`CMakeLists.txt:54`（HEADERS 列表）
- 已存在（本次一并纳入版本控制）: `src/core/DockCommand.h`、`src/core/DockCommand.cpp`、`src/ui/DockControlPanel.h`、`src/ui/DockControlPanel.cpp`

- [ ] **Step 1: 创建 `src/core/DockCommandExecutor.h`**

```cpp
#ifndef DOCKCOMMANDEXECUTOR_H
#define DOCKCOMMANDEXECUTOR_H

#include <QObject>
#include <QTimer>
#include "DockCommand.h"

class MqttClientManager;

// DockCommandExecutor: 机场控制指令执行器
// 发布 services 指令 → 订阅 services_reply → 按 tid 匹配结果 → 超时处理
// 同一时刻仅允许一个进行中指令
class DockCommandExecutor : public QObject {
    Q_OBJECT
public:
    explicit DockCommandExecutor(MqttClientManager* mqtt, QObject* parent = nullptr);

    // 发起指令；已有进行中指令时返回 false
    bool execute(const QString& gatewaySn, DockCommandType type);

    // 由 DeviceManager 转发所有 MQTT 消息，内部过滤 reply topic
    void onMqttMessage(const QString& topic, const QByteArray& payload);

signals:
    void commandStateChanged(const DockCommandResult& result);

private slots:
    void onTimeout();

private:
    void emitState(DockCommandState state, int resultCode, const QString& message);

    MqttClientManager* mMqtt;
    QTimer*            mTimeoutTimer;
    DockCommandRequest mPending;
    QString            mReplyTopic;
    bool               mHasPending = false;

    static constexpr int REPLY_TIMEOUT_MS = 10000;
};

#endif // DOCKCOMMANDEXECUTOR_H
```

- [ ] **Step 2: 创建 `src/core/DockCommandExecutor.cpp`**

```cpp
#include "DockCommandExecutor.h"
#include "MqttClientManager.h"
#include <QJsonDocument>
#include <QDebug>

DockCommandExecutor::DockCommandExecutor(MqttClientManager* mqtt, QObject* parent)
    : QObject(parent)
    , mMqtt(mqtt)
    , mTimeoutTimer(new QTimer(this))
{
    mTimeoutTimer->setSingleShot(true);
    mTimeoutTimer->setInterval(REPLY_TIMEOUT_MS);
    connect(mTimeoutTimer, &QTimer::timeout, this, &DockCommandExecutor::onTimeout);
}

bool DockCommandExecutor::execute(const QString& gatewaySn, DockCommandType type) {
    if (mHasPending) {
        qWarning() << "DockCommandExecutor: command already pending, ignored";
        return false;
    }

    mPending = DockCommandBuilder::build(gatewaySn, type);
    mReplyTopic = QStringLiteral("thing/product/%1/services_reply").arg(mPending.gatewaySn);
    mHasPending = true;

    emitState(DockCommandState::Publishing, -1, QString::fromUtf8("正在发布指令"));

    // 确保已订阅 reply topic（subscribeTopics 内部去重，
    // 防止用户禁用了该 topic 导致收不到回复）
    mMqtt->subscribeTopics({mReplyTopic});

    QJsonDocument doc(mPending.payload);
    mMqtt->publish(mPending.topic, doc.toJson(QJsonDocument::Compact));

    emitState(DockCommandState::WaitingReply, -1, QString::fromUtf8("等待机场响应"));
    mTimeoutTimer->start();
    return true;
}

void DockCommandExecutor::onMqttMessage(const QString& topic, const QByteArray& payload) {
    if (!mHasPending || topic != mReplyTopic)
        return;

    DockCommandReply reply = DockCommandBuilder::parseReply(payload);
    if (!reply.valid || reply.tid != mPending.tid)
        return;  // 非本指令的回复（可能来自其他客户端下发）

    mTimeoutTimer->stop();
    mHasPending = false;

    if (reply.resultCode == 0)
        emitState(DockCommandState::Succeeded, 0, QString::fromUtf8("result = 0"));
    else
        emitState(DockCommandState::Failed, reply.resultCode,
                  QString::fromUtf8("错误码 %1").arg(reply.resultCode));
}

void DockCommandExecutor::onTimeout() {
    if (!mHasPending)
        return;
    mHasPending = false;
    emitState(DockCommandState::TimedOut, -1,
              QString::fromUtf8("10 秒未收到回复，请检查机场在线状态及 services_reply 订阅"));
}

void DockCommandExecutor::emitState(DockCommandState state, int resultCode,
                                    const QString& message) {
    DockCommandResult result;
    result.type       = mPending.type;
    result.state      = state;
    result.gatewaySn  = mPending.gatewaySn;
    result.tid        = mPending.tid;
    result.method     = mPending.method;
    result.resultCode = resultCode;
    result.message    = message;
    emit commandStateChanged(result);
}
```

- [ ] **Step 3: 修改 `CMakeLists.txt`**

SOURCES 列表中 `src/ui/TopicParsePanel.cpp` 之后追加：

```cmake
    src/core/DockCommand.cpp
    src/core/DockCommandExecutor.cpp
    src/ui/DockControlPanel.cpp
```

HEADERS 列表中 `src/ui/TopicParsePanel.h` 之后追加：

```cmake
    src/core/DockCommand.h
    src/core/DockCommandExecutor.h
    src/ui/DockControlPanel.h
```

- [ ] **Step 4: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译成功（新文件被编译，尚无调用方）

- [ ] **Step 5: 提交**

```bash
git add src/core/DockCommand.h src/core/DockCommand.cpp \
        src/core/DockCommandExecutor.h src/core/DockCommandExecutor.cpp \
        src/ui/DockControlPanel.h src/ui/DockControlPanel.cpp CMakeLists.txt
git commit -m "feat: 新增机场控制指令构造器、执行器与控制面板（未接入）"
```

---

### Task 2: DeviceManager 接入执行器

**Files:**
- Modify: `src/core/DeviceManager.h`（include、slot、signal、成员）
- Modify: `src/core/DeviceManager.cpp`（构造函数、onMqttMessage、新方法）

- [ ] **Step 1: 修改 `src/core/DeviceManager.h`**

include 区，`#include "MqttClientManager.h"`（第 12 行）之后追加：

```cpp
#include "DockCommandExecutor.h"
```

`public slots:` 区（第 80-81 行）改为：

```cpp
public slots:
    void publishMessage(const QString& topic, const QString& json);
    void executeDockCommand(const QString& gatewaySn, DockCommandType type);
```

`signals:` 区最后一行 `void publishResult(...)` 之后追加：

```cpp
    void dockCommandStateChanged(const DockCommandResult& result);
```

private 成员区，`MqttClientManager*         mMqttManager;`（第 107 行）之后追加：

```cpp
    DockCommandExecutor*       mDockCmdExecutor;
```

- [ ] **Step 2: 修改 `src/core/DeviceManager.cpp` 构造函数**

构造函数中 `// Publish 结果转发` 代码块（第 42-44 行）之后追加：

```cpp
    // 机场控制指令执行器
    mDockCmdExecutor = new DockCommandExecutor(mMqttManager, this);
    connect(mDockCmdExecutor, &DockCommandExecutor::commandStateChanged,
            this, &DeviceManager::dockCommandStateChanged);
```

- [ ] **Step 3: 修改 `src/core/DeviceManager.cpp` 消息转发与新方法**

`onMqttMessage`（第 433-435 行）改为：

```cpp
void DeviceManager::onMqttMessage(const QString& topic, const QByteArray& payload) {
    mDockCmdExecutor->onMqttMessage(topic, payload);
    parseAndRoute(topic, payload);
}
```

文件末尾 `publishMessage` 实现之后追加：

```cpp
void DeviceManager::executeDockCommand(const QString& gatewaySn, DockCommandType type) {
    mDockCmdExecutor->execute(gatewaySn, type);
}
```

- [ ] **Step 4: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译成功

- [ ] **Step 5: 提交**

```bash
git add src/core/DeviceManager.h src/core/DeviceManager.cpp
git commit -m "feat: DeviceManager 接入机场控制指令执行器"
```

---

### Task 3: MainWindow 接入控制面板

**Files:**
- Modify: `src/ui/MainWindow.h`（include、2 个成员）
- Modify: `src/ui/MainWindow.cpp`（setupLayout、onDeviceSelected、connectSignals）

- [ ] **Step 1: 修改 `src/ui/MainWindow.h`**

include 区 `#include "TopicParsePanel.h"`（第 16 行）之后追加：

```cpp
#include "DockControlPanel.h"
```

成员区 `QPushButton*       mTogglePublishBtn;`（第 46 行）之后追加：

```cpp
    DockControlPanel*  mDockControlPanel;
    QPushButton*       mToggleDockCtrlBtn;
```

- [ ] **Step 2: 修改 `setupLayout()` — 面板加入垂直分割器**

将（`MainWindow.cpp` 第 416-422 行）：

```cpp
    // Topic 下发（折叠）
    mPublishPanel = new PublishPanel(this);
    mPublishPanel->setVisible(false);
    mPublishPanel->setMinimumHeight(120);  // 保证基本可操作区域

    verticalSplitter->addWidget(mPublishPanel);
    verticalSplitter->setStretchFactor(1, 0);
```

改为：

```cpp
    // Topic 下发（折叠）
    mPublishPanel = new PublishPanel(this);
    mPublishPanel->setVisible(false);
    mPublishPanel->setMinimumHeight(120);  // 保证基本可操作区域

    verticalSplitter->addWidget(mPublishPanel);
    verticalSplitter->setStretchFactor(1, 0);

    // 机场控制（折叠）
    mDockControlPanel = new DockControlPanel(this);
    mDockControlPanel->setVisible(false);
    mDockControlPanel->setMinimumHeight(120);
    verticalSplitter->addWidget(mDockControlPanel);
    verticalSplitter->setStretchFactor(2, 0);
```

- [ ] **Step 3: 修改 `setupLayout()` — 底部切换按钮并排**

将（原第 426-435 行）：

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

改为：

```cpp
    mTogglePublishBtn = new QPushButton("▶ Topic 下发", this);
    mTogglePublishBtn->setObjectName("publishToggle");
    mTogglePublishBtn->setCheckable(true);
    mTogglePublishBtn->setCursor(Qt::PointingHandCursor);
    connect(mTogglePublishBtn, &QPushButton::toggled, this, [this](bool checked) {
        mPublishPanel->setVisible(checked);
        mTogglePublishBtn->setText(checked ? "◢ Topic 下发" : "▶ Topic 下发");
    });

    mToggleDockCtrlBtn = new QPushButton("▶ 机场控制", this);
    mToggleDockCtrlBtn->setObjectName("publishToggle");
    mToggleDockCtrlBtn->setCheckable(true);
    mToggleDockCtrlBtn->setCursor(Qt::PointingHandCursor);
    connect(mToggleDockCtrlBtn, &QPushButton::toggled, this, [this](bool checked) {
        mDockControlPanel->setVisible(checked);
        mToggleDockCtrlBtn->setText(checked ? "◢ 机场控制" : "▶ 机场控制");
    });

    auto* toggleRow = new QHBoxLayout;
    toggleRow->setSpacing(4);
    toggleRow->addWidget(mTogglePublishBtn);
    toggleRow->addWidget(mToggleDockCtrlBtn);
    rightLayout->addLayout(toggleRow);
```

- [ ] **Step 4: 修改 `setupLayout()` 末尾 — 初始连接状态**

`mPublishPanel->setConnected(mDevMgr->isConnected());`（第 448 行）之后追加：

```cpp
    mDockControlPanel->setConnected(mDevMgr->isConnected());
```

- [ ] **Step 5: 修改 `onDeviceSelected()` — 两个分支**

取消选中分支：`mPublishPanel->setTopics({});`（第 675 行）之后追加：

```cpp
        mDockControlPanel->clearDevice();
```

选中分支：`mPublishPanel->setTopics(mDevMgr->topicsForDevice(sn));`（第 710 行）之后追加：

```cpp
    // 机场控制面板：机场 → 自身；飞机 → 父机场；其他 → 清空
    if (dev->type == DeviceType::Dock) {
        mDockControlPanel->setDevice(dev->name, dev->sn, dev->online);
    } else if (!dev->parentSn.isEmpty()) {
        DeviceInfo* parentDev = mDevMgr->device(dev->parentSn);
        if (parentDev)
            mDockControlPanel->setDevice(parentDev->name, parentDev->sn, parentDev->online);
        else
            mDockControlPanel->clearDevice();
    } else {
        mDockControlPanel->clearDevice();
    }
```

- [ ] **Step 6: 修改 `connectSignals()`**

`brokerConnected` lambda 中 `mPublishPanel->setConnected(true);`（第 539 行）之后追加：

```cpp
        mDockControlPanel->setConnected(true);
```

`brokerDisconnected` lambda 中 `mPublishPanel->setConnected(false);`（第 571 行）之后追加：

```cpp
        mDockControlPanel->setConnected(false);
```

`// PublishPanel → DeviceManager` 连接块（第 655-660 行）之后追加：

```cpp
    // DockControlPanel ↔ DeviceManager
    connect(mDockControlPanel, &DockControlPanel::commandRequested,
            mDevMgr, &DeviceManager::executeDockCommand);
    connect(mDevMgr, &DeviceManager::dockCommandStateChanged,
            mDockControlPanel, &DockControlPanel::onCommandStateChanged);
```

说明：`deviceOnlineChanged` 无需额外处理——现有 lambda 会 rebuild 设备树并 `selectDevice(prevSelected)`，重新触发 `onDeviceSelected` 从而刷新面板的在线状态。

- [ ] **Step 7: 编译验证**

Run: `cmake --build build_mingw`
Expected: 编译成功

- [ ] **Step 8: 提交**

```bash
git add src/ui/MainWindow.h src/ui/MainWindow.cpp
git commit -m "feat: 主窗口接入机场控制面板，新增折叠切换按钮"
```

---

### Task 4: 手动 GUI 验证

**Files:** 无代码改动

- [ ] **Step 1: 运行程序**

Run: `cd build_mingw && ./DjiCloudApi.exe &`（后台运行）

- [ ] **Step 2: 逐项验证（请用户操作确认）**

1. 右下角出现「▶ 机场控制」按钮，与「▶ Topic 下发」并排；点击展开/收起面板正常
2. 未连接 broker：面板所有按钮禁用，提示"MQTT 未连接"
3. 连接后选中机场（或其子飞机）：面板标题显示机场名称/SN/在线状态
4. 未进入调试模式时：仅「进入远程调试」可点；点击弹出二次确认框
5. 确认下发后状态标签变化：正在发布 → 等待机场响应 → 成功/失败/10s 超时
6. 无真实机场时验证超时路径：10 秒后提示超时信息

Expected: 全部通过（无真机时第 5 项以超时路径代替）

- [ ] **Step 3: 验证通过后由用户确认，无遗留改动需提交**
