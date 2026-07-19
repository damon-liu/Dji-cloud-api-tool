#ifndef DOCKCONTROLPANEL_H
#define DOCKCONTROLPANEL_H

#include <QWidget>
#include <QComboBox>
#include "DockCommand.h"
#include "DeviceInfo.h"

class QLabel;
class QPushButton;
class QPlainTextEdit;

class DockControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit DockControlPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);
    void setAvailableDocks(const QVector<DeviceInfo>& docks, const QString& currentSn,
                           double dockLat, double dockLon);
    QString currentGatewaySn() const { return mGatewaySn; }

public slots:
    void onCommandStateChanged(const DockCommandResult& result);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});

private:
    enum class DebugModeState { Unknown, Disabled, Enabled };

    void setupUi();
    void requestCommand(DockCommandType type, const QJsonObject& data = {});
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);
    void appendHistory(const DockCommandResult& result);

    // --- top row ---
    QComboBox*    mDockCombo = nullptr;
    QLabel*       mOnlineLabel = nullptr;

    // --- status ---
    QLabel*       mDebugModeLabel = nullptr;
    QLabel*       mStatusLabel = nullptr;

    // --- debug mode ---
    QPushButton*  mDebugOpenBtn = nullptr;
    QPushButton*  mDebugCloseBtn = nullptr;

    // --- device controls ---
    QPushButton*  mDroneOpenBtn = nullptr;
    QPushButton*  mDroneCloseBtn = nullptr;
    QPushButton*  mCoverOpenBtn = nullptr;
    QPushButton*  mCoverCloseBtn = nullptr;
    QPushButton*  mCoverForceBtn = nullptr;
    QPushButton*  mChargeOpenBtn = nullptr;
    QPushButton*  mChargeCloseBtn = nullptr;
    QPushButton*  mRebootBtn = nullptr;

    // --- fill light controls ---
    QPushButton*  mFillLightOpenBtn = nullptr;
    QPushButton*  mFillLightCloseBtn = nullptr;

    QPushButton*    mExpandBtn = nullptr;
    QWidget*        mExpandRow = nullptr;

    QPlainTextEdit* mHistoryEdit = nullptr;

    // --- data ---
    QVector<DeviceInfo> mAvailableDocks;
    QString mDisplayName;
    QString mGatewaySn;
    double  mDockLat = 0.0;
    double  mDockLon = 0.0;
    bool    mConnected = false;
    bool    mOnline = false;
    bool    mPending = false;
    DebugModeState mDebugModeState = DebugModeState::Unknown;
    bool    mUpdatingCombo = false;
};

#endif // DOCKCONTROLPANEL_H
