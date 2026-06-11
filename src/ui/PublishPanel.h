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
    explicit PublishPanel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
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

        connect(mSendBtn, &QPushButton::clicked, this, [this]() {
            // TODO v1.1: 实现 MQTT publish
        });
    }

    void setTopics(const QStringList& topics) {
        mTopicCombo->clear();
        mTopicCombo->addItems(topics);
    }

private:
    QComboBox*      mTopicCombo;
    QPlainTextEdit* mEditor;
    QPushButton*    mSendBtn;
};

#endif // PUBLISHPANEL_H
