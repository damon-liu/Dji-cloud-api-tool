#include "OsdPanel.h"
#include <QDateTime>
#include <QScrollArea>

OsdPanel::OsdPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void OsdPanel::setupUi() {
    mMainLayout = new QVBoxLayout(this);
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->setSpacing(6);

    // ——— 设备头部 ———
    auto* headerBox = new QGroupBox("设备信息", this);
    auto* headerLayout = new QFormLayout(headerBox);
    headerLayout->setSpacing(4);
    mDeviceNameLabel = new QLabel("-", this);
    mDeviceNameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1a73e8;");
    mDeviceSnLabel   = new QLabel("-", this);
    mDeviceSnLabel->setStyleSheet("color: #80868b; font-size: 11px;");
    mDeviceTypeLabel = new QLabel("-", this);
    mOnlineLabel     = new QLabel("⚪ 未知", this);
    mUpdateTimeLabel = new QLabel("-", this);
    mUpdateTimeLabel->setStyleSheet("color: #80868b; font-size: 11px;");
    headerLayout->addRow("名称:", mDeviceNameLabel);
    headerLayout->addRow("SN:",   mDeviceSnLabel);
    headerLayout->addRow("类型:", mDeviceTypeLabel);
    headerLayout->addRow("状态:", mOnlineLabel);
    headerLayout->addRow("更新:", mUpdateTimeLabel);
    mMainLayout->addWidget(headerBox);

    // ——— 飞机飞行数据 ———
    mAircraftGroup = new QGroupBox("飞行数据", this);
    auto* airLayout = new QFormLayout(mAircraftGroup);
    airLayout->setSpacing(4);
    mLatitudeAir    = new QLabel("-", this);
    mLongitudeAir   = new QLabel("-", this);
    mAltitudeAir    = new QLabel("-", this);
    mBatteryPercent = new QLabel("-", this);
    mBatteryVoltage = new QLabel("-", this);
    mSpeedH         = new QLabel("-", this);
    mSpeedV         = new QLabel("-", this);
    mHeading        = new QLabel("-", this);
    mPitch          = new QLabel("-", this);
    mRoll           = new QLabel("-", this);
    mYaw            = new QLabel("-", this);
    mHomeDist       = new QLabel("-", this);
    mFlightTime     = new QLabel("-", this);
    mRcSignal       = new QLabel("-", this);
    airLayout->addRow("纬度:", mLatitudeAir);
    airLayout->addRow("经度:", mLongitudeAir);
    airLayout->addRow("海拔:", mAltitudeAir);
    airLayout->addRow("电量:", mBatteryPercent);
    airLayout->addRow("电压:", mBatteryVoltage);
    airLayout->addRow("水平速度:", mSpeedH);
    airLayout->addRow("垂直速度:", mSpeedV);
    airLayout->addRow("航向:", mHeading);
    airLayout->addRow("俯仰:", mPitch);
    airLayout->addRow("横滚:", mRoll);
    airLayout->addRow("偏航:", mYaw);
    airLayout->addRow("距Home:", mHomeDist);
    airLayout->addRow("飞行时间:", mFlightTime);
    airLayout->addRow("信号:", mRcSignal);
    mMainLayout->addWidget(mAircraftGroup);

    // ——— 机场数据 ———
    mDockGroup = new QGroupBox("机场数据", this);
    auto* dockLayout = new QFormLayout(mDockGroup);
    dockLayout->setSpacing(4);
    mLatitudeDock  = new QLabel("-", this);
    mLongitudeDock = new QLabel("-", this);
    mCoverState    = new QLabel("-", this);
    mPutterState   = new QLabel("-", this);
    mWindSpeed     = new QLabel("-", this);
    mAltLandLat    = new QLabel("-", this);
    mAltLandLon    = new QLabel("-", this);
    dockLayout->addRow("纬度:", mLatitudeDock);
    dockLayout->addRow("经度:", mLongitudeDock);
    dockLayout->addRow("舱盖:", mCoverState);
    dockLayout->addRow("推杆:", mPutterState);
    dockLayout->addRow("风速:", mWindSpeed);
    dockLayout->addRow("备降点纬度:", mAltLandLat);
    dockLayout->addRow("备降点经度:", mAltLandLon);
    mMainLayout->addWidget(mDockGroup);

    mMainLayout->addStretch();
}

void OsdPanel::showOsd(const DeviceInfo* device,
                         const AircraftOsd* aircraftOsd,
                         const DockOsd* dockOsd,
                         const QString& rawJson) {
    Q_UNUSED(rawJson)
    if (!device) { clear(); return; }

    mDeviceNameLabel->setText(device->name);
    mDeviceSnLabel->setText(device->sn);
    mDeviceTypeLabel->setText(
        device->type == DeviceType::Dock ? "机场 (Dock)" : "飞机 (Aircraft)");
    mOnlineLabel->setText(device->online ? "🟢 在线" : "🔴 离线");
    mOnlineLabel->setStyleSheet(
        device->online ? "color: #2e7d32; font-weight: bold;" : "color: #c62828;");
    mUpdateTimeLabel->setText(
        QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    if (device->type == DeviceType::Aircraft && aircraftOsd && aircraftOsd->valid) {
        mAircraftGroup->show();
        mDockGroup->hide();
        showAircraftOsd(*aircraftOsd);
    } else if (device->type == DeviceType::Dock && dockOsd && dockOsd->valid) {
        mAircraftGroup->hide();
        mDockGroup->show();
        showDockOsd(*dockOsd);
    } else {
        mAircraftGroup->hide();
        mDockGroup->hide();
    }
}

void OsdPanel::clear() {
    mDeviceNameLabel->setText("-");
    mDeviceSnLabel->setText("-");
    mDeviceTypeLabel->setText("-");
    mOnlineLabel->setText("⚪ 未知");
    mOnlineLabel->setStyleSheet("");
    mUpdateTimeLabel->setText("-");
    mAircraftGroup->hide();
    mDockGroup->hide();
}

static QString formatCoord(double val) {
    if (val == 0.0) return "-";
    return QString::number(val, 'f', 7);
}

static QString coverText(const QString& state) {
    if (state == "open" || state == "1") return "打开";
    if (state == "closed" || state == "0") return "关闭";
    return state.isEmpty() ? "-" : state;
}

static QString putterText(int state) {
    if (state == 0) return "收回";
    if (state == 1) return "推出";
    return state < 0 ? "-" : QString::number(state);
}

void OsdPanel::showAircraftOsd(const AircraftOsd& osd) {
    setFieldValue(mLatitudeAir,  formatCoord(osd.latitude),  true);
    setFieldValue(mLongitudeAir, formatCoord(osd.longitude), true);
    setFieldValue(mAltitudeAir,  osd.altitude > 0 ? QString::number(osd.altitude, 'f', 1) + " m" : "-", false);
    setFieldValue(mBatteryPercent, osd.battery_percent >= 0
        ? QString::number(osd.battery_percent) + "%" : "-", false);
    setFieldValue(mBatteryVoltage,  osd.battery_voltage > 0
        ? QString::number(osd.battery_voltage / 1000.0, 'f', 1) + "V" : "-", false);
    setFieldValue(mSpeedH,  QString::number(osd.speed_horizontal, 'f', 1) + " m/s", true);
    setFieldValue(mSpeedV,  QString::number(osd.speed_vertical, 'f', 1) + " m/s", true);
    setFieldValue(mHeading, QString::number(osd.heading, 'f', 0) + "°", true);
    setFieldValue(mPitch,   QString::number(osd.pitch, 'f', 1) + "°", true);
    setFieldValue(mRoll,    QString::number(osd.roll, 'f', 1) + "°", true);
    setFieldValue(mYaw,     QString::number(osd.yaw, 'f', 1) + "°", true);
    setFieldValue(mHomeDist, osd.home_distance > 0
        ? QString::number(osd.home_distance, 'f', 1) + " m" : "-", false);
    setFieldValue(mFlightTime, osd.flight_time_sec > 0
        ? QString("%1:%2").arg(osd.flight_time_sec / 60)
            .arg(osd.flight_time_sec % 60, 2, 10, QChar('0')) : "-", false);
    setFieldValue(mRcSignal, QString::number(osd.rc_signal_strength), true);
}

void OsdPanel::showDockOsd(const DockOsd& osd) {
    setFieldValue(mLatitudeDock, formatCoord(osd.latitude),  true);
    setFieldValue(mLongitudeDock,formatCoord(osd.longitude), true);
    setFieldValue(mCoverState,   coverText(osd.cover_state), false);
    setFieldValue(mPutterState,  putterText(osd.putter_state), false);
    setFieldValue(mWindSpeed,    osd.wind_speed >= 0
        ? QString::number(osd.wind_speed, 'f', 1) + " m/s" : "-", false);
    setFieldValue(mAltLandLat,   osd.alternate_land_lat != 0
        ? formatCoord(osd.alternate_land_lat) : "-", false);
    setFieldValue(mAltLandLon,   osd.alternate_land_lon != 0
        ? formatCoord(osd.alternate_land_lon) : "-", false);
}

void OsdPanel::setFieldValue(QLabel* label, const QString& value, bool highlight) {
    QString old = label->text();
    label->setText(value);
    if (highlight && old != value) {
        label->setStyleSheet("color: #1a73e8; font-weight: bold;");
        QTimer::singleShot(1200, this, [label]() {
            label->setStyleSheet("");
        });
    }
}
