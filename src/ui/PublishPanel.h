#ifndef PUBLISHPANEL_H
#define PUBLISHPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QTextEdit>
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
    void setTopics(const QStringList& subscribed);
    void setConnected(bool connected);
    void loadTemplates(const QString& path);

signals:
    void publishRequested(const QString& topic, const QString& json);

public slots:
    void onPublishResult(const QString& topic, bool success, const QString& message);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    void updateSendButtonState();
    void appendHistory(const QString& topic, bool success, const QString& message);

    QComboBox*      mTopicCombo    = nullptr;
    QPlainTextEdit* mEditor        = nullptr;
    QPushButton*    mSendBtn       = nullptr;
    QTextEdit*      mHistoryLog    = nullptr;
    QString         mDeviceSn;
    bool            mConnected     = false;
    QStringList     mHistoryLines;
    QMap<QString, QString> mTemplates;              // topic → template JSON
    static QMap<QString, QString> builtinTemplates();  // 内置默认模板
    static constexpr int MAX_HISTORY = 20;
};

#endif // PUBLISHPANEL_H
