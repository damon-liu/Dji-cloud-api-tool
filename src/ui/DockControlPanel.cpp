#include "DockControlPanel.h"

#include <QAbstractItemView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QTextCursor>
#include <QTime>
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

    mainLayout->addLayout(topRow);

    // --- card grid: 2×3 卡片网格 ---
    // 第一行：远程调试 | 飞机电源 | 机场舱盖
    // 第二行：飞机充电 | 机场维护 | 补光灯
    auto* cardGrid = new QGridLayout;
    cardGrid->setSpacing(10);

    auto* debugGroup = new QGroupBox(QString::fromUtf8("远程调试"), this);
    auto* debugLayout = new QVBoxLayout(debugGroup);
    mDebugModeLabel = new QLabel(QString::fromUtf8("状态：未知"), debugGroup);
    debugLayout->addWidget(mDebugModeLabel);
    debugLayout->addStretch();
    auto* debugBtnRow = new QHBoxLayout;
    mDebugOpenBtn  = new QPushButton(QString::fromUtf8("进入"), debugGroup);
    mDebugCloseBtn = new QPushButton(QString::fromUtf8("退出"), debugGroup);
    debugBtnRow->addWidget(mDebugOpenBtn);
    debugBtnRow->addWidget(mDebugCloseBtn);
    debugLayout->addLayout(debugBtnRow);

    auto makeCard = [this](const QString& title, const QString& openText,
                           const QString& closeText,
                           QPushButton*& openBtn, QPushButton*& closeBtn) {
        auto* group = new QGroupBox(title, this);
        auto* v = new QVBoxLayout(group);
        v->addStretch();
        auto* row = new QHBoxLayout;
        openBtn  = new QPushButton(openText, group);
        closeBtn = new QPushButton(closeText, group);
        row->addWidget(openBtn);
        row->addWidget(closeBtn);
        v->addLayout(row);
        return group;
    };

    auto* droneGroup  = makeCard(QString::fromUtf8("飞机电源"),
                                 QString::fromUtf8("开机"),
                                 QString::fromUtf8("关机"),
                                 mDroneOpenBtn, mDroneCloseBtn);

    // 机场舱盖卡片：打开/关闭
    auto* coverGroup = new QGroupBox(QString::fromUtf8("机场舱盖"), this);
    auto* coverLayout = new QVBoxLayout(coverGroup);
    coverLayout->addStretch();
    auto* coverBtnRow = new QHBoxLayout;
    mCoverOpenBtn  = new QPushButton(QString::fromUtf8("打开"), coverGroup);
    mCoverCloseBtn = new QPushButton(QString::fromUtf8("关闭"), coverGroup);
    coverBtnRow->addWidget(mCoverOpenBtn);
    coverBtnRow->addWidget(mCoverCloseBtn);
    coverLayout->addLayout(coverBtnRow);

    auto* chargeGroup = makeCard(QString::fromUtf8("飞机充电"),
                                 QString::fromUtf8("开启"),
                                 QString::fromUtf8("关闭"),
                                 mChargeOpenBtn, mChargeCloseBtn);

    // 机场维护卡片
    auto* maintainGroup = new QGroupBox(QString::fromUtf8("机场维护"), this);
    auto* maintainLayout = new QVBoxLayout(maintainGroup);
    maintainLayout->addStretch();
    auto* maintainBtnRow = new QHBoxLayout;
    mRebootBtn = new QPushButton(QString::fromUtf8("机场重启"), maintainGroup);
    mRebootBtn->setStyleSheet(
        "QPushButton { background: #f29900; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background: #e37400; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    maintainBtnRow->addWidget(mRebootBtn);
    mCoverForceBtn = new QPushButton(QString::fromUtf8("强制关舱门"), maintainGroup);
    mCoverForceBtn->setStyleSheet(
        "QPushButton { border: 1px solid #d93025; border-radius: 4px; color: #d93025;"
        "font-weight: bold; background: #fff; padding: 6px 12px; }"
        "QPushButton:hover { background: #fce8e6; }"
        "QPushButton:disabled { border-color: #dadce0; color: #80868b; background: #f8f9fa; }");
    maintainBtnRow->addWidget(mCoverForceBtn);
    maintainLayout->addLayout(maintainBtnRow);

    // 补光灯卡片
    auto* fillLightGroup = new QGroupBox(QString::fromUtf8("补光灯"), this);
    auto* fillLightLayout = new QVBoxLayout(fillLightGroup);
    fillLightLayout->addStretch();
    auto* fillLightBtnRow = new QHBoxLayout;
    mFillLightOpenBtn  = new QPushButton(QString::fromUtf8("开启"), fillLightGroup);
    mFillLightCloseBtn = new QPushButton(QString::fromUtf8("关闭"), fillLightGroup);
    fillLightBtnRow->addWidget(mFillLightOpenBtn);
    fillLightBtnRow->addWidget(mFillLightCloseBtn);
    fillLightLayout->addLayout(fillLightBtnRow);

    cardGrid->addWidget(debugGroup,    0, 0);
    cardGrid->addWidget(droneGroup,    0, 1);
    cardGrid->addWidget(coverGroup,    0, 2);
    cardGrid->addWidget(chargeGroup,   1, 0);
    cardGrid->addWidget(maintainGroup, 1, 1);
    cardGrid->addWidget(fillLightGroup, 1, 2);
    for (int col = 0; col < 3; ++col)
        cardGrid->setColumnStretch(col, 1);

    mainLayout->addLayout(cardGrid);

    // --- history ---
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);

    const QList<QPushButton*> buttons = {
        mDebugOpenBtn, mDebugCloseBtn, mDroneOpenBtn, mDroneCloseBtn,
        mCoverOpenBtn, mCoverCloseBtn, mCoverForceBtn,
        mChargeOpenBtn, mChargeCloseBtn, mRebootBtn,
        mFillLightOpenBtn, mFillLightCloseBtn
    };
    for (auto* button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(30);
    }

    connect(mDebugOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DebugModeOpen); });
    connect(mDebugCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DebugModeClose); });
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
            [this]() { requestCommand(DockCommandType::DeviceReboot); });

    connect(mFillLightOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::FillLightOpen); });
    connect(mFillLightCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::FillLightClose); });
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
                                         double dockLat, double dockLon) {
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
    appendHistory(result);
    updateButtonStates();
}

void DockControlPanel::updateButtonStates() {
    const bool available = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mDebugOpenBtn->setEnabled(available && mDebugModeState != DebugModeState::Enabled);
    mDebugCloseBtn->setEnabled(available && mDebugModeState != DebugModeState::Disabled);

    const bool debugControlsEnabled = available && mDebugModeState == DebugModeState::Enabled;
    mDroneOpenBtn->setEnabled(debugControlsEnabled);
    mDroneCloseBtn->setEnabled(debugControlsEnabled);
    mCoverOpenBtn->setEnabled(debugControlsEnabled);
    mCoverCloseBtn->setEnabled(debugControlsEnabled);
    mCoverForceBtn->setEnabled(debugControlsEnabled);
    mChargeOpenBtn->setEnabled(debugControlsEnabled);
    mChargeCloseBtn->setEnabled(debugControlsEnabled);
    mRebootBtn->setEnabled(debugControlsEnabled);
    mFillLightOpenBtn->setEnabled(debugControlsEnabled);
    mFillLightCloseBtn->setEnabled(debugControlsEnabled);

    switch (mDebugModeState) {
    case DebugModeState::Enabled:
        mDebugModeLabel->setText(QString::fromUtf8("状态：已开启"));
        mDebugModeLabel->setStyleSheet(QStringLiteral("color: #1e8e3e; font-weight: bold;"));
        break;
    case DebugModeState::Disabled:
        mDebugModeLabel->setText(QString::fromUtf8("状态：已退出"));
        mDebugModeLabel->setStyleSheet(QStringLiteral("color: #5f6368;"));
        break;
    case DebugModeState::Unknown:
        mDebugModeLabel->setText(QString::fromUtf8("状态：未知（以本客户端操作结果为准）"));
        mDebugModeLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
        break;
    }
}

void DockControlPanel::setStatus(const QString& text, bool error) {
    mStatusLabel->setText(text);
    mStatusLabel->setStyleSheet(error
        ? QStringLiteral("color: #d93025; font-weight: bold; padding: 4px;")
        : QStringLiteral("color: #e8710a; font-weight: bold; padding: 4px;"));
}

void DockControlPanel::appendHistory(const DockCommandResult& result) {
    const QString action = DockCommandBuilder::displayName(result.type);
    QString verdict;
    switch (result.state) {
    case DockCommandState::Succeeded:
        verdict = QString::fromUtf8("✅ 成功 (result=0)");
        break;
    case DockCommandState::TimedOut:
        verdict = QString::fromUtf8("❌ 超时");
        break;
    default:
        verdict = QString::fromUtf8("❌ 失败（%1）").arg(result.message);
        break;
    }

    QString block;
    block += QStringLiteral("[%1] %2  %3\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action, verdict);
    block += QString::fromUtf8("Topic: thing/product/%1/services\n").arg(result.gatewaySn);
    block += QString::fromUtf8("下发:\n%1\n").arg(result.requestJson.trimmed());
    block += QString::fromUtf8("响应:\n%1\n").arg(result.replyJson.isEmpty()
        ? QString::fromUtf8("（无响应）") : result.replyJson.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    // 最新记录插入顶部
    mHistoryEdit->moveCursor(QTextCursor::Start);
    mHistoryEdit->insertPlainText(block);
    mHistoryEdit->moveCursor(QTextCursor::Start);
}
