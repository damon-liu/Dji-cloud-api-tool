#include "FlightControlPanel.h"
#include "TakeoffConfigDialog.h"
#include "StyleConstants.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>

FlightControlPanel::FlightControlPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateButtonStates();
}

void FlightControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

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

    auto* ctrlRecordBtn = new QPushButton(QString::fromUtf8("📋 控制记录"), this);
    styleLinkButton(ctrlRecordBtn);
    topRow->addWidget(ctrlRecordBtn);
    connect(ctrlRecordBtn, &QPushButton::clicked, this, [this]() {
        emit historyRequested();
    });

    mainLayout->addLayout(topRow);

    // ===== 可滚动内容区 =====
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    // ===================================================
    // 飞行控制（标题在 QGroupBox 边框上，按钮在框内右上角紧贴顶部）
    // ===================================================
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行控制"), scrollContent);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->setContentsMargins(8, 2, 8, 8);
    flightLayout->setSpacing(8);

    // -- 飞行控制权切换按钮（右对齐，紧贴 QGroupBox 标题栏下沿） --
    {
        auto* toggleRow = new QHBoxLayout;
        toggleRow->addStretch();
        mFlightAuthToggleBtn = new QPushButton(flightGroup);
        mFlightAuthToggleBtn->setCursor(Qt::PointingHandCursor);
        mFlightAuthToggleBtn->setFocusPolicy(Qt::NoFocus);
        mFlightAuthToggleBtn->setStyleSheet(
            "QPushButton {"
            "  border: 1px solid #c4d7f2; border-radius: 4px;"
            "  background: #e8f0fe; color: #202124; font-weight: bold;"
            "  padding: 5px 16px; font-size: 13px;"
            "}"
            "QPushButton:hover {"
            "  border-color: #1a73e8; background: #d2e3fc;"
            "}");
        toggleRow->addWidget(mFlightAuthToggleBtn);
        flightLayout->addLayout(toggleRow);
    }

    // -- 起飞与返航 + DRC控制（左右并排） --
    auto* flightSubRow = new QHBoxLayout;

    // 起飞与返航子栏
    auto* takeoffReturnGroup = new QGroupBox(QString::fromUtf8("起飞与返航"), flightGroup);
    takeoffReturnGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* trLayout = new QVBoxLayout(takeoffReturnGroup);
    auto* trBtnRow = new QHBoxLayout;

    mTakeoffBtn = new QPushButton(QString::fromUtf8("一键起飞"), takeoffReturnGroup);
    stylePrimaryButton(mTakeoffBtn);

    mReturnHomeBtn = new QPushButton(QString::fromUtf8("一键返航"), takeoffReturnGroup);
    styleDangerOutlineButton(mReturnHomeBtn);

    mCancelReturnBtn = new QPushButton(QString::fromUtf8("取消返航"), takeoffReturnGroup);

    trBtnRow->addStretch(1);
    trBtnRow->addWidget(mTakeoffBtn);
    trBtnRow->addStretch(1);
    trBtnRow->addWidget(mReturnHomeBtn);
    trBtnRow->addStretch(1);
    trBtnRow->addWidget(mCancelReturnBtn);
    trBtnRow->addStretch(1);
    trLayout->addLayout(trBtnRow);
    makeGroupEqualWidth({mTakeoffBtn, mReturnHomeBtn, mCancelReturnBtn});
    flightSubRow->addWidget(takeoffReturnGroup, 1);

    // DRC控制子栏
    auto* drcGroup = new QGroupBox(QString::fromUtf8("DRC控制"), flightGroup);
    drcGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* drcLayout = new QVBoxLayout(drcGroup);
    auto* drcBtnRow = new QHBoxLayout;
    drcBtnRow->addStretch();
    mEmergencyStopBtn = new QPushButton(QString::fromUtf8("紧急悬停"), drcGroup);
    styleDangerButton(mEmergencyStopBtn);
    drcBtnRow->addWidget(mEmergencyStopBtn);
    drcBtnRow->addStretch();
    drcLayout->addLayout(drcBtnRow);
    drcLayout->addStretch();
    flightSubRow->addWidget(drcGroup, 1);

    flightLayout->addLayout(flightSubRow);

    contentLayout->addWidget(flightGroup);

    // ===================================================
    // 负载控制（标题在 QGroupBox 边框上，按钮在框内右上角紧贴顶部）
    // ===================================================
    auto* payloadGroup = new QGroupBox(QString::fromUtf8("负载控制"), scrollContent);
    auto* payloadLayout = new QVBoxLayout(payloadGroup);
    payloadLayout->setContentsMargins(8, 2, 8, 8);
    payloadLayout->setSpacing(8);

    // -- 负载控制权切换按钮（右对齐，紧贴 QGroupBox 标题栏下沿） --
    {
        auto* toggleRow = new QHBoxLayout;
        toggleRow->addStretch();
        mPayloadAuthToggleBtn = new QPushButton(payloadGroup);
        mPayloadAuthToggleBtn->setCursor(Qt::PointingHandCursor);
        mPayloadAuthToggleBtn->setFocusPolicy(Qt::NoFocus);
        mPayloadAuthToggleBtn->setStyleSheet(
            "QPushButton {"
            "  border: 1px solid #c4d7f2; border-radius: 4px;"
            "  background: #e8f0fe; color: #202124; font-weight: bold;"
            "  padding: 5px 16px; font-size: 13px;"
            "}"
            "QPushButton:hover {"
            "  border-color: #1a73e8; background: #d2e3fc;"
            "}");
        toggleRow->addWidget(mPayloadAuthToggleBtn);
        payloadLayout->addLayout(toggleRow);
    }

    // -- 拍摄控制 + 云台操作（左右并排） --
    auto* payloadSubRow = new QHBoxLayout;

    // 拍摄控制子栏
    auto* cameraGroup = new QGroupBox(QString::fromUtf8("拍摄操作"), payloadGroup);
    cameraGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto* cameraLayout = new QHBoxLayout(cameraGroup);
    cameraLayout->setContentsMargins(8, 2, 8, 2);
    cameraLayout->addStretch(1);
    mCameraPhotoBtn = new QPushButton(QString::fromUtf8("拍照"), cameraGroup);
    mCameraRecordStartBtn = new QPushButton(QString::fromUtf8("开始录像"), cameraGroup);
    mCameraRecordStopBtn = new QPushButton(QString::fromUtf8("结束录像"), cameraGroup);
    cameraLayout->addWidget(mCameraPhotoBtn);
    cameraLayout->addStretch(1);
    cameraLayout->addWidget(mCameraRecordStartBtn);
    cameraLayout->addStretch(1);
    cameraLayout->addWidget(mCameraRecordStopBtn);
    cameraLayout->addStretch(1);
    makeGroupEqualWidth({mCameraPhotoBtn, mCameraRecordStartBtn, mCameraRecordStopBtn});
    payloadSubRow->addWidget(cameraGroup, 1);

    // 云台操作子栏
    auto* gimbalGroup = new QGroupBox(QString::fromUtf8("云台操作"), payloadGroup);
    gimbalGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto* gimbalGrid = new QGridLayout(gimbalGroup);
    gimbalGrid->setContentsMargins(8, 2, 8, 2);
    gimbalGrid->setSpacing(10);
    gimbalGrid->setRowStretch(0, 1);
    gimbalGrid->setRowStretch(1, 1);
    mGimbalCenterBtn = new QPushButton(QString::fromUtf8("回中"), gimbalGroup);
    mGimbalDownBtn = new QPushButton(QString::fromUtf8("向下"), gimbalGroup);
    mGimbalYawCenterBtn = new QPushButton(QString::fromUtf8("偏航回中"), gimbalGroup);
    mGimbalPitchDownBtn = new QPushButton(QString::fromUtf8("俯仰向下"), gimbalGroup);
    gimbalGrid->addWidget(mGimbalCenterBtn, 0, 0);
    gimbalGrid->addWidget(mGimbalDownBtn, 0, 1);
    gimbalGrid->addWidget(mGimbalYawCenterBtn, 1, 0);
    gimbalGrid->addWidget(mGimbalPitchDownBtn, 1, 1);
    makeGroupEqualWidth({mGimbalCenterBtn, mGimbalDownBtn, mGimbalYawCenterBtn, mGimbalPitchDownBtn});
    gimbalGrid->setColumnStretch(0, 1);
    gimbalGrid->setColumnStretch(1, 1);
    payloadSubRow->addWidget(gimbalGroup, 1);

    payloadLayout->addLayout(payloadSubRow);

    contentLayout->addWidget(payloadGroup);
    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    setMinimumWidth(520);

    // --- 按钮统一样式 ---
    const QList<QPushButton*> defaultButtons = {
        mCancelReturnBtn,
        mCameraPhotoBtn, mCameraRecordStartBtn, mCameraRecordStopBtn,
        mGimbalCenterBtn, mGimbalDownBtn, mGimbalYawCenterBtn, mGimbalPitchDownBtn
    };
    for (auto* btn : defaultButtons)
        styleDefaultButton(btn);

    // --- 信号连接 ---
    connect(mFlightAuthToggleBtn, &QPushButton::clicked, this, [this]() {
        if (mHasFlightAuthority)
            requestCommand(DockCommandType::FlightAuthorityRelease);
        else
            requestCommand(DockCommandType::FlightAuthorityGrab);
    });

    connect(mTakeoffBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasFlightAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取飞行控制权后再执行一键起飞"));
            return;
        }
        TakeoffConfigDialog dlg(mDockLat, mDockLon, mDockAlt,
                                   mDockLatStr, mDockLonStr, mDockAltStr, this);
        if (dlg.exec() == QDialog::Accepted)
            requestCommand(DockCommandType::Takeoff, dlg.takeoffPayload());
    });
    connect(mReturnHomeBtn, &QPushButton::clicked, this, [this]() {
        auto ret = QMessageBox::question(this,
            QString::fromUtf8("确认一键返航"),
            QString::fromUtf8("确定要执行一键返航吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            requestCommand(DockCommandType::Return);
    });
    connect(mCancelReturnBtn, &QPushButton::clicked, this, [this]() {
        auto ret = QMessageBox::question(this,
            QString::fromUtf8("确认取消返航"),
            QString::fromUtf8("确定要取消当前返航任务吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            requestCommand(DockCommandType::ReturnHomeCancel);
    });
    connect(mEmergencyStopBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::EmergencyStop); });

    connect(mPayloadAuthToggleBtn, &QPushButton::clicked, this, [this]() {
        if (mHasPayloadAuthority) {
            requestCommand(DockCommandType::PayloadAuthorityRelease);
        } else {
            QJsonObject data;
            data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
            requestCommand(DockCommandType::PayloadAuthorityGrab, data);
        }
    });

    connect(mCameraPhotoBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        requestCommand(DockCommandType::CameraPhotoTake, data);
    });
    connect(mCameraRecordStartBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        requestCommand(DockCommandType::CameraRecordStart, data);
    });
    connect(mCameraRecordStopBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        requestCommand(DockCommandType::CameraRecordStop, data);
    });

    connect(mGimbalCenterBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        data[QStringLiteral("reset_mode")] = 0;
        requestCommand(DockCommandType::GimbalReset, data);
    });
    connect(mGimbalDownBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        data[QStringLiteral("reset_mode")] = 1;
        requestCommand(DockCommandType::GimbalReset, data);
    });
    connect(mGimbalYawCenterBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        data[QStringLiteral("reset_mode")] = 2;
        requestCommand(DockCommandType::GimbalReset, data);
    });
    connect(mGimbalPitchDownBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasPayloadAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取负载控制权后再执行此操作"));
            return;
        }
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        data[QStringLiteral("reset_mode")] = 3;
        requestCommand(DockCommandType::GimbalReset, data);
    });
}

void FlightControlPanel::setDevice(const QString& displayName, const QString& gatewaySn, bool online) {
    if (mGatewaySn != gatewaySn || mOnline != online) {
        mPending = false;
        mHasFlightAuthority = false;
        mHasPayloadAuthority = false;
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
    mHasFlightAuthority = false;
    mHasPayloadAuthority = false;
    mOnlineLabel->setText(QString());
    mOnlineLabel->setStyleSheet(QString());
    setStatus(QString::fromUtf8("未选择可控制的机场设备"));
    updateButtonStates();
}

void FlightControlPanel::setConnected(bool connected) {
    mConnected = connected;
    if (!connected) {
        mPending = false;
        mHasFlightAuthority = false;
        mHasPayloadAuthority = false;
        setStatus(QString::fromUtf8("MQTT 未连接"), true);
    }
    updateButtonStates();
}

void FlightControlPanel::updateDockPosition(double lat, double lon, double alt,
                                            const QString& latStr, const QString& lonStr,
                                            const QString& altStr) {
    mDockLat = lat;
    mDockLon = lon;
    mDockAlt = alt;
    mDockLatStr = latStr;
    mDockLonStr = lonStr;
    mDockAltStr = altStr;
}

void FlightControlPanel::setAvailableDocks(const QVector<DeviceInfo>& docks,
                                           const QString& currentSn,
                                           double dockLat, double dockLon, double dockAlt,
                                           const QString& dockLatStr, const QString& dockLonStr,
                                           const QString& dockAltStr) {
    mAvailableDocks = docks;
    updateDockPosition(dockLat, dockLon, dockAlt, dockLatStr, dockLonStr, dockAltStr);

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
        if (result.type == DockCommandType::FlightAuthorityGrab)
            mHasFlightAuthority = true;
        else if (result.type == DockCommandType::FlightAuthorityRelease)
            mHasFlightAuthority = false;
        else if (result.type == DockCommandType::PayloadAuthorityGrab)
            mHasPayloadAuthority = true;
        else if (result.type == DockCommandType::PayloadAuthorityRelease)
            mHasPayloadAuthority = false;
        setStatus(QString::fromUtf8("%1成功").arg(action));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    updateButtonStates();
}

void FlightControlPanel::updateButtonStates() {
    const bool available = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;

    // 飞行控制权按钮
    mFlightAuthToggleBtn->setEnabled(available);
    if (mHasFlightAuthority)
        mFlightAuthToggleBtn->setText(QString::fromUtf8("✅ 释放控制权"));
    else
        mFlightAuthToggleBtn->setText(QString::fromUtf8("获取控制权"));

    // 负载控制权按钮
    mPayloadAuthToggleBtn->setEnabled(available);
    if (mHasPayloadAuthority)
        mPayloadAuthToggleBtn->setText(QString::fromUtf8("✅ 释放控制权"));
    else
        mPayloadAuthToggleBtn->setText(QString::fromUtf8("获取控制权"));

    // 飞行动作按钮（需要飞行控制权）
    const bool flightReady = available && mHasFlightAuthority;
    mTakeoffBtn->setEnabled(flightReady);
    mReturnHomeBtn->setEnabled(flightReady);
    mCancelReturnBtn->setEnabled(flightReady);
    mEmergencyStopBtn->setEnabled(flightReady);

    // 负载操作按钮（需要负载控制权）
    const bool payloadReady = available && mHasPayloadAuthority;
    mCameraPhotoBtn->setEnabled(payloadReady);
    mCameraRecordStartBtn->setEnabled(payloadReady);
    mCameraRecordStopBtn->setEnabled(payloadReady);
    mGimbalCenterBtn->setEnabled(payloadReady);
    mGimbalDownBtn->setEnabled(payloadReady);
    mGimbalYawCenterBtn->setEnabled(payloadReady);
    mGimbalPitchDownBtn->setEnabled(payloadReady);
}

void FlightControlPanel::setStatus(const QString& text, bool error) {
    mStatusLabel->setText(text);
    mStatusLabel->setStyleSheet(error
        ? QStringLiteral("color: #d93025; font-weight: bold; padding: 4px;")
        : QStringLiteral("color: #e8710a; font-weight: bold; padding: 4px;"));
}
