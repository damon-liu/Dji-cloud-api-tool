#ifndef OSDPANEL_H
#define OSDPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include "OsdData.h"
#include "DeviceInfo.h"

class OsdPanel : public QWidget {
    Q_OBJECT
public:
    explicit OsdPanel(QWidget* parent = nullptr);

    void showOsd(const DeviceInfo* device,
                 const AircraftOsd* aircraftOsd,
                 const DockOsd* dockOsd,
                 const QString& rawJson);

    void clear();

private:
    void setupUi();
    void showAircraftOsd(const AircraftOsd& osd);
    void showDockOsd(const DockOsd& osd);
    void setFieldValue(QLabel* label, const QString& value, bool highlight);

    // 设备头部信息
    QLabel* mDeviceNameLabel;
    QLabel* mDeviceSnLabel;
    QLabel* mDeviceTypeLabel;
    QLabel* mOnlineLabel;
    QLabel* mUpdateTimeLabel;

    // 飞机专属
    QGroupBox* mAircraftGroup;
    QLabel* mLatitudeAir;
    QLabel* mLongitudeAir;
    QLabel* mAltitudeAir;
    QLabel* mBatteryPercent;
    QLabel* mBatteryVoltage;
    QLabel* mSpeedH;
    QLabel* mSpeedV;
    QLabel* mHeading;
    QLabel* mPitch;
    QLabel* mRoll;
    QLabel* mYaw;
    QLabel* mHomeDist;
    QLabel* mFlightTime;
    QLabel* mRcSignal;

    // 机场专属
    QGroupBox* mDockGroup;
    QLabel* mLatitudeDock;
    QLabel* mLongitudeDock;
    QLabel* mAltitudeDock;
    QLabel* mCoverState;
    QLabel* mPutterState;
    QLabel* mDroneInDock;
    QLabel* mWorkVoltage;
    QLabel* mWorkCurrent;
    QLabel* mBackupBattery;
    QLabel* mWindSpeed;
    QLabel* mEnvTemp;
    QLabel* mEnvHumidity;
    QLabel* mAltLandLat;
    QLabel* mAltLandLon;

    QVBoxLayout* mMainLayout;
};

#endif // OSDPANEL_H
