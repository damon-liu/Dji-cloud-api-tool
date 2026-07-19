#include "FlightControlPanel.h"

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

FlightControlPanel::FlightControlPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateButtonStates();
}

void FlightControlPanel::setupUi() {
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
    setStatus(QString::fromUtf8("连接机场后可使用飞行控制"));
    topRow->addWidget(mStatusLabel);

    mainLayout->addLayout(topRow);

    // --- 飞行控制卡片（后续版本完善） ---
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行控制"), this);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->addStretch();
    auto* flightBtnRow = new QHBoxLayout;
    mTakeoffBtn = new QPushButton(QString::fromUtf8("一键起飞"), flightGroup);
    mTakeoffBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; font-size: 13px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    flightBtnRow->addWidget(mTakeoffBtn);
    flightLayout->addLayout(flightBtnRow);

    auto* flightNotice = new QLabel(
        QString::fromUtf8("⚠ 飞行控制功能将在后续版本中完善，当前暂不可用"), flightGroup);
    flightNotice->setStyleSheet(
        "color: #b06000; font-size: 12px; font-weight: normal; padding: 6px 0 0 0;");
    flightNotice->setWordWrap(true);
    flightLayout->addWidget(flightNotice);

    mainLayout->addWidget(flightGroup);

    // --- 返航控制卡片（后续版本完善） ---
    auto* returnGroup = new QGroupBox(QString::fromUtf8("返航控制"), this);
    auto* returnLayout = new QVBoxLayout(returnGroup);
    returnLayout->addStretch();
    auto* returnBtnRow = new QHBoxLayout;
    mReturnHomeBtn   = new QPushButton(QString::fromUtf8("一键返航"), returnGroup);
    mCancelReturnBtn = new QPushButton(QString::fromUtf8("取消返航"), returnGroup);
    mReturnHomeBtn->setStyleSheet(
        "QPushButton { background: #ea4335; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; font-size: 13px; }"
        "QPushButton:hover { background: #c5221f; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    mCancelReturnBtn->setStyleSheet(
        "QPushButton { background: #f29900; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; font-size: 13px; }"
        "QPushButton:hover { background: #e37400; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    returnBtnRow->addWidget(mReturnHomeBtn);
    returnBtnRow->addWidget(mCancelReturnBtn);
    returnLayout->addLayout(returnBtnRow);

    auto* returnNotice = new QLabel(
        QString::fromUtf8("⚠ 返航控制功能将在后续版本中完善，当前暂不可用"), returnGroup);
    returnNotice->setStyleSheet(
        "color: #b06000; font-size: 12px; font-weight: normal; padding: 6px 0 0 0;");
    returnNotice->setWordWrap(true);
    returnLayout->addWidget(returnNotice);

    mainLayout->addWidget(returnGroup);

    // --- history ---
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);

    mTakeoffBtn->setCursor(Qt::PointingHandCursor);
    mTakeoffBtn->setMinimumHeight(30);
    mReturnHomeBtn->setCursor(Qt::PointingHandCursor);
    mReturnHomeBtn->setMinimumHeight(30);
    mCancelReturnBtn->setCursor(Qt::PointingHandCursor);
    mCancelReturnBtn->setMinimumHeight(30);

    // 所有控制按钮：后续版本完善，当前仅提示
    auto showComingSoon = [this]() {
        QMessageBox::information(this, QString::fromUtf8("功能预告"),
            QString::fromUtf8("该功能将在后续版本中完善，敬请期待！"));
    };
    connect(mTakeoffBtn, &QPushButton::clicked, this, showComingSoon);
    connect(mReturnHomeBtn, &QPushButton::clicked, this, showComingSoon);
    connect(mCancelReturnBtn, &QPushButton::clicked, this, showComingSoon);
}

void FlightControlPanel::setDevice(const QString& displayName, const QString& gatewaySn, bool online) {
    if (mGatewaySn != gatewaySn || mOnline != online) {
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
        setStatus(QString::fromUtf8("机场离线，无法发送飞行控制指令"), true);
    else
        setStatus(QString::fromUtf8("已就绪，可执行飞行控制"));
    updateButtonStates();
}

void FlightControlPanel::clearDevice() {
    mDisplayName.clear();
    mGatewaySn.clear();
    mOnline = false;
    mPending = false;
    mOnlineLabel->setText(QString());
    mOnlineLabel->setStyleSheet(QString());
    setStatus(QString::fromUtf8("未选择可控制的机场设备"));
    updateButtonStates();
}

void FlightControlPanel::setConnected(bool connected) {
    mConnected = connected;
    if (!connected) {
        mPending = false;
        setStatus(QString::fromUtf8("MQTT 未连接"), true);
    }
    updateButtonStates();
}

void FlightControlPanel::setAvailableDocks(const QVector<DeviceInfo>& docks,
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

void FlightControlPanel::requestCommand(DockCommandType type, const QJsonObject& data) {
    if (mGatewaySn.isEmpty() || mPending)
        return;

    mPending = true;
    setStatus(QString::fromUtf8("正在执行：%1").arg(DockCommandBuilder::displayName(type)));
    updateButtonStates();
    emit commandRequested(mGatewaySn, type, data);
}

void FlightControlPanel::onCommandStateChanged(const DockCommandResult& result) {
    if (!mGatewaySn.isEmpty() && result.gatewaySn != mGatewaySn)
        return;

    // 只处理飞行控制相关指令
    if (result.type != DockCommandType::Takeoff && result.type != DockCommandType::Return)
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
        setStatus(QString::fromUtf8("%1成功").arg(action));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    appendHistory(result);
    updateButtonStates();
}

void FlightControlPanel::updateButtonStates() {
    const bool flightAvailable = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mTakeoffBtn->setEnabled(flightAvailable);
    mReturnHomeBtn->setEnabled(flightAvailable);
    mCancelReturnBtn->setEnabled(flightAvailable);
}

void FlightControlPanel::setStatus(const QString& text, bool error) {
    mStatusLabel->setText(text);
    mStatusLabel->setStyleSheet(error
        ? QStringLiteral("color: #d93025; font-weight: bold; padding: 4px;")
        : QStringLiteral("color: #e8710a; font-weight: bold; padding: 4px;"));
}

void FlightControlPanel::appendHistory(const DockCommandResult& result) {
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

    mHistoryEdit->moveCursor(QTextCursor::Start);
    mHistoryEdit->insertPlainText(block);
    mHistoryEdit->moveCursor(QTextCursor::Start);
}
