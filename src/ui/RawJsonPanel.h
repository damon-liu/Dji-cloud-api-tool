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
#include <QFile>
#include <QDir>

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

        mCaptureBtn = new QPushButton("⬤ 抓包");
        mCaptureBtn->setObjectName("copyBtn");
        mCaptureBtn->setCursor(Qt::PointingHandCursor);
        mCaptureBtn->setFixedWidth(80);

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
        header->addWidget(mCaptureBtn);
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
                for (const auto& j : mPendingBuffer)
                    mEditor->appendPlainText(j);
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

        connect(mCaptureBtn, &QPushButton::clicked, this, [this]() {
            if (mCapturing) {
                stopCapture();
            } else {
                startCapture();
            }
        });
    }

    static constexpr const char* SEPARATOR =
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";

    // 格式化单条 JSON：topic 头 + JSON + 可读时间尾
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
                annotated += "\n[time] " + dt.toString("yyyy-MM-dd hh:mm:ss");
            }
        }
        annotated += "\n" + QString::fromUtf8(SEPARATOR);
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
            mEditor->setPlainText(annotated.join("\n"));
        } else {
            mEditor->setPlainText(json);
        }
    }

    void clear() {
        mEditor->clear();
    }

    // 设置抓包目标（TopicListWidget 选中 topic 时由 MainWindow 调用）
    void setCaptureTarget(const QString& sn, const QString& topic) {
        mCaptureSn = sn;
        mCaptureTopic = topic;
    }

    void appendJson(const QString& json, const QString& topic = {}) {
        if (json.isEmpty()) return;

        // 抓包写入文件（仅在被抓包 topic 匹配时写入，与暂停无关）
        if (mCapturing && !mCaptureTopic.isEmpty() && topic == mCaptureTopic) {
            if (mCaptureFile && mCaptureFile->isOpen()) {
                if (mCaptureBytesWritten > 0)
                    mCaptureFile->write(",\n");
                mCaptureFile->write(json.toUtf8());
                mCaptureBytesWritten++;
            }
        }

        QString annotated = annotateJson(json, topic);
        if (mPaused) {
            mPendingBuffer.append(annotated);
            while (mPendingBuffer.size() > MAX_BUFFER)
                mPendingBuffer.removeFirst();
            return;
        }
        mEditor->appendPlainText(annotated);
        QTextCursor cursor = mEditor->textCursor();
        cursor.movePosition(QTextCursor::End);
        mEditor->setTextCursor(cursor);
    }

    void clearHistory() {
        mEditor->clear();
    }

private:
    void startCapture() {
        if (mCaptureSn.isEmpty() || mCaptureTopic.isEmpty())
            return;

        // 从 topic 路径末尾提取类型缩写（如 osd, state, events 等）
        QString topicShort = mCaptureTopic.section('/', -1);
        QString ts = QDateTime::currentDateTime().toString("MMddHHmmss");
        QString filename = mCaptureSn + "-" + topicShort + "-" + ts + ".json";

        QString dir = QApplication::applicationDirPath() + "/captures";
        QDir().mkpath(dir);

        mCaptureFile = new QFile(dir + "/" + filename, this);
        if (!mCaptureFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            delete mCaptureFile;
            mCaptureFile = nullptr;
            return;
        }

        mCapturing = true;
        mCaptureBytesWritten = 0;
        mCaptureBtn->setStyleSheet(
            "QPushButton { background: #c62828; color: #fff; border: none; "
            "border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
            "QPushButton:hover { background: #b71c1c; }");
        mCaptureBtn->setText("⏹ 停止");
    }

    void stopCapture() {
        mCapturing = false;
        if (mCaptureFile) {
            mCaptureFile->close();
            delete mCaptureFile;
            mCaptureFile = nullptr;
        }
        mCaptureBytesWritten = 0;
        mCaptureBtn->setText("⬤ 抓包");
        mCaptureBtn->setStyleSheet("");
    }

    QPlainTextEdit* mEditor;
    QPushButton*    mPauseBtn         = nullptr;
    QPushButton*    mCaptureBtn       = nullptr;
    QFile*          mCaptureFile      = nullptr;
    bool            mPaused           = false;
    bool            mCapturing        = false;
    int             mCaptureBytesWritten = 0;
    QString         mCaptureSn;
    QString         mCaptureTopic;
    QStringList     mPendingBuffer;
    static constexpr int MAX_BUFFER = 1000;
};

#endif // RAWJSONPANEL_H
