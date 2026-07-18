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

    // --- top row: dock selector + online status + execution status ---
    auto* topRow = new QHBoxLayout;

    auto* dockLabel = new QLabel(QString::fromUtf8("控制机场:"), this);
    dockLabel->setStyleSheet("font-weight: bold; color: #333;");
    topRow->addWidget(dockLabel);

    mDockCombo = new QComboBox(this);
    mDockCombo->setEditable(true);
    mDockCombo->setInsertPolicy(QComboBox::NoInsert);
    mDockCombo->setMinimumWidth(200);
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

    // --- card row: debug | drone power | cover | charge | flight ---
    auto* cardRow = new QHBoxLayout;
    cardRow->setSpacing(10);

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

    cardRow->addWidget(makeCard(QString::fromUtf8("飞机电源"),
                                QString::fromUtf8("开机"),
                                QString::fromUtf8("关机"),
                                mDroneOpenBtn, mDroneCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("机场舱盖"),
                                QString::fromUtf8("打开"),
                                QString::fromUtf8("关闭"),
                                mCoverOpenBtn, mCoverCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("飞机充电"),
                                QString::fromUtf8("开启"),
                                QString::fromUtf8("关闭"),
                                mChargeOpenBtn, mChargeCloseBtn), 2);

    // flight control card
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行控制"), this);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->addStretch();
    auto* flightBtnRow = new QHBoxLayout;
    mTakeoffBtn = new QPushButton(QString::fromUtf8("一键起飞"), flightGroup);
    mReturnBtn  = new QPushButton(QString::fromUtf8("一键返航"), flightGroup);
    mTakeoffBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    mReturnBtn->setStyleSheet(
        "QPushButton { background: #ea4335; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background: #c5221f; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    flightBtnRow->addWidget(mTakeoffBtn);
    flightBtnRow->addWidget(mReturnBtn);
    flightLayout->addLayout(flightBtnRow);
    cardRow->addWidget(flightGroup, 2);

    mainLayout->addLayout(cardRow);

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
        mCoverOpenBtn, mCoverCloseBtn, mChargeOpenBtn, mChargeCloseBtn
    };
    for (auto* button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(30);
    }
    mTakeoffBtn->setCursor(Qt::PointingHandCursor);
    mTakeoffBtn->setMinimumHeight(30);
    mReturnBtn->setCursor(Qt::PointingHandCursor);
    mReturnBtn->setMinimumHeight(30);

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

    connect(mTakeoffBtn, &QPushButton::clicked, this, &DockControlPanel::onTakeoffClicked);
    connect(mReturnBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::Return); });
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
        mOnlineLabel->setText(QString::fromUtf8("ð¢ 在线"));
        mOnlineLabel->setStyleSheet("color: #1e8e3e; font-weight: bold; padding: 0 8px;");
    } else {
        mOnlineLabel->setText(QString::fromUtf8("ð´ 离线"));
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
            QString::fromUtf8("ð¢ %1 - %2").arg(d.name, d.sn), d.sn);
        if (d.sn == currentSn)
            selectIdx = i;
    }

    if (selectIdx >= 0)
        mDockCombo->setCurrentIndex(selectIdx);
    else if (!docks.isEmpty())
        mDockCombo->setCurrentIndex(0);

    mUpdatingCombo = false;

    if (!docks.isEmpty() && selectIdx < 0) {
        const auto& d = docks[0];
        setDevice(d.name, d.sn, true);
    }
}

void DockControlPanel::onTakeoffClicked() {
    if (mGatewaySn.isEmpty() || mPending) return;

    TakeoffDialog dlg(mDockLat, mDockLon, this);
    if (dlg.exec() != QDialog::Accepted) return;

    TakeoffParams p = dlg.params();
    QJsonObject data;
    data[QStringLiteral("latitude")]  = p.latitude;
    data[QStringLiteral("longitude")] = p.longitude;
    data[QStringLiteral("height")]    = p.height;
    if (p.speed > 0.0)
        data[QStringLiteral("speed")] = p.speed;

    requestCommand(DockCommandType::Takeoff, data);
}

void DockControlPanel::requestCommand(DockCommandType type, const QJsonObject& data) {
    if (mGatewaySn.isEmpty() || mPending)
        return;

    if (type == DockCommandType::Return) {
        if (QMessageBox::question(this, QString::fromUtf8("确认返航"),
                QString::fromUtf8("确定要让飞机返航吗？"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes)
            return;
    } else if (type != DockCommandType::Takeoff) {
        const QString action = DockCommandBuilder::displayName(type);
        const QString text = QString::fromUtf8("确定要对机场 %1（%2）执行“%3”吗？")
            .arg(mDisplayName, mGatewaySn, action);
        if (QMessageBox::question(this, QString::fromUtf8("确认机场控制"), text,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes)
            return;
    }

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
        setStatus(QString::fromUtf8("%1成功：%2").arg(action, result.message));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    appendHistory(result);
    updateButtonStates();

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

    const bool flightAvailable = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mTakeoffBtn->setEnabled(flightAvailable);
    mReturnBtn->setEnabled(flightAvailable);

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
