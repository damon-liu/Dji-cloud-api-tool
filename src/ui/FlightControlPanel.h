#ifndef FLIGHTCONTROLPANEL_H
#define FLIGHTCONTROLPANEL_H

#include <QWidget>
#include <QComboBox>
#include "DockCommand.h"
#include "DeviceInfo.h"

class QLabel;
class QPushButton;
class QPlainTextEdit;

class FlightControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit FlightControlPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);
    void updateDockPosition(double lat, double lon, double alt);
    void setAvailableDocks(const QVector<DeviceInfo>& docks, const QString& currentSn,
                           double dockLat, double dockLon, double dockAlt);
    QString currentGatewaySn() const { return mGatewaySn; }

public slots:
    void onCommandStateChanged(const DockCommandResult& result);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});

private:
    void setupUi();
    void requestCommand(DockCommandType type, const QJsonObject& data = {});
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);
    void appendHistory(const DockCommandResult& result);

    // --- top row ---
    QComboBox*    mDockCombo = nullptr;
    QLabel*       mOnlineLabel = nullptr;

    // --- status ---
    QLabel*       mStatusLabel = nullptr;

    // --- flight command section ---
    QLabel*       mFlightAuthStatusLabel = nullptr;
    QPushButton*  mFlightAuthGrabBtn = nullptr;
    QPushButton*  mFlightAuthReleaseBtn = nullptr;
    QPushButton*  mTakeoffBtn = nullptr;
    QPushButton*  mReturnHomeBtn = nullptr;
    QPushButton*  mCancelReturnBtn = nullptr;
    QPushButton*  mEmergencyStopBtn = nullptr;

    // --- payload section ---
    QLabel*       mPayloadAuthStatusLabel = nullptr;
    QPushButton*  mPayloadAuthGrabBtn = nullptr;
    QPushButton*  mPayloadAuthReleaseBtn = nullptr;
    QPushButton*  mCameraPhotoBtn = nullptr;
    QPushButton*  mCameraRecordStartBtn = nullptr;
    QPushButton*  mCameraRecordStopBtn = nullptr;
    QPushButton*  mGimbalCenterBtn = nullptr;
    QPushButton*  mGimbalDownBtn = nullptr;
    QPushButton*  mGimbalYawCenterBtn = nullptr;
    QPushButton*  mGimbalPitchDownBtn = nullptr;

    // --- history ---
    QPlainTextEdit* mHistoryEdit = nullptr;

    // --- data ---
    QVector<DeviceInfo> mAvailableDocks;
    QString mDisplayName;
    QString mGatewaySn;
    double  mDockLat = 0.0;
    double  mDockLon = 0.0;
    double  mDockAlt = 0.0;
    bool    mConnected = false;
    bool    mOnline = false;
    bool    mPending = false;
    bool    mHasFlightAuthority = false;
    bool    mHasPayloadAuthority = false;
    bool    mUpdatingCombo = false;
};

#endif // FLIGHTCONTROLPANEL_H
