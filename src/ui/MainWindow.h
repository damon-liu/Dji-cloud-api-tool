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
#include "DeviceManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DeviceManager* devMgr, QWidget* parent = nullptr);

private slots:
    void onDeviceSelected(const QString& sn);
    void onOsdUpdated(const QString& sn, const QString& rawJson);
    void onConnectAction();
    void onDisconnectAction();
    void onAddDevice();
    void onEditTopic();
    void onDeleteDevice();
    void updateStatusBar();

private:
    void setupToolBar();
    void setupLayout();
    void setupStatusBar();
    void connectSignals();

    DeviceManager*     mDevMgr;
    DeviceTreeWidget*  mDeviceTree;
    OsdPanel*          mOsdPanel;
    RawJsonPanel*      mRawJsonPanel;
    PublishPanel*      mPublishPanel;
    QSplitter*         mRightSplitter;   // OSD | JSON (horizontal)
    QPushButton*       mTogglePublishBtn;
    QLabel*            mStatusLabel;
    QLabel*            mDeviceCountLabel;
    QLabel*            mBrokerLabel;

    // Toolbar actions
    QAction*           mConnectAct;
    QAction*           mDisconnectAct;

    // Sidebar buttons
    QPushButton*       mAddDeviceBtn;
    QPushButton*       mEditTopicBtn;
    QPushButton*       mDeleteDeviceBtn;

    // Stylesheet helper
    void applyStyle();
};

#endif // MAINWINDOW_H
