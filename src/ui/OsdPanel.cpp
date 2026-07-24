#include "OsdPanel.h"
#include "DeviceManager.h"
#include <QDateTime>
#include <QFrame>

OsdPanel::OsdPanel(QWidget* parent) : QWidget(parent) {
    setupUi();

    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(1000);
    connect(mRefreshTimer, &QTimer::timeout, this, &OsdPanel::refresh);
    mRefreshTimer->start();
}

static void addFieldRow(QGridLayout* grid, int row, int col,
                         const QString& label, QLabel*& valLabel) {
    auto* nameLabel = new QLabel(label);
    nameLabel->setStyleSheet("color: #5f6368; font-size: 11px;");
    valLabel = new QLabel("-");
    valLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(nameLabel, row, col * 2);
    grid->addWidget(valLabel, row, col * 2 + 1);
}

void OsdPanel::setupDockPanel() {
    mDockPanel = new QGroupBox(QString::fromUtf8("\xf0\x9f\x8f\xa2 \xe6\x9c\xba\xe5\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"), this);
    mDockGrid = new QGridLayout(mDockPanel);
    mDockGrid->setSpacing(6);
    mDockGrid->setContentsMargins(8, 12, 8, 8);
    mDockGrid->setColumnStretch(1, 1);
    mDockGrid->setColumnStretch(3, 1);

    addFieldRow(mDockGrid, 0, 0, QString::fromUtf8("\xe7\xbb\x8f\xe7\xba\xac\xe5\xba\xa6:"), mDockLatLon);
    addFieldRow(mDockGrid, 0, 1, QString::fromUtf8("\xe6\xa4\xad\xe7\x90\x83\xe9\xab\x98\xe5\xba\xa6:"), mDockHeight);
    addFieldRow(mDockGrid, 1, 0, QString::fromUtf8("\xe8\x88\xb1\xe7\x9b\x96:"), mDockCover);
    addFieldRow(mDockGrid, 1, 1, QString::fromUtf8("\xe9\xa3\x9e\xe8\xa1\x8c\xe5\x99\xa8:"), mDockDroneIn);
    addFieldRow(mDockGrid, 2, 0, QString::fromUtf8("\xe8\x88\xb1\xe5\x86\x85\xe6\xb8\xa9\xe5\xba\xa6:"), mDockInsideTemp);
    addFieldRow(mDockGrid, 2, 1, QString::fromUtf8("\xe7\x8e\xaf\xe5\xa2\x83\xe6\xb8\xa9\xe5\xba\xa6:"), mDockEnvTemp);
    addFieldRow(mDockGrid, 3, 0, QString::fromUtf8("\xe9\xa3\x8e\xe9\x80\x9f:"), mDockWind);
    addFieldRow(mDockGrid, 3, 1, QString::fromUtf8("\xe9\x99\x8d\xe9\x9b\xa8\xe9\x87\x8f:"), mDockRain);

    mDockPanel->hide();
}

void OsdPanel::setupAircraftPanel() {
    mAircraftPanel = new QGroupBox(QString::fromUtf8("\xe2\x9c\x88 \xe9\xa3\x9e\xe6\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"), this);
    mAirGrid = new QGridLayout(mAircraftPanel);
    mAirGrid->setSpacing(6);
    mAirGrid->setContentsMargins(8, 12, 8, 8);
    mAirGrid->setColumnStretch(1, 1);
    mAirGrid->setColumnStretch(3, 1);

    addFieldRow(mAirGrid, 0, 0, QString::fromUtf8("\xe9\xa3\x9e\xe8\xa1\x8c\xe7\x8a\xb6\xe6\x80\x81:"), mAirModeCode);
    addFieldRow(mAirGrid, 0, 1, QString::fromUtf8("\xe7\xbb\x8f\xe7\xba\xac\xe5\xba\xa6:"), mAirLatLon);
    addFieldRow(mAirGrid, 1, 0, QString::fromUtf8("\xe7\x94\xb5\xe9\x87\x8f:"), mAirBattery);
    addFieldRow(mAirGrid, 1, 1, QString::fromUtf8("\xe7\x94\xb5\xe6\xb1\xa0\xe6\xb8\xa9\xe5\xba\xa6:"), mAirBattTemp);
    addFieldRow(mAirGrid, 2, 0, QString::fromUtf8("\xe9\xab\x98\xe5\xba\xa6:"), mAirHeight);
    addFieldRow(mAirGrid, 2, 1, QString::fromUtf8("\xe8\xb7\x9dHome:"), mAirHomeDist);
    addFieldRow(mAirGrid, 3, 0, QString::fromUtf8("\xe9\xa3\x8e\xe9\x80\x9f:"), mAirWind);
    addFieldRow(mAirGrid, 3, 1, QString::fromUtf8("RTK\xe6\x90\x9c\xe6\x98\x9f:"), mAirRtk);

    mAircraftPanel->hide();
}

void OsdPanel::setupUi() {
    mMainLayout = new QVBoxLayout(this);
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->setSpacing(6);

    auto* titleBar = new QWidget(this);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(8);

    auto* titleLabel = new QLabel(QString::fromUtf8("\xf0\x9f\x93\xa1 OSD \xe9\x9d\xa2\xe6\x9d\xbf"), this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");

    mIntervalCombo = new QComboBox(this);
    mIntervalCombo->addItem("1s", 1000);
    mIntervalCombo->addItem("2s", 2000);
    mIntervalCombo->addItem("5s", 5000);
    mIntervalCombo->addItem("10s", 10000);
    mIntervalCombo->setCurrentIndex(0);
    mIntervalCombo->setFixedWidth(70);
    connect(mIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        int interval = mIntervalCombo->itemData(idx).toInt();
        mRefreshTimer->setInterval(interval);
        if (mRefreshTimer->isActive())
            mRefreshTimer->start(interval);
    });

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(new QLabel(QString::fromUtf8("\xe5\x88\xb7\xe6\x96\xb0\xe9\x97\xb4\xe9\x9a\x94:"), this));
    titleLayout->addWidget(mIntervalCombo);
    mMainLayout->addWidget(titleBar);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    mMainLayout->addWidget(sep);

    mPanelsRow = new QHBoxLayout;
    mPanelsRow->setSpacing(8);

    setupDockPanel();
    setupAircraftPanel();

    mPanelsRow->addWidget(mDockPanel, 1);
    mPanelsRow->addWidget(mAircraftPanel, 1);
    mMainLayout->addLayout(mPanelsRow, 1);

    mMainLayout->addStretch();
}

static QString onlineIcon(bool online) {
    return online
        ? QString::fromUtf8("\xf0\x9f\x9f\xa2")   // 🟢
        : QString::fromUtf8("\xf0\x9f\x94\xb4");  // 🔴
}

static const DeviceInfo* findChildAircraft(DeviceManager* mgr, const QString& parentSn) {
    if (!mgr) return nullptr;
    for (auto* d : mgr->allDevices()) {
        if (d->parentSn == parentSn && d->type == DeviceType::Aircraft)
            return d;
    }
    return nullptr;
}

void OsdPanel::showOsd(const DeviceInfo* device,
                         const AircraftOsd* aircraftOsd,
                         const DockOsd* dockOsd,
                         const QString& rawJson) {
    Q_UNUSED(rawJson)
    if (!device) { clear(); return; }

    bool typeChanged = (static_cast<int>(device->type) != mLastDeviceType);
    mLastDeviceType = static_cast<int>(device->type);

    QString dockIcon = onlineIcon(device->online);

    if (device->type == DeviceType::Dock) {
        if (typeChanged) {
            mDockPanel->show();
        }
        mDockPanel->setTitle(dockIcon + QString::fromUtf8(" \xf0\x9f\x8f\xa2 \xe6\x9c\xba\xe5\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"));
        if (dockOsd && dockOsd->valid)
            showDockOsd(*dockOsd);

        // 即使 OSD 数据尚未到达，机场有关联飞机时也显示飞机面板
        const DeviceInfo* child = findChildAircraft(mDevMgr, device->sn);
        bool hasAircraft = (aircraftOsd && aircraftOsd->valid);
        if (child) {
            if (typeChanged || mAircraftPanel->isHidden()) {
                mAircraftPanel->show();
            }
            QString airIcon = onlineIcon(child->online);
            mAircraftPanel->setTitle(airIcon + QString::fromUtf8(" \xe2\x9c\x88 \xe9\xa3\x9e\xe6\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"));
            if (hasAircraft) {
                showAircraftOsd(*aircraftOsd);
            }
        } else if (hasAircraft) {
            if (typeChanged || mAircraftPanel->isHidden()) {
                mAircraftPanel->show();
            }
            mAircraftPanel->setTitle(dockIcon + QString::fromUtf8(" \xe2\x9c\x88 \xe9\xa3\x9e\xe6\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"));
            showAircraftOsd(*aircraftOsd);
        }
    } else {
        if (typeChanged) {
            mDockPanel->hide();
            mAircraftPanel->show();
        }
        mAircraftPanel->setTitle(dockIcon + QString::fromUtf8(" \xe2\x9c\x88 \xe9\xa3\x9e\xe6\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"));
        if (aircraftOsd && aircraftOsd->valid)
            showAircraftOsd(*aircraftOsd);
    }
}

void OsdPanel::clear() {
    mDockPanel->hide();
    mAircraftPanel->hide();
    mLastDeviceType = -1;
}

static QString formatCoord(double val) {
    if (val == 0.0) return "-";
    return QString::number(val, 'f', 7);
}

static QString coverText(int state) {
    if (state == 1) return QString::fromUtf8("\xe6\x89\x93\xe5\xbc\x80");
    if (state == 0) return QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad");
    return "-";
}

void OsdPanel::showDockOsd(const DockOsd& osd) {
    if (osd.latitude == 0.0 && osd.longitude == 0.0)
        return;

    setFieldValue(mDockLatLon,
        (osd.latitude != 0 || osd.longitude != 0)
        ? QString("%1, %2").arg(formatCoord(osd.latitude)).arg(formatCoord(osd.longitude))
        : "-");
    setFieldValue(mDockCover, coverText(osd.cover_state));
    setFieldValue(mDockDroneIn, osd.drone_in_dock
        ? QString::fromUtf8("\xe5\x9c\xa8\xe8\x88\xb1\xe5\x86\x85")
        : QString::fromUtf8("\xe5\xb7\xb2\xe7\xa6\xbb\xe8\x88\xb1"));
    setFieldValue(mDockInsideTemp, osd.dock_inside_temp > -200
        ? QString::number(osd.dock_inside_temp, 'f', 1) + " \xe2\x84\x83" : "-");
    setFieldValue(mDockEnvTemp, osd.environment_temp > -200
        ? QString::number(osd.environment_temp, 'f', 1) + " \xe2\x84\x83" : "-");
    setFieldValue(mDockWind, osd.wind_speed >= 0
        ? QString::number(osd.wind_speed, 'f', 1) + " m/s" : "-");
    setFieldValue(mDockRain, osd.rainfall >= 0
        ? QString::number(osd.rainfall, 'f', 1) + " mm" : "-");
    setFieldValue(mDockHeight, osd.height > 0
        ? QString::number(osd.height, 'f', 1) + " m" : "-");
}

static QString modeCodeText(int code) {
    if (code < 0) return "-";
    switch (code) {
        case 0:  return QString::fromUtf8("\xe5\xbe\x85\xe6\x9c\xba");
        case 1:  return QString::fromUtf8("\xe8\xb5\xb7\xe9\xa3\x9e");
        case 4:  return QString::fromUtf8("\xe8\x87\xaa\xe5\x8a\xa8\xe8\xb5\xb7\xe9\xa3\x9e");
        case 5:  return QString::fromUtf8("\xe8\x88\xaa\xe7\xba\xbf\xe9\xa3\x9e\xe8\xa1\x8c");
        case 6:  return QString::fromUtf8("\xe6\x82\xac\xe5\x81\x9c");
        case 7:  return QString::fromUtf8("\xe8\xbf\x94\xe8\x88\xaa");
        case 9:  return QString::fromUtf8("\xe8\x87\xaa\xe5\x8a\xa8\xe8\xbf\x94\xe8\x88\xaa");
        case 10: return QString::fromUtf8("\xe8\x87\xaa\xe5\x8a\xa8\xe9\x99\x8d\xe8\x90\xbd");
        case 12: return QString::fromUtf8("\xe8\xbf\xab\xe9\x99\x8d");
        default: return QString::fromUtf8("\xe6\xa8\xa1\xe5\xbc\x8f") + QString::number(code);
    }
}

void OsdPanel::showAircraftOsd(const AircraftOsd& osd) {
    if (osd.latitude == 0.0 && osd.longitude == 0.0)
        return;

    setFieldValue(mAirModeCode, modeCodeText(osd.mode_code));
    setFieldValue(mAirLatLon,
        (osd.latitude != 0 || osd.longitude != 0)
        ? QString("%1, %2").arg(formatCoord(osd.latitude)).arg(formatCoord(osd.longitude))
        : "-");
    setFieldValue(mAirBattery, osd.battery_percent >= 0
        ? QString::number(osd.battery_percent) + "%" : "-");
    setFieldValue(mAirBattTemp, osd.battery_temperature > -200
        ? QString::number(osd.battery_temperature, 'f', 1) + " \xe2\x84\x83" : "-");
    setFieldValue(mAirHeight, osd.height >= 0
        ? QString::number(osd.height, 'f', 1) + " m" : "-");
    setFieldValue(mAirHomeDist, osd.home_distance >= 0
        ? QString::number(osd.home_distance, 'f', 1) + " m" : "-");
    setFieldValue(mAirWind, osd.wind_speed >= 0
        ? QString::number(osd.wind_speed, 'f', 1) + " m/s" : "-");
    setFieldValue(mAirRtk, osd.rtk_number >= 0
        ? QString::number(osd.rtk_number) + QString::fromUtf8(" \xe9\xa2\x97") : "-");
}

void OsdPanel::setFieldValue(QLabel* label, const QString& value) {
    if (value == "-" || value.isEmpty())
        return;
    label->setText(value);
}

void OsdPanel::refresh() {
    if (!mDevMgr || mCurrentSn.isEmpty())
        return;

    DeviceInfo* dev = mDevMgr->device(mCurrentSn);
    if (!dev)
        return;

    const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(mCurrentSn);
    const DockOsd* dockOsd = mDevMgr->latestDockOsd(mCurrentSn);
    QString rawJson = mDevMgr->latestRawJson(mCurrentSn);

    // 机场设备：同时查找子飞机的 OSD 一起展示
    if (dev->type == DeviceType::Dock && !airOsd) {
        for (auto* d : mDevMgr->allDevices()) {
            if (d->parentSn == mCurrentSn && d->type == DeviceType::Aircraft) {
                airOsd = mDevMgr->latestAircraftOsd(d->sn);
                break;
            }
        }
    }

    showOsd(dev, airOsd, dockOsd, rawJson);
}

void OsdPanel::pause() {
    if (!mAutoPaused) {
        mAutoPaused = true;
        mRefreshTimer->stop();
    }
}

void OsdPanel::resume() {
    if (mAutoPaused) {
        mAutoPaused = false;
        mRefreshTimer->start(mRefreshTimer->interval());
        refresh();
    }
}
