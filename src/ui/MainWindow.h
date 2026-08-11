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
    void clearVideoWindows();  // 销毁所有视频窗口 + 停止 VLC
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
    VideoStreamWindow* findDockVideoWindow();  // 查找机库视频窗口
    void applyVideoLayoutMode();               // 根据当前视频窗口情况自动切换布局

    DeviceManager*     mDevMgr;
    DeviceTreeWidget*  mDeviceTree;
    OsdPanel*          mOsdPanel;
    RawJsonPanel*      mRawJsonPanel;
    PublishPanel*      mPublishPanel;
    QSplitter*         mRightSplitter;   // OSD | JSON (horizontal)
    QSplitter*         mRightColumnWidget = nullptr;       // 右侧栏：原始JSON(上) + 内嵌机库视频(下) — QSplitter
    QWidget*           mDockVideoInlineContainer = nullptr; // 机库视频内嵌占位容器
    QPushButton*       mTogglePublishBtn;
    QTabWidget*        mRightTabWidget = nullptr;   // 右侧标签页（方案E）
    DockControlPanel*  mDockControlPanel = nullptr;
    FlightControlPanel*  mFlightControlPanel = nullptr;
    MaintenancePanel*  mMaintenancePanel = nullptr;
    PsdkSpeakerPanel*  mPsdkSpeakerPanel = nullptr;
    CommandHistoryDialog* mCommandHistoryDialog = nullptr;
    QMap<QWidget*, QWidget*> mPoppedOutDialogs;  // panel → dialog 映射（弹出状态追踪）
    QList<VideoStreamWindow*> mVideoWindows;          // 视频窗口（内嵌面板）
    QMap<QString, QVector<LiveStatusInfo>> mCachedLiveStatus;  // 用于增量对比（反闪烁）
    QWidget*           mVideoPanel = nullptr;        // 视频面板容器（嵌入监控标签页）
    QSplitter*         mVideoSplitter = nullptr;     // 两个视频窗口水平分割器
    QPushButton*       mVideoToggleBtn = nullptr;    // 视频面板折叠/展开按钮
    QLabel*            mVideoLoadingLabel = nullptr; // 视频引擎初始化加载提示
    QLabel*            mDeviceTitleLabel;   // Devices列表 (N)
    QLabel*            mBrokerLabel;

    // 视频布局模式
    enum class VideoLayoutMode { Normal, Compact };
    VideoLayoutMode    mVideoLayoutMode = VideoLayoutMode::Normal;

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
    QString           mSelectedDeviceSn;   // 当前选中的设备 SN（用于视频直播过滤）

    // Stylesheet helper
    void applyStyle();
};

#endif // MAINWINDOW_H
