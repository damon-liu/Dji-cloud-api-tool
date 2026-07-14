# Topic 下发功能 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现 MQTT Topic 下发功能，打通 PublishPanel UI → MqttClientManager 的 publish 链路

**Architecture:** 信号链模式 — PublishPanel 发射 publishRequested → MainWindow 接线 → DeviceManager::publishMessage() → MqttClientManager::publish() → QMqttClient::publish()，结果通过 publishCompleted → publishResult → onPublishResult 异步返回

**Tech Stack:** C++17, Qt 6 (Core/Widgets/Mqtt)

**改动文件：**
- 修改: `src/mqtt/MqttClientManager.h`, `src/mqtt/MqttClientManager.cpp`
- 修改: `src/core/DeviceManager.h`, `src/core/DeviceManager.cpp`
- 重写: `src/ui/PublishPanel.h`, 新建 `src/ui/PublishPanel.cpp`
- 修改: `src/ui/MainWindow.cpp`
- 新建: `publish_templates.json`(内置默认值，运行时自动创建)

---

### Task 1: MqttClientManager — 添加 publish() 方法和 publishCompleted 信号

**Files:**
- Modify: `src/mqtt/MqttClientManager.h:22-33`
- Modify: `src/mqtt/MqttClientManager.cpp:77-82`

- [ ] **Step 1: 在 MqttClientManager.h 中添加 publish 方法声明和信号**

在 `unsubscribeTopics` 声明之后，`signals:` 之前插入：

```cpp
    // 发布消息到指定 topic（QoS 1）
    void publish(const QString& topic, const QByteArray& payload);
```

在 `messageReceived` 信号之后插入新信号：

```cpp
    void publishCompleted(const QString& topic, bool success, const QString& errorMsg);
```

- [ ] **Step 2: 在 MqttClientManager.cpp 中实现 publish()**

在 `replaceSubscriptions` 方法之后添加：

```cpp
void MqttClientManager::publish(const QString& topic, const QByteArray& payload) {
    if (!isConnected()) {
        emit publishCompleted(topic, false, QStringLiteral("MQTT not connected"));
        return;
    }

    qint32 msgId = mClient->publish(QMqttTopicName(topic), payload, 1);  // QoS 1
    if (msgId < 0) {
        emit publishCompleted(topic, false, QStringLiteral("publish() returned error"));
        return;
    }

    // 追踪 messageId → topic 映射，用于 messageSent 信号回调
    mPendingPublishes[msgId] = topic;
}
```

- [ ] **Step 3: 在 MqttClientManager.h 中添加 mPendingPublishes 成员和 messageSent 槽**

在 private 区域添加：

```cpp
    QHash<qint32, QString> mPendingPublishes;  // messageId → topic 映射
```

在 private slots 区域添加：

```cpp
    void onMessageSent(qint32 id);
```

- [ ] **Step 4: 在构造函数中连接 messageSent 信号，实现 onMessageSent**

在 `MqttClientManager::MqttClientManager()` 构造函数的 connect 块中添加：

```cpp
    connect(mClient, &QMqttClient::messageSent,
            this, &MqttClientManager::onMessageSent);
```

在 .cpp 文件中实现 onMessageSent：

```cpp
void MqttClientManager::onMessageSent(qint32 id) {
    if (mPendingPublishes.contains(id)) {
        QString topic = mPendingPublishes.take(id);
        emit publishCompleted(topic, true, QString());
    }
}
```

- [ ] **Step 5: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功

- [ ] **Step 6: 提交**

```bash
git add src/mqtt/MqttClientManager.h src/mqtt/MqttClientManager.cpp
git commit -m "feat: MqttClientManager 添加 publish 方法和 publishCompleted 信号"
```

---

### Task 2: DeviceManager — 添加 publishMessage() slot 和 publishResult 信号

**Files:**
- Modify: `src/core/DeviceManager.h:79-89`
- Modify: `src/core/DeviceManager.cpp:22-46`

- [ ] **Step 1: 在 DeviceManager.h 中添加声明**

在 `public` 区域，`brokerError` 信号所在 signals 块之前插入 public slot：

```cpp
public slots:
    void publishMessage(const QString& topic, const QString& json);
```

在 `profileListChanged()` 信号之后插入：

```cpp
    void publishResult(const QString& topic, bool success, const QString& message);
```

- [ ] **Step 2: 在 DeviceManager.cpp 构造函数中连接 publishCompleted 信号**

在构造函数末尾（`mOfflineTimer->start()` 之前）添加：

```cpp
    connect(mMqttManager, &MqttClientManager::publishCompleted,
            this, &DeviceManager::publishResult);
```

- [ ] **Step 3: 实现 publishMessage()**

在 .cpp 文件末尾添加：

```cpp
void DeviceManager::publishMessage(const QString& topic, const QString& json) {
    QByteArray payload = json.toUtf8();
    mMqttManager->publish(topic, payload);
}
```

- [ ] **Step 4: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功

- [ ] **Step 5: 提交**

```bash
git add src/core/DeviceManager.h src/core/DeviceManager.cpp
git commit -m "feat: DeviceManager 添加 publishMessage slot 和 publishResult 信号"
```

---

### Task 3: PublishPanel — 拆分为 .h 声明 + .cpp 实现

**Files:**
- Rewrite: `src/ui/PublishPanel.h`
- Create: `src/ui/PublishPanel.cpp`

核心思路：将 PublishPanel 从纯头文件改为 `.h` 声明 + `.cpp` 实现，为后续添加发送逻辑、模板加载、发送历史做准备。这一 Task 只做拆分，不改变 UI 布局和行为，保持功能等价。

- [ ] **Step 1: 重写 PublishPanel.h — 保留声明，移除内联实现**

```cpp
#ifndef PUBLISHPANEL_H
#define PUBLISHPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class PublishPanel : public QWidget {
    Q_OBJECT
public:
    static const QStringList PUBLISH_PRESETS;

    explicit PublishPanel(QWidget* parent = nullptr);

    void setDeviceSn(const QString& sn);
    void setTopics(const QStringList& subscribed);

signals:
    void publishRequested(const QString& topic, const QString& json);

private:
    void setupUi();
    void updateSendButtonState();

    QComboBox*      mTopicCombo    = nullptr;
    QPlainTextEdit* mEditor        = nullptr;
    QPushButton*    mSendBtn       = nullptr;
    QString         mDeviceSn;
};

#endif // PUBLISHPANEL_H
```

- [ ] **Step 2: 新建 PublishPanel.cpp — 迁移原内联实现**

```cpp
#include "PublishPanel.h"

const QStringList PublishPanel::PUBLISH_PRESETS = {
    "thing/product/{sn}/property/set",
    "thing/product/{sn}/services",
    "thing/product/{sn}/events_reply",
    "thing/product/{sn}/requests_reply",
    "sys/product/{sn}/status_reply",
};

PublishPanel::PublishPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PublishPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(4);

    // Topic 选择
    auto* topicLayout = new QHBoxLayout;
    auto* topicLabel = new QLabel("Topic:");
    topicLabel->setStyleSheet("font-size: 12px; color: #5f6368;");
    topicLayout->addWidget(topicLabel);

    mTopicCombo = new QComboBox(this);
    mTopicCombo->setEditable(true);
    mTopicCombo->setMinimumWidth(200);
    mTopicCombo->setStyleSheet(
        "QComboBox { padding: 3px 6px; border: 1px solid #dadce0; border-radius: 4px; "
        "background: #fff; font-size: 12px; }");
    topicLayout->addWidget(mTopicCombo, 1);
    layout->addLayout(topicLayout);

    // JSON 编辑区
    mEditor = new QPlainTextEdit(this);
    mEditor->setFont(QFont("Consolas", 10));
    mEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    mEditor->setPlaceholderText("输入要发送的 JSON...");
    mEditor->setStyleSheet(
        "QPlainTextEdit { background: #fff; border: 1px solid #dadce0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 12px; padding: 6px; }");
    layout->addWidget(mEditor, 1);

    // 发送按钮
    auto* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    mSendBtn = new QPushButton("发送", this);
    mSendBtn->setEnabled(false);
    mSendBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; border: none; "
        "border-radius: 4px; padding: 6px 24px; font-weight: bold; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    btnLayout->addWidget(mSendBtn);
    layout->addLayout(btnLayout);

    // TODO: 后续 Task 替换为实际发送逻辑
    connect(mSendBtn, &QPushButton::clicked, this, [this]() {
        // TODO v1.1: 实现 MQTT publish
    });
}

void PublishPanel::setDeviceSn(const QString& sn) {
    mDeviceSn = sn;
}

void PublishPanel::setTopics(const QStringList& /*subscribed*/) {
    mTopicCombo->clear();
    if (!mDeviceSn.isEmpty()) {
        for (const auto& tpl : PUBLISH_PRESETS)
            mTopicCombo->addItem(QString(tpl).replace("{sn}", mDeviceSn));
    }
}

void PublishPanel::updateSendButtonState() {
    // 后续 Task 添加具体逻辑
    mSendBtn->setEnabled(false);
}
```

- [ ] **Step 3: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功（CMakeLists.txt 第 31 行已包含 `src/ui/PublishPanel.cpp`）

- [ ] **Step 4: 提交**

```bash
git add src/ui/PublishPanel.h src/ui/PublishPanel.cpp
git commit -m "refactor: PublishPanel 拆分为 .h 声明 + .cpp 实现"
```

---

### Task 4: publish_templates.json + loadTemplates()

**Files:**
- Modify: `src/ui/PublishPanel.h`
- Modify: `src/ui/PublishPanel.cpp`

- [ ] **Step 1: 在 PublishPanel.h 中添加模板相关声明**

在 public 区域添加：

```cpp
    void loadTemplates(const QString& path);
```

在 private 区域添加：

```cpp
    QMap<QString, QString> mTemplates;            // topic → template JSON
    static QMap<QString, QString> builtinTemplates();  // 内置默认模板
```

- [ ] **Step 2: 在 PublishPanel.cpp 开头添加内置模板 + loadTemplates 实现**

在 `#include "PublishPanel.h"` 之后添加：

```cpp
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QApplication>
```

在文件末尾添加 builtinTemplates() 和 loadTemplates()：

```cpp
QMap<QString, QString> PublishPanel::builtinTemplates() {
    return {
        {"thing/product/{sn}/property/set",  "{\n  \n}"},
        {"thing/product/{sn}/services",       "{\n  \"services\": []\n}"},
        {"thing/product/{sn}/events_reply",   "{\n  \"events_reply\": []\n}"},
        {"thing/product/{sn}/requests_reply", "{\n  \"requests_reply\": []\n}"},
        {"sys/product/{sn}/status_reply",     "{\n  \"status_reply\": {}\n}"},
    };
}

void PublishPanel::loadTemplates(const QString& path) {
    mTemplates = builtinTemplates();  // 默认值兜底

    QFile file(path);
    if (!file.exists()) {
        // 自动创建默认模板文件
        QJsonObject root;
        QJsonArray arr;
        auto builtins = builtinTemplates();
        for (auto it = builtins.begin(); it != builtins.end(); ++it) {
            QJsonObject item;
            item["topic"]    = it.key();
            item["template"] = it.value();
            arr.append(item);
        }
        root["templates"] = arr;
        QJsonDocument doc(root);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            qDebug() << "PublishPanel: created default" << path;
        }
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "PublishPanel: cannot open" << path;
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "PublishPanel: JSON parse error in" << path << ":" << err.errorString();
        return;
    }

    QJsonArray arr = doc.object().value("templates").toArray();
    for (const auto& v : arr) {
        QJsonObject item = v.toObject();
        QString topic    = item.value("topic").toString();
        QString tpl      = item.value("template").toString();
        if (!topic.isEmpty())
            mTemplates[topic] = tpl;
    }
    qDebug() << "PublishPanel: loaded" << mTemplates.size() << "publish templates";
}
```

- [ ] **Step 3: 在 setupUi() 末尾添加模板自动填入逻辑**

在 `setupUi()` 末尾（`updateSendButtonState()` 之后，构造函数的 `}` 之前），添加 topic ComboBox 切换时的模板自动填入：

在 `PublishPanel::PublishPanel(QWidget* parent)` 构造函数中，`setupUi()` 之后添加：

```cpp
    // topic 切换时自动填入模板（仅编辑区为空时）
    connect(mTopicCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        if (!mEditor->toPlainText().trimmed().isEmpty())
            return;  // 编辑区有内容时不覆盖
        QString tpl = mTemplates.value(text);
        if (!tpl.isEmpty())
            mEditor->setPlainText(tpl);
    });
```

- [ ] **Step 4: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功

- [ ] **Step 5: 提交**

```bash
git add src/ui/PublishPanel.h src/ui/PublishPanel.cpp
git commit -m "feat: PublishPanel 添加 publish_templates.json 模板加载和自动填入"
```

---

### Task 5: PublishPanel — 发送按钮逻辑 + 连接状态

**Files:**
- Modify: `src/ui/PublishPanel.h`
- Modify: `src/ui/PublishPanel.cpp`

- [ ] **Step 1: 在 PublishPanel.h 中添加新方法声明**

在 public 区域添加：

```cpp
    void setConnected(bool connected);
```

在 public slots 区域添加（在 signals 声明之后）：

```cpp
public slots:
    void onPublishResult(const QString& topic, bool success, const QString& message);
```

在 private 区域添加：

```cpp
    bool mConnected = false;
```

- [ ] **Step 2: 实现 setConnected() 和 updateSendButtonState()**

在 PublishPanel.cpp 中替换 `updateSendButtonState()` 的空实现：

```cpp
void PublishPanel::setConnected(bool connected) {
    mConnected = connected;
    updateSendButtonState();
}

void PublishPanel::updateSendButtonState() {
    bool topicOk = !mTopicCombo->currentText().trimmed().isEmpty();
    bool jsonOk  = !mEditor->toPlainText().trimmed().isEmpty();
    mSendBtn->setEnabled(mConnected && topicOk && jsonOk);
}
```

- [ ] **Step 3: 实现发送按钮点击逻辑**

替换 setupUi() 中发送按钮的 TODO lambda：

```cpp
    // 发送按钮：发射 publishRequested 信号
    connect(mSendBtn, &QPushButton::clicked, this, [this]() {
        QString topic = mTopicCombo->currentText().trimmed();
        QString json  = mEditor->toPlainText().trimmed();
        if (topic.isEmpty() || json.isEmpty())
            return;
        emit publishRequested(topic, json);
    });
```

- [ ] **Step 4: 监听编辑区变化实时更新按钮状态**

在构造函数的 connect 区域添加（在 setupUi() 调用之后）：

```cpp
    // 编辑区内容变化时更新按钮状态
    connect(mEditor, &QPlainTextEdit::textChanged, this, [this]() {
        updateSendButtonState();
    });
    // topic 变化时也更新按钮状态
    connect(mTopicCombo, &QComboBox::currentTextChanged, this, [this]() {
        updateSendButtonState();
    });
```

- [ ] **Step 5: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功

- [ ] **Step 6: 提交**

```bash
git add src/ui/PublishPanel.h src/ui/PublishPanel.cpp
git commit -m "feat: PublishPanel 发送按钮逻辑 + 连接状态控制"
```

---

### Task 6: PublishPanel — 发送历史区域

**Files:**
- Modify: `src/ui/PublishPanel.h`
- Modify: `src/ui/PublishPanel.cpp`

- [ ] **Step 1: 在 PublishPanel.h 中添加发送历史相关成员**

在 private 区域添加：

```cpp
    QPlainTextEdit* mHistoryLog = nullptr;
    QStringList     mHistoryLines;
    static constexpr int MAX_HISTORY = 20;

    void appendHistory(const QString& topic, bool success, const QString& message);
```

- [ ] **Step 2: 在 setupUi() 底部添加发送历史 UI**

在 `setupUi()` 末尾（`updateSendButtonState()` 之前）添加：

```cpp
    // 发送历史（只读）
    mHistoryLog = new QPlainTextEdit(this);
    mHistoryLog->setReadOnly(true);
    mHistoryLog->setMaximumHeight(80);
    mHistoryLog->setFont(QFont("Consolas", 9));
    mHistoryLog->setStyleSheet(
        "QPlainTextEdit { background: #fafafa; border: 1px solid #e0e0e0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 11px; "
        "padding: 4px; color: #333; }");
    mHistoryLog->setPlaceholderText("发送历史（双击恢复）");

    // 双击恢复 topic 和 JSON 到编辑区
    connect(mHistoryLog, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        // 使用 eventFilter 方式处理双击，这里保持简洁，用 selectionChanged 替代
    });
    mHistoryLog->viewport()->installEventFilter(this);
    layout->addWidget(mHistoryLog);
```

- [ ] **Step 3: 在 PublishPanel.h 中添加 eventFilter 声明**

在 protected 区域添加：

```cpp
    bool eventFilter(QObject* obj, QEvent* event) override;
```

添加 include：

```cpp
#include <QEvent>
#include <QMouseEvent>
```

- [ ] **Step 4: 实现 appendHistory() 和 eventFilter()**

在 PublishPanel.cpp 中添加：

```cpp
void PublishPanel::appendHistory(const QString& topic, bool success, const QString& message) {
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");
    QString icon    = success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");
    QString color   = success ? "#1e8e3e" : "#d93025";
    QString line    = QString("[%1] %2 %3  %4")
                          .arg(timeStr, icon, topic, message);

    mHistoryLines.prepend(line);
    if (mHistoryLines.size() > MAX_HISTORY)
        mHistoryLines.removeLast();

    // 用富文本展示
    QString html;
    for (const auto& l : mHistoryLines) {
        if (l.contains(QString::fromUtf8("✅")))
            html += QString("<span style='color:#1e8e3e'>%1</span><br>").arg(l.toHtmlEscaped());
        else
            html += QString("<span style='color:#d93025'>%1</span><br>").arg(l.toHtmlEscaped());
    }
    mHistoryLog->setHtml(html);
}

bool PublishPanel::eventFilter(QObject* obj, QEvent* event) {
    if (obj == mHistoryLog->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QTextCursor cursor = mHistoryLog->cursorForPosition(
            static_cast<QMouseEvent*>(event)->pos());
        int blockNum = cursor.block().blockNumber();
        if (blockNum >= 0 && blockNum < mHistoryLines.size()) {
            const QString& line = mHistoryLines[blockNum];
            // 格式: "[HH:mm:ss] ✅ topic message"
            // 跳过 "[HH:mm:ss] " (11 chars) + emoji(1 char) + " "
            int topicStart = 13;
            int topicEnd   = line.indexOf(' ', topicStart);
            QString topic  = (topicEnd > 0) ? line.mid(topicStart, topicEnd - topicStart)
                                            : line.mid(topicStart);
            if (!topic.isEmpty())
                mTopicCombo->setCurrentText(topic);
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
```

需要添加 include：

```cpp
#include <QTime>
```

- [ ] **Step 5: 实现 onPublishResult() 槽**

在 PublishPanel.cpp 中添加：

```cpp
void PublishPanel::onPublishResult(const QString& topic, bool success, const QString& message) {
    QString msg = success ? QString::fromUtf8("发送成功")
                          : QString::fromUtf8("发送失败: ") + message;
    appendHistory(topic, success, msg);
}
```

- [ ] **Step 6: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功

- [ ] **Step 7: 提交**

```bash
git add src/ui/PublishPanel.h src/ui/PublishPanel.cpp
git commit -m "feat: PublishPanel 发送历史区域 + 双击恢复 + 结果回调"
```

---

### Task 7: MainWindow — 接线 + 初始化

**Files:**
- Modify: `src/ui/MainWindow.cpp:403-482`

- [ ] **Step 1: 在 connectSignals() 中添加 PublishPanel 信号连接**

在 `brokerConnected` lambda 块内（第 468 行 `updateStatusBar()` 之前），添加：

```cpp
        mPublishPanel->setConnected(true);
```

在 `brokerDisconnected` lambda 块内（第 477 行，`mTopicParsePanel->pause()` 之前或之后），添加：

```cpp
        mPublishPanel->setConnected(false);
```

在 `connectSignals()` 末尾（见已有 brokerConnected/brokerDisconnected 之后合适位置）添加：

```cpp
    // PublishPanel → DeviceManager
    connect(mPublishPanel, &PublishPanel::publishRequested,
            mDevMgr, &DeviceManager::publishMessage);
    // DeviceManager → PublishPanel
    connect(mDevMgr, &DeviceManager::publishResult,
            mPublishPanel, &PublishPanel::onPublishResult);
```

- [ ] **Step 2: 在 setupLayout() 末尾添加模板加载和初始连接状态**

在 `setupLayout()` 函数末尾（`setCentralWidget(mainSplitter)` 之后），添加：

```cpp
    // 加载 publish 模板 + 初始连接状态
    mPublishPanel->loadTemplates(QApplication::applicationDirPath() + "/publish_templates.json");
    mPublishPanel->setConnected(mDevMgr->isConnected());
```

- [ ] **Step 3: 编译验证**

```bash
cmake --build build_mingw
```

Expected: 编译成功

- [ ] **Step 4: 提交**

```bash
git add src/ui/MainWindow.cpp
git commit -m "feat: MainWindow 接线 publish 信号链路 + 模板加载"
```

---

### Task 8: 全量编译 + 功能验证

- [ ] **Step 1: 清理重编译**

```bash
cmake --build build_mingw --clean-first
```

Expected: 编译成功，0 error, 0 warning

- [ ] **Step 2: 推送代码**

```bash
git push origin main
```

---

## 验证清单

编译成功后，功能行为预期：

1. **发送按钮**：MQTT 断开时禁用，连接后启用（还需 topic 和 JSON 非空）
2. **模板自动填入**：选择 topic 后编辑区为空时自动填入模板 JSON
3. **publish_templates.json**：首次运行时自动创建在 exe 同目录
4. **发送流程**：点击发送 → 状态栏无变化（MainWindow 未监听该信号）→ 发送历史显示结果
5. **发送历史**：成功显示绿色 ✅，失败显示红色 ❌，双击恢复 topic
6. **编辑区有内容时不覆盖**：切换 topic 时已有内容的编辑区不被模板覆盖

