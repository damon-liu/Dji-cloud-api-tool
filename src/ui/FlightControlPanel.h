#ifndef FLIGHTCONTROLPANEL_H
#define FLIGHTCONTROLPANEL_H

#include <QWidget>
#include <QComboBox>
#include "DockCommand.h"
#include "DeviceInfo.h"

class QLabel;
class QPushButton;

class FlightControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit FlightControlPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);
    void updateDockPosition(double lat, double lon, double alt,
                            const QString& latStr, const QString& lonStr,
                            const QString& altStr);
    void setAvailableDocks(const QVector<DeviceInfo>& docks, const QString& currentSn,
                           double dockLat, double dockLon, double dockAlt,
                           const QString& dockLatStr, const QString& dockLonStr,
                           const QString& dockAltStr);
    QString currentGatewaySn() const { return mGatewaySn; }

public slots:
    void onCommandStateChanged(const DockCommandResult& result);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});
    void historyRequested();

private:
    void setupUi();
    void requestCommand(DockCommandType type, const QJsonObject& data = {});
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);

    // --- top row ---
    QComboBox*    mDockCombo = nullptr;
    QLabel*       mOnlineLabel = nullptr;

    // --- status ---
    QLabel*       mStatusLabel = nullptr;

    // --- flight command section ---
    QPushButton*  mFlightAuthToggleBtn = nullptr;
    QPushButton*  mTakeoffBtn = nullptr;
    QPushButton*  mReturnHomeBtn = nullptr;
    QPushButton*  mCancelReturnBtn = nullptr;
    QPushButton*  mEmergencyStopBtn = nullptr;

    // --- payload section ---
    QPushButton*  mPayloadAuthToggleBtn = nullptr;
    QPushButton*  mCameraPhotoBtn = nullptr;
    QPushButton*  mCameraRecordStartBtn = nullptr;
    QPushButton*  mCameraRecordStopBtn = nullptr;
    QPushButton*  mGimbalCenterBtn = nullptr;
    QPushButton*  mGimbalDownBtn = nullptr;
    QPushButton*  mGimbalYawCenterBtn = nullptr;
    QPushButton*  mGimbalPitchDownBtn = nullptr;

    // --- data ---
    QVector<DeviceInfo> mAvailableDocks;
    QString mDisplayName;
    QString mGatewaySn;
    double  mDockLat = 0.0;
    double  mDockLon = 0.0;
    double  mDockAlt = 0.0;
    QString mDockLatStr;
    QString mDockLonStr;
    QString mDockAltStr;
    bool    mConnected = false;
    bool    mOnline = false;
    bool    mPending = false;
    bool    mHasFlightAuthority = false;
    bool    mHasPayloadAuthority = false;
    bool    mUpdatingCombo = false;
};

#endif // FLIGHTCONTROLPANEL_H
