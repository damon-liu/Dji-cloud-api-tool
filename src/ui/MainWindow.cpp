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

    // 左侧：配置 + 连接操作
    auto* configAct = toolbar->addAction("⚙ 配置");
    auto* configBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(configAct));
    if (configBtn) configBtn->setObjectName("configBtn");
    connect(configAct, &QAction::triggered, this, [this]() {
        ConfigDialog dlg(mDevMgr->mqttConfig(), this);
        if (dlg.exec() == QDialog::Accepted) {
            mDevMgr->setMqttConfig(dlg.getConfig());
            // 配置成功后自动连接
            if (!mDevMgr->isConnected()) {
                mDevMgr->connectBroker();
            }
        }
    });

    mConnectAct = toolbar->addAction("● 连接");
    auto* connectBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(mConnectAct));
    if (connectBtn) connectBtn->setObjectName("connectBtn");
    connect(mConnectAct, &QAction::triggered, this, &MainWindow::onConnectAction);

    mDisconnectAct = toolbar->addAction("◎ 断开");
    auto* disconnectBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(mDisconnectAct));
    if (disconnectBtn) disconnectBtn->setObjectName("disconnectBtn");
    connect(mDisconnectAct, &QAction::triggered, this, &MainWindow::onDisconnectAction);

    toolbar->addSeparator();

    // Broker 信息标签
    mBrokerLabel = new QLabel(" 未连接", this);
    mBrokerLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 0 12px;");
    toolbar->addWidget(mBrokerLabel);

    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
}

// ——— 主布局 ———
void MainWindow::setupLayout() {
    // === 左侧：设备树面板 ===
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(8, 8, 4, 8);
    leftLayout->setSpacing(4);

    // 区域标题
    auto* titleRow = new QHBoxLayout;
    auto* treeTitle = new QLabel("设备列表");
    treeTitle->setObjectName("sectionTitle");
    titleRow->addWidget(treeTitle);
    titleRow->addStretch();
    leftLayout->addLayout(titleRow);

    // 设备和按钮横向排列：设备树 | 操作按钮
    auto* treeAndBtns = new QHBoxLayout;
    treeAndBtns->setSpacing(6);

    // 设备树
    mDeviceTree = new DeviceTreeWidget(this);
    mDeviceTree->setMinimumWidth(170);
    mDeviceTree->setMaximumWidth(220);
    treeAndBtns->addWidget(mDeviceTree, 1);

    // 按钮竖排
    auto* btnCol = new QVBoxLayout;
    btnCol->setSpacing(4);

    mAddDeviceBtn = new QPushButton("＋", this);
    mAddDeviceBtn->setCursor(Qt::PointingHandCursor);
    mAddDeviceBtn->setFixedSize(32, 32);
    mAddDeviceBtn->setToolTip("添加设备");
    mAddDeviceBtn->setStyleSheet(
        "QPushButton { background: #e8f5e9; color: #2e7d32; border: 1px solid #a5d6a7; "
        "border-radius: 4px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #c8e6c9; }");
    connect(mAddDeviceBtn, &QPushButton::clicked, this, &MainWindow::onAddDevice);
    btnCol->addWidget(mAddDeviceBtn);

    mEditTopicBtn = new QPushButton("✎", this);
    mEditTopicBtn->setCursor(Qt::PointingHandCursor);
    mEditTopicBtn->setEnabled(false);
    mEditTopicBtn->setFixedSize(32, 32);
    mEditTopicBtn->setToolTip("编辑 Topic");
    mEditTopicBtn->setStyleSheet(
        "QPushButton { background: #fff3e0; color: #e65100; border: 1px solid #ffcc80; "
        "border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #ffe0b2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    connect(mEditTopicBtn, &QPushButton::clicked, this, &MainWindow::onEditTopic);
    btnCol->addWidget(mEditTopicBtn);

    mDeleteDeviceBtn = new QPushButton("✕", this);
    mDeleteDeviceBtn->setCursor(Qt::PointingHandCursor);
    mDeleteDeviceBtn->setEnabled(false);
    mDeleteDeviceBtn->setFixedSize(32, 32);
    mDeleteDeviceBtn->setToolTip("删除设备");
    mDeleteDeviceBtn->setStyleSheet(
        "QPushButton { background: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
        "border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #ffcdd2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    connect(mDeleteDeviceBtn, &QPushButton::clicked, this, &MainWindow::onDeleteDevice);
    btnCol->addWidget(mDeleteDeviceBtn);

    btnCol->addStretch();
    treeAndBtns->addLayout(btnCol);

    leftLayout->addLayout(treeAndBtns, 1);

    // === 右侧：OSD + JSON 水平分割 ===
    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 8, 8, 8);
    rightLayout->setSpacing(4);

    // OSD | JSON
    mOsdPanel = new OsdPanel(this);
    mRawJsonPanel = new RawJsonPanel(this);

    auto* osdScroll = new QScrollArea(this);
    osdScroll->setWidget(mOsdPanel);
    osdScroll->setWidgetResizable(true);
    osdScroll->setFrameShape(QFrame::NoFrame);

    mRightSplitter = new QSplitter(Qt::Horizontal, this);
    mRightSplitter->addWidget(osdScroll);
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

    statusBar()->addWidget(mStatusLabel);
    statusBar()->addPermanentWidget(mDeviceCountLabel);
}

// ——— 信号连接 ———
void MainWindow::connectSignals() {
    connect(mDeviceTree, &DeviceTreeWidget::deviceSelected,
            this, &MainWindow::onDeviceSelected);

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
        updateStatusBar();
    });
    connect(mDevMgr, &DeviceManager::deviceRemoved, this, [this]() {
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        mOsdPanel->clear();
        mRawJsonPanel->clear();
        updateStatusBar();
    });

    connect(mDevMgr, &DeviceManager::deviceOnlineChanged,
            this, [this](const QString& sn, bool online) {
        Q_UNUSED(sn) Q_UNUSED(online)
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
    });

    mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
    mDisconnectAct->setEnabled(false);
    updateStatusBar();
}

// ——— 设备选择 ———
void MainWindow::onDeviceSelected(const QString& sn) {
    if (sn.isEmpty()) return;

    DeviceInfo* dev = mDevMgr->device(sn);
    if (!dev) return;

    const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(sn);
    const DockOsd* dockOsd   = mDevMgr->latestDockOsd(sn);
    mOsdPanel->showOsd(dev, airOsd, dockOsd, mDevMgr->latestRawJson(sn));

    mRawJsonPanel->setJson(mDevMgr->latestRawJson(sn));
    mPublishPanel->setTopics(mDevMgr->topicsForDevice(sn));

    // 启用操作按钮
    mEditTopicBtn->setEnabled(true);
    mDeleteDeviceBtn->setEnabled(true);
}

void MainWindow::onOsdUpdated(const QString& sn, const QString& rawJson) {
    Q_UNUSED(rawJson)
    if (mDeviceTree->selectedDeviceSn() == sn) {
        onDeviceSelected(sn);
    }
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
    QString typeStr = QInputDialog::getItem(this, "添加设备", "选择设备类型:",
        {"Dock (机场)", "Pilot (手飞飞机)"}, 0, false);
    if (typeStr.isEmpty()) return;

    DeviceType type = typeStr.contains("Dock") ? DeviceType::Dock : DeviceType::Aircraft;

    QString sn = QInputDialog::getText(this, "添加设备", "设备序列号 (SN):");
    if (sn.trimmed().isEmpty()) return;

    QString name = QInputDialog::getText(this, "添加设备", "设备名称:",
        QLineEdit::Normal, sn.trimmed());
    // 名称未填写则默认使用 SN
    if (name.trimmed().isEmpty())
        name = sn.trimmed();

    // 默认订阅 OSD topic
    QString osdTopic = QString("thing/product/%1/osd").arg(sn.trimmed());
    QStringList defaultTopics;
    defaultTopics << osdTopic;

    DeviceInfo info;
    info.sn   = sn.trimmed();
    info.name = name.trimmed();
    info.type = type;
    mDevMgr->addDevice(info, defaultTopics);
}

void MainWindow::onEditTopic() {
    QString sn = mDeviceTree->selectedDeviceSn();
    if (sn.isEmpty()) {
        QMessageBox::information(this, "提示", "请先在设备列表中选择一个设备。");
        return;
    }

    DeviceInfo* dev = mDevMgr->device(sn);
    if (!dev) return;

    QStringList currentTopics = mDevMgr->topicsForDevice(sn);
    TopicEditDialog dlg(currentTopics, dev->name, dev->sn, this);
    if (dlg.exec() == QDialog::Accepted) {
        QStringList newTopics = dlg.topics();
        QSet<QString> oldSet(currentTopics.begin(), currentTopics.end());
        QSet<QString> newSet(newTopics.begin(), newTopics.end());

        for (const auto& t : currentTopics) {
            if (!newSet.contains(t))
                mDevMgr->removeTopic(sn, t);
        }
        for (const auto& t : newTopics) {
            if (!oldSet.contains(t))
                mDevMgr->addTopic(sn, t);
        }
    }
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
        mEditTopicBtn->setEnabled(false);
        mDeleteDeviceBtn->setEnabled(false);
    }
}

void MainWindow::updateStatusBar() {
    mDeviceCountLabel->setText("设备: " +
        QString::number(mDevMgr->allDevices().size()));
}
