#ifndef RAWJSONPANEL_H
#define RAWJSONPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QClipboard>
#include <QApplication>
#include <QFont>

class RawJsonPanel : public QWidget {
    Q_OBJECT
public:
    explicit RawJsonPanel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        // 标题栏
        auto* header = new QHBoxLayout;
        auto* title  = new QLabel("原始 JSON");
        title->setObjectName("sectionTitle");
        title->setStyleSheet("font-size: 13px; font-weight: bold; color: #5f6368;");

        auto* copyBtn = new QPushButton("📋 复制");
        copyBtn->setObjectName("copyBtn");
        copyBtn->setCursor(Qt::PointingHandCursor);
        copyBtn->setFixedWidth(80);

        header->addWidget(title);
        header->addStretch();
        header->addWidget(copyBtn);
        layout->addLayout(header);

        // JSON 编辑器
        mEditor = new QPlainTextEdit(this);
        mEditor->setReadOnly(true);
        mEditor->setFont(QFont("Consolas", 11));
        mEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
        mEditor->setPlaceholderText("选中设备后显示原始 JSON...");
        layout->addWidget(mEditor, 1);

        connect(copyBtn, &QPushButton::clicked, this, [this]() {
            QClipboard* clip = QApplication::clipboard();
            clip->setText(mEditor->toPlainText());
        });
    }

    void setJson(const QString& json) {
        if (json.isEmpty()) {
            mEditor->clear();
            return;
        }
        // 尝试格式化 JSON
        mEditor->setPlainText(json);
    }

    void clear() {
        mEditor->clear();
    }

private:
    QPlainTextEdit* mEditor;
};

#endif // RAWJSONPANEL_H
