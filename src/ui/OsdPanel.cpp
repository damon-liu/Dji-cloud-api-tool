#include "OsdPanel.h"
#include "DeviceManager.h"
#include <QDateTime>
#include <QScrollArea>
#include <QFrame>

OsdPanel::OsdPanel(QWidget* parent) : QWidget(parent) {
    setupUi();

    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(1000);
    connect(mRefreshTimer, &QTimer::timeout, this, &OsdPanel::refresh);
    mRefreshTimer->start();
}

// —— 辅助：创建 GridLayout 中的 label|value 对 ——
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
    addFieldRow(mDockGrid, 0, 1, QString::fromUtf8("\xe8\x88\xb1\xe7\x9b\x96:"), mDockCover);
    addFieldRow(mDockGrid, 1, 0, QString::fromUtf8("\xe9\xa3\x9e\xe8\xa1\x8c\xe5\x99\xa8:"), mDockDroneIn);
    addFieldRow(mDockGrid, 1, 1, QString::fromUtf8("\xe8\x88\xb1\xe5\x86\x85\xe6\xb8\xa9\xe5\xba\xa6:"), mDockInsideTemp);
    addFieldRow(mDockGrid, 2, 0, QString::fromUtf8("\xe7\x8e\xaf\xe5\xa2\x83\xe6\xb8\xa9\xe5\xba\xa6:"), mDockEnvTemp);
    addFieldRow(mDockGrid, 2, 1, QString::fromUtf8("\xe9\xa3\x8e\xe9\x80\x9f:"), mDockWind);
    addFieldRow(mDockGrid, 3, 0, QString::fromUtf8("\xe9\x99\x8d\xe9\x9b\xa8\xe9\x87\x8f:"), mDockRain);

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
    addFieldRow(mAirGrid, 3, 1, QString::fromUtf8("GPS\xe6\x90\x9c\xe6\x98\x9f:"), mAirGps);

    mAircraftPanel->hide();
}

void OsdPanel::setupUi() {
    mMainLayout = new QVBoxLayout(this);
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->setSpacing(6);

    // 标题栏
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

    // 分隔线
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    mMainLayout->addWidget(sep);

    // 两个子面板并排
    mPanelsRow = new QHBoxLayout;
    mPanelsRow->setSpacing(8);

    setupDockPanel();
    setupAircraftPanel();

    mPanelsRow->addWidget(mDockPanel, 1);
    mPanelsRow->addWidget(mAircraftPanel, 1);
    mMainLayout->addLayout(mPanelsRow, 1);

    mMainLayout->addStretch();
}

// —— 显示逻辑 ——

void OsdPanel::showOsd(const DeviceInfo* device,
                         const AircraftOsd* aircraftOsd,
                         const DockOsd* dockOsd,
                         const QString& rawJson) {
    Q_UNUSED(rawJson)
    if (!device) { clear(); return; }

    // 仅在设备类型变化时才切换面板显隐，避免定时刷新导致闪烁
    bool typeChanged = (static_cast<int>(device->type) != mLastDeviceType);
    mLastDeviceType = static_cast<int>(device->type);

    if (device->type == DeviceType::Dock) {
        if (typeChanged) {
            mDockPanel->show();
        }
        if (dockOsd && dockOsd->valid)
            showDockOsd(*dockOsd);

        if (aircraftOsd && aircraftOsd->valid) {
            if (typeChanged) {
                mAircraftPanel->show();
                mAircraftPanel->setTitle(QString::fromUtf8("\xe2\x9c\x88 \xe9\xa3\x9e\xe6\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"));
            }
            showAircraftOsd(*aircraftOsd);
        }
    } else {
        // 独立手飞
        if (typeChanged) {
            mDockPanel->hide();
            mAircraftPanel->show();
            mAircraftPanel->setTitle(QString::fromUtf8("\xe2\x9c\x88 \xe9\xa3\x9e\xe6\x9c\xba\xe4\xbf\xa1\xe6\x81\xaf"));
        }
        if (aircraftOsd && aircraftOsd->valid)
            showAircraftOsd(*aircraftOsd);
    }
}

void OsdPanel::clear() {
    mDockPanel->hide();
    mAircraftPanel->hide();
    mLastDeviceType = -1;
}

// —— 机场显示 ——

static QString formatCoord(double val) {
    if (val == 0.0) return "-";
    return QString::number(val, 'f', 7);
}

static QString coverText(const QString& state) {
    if (state == "open" || state == "1") return QString::fromUtf8("\xe6\x89\x93\xe5\xbc\x80");
    if (state == "closed" || state == "0") return QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad");
    return state.isEmpty() ? "-" : state;
}

void OsdPanel::showDockOsd(const DockOsd& osd) {
    // 经纬度合并显示
    setFieldValue(mDockLatLon,
        (osd.latitude != 0 || osd.longitude != 0)
        ? QString("%1, %2").arg(formatCoord(osd.latitude)).arg(formatCoord(osd.longitude))
        : "-", true);
    setFieldValue(mDockCover, coverText(osd.cover_state), false);
    setFieldValue(mDockDroneIn, osd.drone_in_dock
        ? QString::fromUtf8("\xe5\x9c\xa8\xe8\x88\xb1\xe5\x86\x85")
        : QString::fromUtf8("\xe5\xb7\xb2\xe7\xa6\xbb\xe8\x88\xb1"), false);
    setFieldValue(mDockInsideTemp, osd.dock_inside_temp > -200
        ? QString::number(osd.dock_inside_temp, 'f', 1) + " \xe2\x84\x83" : "-", false);
    setFieldValue(mDockEnvTemp, osd.environment_temp > -200
        ? QString::number(osd.environment_temp, 'f', 1) + " \xe2\x84\x83" : "-", false);
    setFieldValue(mDockWind, osd.wind_speed >= 0
        ? QString::number(osd.wind_speed, 'f', 1) + " m/s" : "-", false);
    setFieldValue(mDockRain, osd.rainfall >= 0
        ? QString::number(osd.rainfall, 'f', 1) + " mm" : "-", false);
}

// —— 飞机显示 ——

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
    setFieldValue(mAirModeCode, modeCodeText(osd.mode_code), true);
    setFieldValue(mAirLatLon,
        (osd.latitude != 0 || osd.longitude != 0)
        ? QString("%1, %2").arg(formatCoord(osd.latitude)).arg(formatCoord(osd.longitude))
        : "-", true);
    setFieldValue(mAirBattery, osd.battery_percent >= 0
        ? QString::number(osd.battery_percent) + "%" : "-", false);
    setFieldValue(mAirBattTemp, osd.battery_temperature > -200
        ? QString::number(osd.battery_temperature, 'f', 1) + " \xe2\x84\x83" : "-", false);
    setFieldValue(mAirHeight, osd.height > 0
        ? QString::number(osd.height, 'f', 1) + " m" : "-", false);
    setFieldValue(mAirHomeDist, osd.home_distance > 0
        ? QString::number(osd.home_distance, 'f', 1) + " m" : "-", false);
    setFieldValue(mAirWind, osd.wind_speed >= 0
        ? QString::number(osd.wind_speed, 'f', 1) + " m/s" : "-", false);
    setFieldValue(mAirGps, osd.gps_number > 0
        ? QString::number(osd.gps_number) + QString::fromUtf8(" \xe9\xa2\x97") : "-", true);
}

void OsdPanel::setFieldValue(QLabel* label, const QString& value, bool highlight) {
    QString old = label->text();
    label->setText(value);
    if (highlight && old != value) {
        label->setStyleSheet("color: #1a73e8; font-weight: bold; font-size: 12px;");
        QTimer::singleShot(1200, this, [label]() {
            label->setStyleSheet("font-size: 12px; font-weight: 500;");
        });
    }
}

// —— 定时刷新 / 暂停恢复 ——

void OsdPanel::refresh() {
    if (!mDevMgr || mCurrentSn.isEmpty())
        return;

    DeviceInfo* dev = mDevMgr->device(mCurrentSn);
    if (!dev)
        return;

    const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(mCurrentSn);
    const DockOsd* dockOsd = mDevMgr->latestDockOsd(mCurrentSn);
    QString rawJson = mDevMgr->latestRawJson(mCurrentSn);
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
