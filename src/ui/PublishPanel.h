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

signals:
    void publishRequested(const QString& topic, const QString& json);
    void historyRequested();
    void publishCompleted(const QString& topic, const QString& json,
                          bool success, const QString& message);

public slots:
    void onPublishResult(const QString& topic, bool success, const QString& message);

private:
    void setupUi();
    void updateSendButtonState();

    QComboBox*      mTopicCombo       = nullptr;
    QPlainTextEdit* mEditor           = nullptr;
    QPushButton*    mSendBtn          = nullptr;
    QPushButton*    mHistoryBtn       = nullptr;
    QLabel*         mResultLabel      = nullptr;
    QString         mDeviceSn;
    QString         mGatewaySn;
    QString         mLastSentJson;
    bool            mConnected        = false;
    QMap<QString, QString> mTemplates;
    QMap<QString, QString> mTopicToPattern;
    static QMap<QString, QString> builtinTemplates();
};

#endif // PUBLISHPANEL_H
