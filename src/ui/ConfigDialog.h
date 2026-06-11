#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QMqttClient>
#include <QTimer>
#include "ConfigStore.h"

class ConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(const MqttConfig& config, QWidget* parent = nullptr);

    MqttConfig getConfig() const;

private slots:
    void onTestClicked();

private:
    QLineEdit*   mHostEdit;
    QSpinBox*    mPortSpin;
    QLineEdit*   mUsernameEdit;
    QLineEdit*   mPasswordEdit;
    QPushButton* mTestBtn;
    QMqttClient* mTestClient    = nullptr;
    QTimer*      mTestTimer     = nullptr;
    bool         mTestRunning   = false;

    void startTest();
    void cleanupTest();
};

#endif // CONFIGDIALOG_H
