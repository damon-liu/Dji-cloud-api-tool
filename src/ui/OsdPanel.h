#ifndef OSDPANEL_H
#define OSDPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include "OsdData.h"
#include "DeviceInfo.h"

class DeviceManager;

class OsdPanel : public QWidget {
    Q_OBJECT
public:
    explicit OsdPanel(QWidget* parent = nullptr);

    void showOsd(const DeviceInfo* device,
                 const AircraftOsd* aircraftOsd,
                 const DockOsd* dockOsd,
                 const QString& rawJson);

    void clear();

    void setDeviceManager(DeviceManager* mgr) { mDevMgr = mgr; }
    void setCurrentSn(const QString& sn) { mCurrentSn = sn; }
    void pause();
    void resume();

public slots:
    void refresh();

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
    QWidget*   mDockRow;    // 水平容器：设备信息 + 机场数据
    QGroupBox* mDockGroup;
    QLabel* mLatitudeDock;
    QLabel* mLongitudeDock;
    QLabel* mCoverState;
    QLabel* mPutterState;
    QLabel* mWindSpeed;
    QLabel* mAltLandLat;
    QLabel* mAltLandLon;

    QVBoxLayout* mMainLayout;

    // --- 定时刷新 ---
    DeviceManager* mDevMgr = nullptr;
    QTimer*        mRefreshTimer = nullptr;
    QComboBox*     mIntervalCombo = nullptr;
    bool           mAutoPaused = false;
    QString        mCurrentSn;
};

#endif // OSDPANEL_H
