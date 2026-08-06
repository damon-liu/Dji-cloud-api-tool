#include "DockControlPanel.h"
#include "StyleConstants.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>

DockControlPanel::DockControlPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateButtonStates();
}

void DockControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // --- top row: dock selector + online status + execution status ---
    auto* topRow = new QHBoxLayout;

    auto* dockLabel = new QLabel(QString::fromUtf8("控制机场:"), this);
    dockLabel->setStyleSheet("font-weight: bold; color: #333;");
    topRow->addWidget(dockLabel);

    mDockCombo = new QComboBox(this);
    mDockCombo->setEditable(true);
    mDockCombo->setInsertPolicy(QComboBox::NoInsert);
    mDockCombo->setMinimumWidth(280);
    mDockCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    mDockCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    mDockCombo->lineEdit()->setAlignment(Qt::AlignLeft);
    mDockCombo->lineEdit()->setReadOnly(true);
    mDockCombo->view()->setMinimumWidth(320);
    mDockCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 8px;"
        "font-size: 13px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }"
        "QComboBox QAbstractItemView { border: 1px solid #dadce0;"
        "selection-background-color: #e8f0fe; selection-color: #1a73e8; }");
    connect(mDockCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (mUpdatingCombo || idx < 0 || mAvailableDocks.isEmpty()) return;
        const auto& dock = mAvailableDocks[idx];
        setDevice(dock.name, dock.sn, true);
    });
    topRow->addWidget(mDockCombo);

    mOnlineLabel = new QLabel(this);
    topRow->addWidget(mOnlineLabel);

    topRow->addStretch();

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(false);
    setStatus(QString::fromUtf8("连接机场后可使用快捷控制"));
    topRow->addWidget(mStatusLabel);

    auto* ctrlRecordBtn = new QPushButton(QString::fromUtf8("📋 控制记录"), this);
    ctrlRecordBtn->setCursor(Qt::PointingHandCursor);
    ctrlRecordBtn->setFocusPolicy(Qt::NoFocus);
    ctrlRecordBtn->setStyleSheet(
        "QPushButton { border: none; background: transparent; color: #1a73e8;"
        "font-size: 13px; padding: 4px 8px; }"
        "QPushButton:hover { color: #1557b0; text-decoration: underline; }");
    topRow->addWidget(ctrlRecordBtn);
    connect(ctrlRecordBtn, &QPushButton::clicked, this, [this]() {
        emit historyRequested();
    });

    mainLayout->addLayout(topRow);

    // ===================================================
    // 远程调试（标题在 QGroupBox 边框上，按钮在框内右上角紧贴顶部）
    // ===================================================
    auto* debugGroup = new QGroupBox(QString::fromUtf8("远程调试"), this);
    auto* debugLayout = new QVBoxLayout(debugGroup);
    debugLayout->setContentsMargins(8, 2, 8, 8);
    debugLayout->setSpacing(8);

    // -- 调试模式切换按钮（右对齐，紧贴 QGroupBox 标题栏下沿） --
    {
        auto* toggleRow = new QHBoxLayout;
        toggleRow->addStretch();
        mDebugToggleBtn = new QPushButton(debugGroup);
        mDebugToggleBtn->setCursor(Qt::PointingHandCursor);
        mDebugToggleBtn->setFocusPolicy(Qt::NoFocus);
        mDebugToggleBtn->setStyleSheet(
            "QPushButton {"
            "  border: 1px solid #c4d7f2; border-radius: 4px;"
            "  background: #e8f0fe; color: #202124; font-weight: bold;"
            "  padding: 5px 16px; font-size: 13px;"
            "}"
            "QPushButton:hover {"
            "  border-color: #1a73e8; background: #d2e3fc;"
            "}");
        toggleRow->addWidget(mDebugToggleBtn);
        debugLayout->addLayout(toggleRow);
    }

    // -- 子卡片辅助 lambda --
    auto makeCard = [debugGroup](const QString& title, const QString& openText,
                                 const QString& closeText,
                                 QPushButton*& openBtn, QPushButton*& closeBtn) {
        auto* group = new QGroupBox(title, debugGroup);
        group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        auto* v = new QVBoxLayout(group);
        auto* row = new QHBoxLayout;
        openBtn  = new QPushButton(openText, group);
        closeBtn = new QPushButton(closeText, group);
        row->addStretch(1);
        row->addWidget(openBtn);
        row->addStretch(1);
        row->addWidget(closeBtn);
        row->addStretch(1);
        v->addLayout(row);
        return group;
    };

    auto* chargeGroup = makeCard(QString::fromUtf8("飞机充电"),
                                 QString::fromUtf8("开启"),
                                 QString::fromUtf8("关闭"),
                                 mChargeOpenBtn, mChargeCloseBtn);

    auto* droneGroup  = makeCard(QString::fromUtf8("飞机电源"),
                                 QString::fromUtf8("开机"),
                                 QString::fromUtf8("关机"),
                                 mDroneOpenBtn, mDroneCloseBtn);

    // 机场舱盖卡片
    auto* coverGroup = new QGroupBox(QString::fromUtf8("机场舱盖"), debugGroup);
    coverGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* coverLayout = new QVBoxLayout(coverGroup);
    auto* coverBtnRow = new QHBoxLayout;
    mCoverOpenBtn  = new QPushButton(QString::fromUtf8("打开"), coverGroup);
    mCoverCloseBtn = new QPushButton(QString::fromUtf8("关闭"), coverGroup);
    coverBtnRow->addStretch(1);
    coverBtnRow->addWidget(mCoverOpenBtn);
    coverBtnRow->addStretch(1);
    coverBtnRow->addWidget(mCoverCloseBtn);
    coverBtnRow->addStretch(1);
    coverLayout->addLayout(coverBtnRow);

    // 机场维护卡片
    auto* maintainGroup = new QGroupBox(QString::fromUtf8("机场维护"), debugGroup);
    maintainGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* maintainLayout = new QVBoxLayout(maintainGroup);
    auto* maintainBtnRow = new QHBoxLayout;
    mRebootBtn = new QPushButton(QString::fromUtf8("机场重启"), maintainGroup);
    mCoverForceBtn = new QPushButton(QString::fromUtf8("强制关舱门"), maintainGroup);
    maintainBtnRow->addStretch(1);
    maintainBtnRow->addWidget(mRebootBtn);
    maintainBtnRow->addStretch(1);
    maintainBtnRow->addWidget(mCoverForceBtn);
    maintainBtnRow->addStretch(1);
    maintainLayout->addLayout(maintainBtnRow);

    // -- 第一行：飞机充电 | 飞机电源 --
    auto* deviceRow1 = new QHBoxLayout;
    deviceRow1->setSpacing(10);
    deviceRow1->addWidget(chargeGroup, 1);
    deviceRow1->addWidget(droneGroup, 1);
    debugLayout->addLayout(deviceRow1);

    // -- 第二行：机场维护 | 机场舱盖 --
    auto* deviceRow2 = new QHBoxLayout;
    deviceRow2->setSpacing(10);
    deviceRow2->addWidget(maintainGroup, 1);
    deviceRow2->addWidget(coverGroup, 1);
    debugLayout->addLayout(deviceRow2);
    debugLayout->addStretch();

    // --- ScrollArea 包裹（视频展开时保持按钮可滚动访问） ---
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(debugGroup);
    mainLayout->addWidget(scrollArea, 1);

    // --- 按钮统一样式 ---
    styleWarningButton(mRebootBtn);
    styleDangerOutlineButton(mCoverForceBtn);

    const QList<QPushButton*> defaultButtons = {
        mDroneOpenBtn, mDroneCloseBtn,
        mCoverOpenBtn, mCoverCloseBtn,
        mChargeOpenBtn, mChargeCloseBtn
    };
    for (auto* btn : defaultButtons)
        styleDefaultButton(btn);

    connect(mDebugToggleBtn, &QPushButton::clicked, this, [this]() {
        if (mDebugModeState == DebugModeState::Enabled)
            requestCommand(DockCommandType::DebugModeClose);
        else
            requestCommand(DockCommandType::DebugModeOpen);
    });
    connect(mDroneOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DroneOpen); });
    connect(mDroneCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DroneClose); });
    connect(mCoverOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::CoverOpen); });
    connect(mCoverCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::CoverClose); });
    connect(mCoverForceBtn, &QPushButton::clicked, this,
            [this]() {
        auto ret = QMessageBox::warning(this,
            QString::fromUtf8("确认强制关舱门"),
            QString::fromUtf8("强制关舱门将无视传感器状态立即执行，可能导致设备损坏。\n确定要强制关舱门吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            requestCommand(DockCommandType::CoverForceClose);
    });
    connect(mChargeOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeOpen); });
    connect(mChargeCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeClose); });
    connect(mRebootBtn, &QPushButton::clicked, this,
            [this]() {
        auto ret = QMessageBox::warning(this,
            QString::fromUtf8("确认机场重启"),
            QString::fromUtf8("机场重启将中断当前所有任务，确定要重启吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            requestCommand(DockCommandType::DeviceReboot);
    });
}

void DockControlPanel::setDevice(const QString& displayName, const QString& gatewaySn, bool online) {
    if (mGatewaySn != gatewaySn || mOnline != online) {
        mDebugModeState = DebugModeState::Unknown;
        mPending = false;
    }
    mDisplayName = displayName;
    mGatewaySn = gatewaySn;
    mOnline = online;

    if (mGatewaySn.isEmpty()) {
        clearDevice();
        return;
    }

    if (mOnline) {
        mOnlineLabel->setText(QString::fromUtf8("🟢 在线"));
        mOnlineLabel->setStyleSheet("color: #1e8e3e; font-weight: bold; padding: 0 8px;");
    } else {
        mOnlineLabel->setText(QString::fromUtf8("🔴 离线"));
        mOnlineLabel->setStyleSheet("color: #d93025; font-weight: bold; padding: 0 8px;");
    }

    if (!mOnline)
        setStatus(QString::fromUtf8("机场离线，无法发送控制指令"), true);
    else if (mDebugModeState == DebugModeState::Enabled)
        setStatus(QString::fromUtf8("远程调试已开启，可执行常用控制"));
    else
        setStatus(QString::fromUtf8("请先进入远程调试模式"));
    updateButtonStates();
}

void DockControlPanel::clearDevice() {
    mDisplayName.clear();
    mGatewaySn.clear();
    mOnline = false;
    mPending = false;
    mDebugModeState = DebugModeState::Unknown;
    mOnlineLabel->setText(QString());
    mOnlineLabel->setStyleSheet(QString());
    setStatus(QString::fromUtf8("未选择可控制的机场设备"));
    updateButtonStates();
}

void DockControlPanel::setConnected(bool connected) {
    mConnected = connected;
    if (!connected) {
        mPending = false;
        mDebugModeState = DebugModeState::Unknown;
        setStatus(QString::fromUtf8("MQTT 未连接"), true);
    }
    updateButtonStates();
}

void DockControlPanel::setAvailableDocks(const QVector<DeviceInfo>& docks,
                                         const QString& currentSn,
                                         double dockLat, double dockLon,
                                         double dockAlt) {
    Q_UNUSED(dockAlt)
    mAvailableDocks = docks;
    mDockLat = dockLat;
    mDockLon = dockLon;

    mUpdatingCombo = true;
    mDockCombo->clear();

    int selectIdx = -1;
    for (int i = 0; i < docks.size(); ++i) {
        const auto& d = docks[i];
        mDockCombo->addItem(
            QString::fromUtf8("%1 - %2").arg(d.name, d.sn), d.sn);
        if (d.sn == currentSn)
            selectIdx = i;
    }

    if (selectIdx >= 0)
        mDockCombo->setCurrentIndex(selectIdx);
    else if (!docks.isEmpty())
        mDockCombo->setCurrentIndex(0);

    mUpdatingCombo = false;

    if (docks.isEmpty()) {
        clearDevice();
    } else {
        int idx = mDockCombo->currentIndex();
        if (idx >= 0 && idx < docks.size()) {
            const auto& d = docks[idx];
            setDevice(d.name, d.sn, true);
        }
    }
}

void DockControlPanel::requestCommand(DockCommandType type, const QJsonObject& data) {
    if (mGatewaySn.isEmpty() || mPending)
        return;

    mPending = true;
    setStatus(QString::fromUtf8("正在执行：%1").arg(DockCommandBuilder::displayName(type)));
    updateButtonStates();
    emit commandRequested(mGatewaySn, type, data);
}

void DockControlPanel::onCommandStateChanged(const DockCommandResult& result) {
    if (!mGatewaySn.isEmpty() && result.gatewaySn != mGatewaySn)
        return;

    const QString action = DockCommandBuilder::displayName(result.type);
    if (result.state == DockCommandState::Publishing
            || result.state == DockCommandState::WaitingReply) {
        mPending = true;
        setStatus(QString::fromUtf8("%1：%2").arg(action, result.message));
        updateButtonStates();
        return;
    }

    mPending = false;
    if (result.state == DockCommandState::Succeeded) {
        if (result.type == DockCommandType::DebugModeOpen)
            mDebugModeState = DebugModeState::Enabled;
        else if (result.type == DockCommandType::DebugModeClose)
            mDebugModeState = DebugModeState::Disabled;
        setStatus(QString::fromUtf8("%1成功").arg(action));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    updateButtonStates();
}

void DockControlPanel::updateButtonStates() {
    const bool available = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mDebugToggleBtn->setEnabled(available);
    if (mDebugModeState == DebugModeState::Enabled)
        mDebugToggleBtn->setText(QString::fromUtf8("✅ 退出调试模式"));
    else
        mDebugToggleBtn->setText(QString::fromUtf8("进入调试模式"));

    const bool debugControlsEnabled = available && mDebugModeState == DebugModeState::Enabled;
    mDroneOpenBtn->setEnabled(debugControlsEnabled);
    mDroneCloseBtn->setEnabled(debugControlsEnabled);
    mCoverOpenBtn->setEnabled(debugControlsEnabled);
    mCoverCloseBtn->setEnabled(debugControlsEnabled);
    mCoverForceBtn->setEnabled(debugControlsEnabled);
    mChargeOpenBtn->setEnabled(debugControlsEnabled);
    mChargeCloseBtn->setEnabled(debugControlsEnabled);
    mRebootBtn->setEnabled(debugControlsEnabled);
}

void DockControlPanel::setStatus(const QString& text, bool error) {
    mStatusLabel->setText(text);
    mStatusLabel->setStyleSheet(error
        ? QStringLiteral("color: #d93025; font-weight: bold; padding: 4px;")
        : QStringLiteral("color: #e8710a; font-weight: bold; padding: 4px;"));
}
