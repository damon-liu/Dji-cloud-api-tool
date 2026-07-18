#include "DockControlPanel.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

DockControlPanel::DockControlPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateButtonStates();
}

void DockControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    mDeviceLabel = new QLabel(QString::fromUtf8("请选择机场设备"), this);
    mDeviceLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #333;"));
    mainLayout->addWidget(mDeviceLabel);

    auto* debugGroup = new QGroupBox(QString::fromUtf8("远程调试模式"), this);
    auto* debugLayout = new QHBoxLayout(debugGroup);
    mDebugModeLabel = new QLabel(QString::fromUtf8("状态：未知"), debugGroup);
    debugLayout->addWidget(mDebugModeLabel);
    debugLayout->addStretch();

    mDebugOpenBtn = new QPushButton(QString::fromUtf8("进入远程调试"), debugGroup);
    mDebugCloseBtn = new QPushButton(QString::fromUtf8("退出远程调试"), debugGroup);
    debugLayout->addWidget(mDebugOpenBtn);
    debugLayout->addWidget(mDebugCloseBtn);
    mainLayout->addWidget(debugGroup);

    auto* controlGroup = new QGroupBox(QString::fromUtf8("常用控制"), this);
    auto* grid = new QGridLayout(controlGroup);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    grid->addWidget(new QLabel(QString::fromUtf8("飞机电源"), controlGroup), 0, 0);
    mDroneOpenBtn = new QPushButton(QString::fromUtf8("开机"), controlGroup);
    mDroneCloseBtn = new QPushButton(QString::fromUtf8("关机"), controlGroup);
    grid->addWidget(mDroneOpenBtn, 0, 1);
    grid->addWidget(mDroneCloseBtn, 0, 2);

    grid->addWidget(new QLabel(QString::fromUtf8("机场舱盖"), controlGroup), 1, 0);
    mCoverOpenBtn = new QPushButton(QString::fromUtf8("打开"), controlGroup);
    mCoverCloseBtn = new QPushButton(QString::fromUtf8("关闭"), controlGroup);
    grid->addWidget(mCoverOpenBtn, 1, 1);
    grid->addWidget(mCoverCloseBtn, 1, 2);

    grid->addWidget(new QLabel(QString::fromUtf8("飞机充电"), controlGroup), 2, 0);
    mChargeOpenBtn = new QPushButton(QString::fromUtf8("开启"), controlGroup);
    mChargeCloseBtn = new QPushButton(QString::fromUtf8("关闭"), controlGroup);
    grid->addWidget(mChargeOpenBtn, 2, 1);
    grid->addWidget(mChargeCloseBtn, 2, 2);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    mainLayout->addWidget(controlGroup);

    mStatusLabel = new QLabel(QString::fromUtf8("连接机场后可使用快捷控制"), this);
    mStatusLabel->setWordWrap(true);
    setStatus(mStatusLabel->text());
    mainLayout->addWidget(mStatusLabel);
    mainLayout->addStretch();

    const QList<QPushButton*> buttons = {
        mDebugOpenBtn, mDebugCloseBtn, mDroneOpenBtn, mDroneCloseBtn,
        mCoverOpenBtn, mCoverCloseBtn, mChargeOpenBtn, mChargeCloseBtn
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
    connect(mChargeOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeOpen); });
    connect(mChargeCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeClose); });
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

    mDeviceLabel->setText(QString::fromUtf8("当前机场：%1（%2）%3")
        .arg(mDisplayName, mGatewaySn, mOnline ? QString::fromUtf8(" · 在线")
                                             : QString::fromUtf8(" · 离线")));
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
    mDeviceLabel->setText(QString::fromUtf8("请选择机场或其关联飞机"));
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

void DockControlPanel::requestCommand(DockCommandType type) {
    if (mGatewaySn.isEmpty() || mPending)
        return;

    const QString action = DockCommandBuilder::displayName(type);
    const QString text = QString::fromUtf8("确定要对机场 %1（%2）执行“%3”吗？")
        .arg(mDisplayName, mGatewaySn, action);
    if (QMessageBox::question(this, QString::fromUtf8("确认机场控制"), text,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;

    mPending = true;
    setStatus(QString::fromUtf8("正在执行：%1").arg(action));
    updateButtonStates();
    emit commandRequested(mGatewaySn, type);
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
        setStatus(QString::fromUtf8("%1成功：%2").arg(action, result.message));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
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
    mChargeOpenBtn->setEnabled(debugControlsEnabled);
    mChargeCloseBtn->setEnabled(debugControlsEnabled);

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
        ? QStringLiteral("color: #d93025; padding: 4px;")
        : QStringLiteral("color: #5f6368; padding: 4px;"));
}
