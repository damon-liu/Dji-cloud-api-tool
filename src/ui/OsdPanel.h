#ifndef OSDPANEL_H
#define OSDPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include <QComboBox>
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
    void setupDockPanel();
    void setupAircraftPanel();
    void showAircraftOsd(const AircraftOsd& osd);
    void showDockOsd(const DockOsd& osd);
    void setFieldValue(QLabel* label, const QString& value, bool highlight);
    void setDockRow(int row, const QString& label, QLabel*& valLabel);
    void setAirRow(int row, const QString& label, QLabel*& valLabel);

    // 布局
    QVBoxLayout*  mMainLayout;
    QHBoxLayout*  mPanelsRow;
    QGroupBox*    mDockPanel;
    QGroupBox*    mAircraftPanel;
    QGridLayout*  mDockGrid;
    QGridLayout*  mAirGrid;

    // 机场字段
    QLabel* mDockLatLon;
    QLabel* mDockCover;
    QLabel* mDockDroneIn;
    QLabel* mDockInsideTemp;
    QLabel* mDockEnvTemp;
    QLabel* mDockWind;
    QLabel* mDockRain;

    // 飞机字段
    QLabel* mAirModeCode;
    QLabel* mAirLatLon;
    QLabel* mAirBattery;
    QLabel* mAirBattTemp;
    QLabel* mAirHeight;
    QLabel* mAirHomeDist;
    QLabel* mAirWind;
    QLabel* mAirGps;

    // 定时刷新
    DeviceManager* mDevMgr = nullptr;
    QTimer*        mRefreshTimer = nullptr;
    QComboBox*     mIntervalCombo = nullptr;
    bool           mAutoPaused = false;
    QString        mCurrentSn;
    int            mLastDeviceType = -1;
};

#endif // OSDPANEL_H
