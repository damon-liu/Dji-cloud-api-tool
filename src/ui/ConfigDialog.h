#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QMqttClient>
#include <QTimer>
#include "ConfigStore.h"

class DeviceManager;

class ConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(DeviceManager* devMgr, QWidget* parent = nullptr);

    MqttConfig getConfig() const;
    QString selectedProfile() const { return mSelectedProfile; }

private slots:
    void onTestClicked();
    void onProfileSelected(const QString& name);
    void onAddProfile();
    void onRenameProfile();
    void onDeleteProfile();

private:
    DeviceManager* mDevMgr;
    QString        mSelectedProfile;

    QComboBox*     mProfileCombo;
    QPushButton*   mAddProfileBtn;
    QPushButton*   mRenameProfileBtn;
    QPushButton*   mDeleteProfileBtn;

    QLineEdit*     mHostEdit;
    QSpinBox*      mPortSpin;
    QLineEdit*     mUsernameEdit;
    QLineEdit*     mPasswordEdit;
    QPushButton*   mTestBtn;
    QMqttClient*   mTestClient    = nullptr;
    QTimer*        mTestTimer     = nullptr;
    bool           mTestRunning   = false;

    void startTest();
    void cleanupTest();
    void loadProfile(const QString& name);
    void refreshProfileList();
};

#endif // CONFIGDIALOG_H
