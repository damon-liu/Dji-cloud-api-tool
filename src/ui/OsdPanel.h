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

    // 显示设备的 OSD 数据
    void showOsd(const DeviceInfo* device,
                 const AircraftOsd* aircraftOsd,
                 const DockOsd* dockOsd,
                 const QString& rawJson);

    // 清空显示
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

    QVBoxLayout* mMainLayout;
};

#endif // OSDPANEL_H
