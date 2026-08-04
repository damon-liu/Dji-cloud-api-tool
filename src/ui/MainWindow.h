#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include "DeviceTreeWidget.h"
#include "OsdPanel.h"
#include "RawJsonPanel.h"
#include "PublishPanel.h"
#include "TopicListWidget.h"
#include "TopicParsePanel.h"
#include "DockControlDialog.h"
#include "FlightControlDialog.h"
#include "MaintenanceDialog.h"
#include "PsdkSpeakerDialog.h"
#include "DeviceManager.h"
#include "VideoStreamWindow.h"

// libVLC 前向声明（全局作用域，与 vlc/vlc.h 兼容）
struct libvlc_instance_t;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DeviceManager* devMgr, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onDeviceSelected(const QString& sn);
    void onOsdUpdated(const QString& sn, const QString& topic, const QString& rawJson);
    void onConnectAction();
    void onDisconnectAction();
    void onAddDevice();
    void onDeleteDevice();
    void updateStatusBar();

private:
    void setupToolBar();
    void setupLayout();
    void setupStatusBar();
    void connectSignals();
    void refreshTopicList(const QString& sn);
    void refreshDockControlList(const QString& currentSn);
    void showVideoWindows();
    void hideVideoWindows();

    DeviceManager*     mDevMgr;
    DeviceTreeWidget*  mDeviceTree;
    OsdPanel*          mOsdPanel;
    RawJsonPanel*      mRawJsonPanel;
    PublishPanel*      mPublishPanel;
    QSplitter*         mRightSplitter;   // OSD | JSON (horizontal)
    QPushButton*       mTogglePublishBtn;
    DockControlPanel*  mDockControlPanel = nullptr;   // 指向对话框内的面板
    DockControlDialog* mDockCtrlDialog = nullptr;
    FlightControlPanel*  mFlightControlPanel = nullptr;   // 指向飞行控制对话框内的面板
    FlightControlDialog* mFlightCtrlDialog = nullptr;
    MaintenancePanel*    mMaintenancePanel = nullptr;      // 指向运维模式对话框内的面板
    MaintenanceDialog*   mMaintenanceDialog = nullptr;
    QList<VideoStreamWindow*> mVideoWindows;
#ifdef HAS_VLC
    libvlc_instance_t* mVlcInstance = nullptr;
#endif
    PsdkSpeakerPanel*    mPsdkSpeakerPanel = nullptr;       // 指向PSDK喊话器对话框内的面板
    PsdkSpeakerDialog*   mPsdkSpeakerDialog = nullptr;
    QLabel*            mStatusLabel;
    QLabel*            mDeviceCountLabel;
    QLabel*            mVersionLabel;
    QLabel*            mBrokerLabel;

    // Toolbar actions
    QAction*           mConnectAct;
    QAction*           mDisconnectAct;

    // Sidebar buttons
    QPushButton*       mAddDeviceBtn;
    QPushButton*       mDeleteDeviceBtn;
    TopicListWidget*   mTopicListWidget;
    TopicParsePanel*  mTopicParsePanel;
    TopicMapping*     mTopicMapping;

    // 用户手动取消选中后不再自动选中
    bool              mUserDeselected = false;

    // Stylesheet helper
    void applyStyle();
};

#endif // MAINWINDOW_H
