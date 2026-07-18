#include "DockControlPanel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
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

    // ——— 顶行：设备信息（左） + 执行状态（右） ———
    auto* topRow = new QHBoxLayout;
    mDeviceLabel = new QLabel(QString::fromUtf8("请选择机场设备"), this);
    mDeviceLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #333;"));
    topRow->addWidget(mDeviceLabel);
    topRow->addStretch();
    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(false);
    setStatus(QString::fromUtf8("连接机场后可使用快捷控制"));
    topRow->addWidget(mStatusLabel);
    mainLayout->addLayout(topRow);

    // ——— 卡片行：远程调试 | 飞机电源 | 机场舱盖 | 飞机充电 ———
    auto* cardRow = new QHBoxLayout;
    cardRow->setSpacing(10);

    // 远程调试卡片（含状态标签）
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
    cardRow->addWidget(debugGroup, 3);

    // 功能卡片：标题 + 底部按钮行
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

    cardRow->addWidget(makeCard(QString::fromUtf8("飞机电源"), QString::fromUtf8("开机"),
                                QString::fromUtf8("关机"), mDroneOpenBtn, mDroneCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("机场舱盖"), QString::fromUtf8("打开"),
                                QString::fromUtf8("关闭"), mCoverOpenBtn, mCoverCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("飞机充电"), QString::fromUtf8("开启"),
                                QString::fromUtf8("关闭"), mChargeOpenBtn, mChargeCloseBtn), 2);

    mainLayout->addLayout(cardRow);

    // ——— 下发记录：日志流文本块，最新在最上 ———
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);

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
    appendHistory(result);
    updateButtonStates();

    // 终态弹窗提醒（放在最后，模态弹窗不阻塞状态刷新）
    if (result.state == DockCommandState::Succeeded)
        QMessageBox::information(this, QString::fromUtf8("控制成功"),
            QString::fromUtf8("“%1”执行成功（%2）").arg(action, result.message));
    else
        QMessageBox::warning(this, QString::fromUtf8("控制失败"),
            QString::fromUtf8("“%1”执行失败：%2").arg(action, result.message));
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
