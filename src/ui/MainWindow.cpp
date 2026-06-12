#include "MainWindow.h"
#include "ConfigDialog.h"
#include "TopicEditDialog.h"
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSet>
#include <QFrame>
#include <QInputDialog>
#include <QToolButton>
#include <QFile>
#include "TopicMapping.h"

// 内置默认 topic 映射 JSON（文件缺失时降级使用）
static const char* TOPIC_MAPPINGS_BUILTIN = R"(
{
    "topics": {
        "thing/product/{sn}/osd": {
            "description": "OSD 遥测数据",
            "fields": {
                "job_number": {"zh":"累计作业次数","unit":"次"},
                "electric_supply_voltage": {"zh":"供电电压","unit":"mV"},
                "working_voltage": {"zh":"工作电压","unit":"mV"},
                "wind_speed": {"zh":"风速","unit":"m/s"},
                "environment_temperature": {"zh":"环境温度","unit":"℃"},
                "humidity": {"zh":"湿度","unit":"%"},
                "latitude": {"zh":"纬度","unit":"°"},
                "longitude": {"zh":"经度","unit":"°"},
                "height": {"zh":"海拔高度","unit":"m"},
                "battery.capacity_percent": {"zh":"电池电量","unit":"%"},
                "horizontal_speed": {"zh":"水平速度","unit":"m/s"},
                "vertical_speed": {"zh":"垂直速度","unit":"m/s"},
                "attitude_head": {"zh":"航向角","unit":"°"},
                "attitude_pitch": {"zh":"俯仰角","unit":"°"},
                "attitude_roll": {"zh":"横滚角","unit":"°"},
                "home_distance": {"zh":"距Home距离","unit":"m"},
                "mode_code": {"zh":"模式码","unit":"","values":{"0":"待机","4":"自动起飞","5":"航线飞行","9":"自动返航","10":"自动降落"}},
                "drone_in_dock": {"zh":"飞机在舱","unit":"","values":{"0":"否","1":"是"}},
                "cover_state": {"zh":"舱盖","unit":"","values":{"0":"关闭","1":"打开"}},
                "position_state.gps_number": {"zh":"GPS搜星","unit":""},
                "position_state.rtk_number": {"zh":"RTK搜星","unit":""}
            },
            "groups": [
                {"id":"basic","label":"📋 基础信息","keys":["job_number","mode_code","drone_in_dock","cover_state"]},
                {"id":"power","label":"🔋 电源","keys":["electric_supply_voltage","working_voltage","battery.capacity_percent"]},
                {"id":"flight","label":"✈ 飞行","keys":["horizontal_speed","vertical_speed","attitude_head","attitude_pitch","attitude_roll","home_distance"]},
                {"id":"position","label":"📍 定位","keys":["latitude","longitude","height","position_state.gps_number","position_state.rtk_number"]},
                {"id":"environment","label":"🌡 环境","keys":["wind_speed","environment_temperature","humidity"]}
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
            font-size: 12px;
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
    auto* configAct = toolbar->addAction("⚙ 配置");
    auto* configBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(configAct));
    if (configBtn) configBtn->setObjectName("configBtn");
    connect(configAct, &QAction::triggered, this, [this]() {
        ConfigDialog dlg(mDevMgr->mqttConfig(), this);
        if (dlg.exec() == QDialog::Accepted) {
            mDevMgr->setMqttConfig(dlg.getConfig());
            if (!mDevMgr->isConnected()) {
                mDevMgr->connectBroker();
            }
        }
    });

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
    auto* treeTitle = new QLabel("设备列表");
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

    // 设备树（占满宽度）
    mDeviceTree = new DeviceTreeWidget(this);
    mDeviceTree->setMinimumWidth(310);
    mDeviceTree->setMaximumWidth(440);
    leftLayout->addWidget(mDeviceTree, 1);

    // === Topic 列表面板（设备树下方） ===
    mTopicListWidget = new TopicListWidget(this);
    mTopicListWidget->setMaximumHeight(200);
    leftLayout->addWidget(mTopicListWidget);

    // === 右侧：OSD + JSON 水平分割 ===
    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 8, 8, 8);
    rightLayout->setSpacing(4);

    // OSD | JSON
    mOsdPanel = new OsdPanel(this);
    mRawJsonPanel = new RawJsonPanel(this);
    mOsdParsePanel = new OsdParsePanel(this);

    auto* osdScroll = new QScrollArea(this);
    osdScroll->setWidget(mOsdPanel);
    osdScroll->setWidgetResizable(true);
    osdScroll->setFrameShape(QFrame::NoFrame);

    auto* parseScroll = new QScrollArea(this);
    parseScroll->setWidget(mOsdParsePanel);
    parseScroll->setWidgetResizable(true);
    parseScroll->setFrameShape(QFrame::NoFrame);

    // 左半区：OSD 面板 + JSON 解析面板 垂直堆叠
    auto* leftHalf = new QWidget(this);
    auto* leftHalfLayout = new QVBoxLayout(leftHalf);
    leftHalfLayout->setContentsMargins(0, 0, 0, 0);
    leftHalfLayout->setSpacing(4);
    leftHalfLayout->addWidget(osdScroll, 1);       // OSD 设备信息
    leftHalfLayout->addWidget(parseScroll, 3);     // JSON 解析

    mRightSplitter = new QSplitter(Qt::Horizontal, this);
    mRightSplitter->addWidget(leftHalf);
    mRightSplitter->addWidget(mRawJsonPanel);
    mRightSplitter->setStretchFactor(0, 2);
    mRightSplitter->setStretchFactor(1, 3);
    mRightSplitter->setSizes({400, 600});

    rightLayout->addWidget(mRightSplitter, 1);

    // Topic 下发（折叠）
    mPublishPanel = new PublishPanel(this);
    mPublishPanel->setVisible(false);
    mPublishPanel->setMaximumHeight(200);

    mTogglePublishBtn = new QPushButton("▶ Topic 下发", this);
    mTogglePublishBtn->setObjectName("publishToggle");
    mTogglePublishBtn->setCheckable(true);
    mTogglePublishBtn->setCursor(Qt::PointingHandCursor);
    connect(mTogglePublishBtn, &QPushButton::toggled, this, [this](bool checked) {
        mPublishPanel->setVisible(checked);
        mTogglePublishBtn->setText(checked ? "◢ Topic 下发" : "▶ Topic 下发");
    });

    rightLayout->addWidget(mTogglePublishBtn);
    rightLayout->addWidget(mPublishPanel);

    // === 主分割器 ===
    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);

    setCentralWidget(mainSplitter);
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
    mVersionLabel = new QLabel("v1.0 · github.com/damon-liu/Dji-cloud-api-tool");
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

    // Topic 选中变化 → 原始 JSON 按 topic 过滤
    connect(mTopicListWidget, &TopicListWidget::topicSelectionChanged,
            this, [this](const QString& selectedTopic) {
        QString sn = mDeviceTree->selectedDeviceSn();
        if (!sn.isEmpty())
            mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn, selectedTopic));
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
        updateStatusBar();
    });
    connect(mDevMgr, &DeviceManager::brokerDisconnected, this, [this]() {
        mStatusLabel->setText("🔴 未连接");
        mBrokerLabel->setText(" 未连接");
        mBrokerLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 0 12px;");
        mConnectAct->setEnabled(true);
        mDisconnectAct->setEnabled(false);
    });
    connect(mDevMgr, &DeviceManager::brokerError, this, [this](const QString& err) {
        statusBar()->showMessage("MQTT 错误: " + err, 5000);
    });

    connect(mDevMgr, &DeviceManager::deviceAdded, this, [this]() {
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        // Refresh topic list for currently selected device
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshTopicList(currentSn);
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
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
    });

    // OsdParsePanel: topic 选中变化 → 更新解析面板
    connect(mTopicListWidget, &TopicListWidget::topicSelectionChanged,
            mOsdParsePanel, [this](const QString& selectedTopic) {
        QString sn = mDeviceTree->selectedDeviceSn();
        mOsdParsePanel->setTopic(sn, selectedTopic);
    });

    // 加载 topic 映射配置
    {
        mTopicMapping = new TopicMapping();
        QString mappingPath = QApplication::applicationDirPath() + "/topic_mappings.json";
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
        mOsdParsePanel->setTopicMapping(mTopicMapping);
    }
    mOsdParsePanel->setDeviceManager(mDevMgr);

    mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
    mDisconnectAct->setEnabled(false);
    updateStatusBar();
}

// ——— 设备选择 ———
void MainWindow::onDeviceSelected(const QString& sn) {
    if (sn.isEmpty()) {
        // 取消选中：清空所有面板
        mOsdPanel->clear();
        mRawJsonPanel->clear();
        mPublishPanel->setTopics({});
        mTopicListWidget->clearTopics();
        mOsdParsePanel->clear();
        mDeleteDeviceBtn->setEnabled(false);
        mAddDeviceBtn->setEnabled(true);
        return;
    }

    DeviceInfo* dev = mDevMgr->device(sn);
    if (!dev) return;

    const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(sn);
    const DockOsd* dockOsd   = mDevMgr->latestDockOsd(sn);
    mOsdPanel->showOsd(dev, airOsd, dockOsd, mDevMgr->latestRawJson(sn));

    mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn));
    mPublishPanel->setTopics(mDevMgr->topicsForDevice(sn));

    // 刷新 topic 列表
    refreshTopicList(sn);

    // 启用操作按钮
    mDeleteDeviceBtn->setEnabled(true);

    // 根据设备类型控制添加按钮
    if (dev->type == DeviceType::Aircraft) {
        mAddDeviceBtn->setEnabled(false);
    } else {
        mAddDeviceBtn->setEnabled(true);
    }

    // 更新 OsdParsePanel
    QString selectedTopic = mTopicListWidget->selectedTopic();
    mOsdParsePanel->setTopic(sn, selectedTopic);
}

void MainWindow::onOsdUpdated(const QString& sn, const QString& topic, const QString& rawJson) {
    if (mDeviceTree->selectedDeviceSn() != sn)
        return;

    // 刷新 OSD 面板数据（轻量更新，不重建 JSON 历史）
    DeviceInfo* dev = mDevMgr->device(sn);
    if (dev) {
        const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(sn);
        const DockOsd* dockOsd   = mDevMgr->latestDockOsd(sn);
        mOsdPanel->showOsd(dev, airOsd, dockOsd, mDevMgr->latestRawJson(sn));
    }

    // 按用户选中的 topic 过滤追加
    QString selectedTopic = mTopicListWidget->selectedTopic();
    if (!selectedTopic.isEmpty() && topic != selectedTopic)
        return;  // 不是用户选中的 topic，跳过

    if (!rawJson.isEmpty())
        mRawJsonPanel->appendJson(rawJson);
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

    // Determine if we're adding a child device to a Dock
    bool addingChild = (selectedDev && selectedDev->type == DeviceType::Dock);

    QString sn;
    QString name;
    DeviceType type;

    if (addingChild) {
        // Adding a child Aircraft to the selected Dock
        sn = QInputDialog::getText(this, "添加手飞无人机",
            QString("为机场「%1」添加手飞无人机\n设备序列号 (SN):").arg(selectedDev->name));
        if (sn.trimmed().isEmpty()) return;

        name = QInputDialog::getText(this, "添加手飞无人机", "设备名称:",
            QLineEdit::Normal, sn.trimmed());
        if (name.trimmed().isEmpty())
            name = sn.trimmed();
        type = DeviceType::Aircraft;
    } else {
        // Adding a top-level device
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
    QStringList topics = mDevMgr->topicsForDevice(sn);
    // Collect disabled topics from DeviceManager
    QSet<QString> disabled;
    for (const auto& t : topics) {
        if (!mDevMgr->isTopicEnabled(sn, t))
            disabled.insert(t);
    }
    mTopicListWidget->setTopics(sn, topics, disabled);
}

void MainWindow::updateStatusBar() {
    mDeviceCountLabel->setText("设备: " +
        QString::number(mDevMgr->allDevices().size()));
}
