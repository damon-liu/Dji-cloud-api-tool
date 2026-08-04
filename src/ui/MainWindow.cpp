#include "MainWindow.h"
#include "ConfigDialog.h"
#include "TopicEditDialog.h"
#include <QAction>
#include <QCloseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSet>
#include <QFrame>
#include <QInputDialog>
#include <QToolButton>
#include <QMenu>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include "TopicMapping.h"

#ifdef HAS_VLC
#include <vlc/vlc.h>
#endif

// 内置默认 topic 映射 JSON（文件缺失时降级使用）
static const char* TOPIC_MAPPINGS_BUILTIN = R"(
{
  "topics": {
    "dock/thing/product/{sn}/osd": {
      "description": "机场 OSD 遥测数据",
      "fields": {
        "acc_time": {"zh":"累计运行时间","unit":"秒"},
        "cover_state": {"zh":"舱盖状态","unit":"","values":{"0":"关闭","1":"打开","2":"半开","3":"异常"}},
        "drone_in_dock": {"zh":"飞机在舱内","unit":"","values":{"0":"舱外","1":"舱内"}},
        "electric_supply_voltage": {"zh":"市电供电电压","unit":"mV"},
        "emergency_stop_state": {"zh":"急停按钮","unit":"","values":{"0":"正常","1":"按下"}},
        "environment_temperature": {"zh":"外部环境温度","unit":"℃"},
        "flighttask_step_code": {"zh":"机场任务状态","unit":"","values":{"0":"作业准备中","1":"飞行作业中","2":"作业后状态恢复","5":"任务空闲"}},
        "heading": {"zh":"机场朝向角","unit":"°"},
        "height": {"zh":"椭球高度","unit":"m"},
        "humidity": {"zh":"舱内湿度","unit":"%"},
        "job_number": {"zh":"累计作业次数","unit":"次"},
        "latitude": {"zh":"机场纬度","unit":"°"},
        "longitude": {"zh":"机场经度","unit":"°"},
        "mode_code": {"zh":"机场运行模式","unit":"","values":{"0":"空闲","1":"现场调试","2":"远程调试","4":"作业中"}},
        "position_state.gps_number": {"zh":"GPS搜星数","unit":""},
        "position_state.is_fixed": {"zh":"定位状态","unit":"","values":{"0":"未开始","1":"定位中","2":"定位成功","3":"定位失败"}},
        "position_state.rtk_number": {"zh":"RTK搜星数","unit":""},
        "putter_state": {"zh":"推杆状态","unit":"","values":{"0":"收回","1":"推出","2":"半推出","3":"异常"}},
        "supplement_light_state": {"zh":"补光灯状态","unit":"","values":{"0":"关闭","1":"打开"}},
        "temperature": {"zh":"舱内温度","unit":"℃"},
        "wind_speed": {"zh":"风速","unit":"m/s"},
        "working_current": {"zh":"工作电流","unit":"mA"},
        "working_voltage": {"zh":"工作电压","unit":"mV"}
      },
      "groups": [
        {"id":"common","label":"📋 常用信息","keys":[["cover_state","putter_state","drone_in_dock"],["latitude","longitude"],["temperature","environment_temperature","wind_speed"]]},
        {"id":"position","label":"📍 定位","keys":[["position_state.is_fixed","position_state.gps_number","position_state.rtk_number"]]},
        {"id":"operation","label":"🔧 运行状态","keys":[["mode_code","flighttask_step_code"],["emergency_stop_state","supplement_light_state"]]},
        {"id":"statistics","label":"⏱ 运行统计","keys":[["job_number","acc_time"]]}
      ]
    },
    "aircraft/thing/product/{sn}/osd": {
      "description": "飞机 OSD 遥测数据",
      "fields": {
        "attitude_head": {"zh":"航向角","unit":"°"},
        "attitude_pitch": {"zh":"俯仰角","unit":"°"},
        "attitude_roll": {"zh":"横滚角","unit":"°"},
        "battery.capacity_percent": {"zh":"总电量","unit":"%"},
        "battery.remain_flight_time": {"zh":"剩余飞行时间","unit":"秒"},
        "battery.return_home_power": {"zh":"返航所需电量","unit":"%"},
        "battery.batteries[0].temperature": {"zh":"左电池温度","unit":"℃"},
        "battery.batteries[0].voltage": {"zh":"左电池电压","unit":"mV"},
        "battery.batteries[1].temperature": {"zh":"右电池温度","unit":"℃"},
        "battery.batteries[1].voltage": {"zh":"右电池电压","unit":"mV"},
        "elevation": {"zh":"相对起飞点高度","unit":"m"},
        "height": {"zh":"椭球高度","unit":"m"},
        "home_distance": {"zh":"距Home点距离","unit":"m"},
        "horizontal_speed": {"zh":"水平速度","unit":"m/s"},
        "latitude": {"zh":"纬度","unit":"°"},
        "longitude": {"zh":"经度","unit":"°"},
        "mode_code": {"zh":"飞行状态","unit":"","values":{"0":"待命","4":"自动起飞","5":"航线飞行","9":"自动返航","10":"自动降落"}},
        "position_state.gps_number": {"zh":"GPS搜星数","unit":""},
        "position_state.is_fixed": {"zh":"定位收敛状态","unit":"","values":{"0":"未开始","1":"收敛中","2":"已收敛","3":"失败"}},
        "position_state.rtk_number": {"zh":"RTK搜星数","unit":""},
        "vertical_speed": {"zh":"垂直速度","unit":"m/s"},
        "wind_speed": {"zh":"风速","unit":"m/s"}
      },
      "groups": [
        {"id":"common","label":"📋 常用信息","keys":[["mode_code"],["latitude","longitude"],["height","elevation","home_distance"],["horizontal_speed","vertical_speed"],["battery.capacity_percent","battery.remain_flight_time"]]},
        {"id":"position","label":"📍 定位","keys":[["position_state.is_fixed","position_state.gps_number","position_state.rtk_number"]]},
        {"id":"attitude","label":"✈️ 姿态","keys":[["attitude_head","attitude_pitch","attitude_roll"]]},
        {"id":"battery","label":"🔋 电池","keys":[["battery.capacity_percent","battery.remain_flight_time","battery.return_home_power"],["battery.batteries[0].voltage","battery.batteries[0].temperature"],["battery.batteries[1].voltage","battery.batteries[1].temperature"]]}
      ]
    }
  }
}
)";

MainWindow::MainWindow(DeviceManager* devMgr, QWidget* parent)
    : QMainWindow(parent), mDevMgr(devMgr)
{
    setWindowTitle("DJI-CLOUD-API 监控客户端");
    resize(1280, 760);
    setMinimumSize(960, 560);
    applyStyle();
    setupToolBar();
    setupLayout();
    setupStatusBar();
    connectSignals();
}

// ——— 全局样式 ———
void MainWindow::applyStyle() {
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f6fa;
        }
        QToolBar {
            background: #ffffff;
            border-bottom: 1px solid #e0e0e0;
            padding: 4px 8px;
            spacing: 6px;
        }
        QToolBar QToolButton {
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 6px 14px;
            font-size: 13px;
            color: #333;
        }
        QToolBar QToolButton:hover {
            background: #e8f0fe;
            border-color: #c4d7f2;
        }
        QToolBar QToolButton:pressed {
            background: #d2e3fc;
        }
        QToolBar QToolButton#connectBtn {
            background: #1a73e8;
            color: #fff;
            font-weight: bold;
            border: none;
        }
        QToolBar QToolButton#connectBtn:hover {
            background: #1557b0;
        }
        QToolBar QToolButton#disconnectBtn {
            background: #5f6368;
            color: #fff;
            border: none;
        }
        QToolBar QToolButton#disconnectBtn:hover {
            background: #44474a;
        }
        QToolBar QToolButton#configBtn {
            border: 1px solid #dadce0;
            background: #fff;
        }
        QToolBar QToolButton#configBtn:hover {
            background: #f1f3f4;
        }
        QToolBar QToolButton#helpBtn {
            border: 1px solid #dadce0;
            background: #fff;
        }
        QToolBar QToolButton#helpBtn:hover {
            background: #f1f3f4;
        }
        QToolBar QToolButton#helpBtn::menu-indicator {
            image: none;
        }
        QMenu {
            background: #ffffff;
            border: 1px solid #dadce0;
            border-radius: 8px;
            padding: 4px 0;
        }
        QMenu::item {
            padding: 8px 32px 8px 16px;
            font-size: 13px;
            color: #333;
        }
        QMenu::item:selected {
            background: #e8f0fe;
            color: #1a73e8;
        }
        QMenu::separator {
            height: 1px;
            background: #e0e0e0;
            margin: 4px 8px;
        }
        QSplitter::handle {
            background: #e0e0e0;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }
        QTreeWidget {
            background: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 6px;
            font-size: 13px;
            padding: 4px;
        }
        QTreeWidget::item {
            padding: 6px 4px;
            border-radius: 3px;
        }
        QTreeWidget::item:selected {
            background: #e8f0fe;
            color: #1a73e8;
        }
        QTreeWidget::item:hover:!selected {
            background: #f1f3f4;
        }
        QGroupBox {
            font-weight: bold;
            color: #333;
            border: 1px solid #e0e0e0;
            border-radius: 6px;
            margin-top: 12px;
            padding: 16px 12px 12px 12px;
            background: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
        }
        QPlainTextEdit {
            background: #1e1e1e;
            color: #d4d4d4;
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
            padding: 8px;
        }
        QStatusBar {
            background: #ffffff;
            border-top: 1px solid #e0e0e0;
            font-size: 12px;
            padding: 2px 8px;
        }
        QLabel#sectionTitle {
            font-size: 13px;
            font-weight: bold;
            color: #5f6368;
            padding: 2px 0;
        }
        QPushButton#publishToggle {
            border: none;
            background: #f1f3f4;
            color: #5f6368;
            font-size: 13px;
            font-weight: bold;
            padding: 4px 8px;
            border-radius: 4px;
        }
        QPushButton#publishToggle:hover {
            background: #e8eaed;
        }
        QPushButton#copyBtn {
            border: 1px solid #dadce0;
            border-radius: 4px;
            padding: 4px 12px;
            font-size: 12px;
            background: #fff;
            color: #5f6368;
        }
        QPushButton#copyBtn:hover {
            background: #f1f3f4;
        }
        QScrollArea {
            border: none;
            background: transparent;
        }
    )");
}

// ——— 工具栏 ———
void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar("main");
    toolbar->setMovable(false);
    toolbar->setFloatable(false);

    // 左侧：配置按钮
    auto* configAct = toolbar->addAction("⚙ 配置中心");
    auto* configBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(configAct));
    if (configBtn) configBtn->setObjectName("configBtn");
    connect(configAct, &QAction::triggered, this, [this]() {
        ConfigDialog dlg(mDevMgr, this);
        if (dlg.exec() == QDialog::Accepted) {
            // Profile 切换在对话框内已完成，这里只需保存配置并重连
            mDevMgr->saveConfig(QApplication::applicationDirPath() + "/config/config.json");
            if (!mDevMgr->isConnected()) {
                mDevMgr->connectBroker();
            } else {
                // 已连接但配置可能变了，断开重连
                mDevMgr->disconnectBroker();
                mDevMgr->connectBroker();
            }
        }
    });

    // 功能中心按钮（配置与帮助之间）
    auto* featureBtn = new QToolButton(this);
    featureBtn->setText("🧰 功能中心");
    featureBtn->setObjectName("helpBtn");   // 复用帮助按钮样式
    featureBtn->setPopupMode(QToolButton::InstantPopup);
    featureBtn->setCursor(Qt::PointingHandCursor);
    {
        auto* menu = new QMenu(featureBtn);
        menu->addAction("🎮 远程调试", this, [this]() {
            if (!mDockCtrlDialog) return;
            mDockCtrlDialog->show();
            mDockCtrlDialog->raise();
            mDockCtrlDialog->activateWindow();
        });
        menu->addAction("🛫 飞行控制", this, [this]() {
            if (!mFlightCtrlDialog) return;
            mFlightCtrlDialog->show();
            mFlightCtrlDialog->raise();
            mFlightCtrlDialog->activateWindow();
        });
        menu->addSeparator();
        menu->addAction("📺 视频直播", this, [this]() {
            showVideoWindows();
        });
        menu->addAction("📢 PSDK功能", this, [this]() {
            if (!mPsdkSpeakerDialog) return;
            mPsdkSpeakerDialog->show();
            mPsdkSpeakerDialog->raise();
            mPsdkSpeakerDialog->activateWindow();
        });
        menu->addAction("🔧 运维工具", this, [this]() {
            if (!mMaintenanceDialog) return;
            mMaintenanceDialog->show();
            mMaintenanceDialog->raise();
            mMaintenanceDialog->activateWindow();
        });
        featureBtn->setMenu(menu);
    }
    toolbar->addWidget(featureBtn);

    // 帮助按钮（配置按钮右侧）
    auto* helpBtn = new QToolButton(this);
    helpBtn->setText("💡 帮助");
    helpBtn->setObjectName("helpBtn");
    helpBtn->setPopupMode(QToolButton::InstantPopup);
    helpBtn->setCursor(Qt::PointingHandCursor);
    {
        auto* menu = new QMenu(helpBtn);
        menu->addAction("🛠️ GitHub项目地址", this, []() {
            QDesktopServices::openUrl(QUrl("https://github.com/damon-liu/Dji-cloud-api-tool"));
        });
        // menu->addAction("🛠️ Gitee项目地址", this, []() {
        //     QDesktopServices::openUrl(QUrl("https://gitee.com/damon123-liu/Dji-cloud-api-tool"));
        // });
        menu->addSeparator();
        menu->addAction("📖 大疆上云 API 文档", this, []() {
            QDesktopServices::openUrl(QUrl("https://developer.dji.com/doc/cloud-api-tutorial/cn/api-reference/dock-to-cloud/mqtt/dock/dock3/properties.html"));
        });
        helpBtn->setMenu(menu);
    }
    toolbar->addWidget(helpBtn);

    // Spacer: pushes everything after it to the right
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Broker 信息标签（spacer 右侧）
    mBrokerLabel = new QLabel(" 未连接", this);
    mBrokerLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 0 12px;");
    toolbar->addWidget(mBrokerLabel);

    // 连接/断开按钮（右侧）
    mConnectAct = toolbar->addAction("● 连接");
    auto* connectBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(mConnectAct));
    if (connectBtn) connectBtn->setObjectName("connectBtn");
    connect(mConnectAct, &QAction::triggered, this, &MainWindow::onConnectAction);

    mDisconnectAct = toolbar->addAction("◎ 断开");
    auto* disconnectBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(mDisconnectAct));
    if (disconnectBtn) disconnectBtn->setObjectName("disconnectBtn");
    connect(mDisconnectAct, &QAction::triggered, this, &MainWindow::onDisconnectAction);
}

// ——— 主布局 ———
void MainWindow::setupLayout() {
    // === 左侧：设备树面板 ===
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(8, 8, 4, 8);
    leftLayout->setSpacing(4);

    // 区域标题 + 操作按钮
    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(4);
    auto* treeTitle = new QLabel(QString::fromUtf8("Devices\xe5\x88\x97\xe8\xa1\xa8"));
    treeTitle->setObjectName("sectionTitle");
    titleRow->addWidget(treeTitle);
    titleRow->addStretch();

    mAddDeviceBtn = new QPushButton("＋", this);
    mAddDeviceBtn->setCursor(Qt::PointingHandCursor);
    mAddDeviceBtn->setFixedSize(28, 28);
    mAddDeviceBtn->setToolTip("添加设备");
    mAddDeviceBtn->setStyleSheet(
        "QPushButton { background: #e8f5e9; color: #2e7d32; border: 1px solid #a5d6a7; "
        "border-radius: 4px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #c8e6c9; }");
    connect(mAddDeviceBtn, &QPushButton::clicked, this, &MainWindow::onAddDevice);
    titleRow->addWidget(mAddDeviceBtn);

    mDeleteDeviceBtn = new QPushButton("✕", this);
    mDeleteDeviceBtn->setCursor(Qt::PointingHandCursor);
    mDeleteDeviceBtn->setEnabled(false);
    mDeleteDeviceBtn->setFixedSize(28, 28);
    mDeleteDeviceBtn->setToolTip("删除设备");
    mDeleteDeviceBtn->setStyleSheet(
        "QPushButton { background: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
        "border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background: #ffcdd2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    connect(mDeleteDeviceBtn, &QPushButton::clicked, this, &MainWindow::onDeleteDevice);
    titleRow->addWidget(mDeleteDeviceBtn);

    leftLayout->addLayout(titleRow);

    // 设备树 + Topic 列表 垂直分割器（可拖拽调整高度）
    auto* leftSplitter = new QSplitter(Qt::Vertical, this);

    // 设备树
    mDeviceTree = new DeviceTreeWidget(this);
    mDeviceTree->setMinimumWidth(380);
    mDeviceTree->setMaximumWidth(520);
    leftSplitter->addWidget(mDeviceTree);

    // Topic 列表面板（设备树下方）
    mTopicListWidget = new TopicListWidget(this);
    leftSplitter->addWidget(mTopicListWidget);

    leftSplitter->setStretchFactor(0, 3);   // 设备树占 3/5
    leftSplitter->setStretchFactor(1, 2);   // Topic 列表占 2/5
    leftSplitter->setChildrenCollapsible(false);

    leftLayout->addWidget(leftSplitter, 1);

    // === 右侧：OSD + JSON 水平分割 ===
    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 8, 8, 8);
    rightLayout->setSpacing(4);

    // OSD | JSON
    mOsdPanel = new OsdPanel(this);
    mRawJsonPanel = new RawJsonPanel(this);
    mTopicParsePanel = new TopicParsePanel(this);

    auto* osdScroll = new QScrollArea(this);
    osdScroll->setWidget(mOsdPanel);
    osdScroll->setWidgetResizable(true);
    osdScroll->setFrameShape(QFrame::NoFrame);
    osdScroll->setMinimumHeight(180);  // 确保并排的 GroupBox 有足够空间

    auto* parseScroll = new QScrollArea(this);
    parseScroll->setWidget(mTopicParsePanel);
    parseScroll->setWidgetResizable(true);
    parseScroll->setFrameShape(QFrame::NoFrame);

    // 左半区：OSD 面板 + JSON 解析面板 垂直堆叠
    auto* leftHalf = new QWidget(this);
    auto* leftHalfLayout = new QVBoxLayout(leftHalf);
    leftHalfLayout->setContentsMargins(0, 0, 0, 0);
    leftHalfLayout->setSpacing(4);
    leftHalfLayout->addWidget(osdScroll, 1);       // OSD 设备信息
    leftHalfLayout->addWidget(parseScroll, 4);     // JSON 解析

    mRawJsonPanel->setMaximumWidth(520);  // 与设备列表宽度一致

    mRightSplitter = new QSplitter(Qt::Horizontal, this);
    mRightSplitter->addWidget(leftHalf);
    mRightSplitter->addWidget(mRawJsonPanel);
    mRightSplitter->setStretchFactor(0, 3);
    mRightSplitter->setStretchFactor(1, 1);
    mRightSplitter->setSizes({600, 440});

    // 垂直分割器：上方 OSD/JSON 区域 | 下方 Topic 下发面板（支持拖拽调整高度）
    auto* verticalSplitter = new QSplitter(Qt::Vertical, this);
    verticalSplitter->addWidget(mRightSplitter);
    verticalSplitter->setStretchFactor(0, 1);

    // Topic 下发（折叠）
    mPublishPanel = new PublishPanel(this);
    mPublishPanel->setVisible(false);
    mPublishPanel->setMinimumHeight(120);  // 保证基本可操作区域

    verticalSplitter->addWidget(mPublishPanel);
    verticalSplitter->setStretchFactor(1, 0);

    rightLayout->addWidget(verticalSplitter, 1);

    mTogglePublishBtn = new QPushButton("▶ Topic 下发", this);
    mTogglePublishBtn->setObjectName("publishToggle");
    mTogglePublishBtn->setCheckable(true);
    mTogglePublishBtn->setCursor(Qt::PointingHandCursor);
    connect(mTogglePublishBtn, &QPushButton::toggled, this, [this](bool checked) {
        mPublishPanel->setVisible(checked);
        mTogglePublishBtn->setText(checked ? "◢ Topic 下发" : "▶ Topic 下发");
    });

    rightLayout->addWidget(mTogglePublishBtn);

    // === 主分割器 ===
    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);

    setCentralWidget(mainSplitter);

    // 加载 publish 模板 + 初始连接状态
    mPublishPanel->loadTemplates(QApplication::applicationDirPath() + "/config/topic-send-construct/topic-send-construct.md");
    mPublishPanel->setConnected(mDevMgr->isConnected());

    // 机场控制独立窗口（功能中心菜单打开）
    mDockCtrlDialog = new DockControlDialog(this);
    mDockControlPanel = mDockCtrlDialog->panel();
    mDockControlPanel->setConnected(mDevMgr->isConnected());

    // 飞行控制独立窗口（功能中心菜单打开）
    mFlightCtrlDialog = new FlightControlDialog(this);
    mFlightControlPanel = mFlightCtrlDialog->panel();
    mFlightControlPanel->setConnected(mDevMgr->isConnected());

    // 运维模式独立窗口（功能中心菜单打开）
    mMaintenanceDialog = new MaintenanceDialog(this);
    mMaintenancePanel = mMaintenanceDialog->panel();

    // PSDK喊话器独立窗口（功能中心菜单打开）
    mPsdkSpeakerDialog = new PsdkSpeakerDialog(this);
    mPsdkSpeakerPanel = mPsdkSpeakerDialog->panel();
    mPsdkSpeakerPanel->setConnected(mDevMgr->isConnected());

}

// ——— 状态栏 ———
void MainWindow::setupStatusBar() {
    mStatusLabel      = new QLabel("🔴 未连接");
    mDeviceCountLabel = new QLabel("设备: 0");

    mStatusLabel->setStyleSheet("font-weight: bold; padding: 0 8px;");
    mDeviceCountLabel->setStyleSheet("padding: 0 8px;");

    // 版本信息 — 真正居中
    auto* versionContainer = new QWidget(this);
    auto* versionLayout = new QHBoxLayout(versionContainer);
    versionLayout->setContentsMargins(0, 0, 0, 0);
    versionLayout->setAlignment(Qt::AlignCenter);
    mVersionLabel = new QLabel("v1.0.4 · github.com/damon-liu/Dji-cloud-api-tool");
    mVersionLabel->setStyleSheet(
        "color: #80868b; font-size: 11px; letter-spacing: 0.5px;");
    versionLayout->addWidget(mVersionLabel);

    statusBar()->addWidget(mStatusLabel);
    statusBar()->addWidget(versionContainer, 1);
    statusBar()->addPermanentWidget(mDeviceCountLabel);
}

// ——— 信号连接 ———
void MainWindow::connectSignals() {
    connect(mDeviceTree, &DeviceTreeWidget::deviceSelected,
            this, &MainWindow::onDeviceSelected);
    connect(mDeviceTree, &DeviceTreeWidget::deviceRenameRequested,
            this, [this](const QString& sn, const QString& newName) {
        mDevMgr->renameDevice(sn, newName);
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        // 更新 OSD 面板中的设备名称
        QString currentSn = mDeviceTree->selectedDeviceSn();
        if (currentSn == sn)
            onDeviceSelected(sn);
    });

    // TopicListWidget signals → DeviceManager
    connect(mTopicListWidget, &TopicListWidget::topicAdded,
            this, [this](const QString& sn, const QString& topic) {
        mDevMgr->addTopic(sn, topic);
        refreshTopicList(sn);
    });
    connect(mTopicListWidget, &TopicListWidget::topicToggled,
            this, [this](const QString& sn, const QString& topic) {
        bool currentlyEnabled = mDevMgr->isTopicEnabled(sn, topic);
        mDevMgr->setTopicEnabled(sn, topic, !currentlyEnabled);
        refreshTopicList(sn);
    });
    connect(mTopicListWidget, &TopicListWidget::topicRemoved,
            this, [this](const QString& sn, const QString& topic) {
        mDevMgr->removeTopic(sn, topic);
        refreshTopicList(sn);
    });

    // Topic 排序信号 → DeviceManager 持久化
    connect(mTopicListWidget, &TopicListWidget::topicOrderChanged,
            this, [this](const QString& sn, const QStringList& ordered) {
        mDevMgr->reorderTopics(sn, ordered);
        refreshTopicList(sn);
    });

    // Topic 选中变化 → 原始 JSON 按 topic 过滤（仅显示已启用/订阅的 topic）
    connect(mTopicListWidget, &TopicListWidget::topicSelectionChanged,
            this, [this](const QString& selectedTopic) {
        QString sn = mDeviceTree->selectedDeviceSn();
        if (sn.isEmpty()) return;
        // 仅当 topic 已启用（订阅中）时才显示数据，禁用的 topic 不显示
        if (!selectedTopic.isEmpty() && mDevMgr->isTopicEnabled(sn, selectedTopic)) {
            mRawJsonPanel->setCaptureTarget(sn, selectedTopic);
            mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn, selectedTopic), selectedTopic);
        }
        else
            mRawJsonPanel->setJson({});
    });

    connect(mDevMgr, &DeviceManager::deviceOsdUpdated,
            this, &MainWindow::onOsdUpdated);

    connect(mDevMgr, &DeviceManager::brokerConnected, this, [this]() {
        mStatusLabel->setText("🟢 已连接");
        mBrokerLabel->setText(" " + mDevMgr->mqttConfig().host + ":" +
            QString::number(mDevMgr->mqttConfig().port));
        mBrokerLabel->setStyleSheet("color: #2e7d32; font-size: 12px; padding: 0 12px;");
        mConnectAct->setEnabled(false);
        mDisconnectAct->setEnabled(true);
        mOsdPanel->resume();
        mTopicParsePanel->resume();
        mPublishPanel->setConnected(true);
        mDockControlPanel->setConnected(true);
        mFlightControlPanel->setConnected(true);
        mPsdkSpeakerPanel->setConnected(true);
        updateStatusBar();

        // 连接成功后自动选中首个设备（优先机场，方便查看控制面板）
        if (!mUserDeselected && mDeviceTree->selectedDeviceSn().isEmpty()) {
            const auto& allDevs = mDevMgr->allDevices();
            const DeviceInfo* target = nullptr;
            // 优先选机场
            for (auto* d : allDevs) {
                if (d->type == DeviceType::Dock) {
                    target = d;
                    break;
                }
            }
            // 无机建则选首个顶级设备
            if (!target) {
                const auto& topLevel = mDevMgr->topLevelDevices();
                if (!topLevel.isEmpty())
                    target = topLevel.first();
            }
            if (target)
                mDeviceTree->selectDevice(target->sn);
        }
    });
    connect(mDevMgr, &DeviceManager::brokerDisconnected, this, [this]() {
        mStatusLabel->setText("🔴 未连接");
        mBrokerLabel->setText(" 未连接");
        mBrokerLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 0 12px;");
        mConnectAct->setEnabled(true);
        mDisconnectAct->setEnabled(false);
        mOsdPanel->pause();
        mTopicParsePanel->pause();
        mPublishPanel->setConnected(false);
        mDockControlPanel->setConnected(false);
        mFlightControlPanel->setConnected(false);
        mPsdkSpeakerPanel->setConnected(false);
        mUserDeselected = false;
        hideVideoWindows();
    });
    connect(mDevMgr, &DeviceManager::brokerError, this, [this](const QString& err) {
        statusBar()->showMessage("MQTT 错误: " + err, 5000);
    });
    connect(mDevMgr, &DeviceManager::profileSwitched, this, [this](const QString& name) {
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        mOsdPanel->clear();
        mRawJsonPanel->clear();
        mTopicListWidget->clearTopics();
        mTopicParsePanel->clear();
        mUserDeselected = false;
        updateStatusBar();
    });

    connect(mDevMgr, &DeviceManager::deviceAdded, this, [this]() {
        QString prevSelected = mDeviceTree->selectedDeviceSn();
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        // 恢复之前的选中状态
        if (!prevSelected.isEmpty())
            mDeviceTree->selectDevice(prevSelected);
        refreshTopicList(prevSelected);
        updateStatusBar();
    });
    connect(mDevMgr, &DeviceManager::deviceRemoved, this, [this]() {
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        mOsdPanel->clear();
        mRawJsonPanel->clear();
        mTopicListWidget->clearTopics();
        updateStatusBar();
    });

    connect(mDevMgr, &DeviceManager::deviceOnlineChanged,
            this, [this](const QString& sn, bool online) {
        Q_UNUSED(sn) Q_UNUSED(online)
        QString prevSelected = mDeviceTree->selectedDeviceSn();
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        // rebuild 会清空选中，恢复之前自动选中的设备
        if (!prevSelected.isEmpty())
            mDeviceTree->selectDevice(prevSelected);
    });

    // TopicParsePanel: topic 选中变化 → 更新解析面板
    // 使用 topic 所属设备的 SN 和类型查找映射配置，确保 dock/aircraft OSD 使用各自的 groups
    connect(mTopicListWidget, &TopicListWidget::topicSelectionChanged,
            mTopicParsePanel, [this](const QString& selectedTopic) {
        QString dataSn;
        QString devType;
        if (!selectedTopic.isEmpty()) {
            // 优先用 topic 的所属设备（topic 可能是 dock 列表中携带的 aircraft OSD）
            QString topicSn = mDevMgr->deviceForTopic(selectedTopic);
            if (topicSn.isEmpty())
                topicSn = mDeviceTree->selectedDeviceSn();
            DeviceInfo* topicDev = mDevMgr->device(topicSn);
            if (topicDev && mDevMgr->isTopicEnabled(topicSn, selectedTopic)) {
                dataSn = topicSn;
                devType = (topicDev->type == DeviceType::Dock) ? "dock" : "aircraft";
            }
        }
        mTopicParsePanel->setTopic(dataSn, selectedTopic, devType);
    });

    // 加载 topic 映射配置
    {
        mTopicMapping = new TopicMapping();
        QString mappingPath = QApplication::applicationDirPath() + "/config/topic_mappings.json";
        if (!mTopicMapping->load(mappingPath)) {
            qWarning() << "MainWindow: failed to load topic_mappings.json, using built-in fallback";
            mTopicMapping->loadFromString(TOPIC_MAPPINGS_BUILTIN);
            // Only auto-generate if file is genuinely missing (not just corrupt)
            if (!QFile::exists(mappingPath)) {
                QFile outFile(mappingPath);
                if (outFile.open(QIODevice::WriteOnly)) {
                    outFile.write(TOPIC_MAPPINGS_BUILTIN);
                    outFile.close();
                    qDebug() << "MainWindow: auto-generated default topic_mappings.json";
                }
            }
        }
        mTopicParsePanel->setTopicMapping(mTopicMapping);
    }
    mTopicParsePanel->setDeviceManager(mDevMgr);

    // Topic 全量切换
    connect(mTopicListWidget, &TopicListWidget::topicAllToggled,
            this, [this](const QString& sn, bool enabled) {
        mDevMgr->setAllTopicsEnabled(sn, enabled);
        refreshTopicList(sn);
    });

    mOsdPanel->setDeviceManager(mDevMgr);

    // PublishPanel → DeviceManager
    connect(mPublishPanel, &PublishPanel::publishRequested,
            mDevMgr, &DeviceManager::publishMessage);
    // DeviceManager → PublishPanel
    connect(mDevMgr, &DeviceManager::publishResult,
            mPublishPanel, &PublishPanel::onPublishResult);

    // DockControlPanel ↔ DeviceManager
    connect(mDockControlPanel, &DockControlPanel::commandRequested,
            mDevMgr, &DeviceManager::executeDockCommand);
    connect(mDevMgr, &DeviceManager::dockCommandStateChanged,
            mDockControlPanel, &DockControlPanel::onCommandStateChanged);
    // 指令结果同步到 Topic 下发记录
    connect(mDevMgr, &DeviceManager::dockCommandStateChanged,
            this, [this](const DockCommandResult& result) {
        if (result.state == DockCommandState::Publishing
            || result.state == DockCommandState::WaitingReply)
            return;
        bool success = (result.state == DockCommandState::Succeeded);
        QString topic = QStringLiteral("thing/product/%1/services").arg(result.gatewaySn);
        mPublishPanel->appendCommandRecord(topic, result.requestJson, result.replyJson,
                                            success, DockCommandBuilder::displayName(result.type));
    });

    // FlightControlPanel ↔ DeviceManager
    connect(mFlightControlPanel, &FlightControlPanel::commandRequested,
            mDevMgr, &DeviceManager::executeDockCommand);
    connect(mDevMgr, &DeviceManager::dockCommandStateChanged,
            mFlightControlPanel, &FlightControlPanel::onCommandStateChanged);

    // PsdkSpeakerPanel ↔ DeviceManager
    connect(mPsdkSpeakerPanel, &PsdkSpeakerPanel::commandRequested,
            mDevMgr, &DeviceManager::executeDockCommand);
    connect(mDevMgr, &DeviceManager::dockCommandStateChanged,
            mPsdkSpeakerPanel, &PsdkSpeakerPanel::onCommandStateChanged);
    connect(mDevMgr, &DeviceManager::speakerProgressUpdated,
            mPsdkSpeakerPanel, &PsdkSpeakerPanel::onSpeakerProgress);

    // è®¾å¤å¨çº¿ç¶æåå â å·æ°æºåºåè¡¨
    connect(mDevMgr, &DeviceManager::deviceOnlineChanged,
            this, [this](const QString& sn, bool online) {
        Q_UNUSED(sn); Q_UNUSED(online);
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshDockControlList(currentSn);
    });

    connect(mDevMgr, &DeviceManager::deviceAdded,
            this, [this](const QString&) {
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshDockControlList(currentSn);
    });
    connect(mDevMgr, &DeviceManager::deviceRemoved,
            this, [this](const QString&) {
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshDockControlList(currentSn);
    });

    mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
    mDisconnectAct->setEnabled(false);
    updateStatusBar();
}

void MainWindow::refreshDockControlList(const QString& currentSn) {
    const auto& allDevs = mDevMgr->allDevices();
    QVector<DeviceInfo> onlineDocks;
    double dockLat = 0.0;
    double dockLon = 0.0;
    double dockAlt = 0.0;
    QString dockLatStr;
    QString dockLonStr;
    QString dockAltStr;

    for (auto* d : allDevs) {
        if (d->type == DeviceType::Dock && d->online)
            onlineDocks.append(*d);
    }

    auto resolveDockSn = [&](const QString& sn) -> QString {
        if (!sn.isEmpty()) {
            DeviceInfo* dev = mDevMgr->device(sn);
            if (dev && dev->type == DeviceType::Dock)
                return sn;
            if (dev && !dev->parentSn.isEmpty())
                return dev->parentSn;
        }
        return {};
    };

    QString dockSn = resolveDockSn(currentSn);
    if (dockSn.isEmpty() && !onlineDocks.isEmpty()) {
        dockSn = onlineDocks.first().sn;
    }

    if (!dockSn.isEmpty()) {
        const DockOsd* osd = mDevMgr->latestDockOsd(dockSn);
        if (osd && osd->valid) {
            dockLat = osd->latitude;
            dockLon = osd->longitude;
            dockAlt = osd->height;
            dockLatStr = osd->latitudeStr;
            dockLonStr = osd->longitudeStr;
            dockAltStr = osd->heightStr;
        }
    }

    mDockControlPanel->setAvailableDocks(onlineDocks, currentSn, dockLat, dockLon, dockAlt);
    mFlightControlPanel->setAvailableDocks(onlineDocks, currentSn, dockLat, dockLon, dockAlt,
                                           dockLatStr, dockLonStr, dockAltStr);
    mPsdkSpeakerPanel->setAvailableDocks(onlineDocks, currentSn, dockLat, dockLon);
}

// ——— 设备选择 ———
void MainWindow::onDeviceSelected(const QString& sn) {
    if (sn.isEmpty()) {
        // 用户手动取消选中，清空所有面板
        mUserDeselected = true;
        mOsdPanel->clear();
        mRawJsonPanel->clear();
        mPublishPanel->setGatewaySn({});
        mPublishPanel->setTopics({});
        mDockControlPanel->clearDevice();
        mFlightControlPanel->clearDevice();
        mPsdkSpeakerPanel->clearDevice();
        mTopicListWidget->clearTopics();
        mTopicParsePanel->clear();
        mDeleteDeviceBtn->setEnabled(false);
        mAddDeviceBtn->setEnabled(true);
        return;
    }

    DeviceInfo* dev = mDevMgr->device(sn);
    if (!dev) return;

    const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(sn);
    const DockOsd* dockOsd   = mDevMgr->latestDockOsd(sn);

    // 机场设备：同时查找子飞机的 OSD 一起展示
    if (dev->type == DeviceType::Dock && !airOsd) {
        const auto& allDevs = mDevMgr->allDevices();
        for (auto* d : allDevs) {
            if (d->parentSn == sn && d->type == DeviceType::Aircraft) {
                airOsd = mDevMgr->latestAircraftOsd(d->sn);
                break;
            }
        }
    }

    mOsdPanel->showOsd(dev, airOsd, dockOsd, mDevMgr->latestRawJson(sn));
    mOsdPanel->setCurrentSn(sn);

    mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn));
    mPublishPanel->setDeviceSn(sn);
    // gateway_sn: 机场设备 = 自身 SN，飞机设备 = 父机场 SN
    if (dev->type == DeviceType::Dock)
        mPublishPanel->setGatewaySn(sn);
    else if (!dev->parentSn.isEmpty())
        mPublishPanel->setGatewaySn(dev->parentSn);
    mPublishPanel->setTopics(mDevMgr->topicsForDevice(sn));

    // 机场控制面板：机场 → 自身；飞机 → 父机场；其他 → 清空
    QString dockSn;
    if (dev->type == DeviceType::Dock)
        dockSn = sn;
    else if (!dev->parentSn.isEmpty())
        dockSn = dev->parentSn;

    if (!dockSn.isEmpty()) {
        DeviceInfo* dockDev = mDevMgr->device(dockSn);
        if (dockDev) {
            mDockControlPanel->setDevice(dockDev->name, dockDev->sn, dockDev->online);
            mFlightControlPanel->setDevice(dockDev->name, dockDev->sn, dockDev->online);
        } else {
            mDockControlPanel->clearDevice();
            mFlightControlPanel->clearDevice();
        }
    } else {
        mDockControlPanel->clearDevice();
        mFlightControlPanel->clearDevice();
    }

    refreshDockControlList(dockSn);

    // 刷新 topic 列表
    refreshTopicList(sn);

    // 启用操作按钮
    mDeleteDeviceBtn->setEnabled(true);

    // 添加按钮始终可用
    mAddDeviceBtn->setEnabled(true);

    // 更新 TopicParsePanel
    // 使用 topic 所属设备的 SN 和类型，确保 dock/aircraft OSD 使用各自的 groups
    QString selectedTopic = mTopicListWidget->selectedTopic();
    QString topicDataType;
    QString dataSn = sn;
    if (!selectedTopic.isEmpty()) {
        QString topicSn = mDevMgr->deviceForTopic(selectedTopic);
        if (!topicSn.isEmpty())
            dataSn = topicSn;
        DeviceInfo* topicDev = mDevMgr->device(dataSn);
        QString devType = topicDev
            ? (topicDev->type == DeviceType::Dock ? "dock" : "aircraft")
            : QString();
        topicDataType = devType;
    }
    mTopicParsePanel->setTopic(dataSn, selectedTopic, topicDataType);
}

void MainWindow::onOsdUpdated(const QString& sn, const QString& topic, const QString& rawJson) {
    QString selectedSn = mDeviceTree->selectedDeviceSn();
    if (selectedSn.isEmpty() || !mDevMgr)
        return;

    // 确认当前消息应触发 UI 更新：
    //   - SN 直接匹配选中设备；或
    //   - 选中机场，且消息来自该机场关联的子飞机
    bool shouldUpdate = (selectedSn == sn);
    if (!shouldUpdate) {
        DeviceInfo* selectedDev = mDevMgr->device(selectedSn);
        if (selectedDev && selectedDev->type == DeviceType::Dock) {
            for (auto* d : mDevMgr->allDevices()) {
                if (d->sn == sn && d->parentSn == selectedSn) {
                    shouldUpdate = true;
                    break;
                }
            }
        }
    }
    if (!shouldUpdate)
        return;

    // 刷新 OSD 面板数据（轻量更新，不重建 JSON 历史）
    DeviceInfo* dev = mDevMgr->device(selectedSn);
    if (dev) {
        const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(selectedSn);
        const DockOsd* dockOsd   = mDevMgr->latestDockOsd(selectedSn);

        // 机场设备：同时查找子飞机的 OSD 一起展示
        if (dev->type == DeviceType::Dock && !airOsd) {
            const auto& allDevs = mDevMgr->allDevices();
            for (auto* d : allDevs) {
                if (d->parentSn == selectedSn && d->type == DeviceType::Aircraft) {
                    airOsd = mDevMgr->latestAircraftOsd(d->sn);
                    break;
                }
            }
        }

        mOsdPanel->showOsd(dev, airOsd, dockOsd, mDevMgr->latestRawJson(selectedSn, topic));
    }

    // 实时同步机场坐标到飞行控制面板
    QString flightGwSn = mFlightControlPanel->currentGatewaySn();
    if (!flightGwSn.isEmpty()) {
        const DockOsd* flightDockOsd = mDevMgr->latestDockOsd(flightGwSn);
        if (flightDockOsd && flightDockOsd->valid) {
            mFlightControlPanel->updateDockPosition(
                flightDockOsd->latitude, flightDockOsd->longitude,
                flightDockOsd->height,
                flightDockOsd->latitudeStr, flightDockOsd->longitudeStr,
                flightDockOsd->heightStr);
        }
    }

    // 按用户选中的 topic 过滤追加
    QString selectedTopic = mTopicListWidget->selectedTopic();
    if (!selectedTopic.isEmpty() && topic != selectedTopic)
        return;  // 不是用户选中的 topic，跳过

    if (!rawJson.isEmpty())
        mRawJsonPanel->appendJson(rawJson, topic);
}

// ——— 连接操作 ———
void MainWindow::onConnectAction() {
    mDevMgr->connectBroker();
}

void MainWindow::onDisconnectAction() {
    mDevMgr->disconnectBroker();
}

// ——— 设备操作 ———
void MainWindow::onAddDevice() {
    QString selectedSn = mDeviceTree->selectedDeviceSn();
    DeviceInfo* selectedDev = nullptr;
    if (!selectedSn.isEmpty())
        selectedDev = mDevMgr->device(selectedSn);

    // 选中机场时添加子飞机，选中飞机或无选中时添加顶级设备
    bool addingChild = (selectedDev && selectedDev->type == DeviceType::Dock);

    QString sn;
    QString name;
    DeviceType type;

    if (addingChild) {
        sn = QInputDialog::getText(this, "添加手飞无人机",
            QString("为机场「%1」添加手飞无人机\n设备序列号 (SN):").arg(selectedDev->name));
        if (sn.trimmed().isEmpty()) return;

        name = QInputDialog::getText(this, "添加手飞无人机", "设备名称:",
            QLineEdit::Normal, sn.trimmed());
        if (name.trimmed().isEmpty())
            name = sn.trimmed();
        type = DeviceType::Aircraft;
    } else {
        QString typeStr = QInputDialog::getItem(this, "添加设备", "选择设备类型:",
            {"Dock (机场)", "Pilot (手飞飞机)"}, 0, false);
        if (typeStr.isEmpty()) return;

        type = typeStr.contains("Dock") ? DeviceType::Dock : DeviceType::Aircraft;

        sn = QInputDialog::getText(this, "添加设备", "设备序列号 (SN):");
        if (sn.trimmed().isEmpty()) return;

        name = QInputDialog::getText(this, "添加设备", "设备名称:",
            QLineEdit::Normal, sn.trimmed());
        if (name.trimmed().isEmpty())
            name = sn.trimmed();
    }

    // 默认订阅 OSD topic
    QString osdTopic = QString("thing/product/%1/osd").arg(sn.trimmed());
    QStringList defaultTopics;
    defaultTopics << osdTopic;

    DeviceInfo info;
    info.sn   = sn.trimmed();
    info.name = name.trimmed();
    info.type = type;
    if (addingChild)
        info.parentSn = selectedDev->sn;

    mDevMgr->addDevice(info, defaultTopics);
}

void MainWindow::onDeleteDevice() {
    QString sn = mDeviceTree->selectedDeviceSn();
    if (sn.isEmpty()) {
        QMessageBox::information(this, "提示", "请先在设备列表中选择一个设备。");
        return;
    }

    auto ret = QMessageBox::question(this, "确认删除",
        "确定要删除该设备及其所有 Topic？",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        mDevMgr->removeDevice(sn);
        mDeleteDeviceBtn->setEnabled(false);
    }
}

void MainWindow::refreshTopicList(const QString& sn) {
    if (sn.isEmpty()) {
        mTopicListWidget->clearTopics();
        return;
    }
    QStringList allTopics = mDevMgr->topicsForDevice(sn);

    // 方案A：dock 设备的 topic 列表只显示属于自己的 topic，
    // 子设备（飞机）的 topic 在选中子设备节点时才显示
    DeviceInfo* dev = mDevMgr->device(sn);
    QStringList filteredTopics;
    if (dev && dev->type == DeviceType::Dock) {
        for (const auto& t : allTopics) {
            QString ownerSn = mDevMgr->deviceForTopic(t);
            // ownerSn 为空（auto-detection 前）或等于本设备 → 保留
            if (ownerSn.isEmpty() || ownerSn == sn)
                filteredTopics.append(t);
        }
    } else {
        filteredTopics = allTopics;
    }

    // Collect disabled topics from DeviceManager
    QSet<QString> disabled;
    for (const auto& t : filteredTopics) {
        if (!mDevMgr->isTopicEnabled(sn, t))
            disabled.insert(t);
    }
    mTopicListWidget->setTopics(sn, filteredTopics, disabled);
}

void MainWindow::updateStatusBar() {
    mDeviceCountLabel->setText("设备: " +
        QString::number(mDevMgr->allDevices().size()));
}

void MainWindow::showVideoWindows() {
#ifdef HAS_VLC
    if (!mVlcInstance) {
        mVlcInstance = libvlc_new(0, nullptr);
        if (!mVlcInstance) {
            qWarning() << "MainWindow: libvlc_new failed";
        } else {
            qDebug() << "MainWindow: VLC initialized successfully (lazy)";
        }
    }
#endif

    StreamUrlConfig urls = mDevMgr->streamUrls();

    if (mVideoWindows.isEmpty()) {
        // 窗口0: 飞机 — 切换镜头：红外(默认)/变焦/广角
        auto* aircraftWin = new VideoStreamWindow(
            0,
            QString::fromUtf8("飞机"),
            QString::fromUtf8("红外相机"),
            QStringList{
                QString::fromUtf8("红外相机"),
                QString::fromUtf8("变焦相机"),
                QString::fromUtf8("广角相机")
            },
            QString::fromUtf8("切换镜头"),
            nullptr);
#ifdef HAS_VLC
        aircraftWin->setVlcInstance(mVlcInstance);
#endif
        aircraftWin->setStreamUrls(urls.aircraft);
        mVideoWindows.append(aircraftWin);

        // 窗口1: 机场 — 切换视频：机场外(默认)/机场内
        auto* dockWin = new VideoStreamWindow(
            1,
            QString::fromUtf8("机场"),
            QString::fromUtf8("机场外视频"),
            QStringList{
                QString::fromUtf8("机场外视频"),
                QString::fromUtf8("机场内视频")
            },
            QString::fromUtf8("切换视频"),
            nullptr);
#ifdef HAS_VLC
        dockWin->setVlcInstance(mVlcInstance);
#endif
        dockWin->setStreamUrls(urls.dock);
        mVideoWindows.append(dockWin);
    }

    // 缩小至原来的 2/3，固定在屏幕右下角
    int videoW = 426;
    int videoH = 266;
    int gap = 20;
    int bottomMargin = 40;

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeo = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    int x = screenGeo.right() - videoW - gap;
    int y = screenGeo.bottom() - (2 * videoH + gap) - bottomMargin;

    for (int i = 0; i < mVideoWindows.size(); ++i) {
        mVideoWindows[i]->resize(videoW, videoH);
        mVideoWindows[i]->move(x, y + i * (videoH + gap));
        mVideoWindows[i]->show();
        mVideoWindows[i]->raise();
    }
}

void MainWindow::hideVideoWindows() {
    for (auto* win : mVideoWindows) {
        win->hide();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // 关闭所有视频直播窗口
    for (auto* win : mVideoWindows) {
        win->close();       // non-spontaneous → accept → 真正关闭
        win->deleteLater();
    }
    mVideoWindows.clear();

#ifdef HAS_VLC
    if (mVlcInstance) {
        libvlc_release(mVlcInstance);
        mVlcInstance = nullptr;
    }
#endif

    event->accept();
}
