#ifndef PUBLISHPANEL_H
#define PUBLISHPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QEvent>

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
    void appendHistory(const QString& topic, const QString& json, bool success, const QString& message,
                       const QString& replyJson = {});

    QComboBox*      mTopicCombo       = nullptr;
    QPlainTextEdit* mEditor           = nullptr;
    QPushButton*    mSendBtn          = nullptr;
    QPlainTextEdit* mHistoryLog       = nullptr;
    QString         mDeviceSn;
    QString         mGatewaySn;
    QString         mLastSentJson;
    bool            mConnected        = false;
    QMap<QString, QString> mTemplates;
    QMap<QString, QString> mTopicToPattern;
    static QMap<QString, QString> builtinTemplates();
};

#endif // PUBLISHPANEL_H
