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
    "sys/product/{sn}/status_reply",
};

QMap<QString, QString> PublishPanel::builtinTemplates() {
    return {
        {"thing/product/{sn}/property/set",  "{\n  \n}"},
        {"thing/product/{sn}/services",       "{\n  \"services\": []\n}"},
        {"thing/product/{sn}/events_reply",   "{\n  \"events_reply\": []\n}"},
        {"thing/product/{sn}/requests_reply", "{\n  \"requests_reply\": []\n}"},
        {"sys/product/{sn}/status_reply",     "{\n  \"status_reply\": {}\n}"},
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
        QString tpl = mTemplates.value(text);
        if (!tpl.isEmpty())
            mEditor->setPlainText(tpl);
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
    mEditor->setPlaceholderText(QString::fromUtf8("输入要发送的 JSON..."));
    mEditor->setStyleSheet(
        "QPlainTextEdit { background: #fff; border: 1px solid #dadce0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 12px; padding: 6px; }");
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
        emit publishRequested(topic, json);
    });

    // 发送历史（只读）
    mHistoryLog = new QTextEdit(this);
    mHistoryLog->setReadOnly(true);
    mHistoryLog->setMaximumHeight(80);
    mHistoryLog->setFont(QFont("Consolas", 9));
    mHistoryLog->setStyleSheet(
        "QTextEdit { background: #fafafa; border: 1px solid #e0e0e0; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 11px; "
        "padding: 4px; color: #333; }");
    mHistoryLog->setPlaceholderText(QString::fromUtf8("发送历史（双击恢复）"));
    mHistoryLog->viewport()->installEventFilter(this);
    layout->addWidget(mHistoryLog);
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
    appendHistory(topic, success, msg);
}

void PublishPanel::appendHistory(const QString& topic, bool success, const QString& message) {
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");
    QString icon    = success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");
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
