#include "PublishPanel.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QTime>
#include <QEvent>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QGroupBox>

const QStringList PublishPanel::PUBLISH_PRESETS = {
    "thing/product/{gateway_sn}/services",
    "thing/product/{sn}/property/set",
    "thing/product/{sn}/services",
    "thing/product/{sn}/events_reply",
    "thing/product/{sn}/requests_reply",
    "thing/product/{gateway_sn}/drc/down",
    "sys/product/{sn}/status_reply",
};

QMap<QString, QString> PublishPanel::builtinTemplates() {
    return {
        {"thing/product/{sn}/property/set",        "{\n    \"tid\": \"6a7bfe89-c386-4043-b600-b518e10096cc\",\n    \"bid\": \"42a19f36-5117-4520-bd13-fd61d818d52e\",\n    \"timestamp\": 1598411295123,\n    \"data\": {\n        \"some_property\": 0\n    }\n}"},
        {"thing/product/{sn}/services",             "{\n    \"tid\": \"6a7bfe89-c386-4043-b600-b518e10096cc\",\n    \"bid\": \"42a19f36-5117-4520-bd13-fd61d818d52e\",\n    \"timestamp\": 1598411295123,\n    \"gateway\": \"{gateway_sn}\",\n    \"method\": \"some_method\",\n    \"data\": {}\n}"},
        {"thing/product/{sn}/events_reply",         "{\n    \"tid\": \"6a7bfe89-c386-4043-b600-b518e10096cc\",\n    \"bid\": \"42a19f36-5117-4520-bd13-fd61d818d52e\",\n    \"timestamp\": 1598411295123,\n    \"gateway\": \"{gateway_sn}\",\n    \"method\": \"some_method\",\n    \"data\": {\n        \"result\": 0\n    }\n}"},
        {"thing/product/{sn}/requests_reply",       "{\n    \"tid\": \"6a7bfe89-c386-4043-b600-b518e10096cc\",\n    \"bid\": \"42a19f36-5117-4520-bd13-fd61d818d52e\",\n    \"timestamp\": 1598411295123,\n    \"gateway\": \"{gateway_sn}\",\n    \"method\": \"some_method\",\n    \"data\": {\n        \"result\": 0,\n        \"output\": {}\n    }\n}"},
        {"thing/product/{gateway_sn}/drc/down",     "{\n    \"method\": \"drone_control\",\n    \"data\": {}\n}"},
        {"sys/product/{sn}/status_reply",           "{\n    \"tid\": \"6a7bfe89-c386-4043-b600-b518e10096cc\",\n    \"bid\": \"42a19f36-5117-4520-bd13-fd61d818d52e\",\n    \"timestamp\": 1598411295123,\n    \"method\": \"update_topo\",\n    \"data\": {\n        \"result\": 0\n    }\n}"},
    };
}

PublishPanel::PublishPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();

    // topic 切换时自动填入对应模板
    connect(mTopicCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        // 先通过反向映射找到 pattern，再用 pattern 查模板
        QString pattern = mTopicToPattern.value(text);
        QString tpl = mTemplates.value(pattern.isEmpty() ? text : pattern);
        if (!tpl.isEmpty()) {
            // 从 topic 中提取 gateway SN（格式: thing/product/{gateway_sn}/... 或 sys/product/{gateway_sn}/...）
            QStringList parts = text.split('/');
            if (parts.size() >= 3)
                tpl.replace("{gateway_sn}", parts[2]);
            mEditor->setPlainText(tpl);
        }
    });

    // 编辑区内容变化时更新按钮状态
    connect(mEditor, &QPlainTextEdit::textChanged, this, [this]() {
        updateSendButtonState();
    });
    // topic 变化时也更新按钮状态
    connect(mTopicCombo, &QComboBox::currentTextChanged, this, [this]() {
        updateSendButtonState();
    });
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
    mEditor->setPlaceholderText("");
    mEditor->setStyleSheet(
        "QPlainTextEdit { background: #fff; border: 1px solid #dadce0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 12px; "
        "padding: 6px; color: #333; }");
    layout->addWidget(mEditor, 1);

    // 发送按钮
    auto* btnLayout = new QHBoxLayout;

    btnLayout->addStretch();
    mSendBtn = new QPushButton(QString::fromUtf8("发送"), this);
    mSendBtn->setEnabled(false);
    mSendBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; border: none; "
        "border-radius: 4px; padding: 6px 24px; font-weight: bold; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    btnLayout->addWidget(mSendBtn);
    layout->addLayout(btnLayout);

    // 发送按钮：发射 publishRequested 信号
    connect(mSendBtn, &QPushButton::clicked, this, [this]() {
        QString topic = mTopicCombo->currentText().trimmed();
        QString json  = mEditor->toPlainText().trimmed();
        if (topic.isEmpty() || json.isEmpty())
            return;
        mLastSentJson = json;  // 暂存 JSON，供结果回调使用
        emit publishRequested(topic, json);
    });

    // 下发记录（只读），始终可见
    mHistoryGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(mHistoryGroup);
    mHistoryLog = new QPlainTextEdit(mHistoryGroup);
    mHistoryLog->setReadOnly(true);
    mHistoryLog->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    mHistoryLog->setStyleSheet(
        "QPlainTextEdit { background: #fafafa; border: 1px solid #e0e0e0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 11px; "
        "padding: 4px; color: #333; }");
    mHistoryLog->viewport()->installEventFilter(this);
    historyLayout->addWidget(mHistoryLog);
    layout->addWidget(mHistoryGroup, 1);
}

void PublishPanel::setDeviceSn(const QString& sn) {
    mDeviceSn = sn;
}

void PublishPanel::setTopics(const QStringList& /*subscribed*/) {
    mTopicCombo->clear();
    mTopicToPattern.clear();
    if (!mDeviceSn.isEmpty()) {
        for (const auto& tpl : PUBLISH_PRESETS) {
            QString topic = QString(tpl).replace("{sn}", mDeviceSn);
            if (!mGatewaySn.isEmpty())
                topic.replace("{gateway_sn}", mGatewaySn);
            mTopicToPattern[topic] = tpl;  // 反向映射必须先于 addItem（addItem 触发 currentTextChanged）
            mTopicCombo->addItem(topic);
        }
    }
}

void PublishPanel::setGatewaySn(const QString& sn) {
    mGatewaySn = sn;
}

void PublishPanel::setConnected(bool connected) {
    mConnected = connected;
    updateSendButtonState();
}

void PublishPanel::updateSendButtonState() {
    bool topicOk = !mTopicCombo->currentText().trimmed().isEmpty();
    bool jsonOk  = !mEditor->toPlainText().trimmed().isEmpty();
    mSendBtn->setEnabled(mConnected && topicOk && jsonOk);
}

void PublishPanel::onPublishResult(const QString& topic, bool success, const QString& message) {
    QString msg = success ? QString::fromUtf8("发送成功")
                          : QString::fromUtf8("发送失败: ") + message;
    appendHistory(topic, mLastSentJson, success, msg);
    mLastSentJson.clear();
}

void PublishPanel::appendHistory(const QString& topic, const QString& json, bool success, const QString& message) {
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");

    HistoryEntry entry;
    entry.timeStr = timeStr;
    entry.topic   = topic;
    entry.json    = json;
    entry.success = success;
    entry.message = message;

    mHistoryEntries.prepend(entry);
    if (mHistoryEntries.size() > MAX_HISTORY)
        mHistoryEntries.removeLast();

    // 重建 block 起始索引（最新记录在最前面）
    mHistoryBlockStarts.clear();
    mHistoryLog->clear();

    for (const auto& e : mHistoryEntries) {
        mHistoryBlockStarts.append(mHistoryLog->blockCount());

        QString icon = e.success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");
        QString block;
        block += QStringLiteral("[%1] %2 %3\n")
            .arg(e.timeStr, icon, e.message);
        block += QString::fromUtf8("Topic: %1\n").arg(e.topic);
        block += QString::fromUtf8("下发:\n%1\n").arg(e.json.trimmed());
        block += QString::fromUtf8("────────────────────────────\n");

        mHistoryLog->moveCursor(QTextCursor::End);
        mHistoryLog->insertPlainText(block);
    }

    // 滚动到顶部显示最新记录
    mHistoryLog->moveCursor(QTextCursor::Start);
}

void PublishPanel::appendCommandRecord(const QString& topic, const QString& requestJson,
                                        const QString& replyJson, bool success, const QString& message) {
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");
    QString icon = success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");

    QString block;
    block += QStringLiteral("[%1] %2 %3\n").arg(timeStr, icon, message);
    block += QString::fromUtf8("Topic: %1\n").arg(topic);
    block += QString::fromUtf8("下发:\n%1\n").arg(requestJson.trimmed());
    block += QString::fromUtf8("响应:\n%1\n").arg(replyJson.isEmpty()
        ? QString::fromUtf8("（无响应）") : replyJson.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    // 最新记录插入顶部
    mHistoryLog->moveCursor(QTextCursor::Start);
    mHistoryLog->insertPlainText(block);
    mHistoryLog->moveCursor(QTextCursor::Start);
}

bool PublishPanel::eventFilter(QObject* obj, QEvent* event) {
    if (obj == mHistoryLog->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QTextCursor cursor = mHistoryLog->cursorForPosition(
            static_cast<QMouseEvent*>(event)->pos());
        int blockNum = cursor.block().blockNumber();
        // 查找该 block 属于哪条历史记录
        for (int i = mHistoryBlockStarts.size() - 1; i >= 0; --i) {
            if (blockNum >= mHistoryBlockStarts[i]) {
                if (i < mHistoryEntries.size()) {
                    const HistoryEntry& entry = mHistoryEntries[i];
                    if (!entry.topic.isEmpty())
                        mTopicCombo->setCurrentText(entry.topic);
                    if (!entry.json.isEmpty())
                        mEditor->setPlainText(entry.json);
                }
                break;
            }
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void PublishPanel::loadTemplates(const QString& path) {
    mTemplates = builtinTemplates();  // 兜底（含 drc/down 等 MD 中没有的模板）

    QFile file(path);
    if (!file.exists()) {
        qWarning() << "PublishPanel: template file not found:" << path << ", using builtins";
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "PublishPanel: cannot open" << path << ", using builtins";
        return;
    }

    // ——— 解析 topic-send-construct.md ———
    // 格式: *topic*: <pattern> 后跟 ```json ... ``` 代码块
    QString currentTopic;
    QString currentJson;
    bool    inCodeBlock = false;
    QMap<QString, QString> parsed;  // MD pattern → template JSON

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();

        // 匹配 *topic*: <pattern>
        if (!inCodeBlock && line.startsWith("*topic*:")) {
            if (!currentTopic.isEmpty() && !currentJson.isEmpty())
                parsed[currentTopic] = currentJson;
            currentTopic = line.mid(8).trimmed();   // 去掉 "*topic*:" 前缀
            currentJson.clear();
        }
        // 代码块开始
        else if (!inCodeBlock && !currentTopic.isEmpty() &&
                 (line == "```json" || line == "```")) {
            inCodeBlock = true;
            currentJson.clear();
        }
        // 代码块结束
        else if (inCodeBlock && line == "```") {
            inCodeBlock = false;
        }
        // 代码块内容
        else if (inCodeBlock) {
            if (!currentJson.isEmpty())
                currentJson += "\n";
            currentJson += line;
        }
    }
    // 保存最后一个
    if (!currentTopic.isEmpty() && !currentJson.isEmpty())
        parsed[currentTopic] = currentJson;
    file.close();

    if (parsed.isEmpty()) {
        qWarning() << "PublishPanel: no templates parsed from MD, using builtins";
        return;
    }

    // 将 MD 模板合并到 mTemplates（key 映射为 PUBLISH_PRESETS 的 {sn} 格式）
    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        QString mdPattern = it.key();

        // 跳过 drc/up（上行 topic，下发用的是 drc/down，保留 builtin 兜底）
        if (mdPattern.contains("drc/up"))
            continue;

        // key 映射: {gateway_sn} → {sn}，匹配 PUBLISH_PRESETS
        QString key = mdPattern;
        key.replace("{gateway_sn}", "{sn}");

        QString tpl = it.value();
        // "gateway":"sn" → "gateway":"{gateway_sn}" 占位符（运行时由构造函数提取实际 SN）
        tpl.replace("\"gateway\": \"sn\"", "\"gateway\": \"{gateway_sn}\"");
        tpl.replace("\"gateway\":\"sn\"",  "\"gateway\":\"{gateway_sn}\"");
        tpl.replace("\"gateway\": \"SN\"", "\"gateway\": \"{gateway_sn}\"");
        tpl.replace("\"gateway\":\"SN\"",  "\"gateway\":\"{gateway_sn}\"");

        mTemplates[key] = tpl;
    }

    qDebug() << "PublishPanel: loaded" << parsed.size() << "templates from MD,"
             << mTemplates.size() << "total (with builtins)";
}
