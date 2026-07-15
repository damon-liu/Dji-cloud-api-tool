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

    // topic 切换时自动填入模板（仅编辑区为空时）
    connect(mTopicCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        if (!mEditor->toPlainText().trimmed().isEmpty())
            return;
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

    // 发送按钮 + 历史展开按钮
    auto* btnLayout = new QHBoxLayout;

    mHistoryToggleBtn = new QPushButton(QString::fromUtf8("📋 历史"), this);
    mHistoryToggleBtn->setCheckable(true);
    mHistoryToggleBtn->setCursor(Qt::PointingHandCursor);
    mHistoryToggleBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px; "
        "padding: 4px 12px; font-size: 12px; background: #fff; color: #5f6368; }"
        "QPushButton:hover { background: #f1f3f4; }"
        "QPushButton:checked { background: #e8f0fe; color: #1a73e8; border-color: #1a73e8; }");
    connect(mHistoryToggleBtn, &QPushButton::toggled, this, [this](bool checked) {
        mHistoryLog->setVisible(checked);
        if (checked)
            mHistoryTimer->stop();
    });
    btnLayout->addWidget(mHistoryToggleBtn);

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

    // 发送历史（只读），默认隐藏
    mHistoryLog = new QTextEdit(this);
    mHistoryLog->setReadOnly(true);
    mHistoryLog->setMaximumHeight(100);
    mHistoryLog->setFont(QFont("Consolas", 9));
    mHistoryLog->setStyleSheet(
        "QTextEdit { background: #fafafa; border: 1px solid #e0e0e0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 11px; "
        "padding: 4px; color: #333; }");
    mHistoryLog->setPlaceholderText(QString::fromUtf8("发送历史（双击恢复 topic 和参数）"));
    mHistoryLog->viewport()->installEventFilter(this);
    mHistoryLog->setVisible(false);
    layout->addWidget(mHistoryLog);

    // 成功发送后 3s 自动隐藏历史
    mHistoryTimer = new QTimer(this);
    mHistoryTimer->setSingleShot(true);
    connect(mHistoryTimer, &QTimer::timeout, this, [this]() {
        mHistoryLog->setVisible(false);
        mHistoryToggleBtn->setChecked(false);
    });
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

    // 成功发送后显示历史并 3s 自动隐藏
    if (success) {
        mHistoryLog->setVisible(true);
        mHistoryToggleBtn->setChecked(true);
        mHistoryTimer->start(3000);
    }
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

    // 用富文本展示（含 JSON 摘要）
    QString html;
    for (const auto& e : mHistoryEntries) {
        QString icon = e.success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");
        QString color = e.success ? "#1e8e3e" : "#d93025";
        // JSON 截短显示（最多 80 字符）
        QString jsonPreview = e.json.left(80);
        if (e.json.length() > 80)
            jsonPreview += "...";
        html += QString("<span style='color:%1'>[%2] %3 %4  %5</span>"
                        "<span style='color:#80868b; font-size:10px;'>  %6</span><br>")
                    .arg(color, e.timeStr, icon, e.topic, e.message, jsonPreview.toHtmlEscaped());
    }
    mHistoryLog->setHtml(html);
}

bool PublishPanel::eventFilter(QObject* obj, QEvent* event) {
    if (obj == mHistoryLog->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QTextCursor cursor = mHistoryLog->cursorForPosition(
            static_cast<QMouseEvent*>(event)->pos());
        int blockNum = cursor.block().blockNumber();
        if (blockNum >= 0 && blockNum < mHistoryEntries.size()) {
            const HistoryEntry& entry = mHistoryEntries[blockNum];
            if (!entry.topic.isEmpty())
                mTopicCombo->setCurrentText(entry.topic);
            if (!entry.json.isEmpty())
                mEditor->setPlainText(entry.json);
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
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
