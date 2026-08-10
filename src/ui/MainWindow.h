#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QMap>
#include "DeviceTreeWidget.h"
#include "OsdPanel.h"
#include "RawJsonPanel.h"
#include "PublishPanel.h"
#include "TopicListWidget.h"
#include "TopicParsePanel.h"
#include "DockControlPanel.h"
#include "FlightControlPanel.h"
#include "MaintenancePanel.h"
#include "PsdkSpeakerPanel.h"
#include "CommandHistoryDialog.h"
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
    bool initVlc();  // 懒加载 VLC，显示加载中提示
    void onLiveStatusChanged(const QString& sn, const QVector<LiveStatusInfo>& list);
    void removeVideoWindowsForDevice(const QString& sn);
    void refreshVideoWindows();  // 从当前配置刷新已有窗口的推流地址 + live_status
    QString buildStreamUrl(const StreamMediaConfig& ss, const QString& gatewaySn,
                           const QString& deviceSn, const QString& videoId,
                           const QString& cameraSuffix);
    void connectPushControlSignals(VideoStreamWindow* win, const QString& gatewaySn);
    void showFunctionInTab(int tabIndex);
    void popOutCurrentTab();
    void popInPanel(QWidget* panel);

    DeviceManager*     mDevMgr;
    DeviceTreeWidget*  mDeviceTree;
    OsdPanel*          mOsdPanel;
    RawJsonPanel*      mRawJsonPanel;
    PublishPanel*      mPublishPanel;
    QSplitter*         mRightSplitter;   // OSD | JSON (horizontal)
    QPushButton*       mTogglePublishBtn;
    QTabWidget*        mRightTabWidget = nullptr;   // 右侧标签页（方案E）
    DockControlPanel*  mDockControlPanel = nullptr;
    FlightControlPanel*  mFlightControlPanel = nullptr;
    MaintenancePanel*  mMaintenancePanel = nullptr;
    PsdkSpeakerPanel*  mPsdkSpeakerPanel = nullptr;
    CommandHistoryDialog* mCommandHistoryDialog = nullptr;
    QMap<QWidget*, QWidget*> mPoppedOutDialogs;  // panel → dialog 映射（弹出状态追踪）
    QList<VideoStreamWindow*> mVideoWindows;
    QMap<QString, QVector<LiveStatusInfo>> mCachedLiveStatus;  // 用于增量对比（反闪烁）
    QWidget*           mVideoPanel = nullptr;        // 视频面板容器（嵌入监控标签页）
    QSplitter*         mVideoSplitter = nullptr;     // 两个视频窗口水平分割器
    QPushButton*       mVideoToggleBtn = nullptr;    // 视频面板折叠/展开按钮
    QLabel*            mVideoLoadingLabel = nullptr; // 视频引擎初始化加载提示
#ifdef HAS_VLC
    libvlc_instance_t* mVlcInstance = nullptr;
#endif
    QLabel*            mDeviceTitleLabel;   // Devices列表 (N)
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
