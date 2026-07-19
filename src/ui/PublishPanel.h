#ifndef PUBLISHPANEL_H
#define PUBLISHPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMap>
#include <QList>
#include <QEvent>

// 发送历史条目
struct HistoryEntry {
    QString timeStr;
    QString topic;
    QString json;       // 发送的 JSON 参数
    bool    success;
    QString message;
};

class PublishPanel : public QWidget {
    Q_OBJECT
public:
    static const QStringList PUBLISH_PRESETS;

    explicit PublishPanel(QWidget* parent = nullptr);

    void setDeviceSn(const QString& sn);
    void setGatewaySn(const QString& sn);
    void setTopics(const QStringList& subscribed);
    void setConnected(bool connected);
    void loadTemplates(const QString& path);

    // 追加外部指令记录（供机场控制/飞行控制/运维模式使用）
    void appendCommandRecord(const QString& topic, const QString& requestJson,
                             const QString& replyJson, bool success, const QString& message);

signals:
    void publishRequested(const QString& topic, const QString& json);

public slots:
    void onPublishResult(const QString& topic, bool success, const QString& message);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    void updateSendButtonState();
    void appendHistory(const QString& topic, const QString& json, bool success, const QString& message);

    QComboBox*      mTopicCombo       = nullptr;
    QPlainTextEdit* mEditor           = nullptr;
    QPushButton*    mSendBtn          = nullptr;
    QGroupBox*      mHistoryGroup     = nullptr;
    QPlainTextEdit* mHistoryLog       = nullptr;
    QString         mDeviceSn;
    QString         mGatewaySn;
    QString         mLastSentJson;                 // 发送时暂存 JSON，供结果回调使用
    bool            mConnected        = false;
    QList<HistoryEntry> mHistoryEntries;           // 发送历史（含 JSON 参数）
    QList<int>          mHistoryBlockStarts;        // 每条记录在 mHistoryLog 中的起始 block 号
    QMap<QString, QString> mTemplates;             // topic pattern → template JSON
    QMap<QString, QString> mTopicToPattern;        // 替换后 topic → pattern（用于模板查找）
    static QMap<QString, QString> builtinTemplates();  // 内置默认模板
    static constexpr int MAX_HISTORY = 20;
};

#endif // PUBLISHPANEL_H
