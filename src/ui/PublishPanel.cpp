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
        QString pattern = mTopicToPattern.value(text);
        QString tpl = mTemplates.value(pattern.isEmpty() ? text : pattern);
        // 兜底：{gateway_sn} 的 pattern 可能只以 {sn} 形式存在于模板中
        if (tpl.isEmpty() && pattern.contains("{gateway_sn}")) {
            QString fallbackKey = pattern;
            fallbackKey.replace("{gateway_sn}", "{sn}");
            tpl = mTemplates.value(fallbackKey);
        }
        if (!tpl.isEmpty()) {
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

    // JSON 编辑区（固定高度，不参与拉伸）
    mEditor = new QPlainTextEdit(this);
    mEditor->setFont(QFont("Consolas", 10));
    mEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    mEditor->setPlaceholderText("");
    mEditor->setMinimumHeight(80);
    mEditor->setMaximumHeight(140);
    mEditor->setStyleSheet(
        "QPlainTextEdit { background: #fff; border: 1px solid #dadce0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 12px; "
        "padding: 6px; color: #333; }");
    layout->addWidget(mEditor, 0);

    // 发送按钮（顶到编辑区下方）
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
        mLastSentJson = json;
        emit publishRequested(topic, json);
    });

    // 下发记录标签
    auto* historyLabel = new QLabel(QString::fromUtf8("下发记录:"));
    historyLabel->setStyleSheet("font-size: 12px; color: #5f6368;");
    layout->addWidget(historyLabel);

    // 下发记录（只读），占据剩余空间
    mHistoryLog = new QPlainTextEdit(this);
    mHistoryLog->setReadOnly(true);
    mHistoryLog->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    mHistoryLog->setStyleSheet(
        "QPlainTextEdit { background: #fafafa; border: 1px solid #e0e0e0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 11px; "
        "padding: 4px; color: #333; }");
    mHistoryLog->viewport()->installEventFilter(this);
    layout->addWidget(mHistoryLog, 1);
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
            mTopicToPattern[topic] = tpl;
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

void PublishPanel::appendHistory(const QString& topic, const QString& json,
                                 bool success, const QString& message,
                                 const QString& replyJson) {
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");
    QString icon = success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");

    QString block;
    block += QStringLiteral("[%1] %2 %3\n").arg(timeStr, icon, message);
    block += QString::fromUtf8("Topic: %1\n").arg(topic);
    block += QString::fromUtf8("下发:\n%1\n").arg(json.trimmed());
    if (!replyJson.isEmpty())
        block += QString::fromUtf8("响应:\n%1\n").arg(replyJson.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    // 最新记录插入顶部
    mHistoryLog->moveCursor(QTextCursor::Start);
    mHistoryLog->insertPlainText(block);
    mHistoryLog->moveCursor(QTextCursor::Start);
}

void PublishPanel::appendCommandRecord(const QString& topic, const QString& requestJson,
                                        const QString& replyJson, bool success, const QString& message) {
    QString msg = success ? message
                          : QString::fromUtf8("失败（%1）").arg(message);
    appendHistory(topic, requestJson, success, msg, replyJson);
}

bool PublishPanel::eventFilter(QObject* obj, QEvent* event) {
    Q_UNUSED(obj) Q_UNUSED(event)
    return QWidget::eventFilter(obj, event);
}

void PublishPanel::loadTemplates(const QString& path) {
    mTemplates = builtinTemplates();

    QFile file(path);
    if (!file.exists()) {
        qWarning() << "PublishPanel: template file not found:" << path << ", using builtins";
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "PublishPanel: cannot open" << path << ", using builtins";
        return;
    }

    QString currentTopic;
    QString currentJson;
    bool    inCodeBlock = false;
    QMap<QString, QString> parsed;

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();

        if (!inCodeBlock && line.startsWith("*topic*:")) {
            if (!currentTopic.isEmpty() && !currentJson.isEmpty())
                parsed[currentTopic] = currentJson;
            currentTopic = line.mid(8).trimmed();
            currentJson.clear();
        }
        else if (!inCodeBlock && !currentTopic.isEmpty() &&
                 (line == "```json" || line == "```")) {
            inCodeBlock = true;
            currentJson.clear();
        }
        else if (inCodeBlock && line == "```") {
            inCodeBlock = false;
        }
        else if (inCodeBlock) {
            if (!currentJson.isEmpty())
                currentJson += "\n";
            currentJson += line;
        }
    }
    if (!currentTopic.isEmpty() && !currentJson.isEmpty())
        parsed[currentTopic] = currentJson;
    file.close();

    if (parsed.isEmpty()) {
        qWarning() << "PublishPanel: no templates parsed from MD, using builtins";
        return;
    }

    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        QString mdPattern = it.key();

        if (mdPattern.contains("drc/up"))
            continue;

        QString key = mdPattern;
        key.replace("{gateway_sn}", "{sn}");

        QString tpl = it.value();
        tpl.replace("\"gateway\": \"sn\"", "\"gateway\": \"{gateway_sn}\"");
        tpl.replace("\"gateway\":\"sn\"",  "\"gateway\":\"{gateway_sn}\"");
        tpl.replace("\"gateway\": \"SN\"", "\"gateway\": \"{gateway_sn}\"");
        tpl.replace("\"gateway\":\"SN\"",  "\"gateway\":\"{gateway_sn}\"");

        mTemplates[key] = tpl;
    }

    qDebug() << "PublishPanel: loaded" << parsed.size() << "templates from MD,"
             << mTemplates.size() << "total (with builtins)";
}
