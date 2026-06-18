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
#include <QTextCursor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

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

        mPauseBtn = new QPushButton("⏸ 暂停");
        mPauseBtn->setObjectName("copyBtn");
        mPauseBtn->setCursor(Qt::PointingHandCursor);
        mPauseBtn->setFixedWidth(80);

        auto* copyBtn = new QPushButton("📋 复制");
        copyBtn->setObjectName("copyBtn");
        copyBtn->setCursor(Qt::PointingHandCursor);
        copyBtn->setFixedWidth(80);

        auto* clearBtn = new QPushButton("🗑 清除");
        clearBtn->setObjectName("copyBtn");
        clearBtn->setCursor(Qt::PointingHandCursor);
        clearBtn->setFixedWidth(80);

        header->addWidget(title);
        header->addStretch();
        header->addWidget(mPauseBtn);
        header->addWidget(copyBtn);
        header->addWidget(clearBtn);
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

        connect(mPauseBtn, &QPushButton::clicked, this, [this]() {
            mPaused = !mPaused;
            mPauseBtn->setText(mPaused ? "▶ 继续" : "⏸ 暂停");
            if (!mPaused) {
                for (const auto& j : mPendingBuffer) {
                    if (!mEditor->toPlainText().isEmpty())
                        mEditor->appendPlainText("---");
                    mEditor->appendPlainText(j);
                }
                mPendingBuffer.clear();
                QTextCursor cursor = mEditor->textCursor();
                cursor.movePosition(QTextCursor::End);
                mEditor->setTextCursor(cursor);
            }
        });

        connect(clearBtn, &QPushButton::clicked, this, [this]() {
            mEditor->clear();
            mPendingBuffer.clear();
        });
    }

    // 格式化单条 JSON 的前后缀（topic + timestamp）
    static QString annotateJson(const QString& json, const QString& topic) {
        QString annotated;
        if (!topic.isEmpty())
            annotated += "[topic] " + topic + "\n";
        annotated += json;

        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (doc.isObject()) {
            QJsonValue tsVal = doc.object().value("timestamp");
            if (!tsVal.isUndefined()) {
                qint64 ts = tsVal.toVariant().toLongLong();
                QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts);
                annotated += "\n[timestamp] " + dt.toString("yyyy-MM-dd hh:mm:ss.zzz");
            }
        }
        return annotated;
    }

    void setJson(const QString& json, const QString& topic = {}) {
        if (json.isEmpty()) {
            mEditor->clear();
            return;
        }
        // 如果有 topic 信息，拆分历史条目并逐条标注
        if (!topic.isEmpty()) {
            QStringList entries = json.split("\n---\n");
            QStringList annotated;
            for (const QString& entry : entries) {
                if (!entry.trimmed().isEmpty())
                    annotated.append(annotateJson(entry, topic));
            }
            mEditor->setPlainText(annotated.join("\n---\n"));
        } else {
            mEditor->setPlainText(json);
        }
    }

    void clear() {
        mEditor->clear();
    }

    void appendJson(const QString& json, const QString& topic = {}) {
        if (json.isEmpty()) return;
        QString annotated = annotateJson(json, topic);
        if (mPaused) {
            mPendingBuffer.append(annotated);
            while (mPendingBuffer.size() > MAX_BUFFER)
                mPendingBuffer.removeFirst();
            return;
        }
        if (!mEditor->toPlainText().isEmpty())
            mEditor->appendPlainText("---");
        mEditor->appendPlainText(annotated);
        QTextCursor cursor = mEditor->textCursor();
        cursor.movePosition(QTextCursor::End);
        mEditor->setTextCursor(cursor);
    }

    void clearHistory() {
        mEditor->clear();
    }

private:
    QPlainTextEdit* mEditor;
    QPushButton*    mPauseBtn       = nullptr;
    bool            mPaused         = false;
    QStringList     mPendingBuffer;
    static constexpr int MAX_BUFFER = 1000;
};

#endif // RAWJSONPANEL_H
