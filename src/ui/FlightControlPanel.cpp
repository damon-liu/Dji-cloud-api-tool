#include "FlightControlPanel.h"
#include "TakeoffConfigDialog.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
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

    mainLayout->addLayout(topRow);

    // ===================================================
    // 飞行控制
    // ===================================================
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行控制"), this);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->setSpacing(8);

    // -- 权限状态行 --
    auto* flightAuthRow = new QHBoxLayout;
    auto* flightAuthLabel = new QLabel(QString::fromUtf8("控制权:"), flightGroup);
    flightAuthLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 13px;");
    flightAuthRow->addWidget(flightAuthLabel);

    mFlightAuthStatusLabel = new QLabel(QString::fromUtf8("🔴 未获取"), flightGroup);
    mFlightAuthStatusLabel->setStyleSheet("color: #d93025; font-size: 13px; padding: 0 8px;");
    flightAuthRow->addWidget(mFlightAuthStatusLabel);

    flightAuthRow->addStretch();

    mFlightAuthGrabBtn = new QPushButton(QString::fromUtf8("获取"), flightGroup);
    mFlightAuthReleaseBtn = new QPushButton(QString::fromUtf8("释放"), flightGroup);
    flightAuthRow->addWidget(mFlightAuthGrabBtn);
    flightAuthRow->addWidget(mFlightAuthReleaseBtn);
    flightLayout->addLayout(flightAuthRow);

    // 分隔线
    auto* flightSep = new QFrame(flightGroup);
    flightSep->setFrameShape(QFrame::HLine);
    flightSep->setFrameShadow(QFrame::Sunken);
    flightLayout->addWidget(flightSep);

    // -- 起飞与返航 + DRC控制（左右并排） --
    auto* flightSubRow = new QHBoxLayout;

    // 起飞与返航子栏
    auto* takeoffReturnGroup = new QGroupBox(QString::fromUtf8("起飞与返航"), flightGroup);
    auto* trLayout = new QVBoxLayout(takeoffReturnGroup);
    auto* trBtnRow = new QHBoxLayout;

    mTakeoffBtn = new QPushButton(QString::fromUtf8("一键起飞"), takeoffReturnGroup);
    mTakeoffBtn->setCursor(Qt::PointingHandCursor);
    mTakeoffBtn->setFocusPolicy(Qt::NoFocus);
    mTakeoffBtn->setFixedHeight(34);
    mTakeoffBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mTakeoffBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 20px; font-size: 13px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");

    mReturnHomeBtn = new QPushButton(QString::fromUtf8("一键返航"), takeoffReturnGroup);
    mReturnHomeBtn->setCursor(Qt::PointingHandCursor);
    mReturnHomeBtn->setFocusPolicy(Qt::NoFocus);
    mReturnHomeBtn->setFixedHeight(34);
    mReturnHomeBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mReturnHomeBtn->setStyleSheet(
        "QPushButton { border: 1px solid #d93025; border-radius: 4px; color: #d93025;"
        "font-weight: bold; background: #fff; padding: 6px 20px; font-size: 13px; }"
        "QPushButton:hover { background: #fce8e6; }"
        "QPushButton:disabled { border-color: #dadce0; color: #80868b; background: #f8f9fa; }");

    mCancelReturnBtn = new QPushButton(QString::fromUtf8("取消返航"), takeoffReturnGroup);

    trBtnRow->addWidget(mTakeoffBtn);
    trBtnRow->addWidget(mReturnHomeBtn);
    trBtnRow->addWidget(mCancelReturnBtn);
    trBtnRow->addStretch();
    trLayout->addLayout(trBtnRow);
    flightSubRow->addWidget(takeoffReturnGroup, 1);

    // DRC控制子栏
    auto* drcGroup = new QGroupBox(QString::fromUtf8("DRC控制"), flightGroup);
    auto* drcLayout = new QVBoxLayout(drcGroup);
    mEmergencyStopBtn = new QPushButton(QString::fromUtf8("紧急悬停"), drcGroup);
    mEmergencyStopBtn->setCursor(Qt::PointingHandCursor);
    mEmergencyStopBtn->setFocusPolicy(Qt::NoFocus);
    mEmergencyStopBtn->setFixedHeight(34);
    mEmergencyStopBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mEmergencyStopBtn->setStyleSheet(
        "QPushButton { background: #d93025; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 20px; font-size: 13px; }"
        "QPushButton:hover { background: #b3261e; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    drcLayout->addWidget(mEmergencyStopBtn);
    drcLayout->addStretch();
    flightSubRow->addWidget(drcGroup, 1);

    flightLayout->addLayout(flightSubRow);

    mainLayout->addWidget(flightGroup);

    // ===================================================
    // 负载控制
    // ===================================================
    auto* payloadGroup = new QGroupBox(QString::fromUtf8("负载控制"), this);
    auto* payloadLayout = new QVBoxLayout(payloadGroup);
    payloadLayout->setSpacing(8);

    // -- 权限状态行 --
    auto* payloadAuthRow = new QHBoxLayout;
    auto* payloadAuthLabel = new QLabel(QString::fromUtf8("控制权:"), payloadGroup);
    payloadAuthLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 13px;");
    payloadAuthRow->addWidget(payloadAuthLabel);

    mPayloadAuthStatusLabel = new QLabel(QString::fromUtf8("🔴 未获取"), payloadGroup);
    mPayloadAuthStatusLabel->setStyleSheet("color: #d93025; font-size: 13px; padding: 0 8px;");
    payloadAuthRow->addWidget(mPayloadAuthStatusLabel);

    payloadAuthRow->addStretch();

    mPayloadAuthGrabBtn = new QPushButton(QString::fromUtf8("获取"), payloadGroup);
    mPayloadAuthReleaseBtn = new QPushButton(QString::fromUtf8("释放"), payloadGroup);
    payloadAuthRow->addWidget(mPayloadAuthGrabBtn);
    payloadAuthRow->addWidget(mPayloadAuthReleaseBtn);
    payloadLayout->addLayout(payloadAuthRow);

    // 分隔线
    auto* payloadSep = new QFrame(payloadGroup);
    payloadSep->setFrameShape(QFrame::HLine);
    payloadSep->setFrameShadow(QFrame::Sunken);
    payloadLayout->addWidget(payloadSep);

    // -- 拍摄控制 + 云台操作（左右并排） --
    auto* payloadSubRow = new QHBoxLayout;

    // 拍摄控制子栏
    auto* cameraGroup = new QGroupBox(QString::fromUtf8("拍摄操作"), payloadGroup);
    auto* cameraLayout = new QVBoxLayout(cameraGroup);
    mCameraPhotoBtn = new QPushButton(QString::fromUtf8("拍照"), cameraGroup);
    mCameraRecordStartBtn = new QPushButton(QString::fromUtf8("开始录像"), cameraGroup);
    mCameraRecordStopBtn = new QPushButton(QString::fromUtf8("结束录像"), cameraGroup);
    cameraLayout->addWidget(mCameraPhotoBtn);
    cameraLayout->addWidget(mCameraRecordStartBtn);
    cameraLayout->addWidget(mCameraRecordStopBtn);
    cameraLayout->addStretch();
    payloadSubRow->addWidget(cameraGroup);

    // 云台操作子栏
    auto* gimbalGroup = new QGroupBox(QString::fromUtf8("云台操作"), payloadGroup);
    auto* gimbalGrid = new QGridLayout(gimbalGroup);
    gimbalGrid->setSpacing(6);
    mGimbalCenterBtn = new QPushButton(QString::fromUtf8("回中"), gimbalGroup);
    mGimbalDownBtn = new QPushButton(QString::fromUtf8("向下"), gimbalGroup);
    mGimbalYawCenterBtn = new QPushButton(QString::fromUtf8("偏航回中"), gimbalGroup);
    mGimbalPitchDownBtn = new QPushButton(QString::fromUtf8("俯仰向下"), gimbalGroup);
    gimbalGrid->addWidget(mGimbalCenterBtn, 0, 0);
    gimbalGrid->addWidget(mGimbalDownBtn, 0, 1);
    gimbalGrid->addWidget(mGimbalYawCenterBtn, 1, 0);
    gimbalGrid->addWidget(mGimbalPitchDownBtn, 1, 1);
    payloadSubRow->addWidget(gimbalGroup);

    payloadLayout->addLayout(payloadSubRow);

    mainLayout->addWidget(payloadGroup);

    // ===================================================
    // 下发记录
    // ===================================================
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);

    // 设置最小宽度，防止负载控制子栏被压缩
    setMinimumWidth(520);

    // --- 默认按钮统一样式（对齐 DockControlPanel） ---
    const QList<QPushButton*> defaultButtons = {
        mFlightAuthGrabBtn, mFlightAuthReleaseBtn,
        mCancelReturnBtn,
        mPayloadAuthGrabBtn, mPayloadAuthReleaseBtn,
        mCameraPhotoBtn, mCameraRecordStartBtn, mCameraRecordStopBtn,
        mGimbalCenterBtn, mGimbalDownBtn, mGimbalYawCenterBtn, mGimbalPitchDownBtn
    };
    for (auto* btn : defaultButtons) {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setFixedHeight(34);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setStyleSheet(
            "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
            "background: #fff; color: #333; font-weight: bold;"
            "padding: 6px 20px; font-size: 13px; }"
            "QPushButton:hover { border-color: #1a73e8; color: #1a73e8; background: #e8f0fe; }"
            "QPushButton:disabled { border-color: #dadce0; color: #80868b; background: #f8f9fa; }");
    }

    // --- 信号连接 ---
    connect(mFlightAuthGrabBtn, &QPushButton::clicked, this, [this]() {
        requestCommand(DockCommandType::FlightAuthorityGrab);
    });
    connect(mFlightAuthReleaseBtn, &QPushButton::clicked, this, [this]() {
        requestCommand(DockCommandType::FlightAuthorityRelease);
    });

    connect(mTakeoffBtn, &QPushButton::clicked, this, [this]() {
        if (!mHasFlightAuthority) {
            QMessageBox::warning(this, QString::fromUtf8("提示"),
                QString::fromUtf8("请先获取飞行控制权后再执行一键起飞"));
            return;
        }
        TakeoffConfigDialog dlg(mDockLat, mDockLon, mDockAlt, this);
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

    connect(mPayloadAuthGrabBtn, &QPushButton::clicked, this, [this]() {
        QJsonObject data;
        data[QStringLiteral("payload_index")] = QStringLiteral("39-0-7");
        requestCommand(DockCommandType::PayloadAuthorityGrab, data);
    });
    connect(mPayloadAuthReleaseBtn, &QPushButton::clicked, this, [this]() {
        requestCommand(DockCommandType::PayloadAuthorityRelease);
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

void FlightControlPanel::updateDockPosition(double lat, double lon, double alt) {
    mDockLat = lat;
    mDockLon = lon;
    mDockAlt = alt;
}

void FlightControlPanel::setAvailableDocks(const QVector<DeviceInfo>& docks,
                                           const QString& currentSn,
                                           double dockLat, double dockLon,
                                           double dockAlt) {
    mAvailableDocks = docks;
    mDockLat = dockLat;
    mDockLon = dockLon;
    mDockAlt = dockAlt;

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
        setStatus(QString::fromUtf8("%1成功").arg(action));

        if (result.type == DockCommandType::FlightAuthorityGrab) {
            mHasFlightAuthority = true;
        } else if (result.type == DockCommandType::FlightAuthorityRelease
                   || result.type == DockCommandType::PayloadAuthorityRelease) {
            mHasFlightAuthority = false;
            mHasPayloadAuthority = false;
        } else if (result.type == DockCommandType::PayloadAuthorityGrab) {
            mHasPayloadAuthority = true;
        }
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    appendHistory(result);
    updateButtonStates();
}

void FlightControlPanel::updateButtonStates() {
    const bool baseAvailable = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;

    // -- 飞行控制权状态 --
    if (mHasFlightAuthority) {
        mFlightAuthStatusLabel->setText(QString::fromUtf8("🟢 已获取"));
        mFlightAuthStatusLabel->setStyleSheet("color: #1e8e3e; font-weight: bold; font-size: 13px; padding: 0 8px;");
    } else {
        mFlightAuthStatusLabel->setText(QString::fromUtf8("🔴 未获取"));
        mFlightAuthStatusLabel->setStyleSheet("color: #d93025; font-size: 13px; padding: 0 8px;");
    }

    mFlightAuthGrabBtn->setEnabled(baseAvailable);
    mFlightAuthReleaseBtn->setEnabled(baseAvailable);
    mTakeoffBtn->setEnabled(baseAvailable && mHasFlightAuthority);
    mReturnHomeBtn->setEnabled(baseAvailable);
    mCancelReturnBtn->setEnabled(baseAvailable);
    mEmergencyStopBtn->setEnabled(baseAvailable);

    // -- 负载控制权状态 --
    if (mHasPayloadAuthority) {
        mPayloadAuthStatusLabel->setText(QString::fromUtf8("🟢 已获取"));
        mPayloadAuthStatusLabel->setStyleSheet("color: #1e8e3e; font-weight: bold; font-size: 13px; padding: 0 8px;");
    } else {
        mPayloadAuthStatusLabel->setText(QString::fromUtf8("🔴 未获取"));
        mPayloadAuthStatusLabel->setStyleSheet("color: #d93025; font-size: 13px; padding: 0 8px;");
    }

    mPayloadAuthGrabBtn->setEnabled(baseAvailable);
    mPayloadAuthReleaseBtn->setEnabled(baseAvailable);
    mCameraPhotoBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
    mCameraRecordStartBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
    mCameraRecordStopBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
    mGimbalCenterBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
    mGimbalDownBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
    mGimbalYawCenterBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
    mGimbalPitchDownBtn->setEnabled(baseAvailable && mHasPayloadAuthority);
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
