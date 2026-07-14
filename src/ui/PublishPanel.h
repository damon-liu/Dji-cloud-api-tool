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
    static const QStringList PUBLISH_PRESETS;

    explicit PublishPanel(QWidget* parent = nullptr);

    void setDeviceSn(const QString& sn);
    void setTopics(const QStringList& subscribed);

signals:
    void publishRequested(const QString& topic, const QString& json);

private:
    void setupUi();
    void updateSendButtonState();

    QComboBox*      mTopicCombo    = nullptr;
    QPlainTextEdit* mEditor        = nullptr;
    QPushButton*    mSendBtn       = nullptr;
    QString         mDeviceSn;
};

#endif // PUBLISHPANEL_H
