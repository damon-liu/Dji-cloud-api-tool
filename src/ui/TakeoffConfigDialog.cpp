#include "TakeoffConfigDialog.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>

TakeoffConfigDialog::TakeoffConfigDialog(double dockLat, double dockLon,
                                           QWidget* parent)
    : QDialog(parent)
    , mDockLat(dockLat)
    , mDockLon(dockLon)
{
    setWindowTitle(QString::fromUtf8("一键起飞参数配置"));
    setModal(true);
    setMinimumWidth(420);
    setMaximumHeight(520);

    setupUi();
}

void TakeoffConfigDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(14);

    // --- 机场当前位置提示 ---
    mDockInfoLabel = new QLabel(this);
    mDockInfoLabel->setWordWrap(true);
    if (mDockLat != 0.0 || mDockLon != 0.0) {
        mDockInfoLabel->setText(
            QString::fromUtf8("机场当前坐标: %1, %2")
                .arg(mDockLat, 0, 'f', 7)
                .arg(mDockLon, 0, 'f', 7));
        mDockInfoLabel->setStyleSheet(
            "color: #1a73e8; font-size: 13px; font-weight: bold;"
            "padding: 6px 10px; background: #e8f0fe; border-radius: 4px;");
    } else {
        mDockInfoLabel->setText(
            QString::fromUtf8("⚠ 未能获取机场坐标，请手动输入目标位置"));
        mDockInfoLabel->setStyleSheet(
            "color: #d93025; font-size: 13px; font-weight: bold;"
            "padding: 6px 10px; background: #fce8e6; border-radius: 4px;");
    }
    mainLayout->addWidget(mDockInfoLabel);

    // --- 目标位置 ---
    auto* posGroup = new QGroupBox(QString::fromUtf8("目标位置"), this);
    auto* posLayout = new QVBoxLayout(posGroup);
    posLayout->setSpacing(10);

    auto makeRow = [this](const QString& label, QDoubleSpinBox* spin,
                           const QString& unit, double min, double max,
                           double step, int decimals, double defaultValue) {
        auto* row = new QHBoxLayout;
        auto* lbl = new QLabel(label, this);
        lbl->setFixedWidth(120);
        lbl->setStyleSheet("font-size: 13px; color: #333;");
        row->addWidget(lbl);
        spin->setRange(min, max);
        spin->setDecimals(decimals);
        spin->setSingleStep(step);
        spin->setValue(defaultValue);
        spin->setMinimumWidth(180);
        row->addWidget(spin);
        if (!unit.isEmpty()) {
            auto* ul = new QLabel(unit, this);
            ul->setStyleSheet("font-size: 12px; color: #666; padding-left: 4px;");
            row->addWidget(ul);
        }
        row->addStretch();
        return row;
    };

    mTargetLat = new QDoubleSpinBox(this);
    posLayout->addLayout(makeRow(QString::fromUtf8("目标纬度:"), mTargetLat,
                                 QString::fromUtf8("°"), -90.0, 90.0,
                                 0.000001, 6, mDockLat));

    mTargetLon = new QDoubleSpinBox(this);
    posLayout->addLayout(makeRow(QString::fromUtf8("目标经度:"), mTargetLon,
                                 QString::fromUtf8("°"), -180.0, 180.0,
                                 0.000001, 6, mDockLon));

    mTargetHeight = new QDoubleSpinBox(this);
    posLayout->addLayout(makeRow(QString::fromUtf8("目标点高度:"), mTargetHeight,
                                 QString::fromUtf8("米 (椭球高WGS84)"), 2.0, 1500.0,
                                 1.0, 1, 50.0));

    mainLayout->addWidget(posGroup);

    // --- 飞行参数 ---
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行参数"), this);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->setSpacing(10);

    mSafeTakeoffHeight = new QDoubleSpinBox(this);
    flightLayout->addLayout(makeRow(QString::fromUtf8("安全起飞高度:"), mSafeTakeoffHeight,
                                    QString::fromUtf8("米"), 1.0, 500.0,
                                    1.0, 1, 50.0));

    mRthAltitude = new QDoubleSpinBox(this);
    flightLayout->addLayout(makeRow(QString::fromUtf8("返航高度:"), mRthAltitude,
                                    QString::fromUtf8("米"), 20.0, 1500.0,
                                    1.0, 1, 100.0));

    mainLayout->addWidget(flightGroup);
    mainLayout->addStretch();

    // --- 安全确认 ---
    mSafetyConfirm = new QCheckBox(
        QString::fromUtf8("我已确认周围环境符合飞行安全要求，可以执行一键起飞"), this);
    mSafetyConfirm->setStyleSheet(
        "QCheckBox { color: #d93025; font-weight: bold; font-size: 13px; padding: 8px 0; }");
    connect(mSafetyConfirm, &QCheckBox::toggled,
            this, &TakeoffConfigDialog::updateConfirmButton);
    mainLayout->addWidget(mSafetyConfirm);

    // --- 底部按钮 ---
    auto* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    auto* cancelBtn = new QPushButton(QString::fromUtf8("取消"), this);
    cancelBtn->setMinimumWidth(100);
    cancelBtn->setStyleSheet(
        "QPushButton { background: #f1f3f4; color: #333; border: 1px solid #dadce0;"
        "border-radius: 4px; padding: 8px 20px; font-size: 13px; }"
        "QPushButton:hover { background: #e8eaed; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    mConfirmBtn = new QPushButton(QString::fromUtf8("确认起飞"), this);
    mConfirmBtn->setMinimumWidth(120);
    mConfirmBtn->setEnabled(false);
    mConfirmBtn->setStyleSheet(
        "QPushButton { background: #ea4335; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 8px 20px; font-size: 13px; }"
        "QPushButton:hover { background: #c5221f; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    connect(mConfirmBtn, &QPushButton::clicked, this, [this]() {
        if (validateInputs())
            accept();
    });
    btnLayout->addWidget(mConfirmBtn);

    mainLayout->addLayout(btnLayout);
}

void TakeoffConfigDialog::updateConfirmButton() {
    mConfirmBtn->setEnabled(mSafetyConfirm->isChecked());
}

bool TakeoffConfigDialog::validateInputs() {
    if (mTargetLat->value() == 0.0 && mTargetLon->value() == 0.0) {
        QMessageBox::warning(this, QString::fromUtf8("参数校验"),
            QString::fromUtf8("目标经纬度不能都为 0，请设置有效的目标位置。"));
        mTargetLat->setFocus();
        return false;
    }
    return true;
}

QJsonObject TakeoffConfigDialog::takeoffPayload() const {
    QJsonObject data;
    data[QStringLiteral("target_latitude")] = mTargetLat->value();
    data[QStringLiteral("target_longitude")] = mTargetLon->value();
    data[QStringLiteral("target_height")] = mTargetHeight->value();

    data[QStringLiteral("security_takeoff_height")] = mSafeTakeoffHeight->value();
    data[QStringLiteral("rth_mode")] = 1;
    data[QStringLiteral("rth_altitude")] = mRthAltitude->value();
    data[QStringLiteral("rc_lost_action")] = 0;
    data[QStringLiteral("commander_mode_lost_action")] = 1;
    data[QStringLiteral("commander_flight_mode")] = 1;
    data[QStringLiteral("commander_flight_height")] = 80.0;
    data[QStringLiteral("max_speed")] = 8;
    data[QStringLiteral("flight_safety_advance_check")] = 1;
    data[QStringLiteral("flight_id")] = QUuid::createUuid().toString(QUuid::WithoutBraces).toUpper();

    return data;
}
