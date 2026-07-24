#include "TakeoffConfigDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>
#include <cmath>

TakeoffConfigDialog::TakeoffConfigDialog(double dockLat, double dockLon,
                                           double dockAlt,
                                           const QString& dockLatStr,
                                           const QString& dockLonStr,
                                           const QString& dockAltStr,
                                           QWidget* parent)
    : QDialog(parent)
    , mDockLat(dockLat)
    , mDockLon(dockLon)
    , mDockAlt(dockAlt)
    , mDockLatStr(dockLatStr)
    , mDockLonStr(dockLonStr)
    , mDockAltStr(dockAltStr)
{
    setWindowTitle(QString::fromUtf8("一键起飞参数配置"));
    setModal(true);
    setMinimumWidth(420);
    setMaximumHeight(560);

    setupUi();
}

void TakeoffConfigDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(14);

    // --- 机场当前位置 + 椭球高（卡片式单行） ---
    auto* dockInfoCard = new QFrame(this);
    dockInfoCard->setStyleSheet(
        "QFrame#dockInfoCard {"
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "  stop:0 #e8f0fe, stop:0.04 #1a73e8, stop:0.04 #e8f0fe, stop:1 #f8f9fa);"
        "border: 1px solid #c4d7f0; border-radius: 6px;"
        "padding: 4px 0px; }");
    dockInfoCard->setObjectName("dockInfoCard");

    auto* dockInfoRow = new QHBoxLayout(dockInfoCard);
    dockInfoRow->setContentsMargins(14, 8, 12, 8);
    dockInfoRow->setSpacing(16);

    if (mDockLat != 0.0 || mDockLon != 0.0) {
        // 优先使用 OSD 原始字符串保留小数位数，fallback 到 double 格式化
        QString latText = !mDockLatStr.isEmpty() ? mDockLatStr
                                                  : QString::number(mDockLat, 'f', 7);
        QString lonText = !mDockLonStr.isEmpty() ? mDockLonStr
                                                  : QString::number(mDockLon, 'f', 7);

        // 坐标部分
        auto* coordLabel = new QLabel(this);
        coordLabel->setText(
            QString::fromUtf8("<span style='font-weight:bold;color:#1a73e8;'>"
                              "机场坐标</span>"
                              "<span style='color:#202124;'> %1, %2</span>")
                .arg(latText, lonText));
        coordLabel->setStyleSheet("font-size: 13px; border:none; background:transparent;");
        dockInfoRow->addWidget(coordLabel);

        // 分隔线
        auto* sep = new QFrame(this);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("border: none; background: #c4d7f0; min-width: 1px; max-width: 1px;");
        dockInfoRow->addWidget(sep);

        // 椭球高部分（固定 2 位小数）
        auto* altLabel = new QLabel(this);
        altLabel->setText(
            QString::fromUtf8("<span style='font-weight:bold;color:#1a73e8;'>"
                              "椭球高</span>"
                              "<span style='color:#202124;'> %1 m</span>")
                .arg(mDockAlt, 0, 'f', 2));
        altLabel->setStyleSheet("font-size: 13px; border:none; background:transparent;");
        dockInfoRow->addWidget(altLabel);
    } else {
        auto* warnLabel = new QLabel(this);
        warnLabel->setText(
            QString::fromUtf8("<span style='color:#d93025;font-weight:bold;'>⚠ 未能获取机场坐标</span>"
                              "<span style='color:#5f6368;'> — 请手动输入目标位置</span>"));
        warnLabel->setStyleSheet("font-size: 13px; border:none; background:transparent;");
        dockInfoRow->addWidget(warnLabel);

        auto* sep = new QFrame(this);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("border: none; background: #dadce0; min-width: 1px; max-width: 1px;");
        dockInfoRow->addWidget(sep);

        auto* altLabel = new QLabel(this);
        altLabel->setText(
            QString::fromUtf8("<span style='font-weight:bold;color:#1a73e8;'>"
                              "椭球高</span>"
                              "<span style='color:#d93025;'> ⚠ 无数据</span>"));
        altLabel->setStyleSheet("font-size: 13px; border:none; background:transparent;");
        dockInfoRow->addWidget(altLabel);
    }

    dockInfoRow->addStretch();
    mainLayout->addWidget(dockInfoCard);

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

    // 从 OSD 原始字符串中获取小数位数，确保目标经纬度精度与上报一致
    auto decimalPlaces = [](const QString& s) -> int {
        int dot = s.lastIndexOf('.');
        return (dot >= 0) ? s.length() - dot - 1 : 0;
    };
    int latDecimals = qMax(decimalPlaces(mDockLatStr), 6);
    int lonDecimals = qMax(decimalPlaces(mDockLonStr), 6);
    double latStep = std::pow(10.0, -latDecimals);
    double lonStep = std::pow(10.0, -lonDecimals);

    mTargetLat = new QDoubleSpinBox(this);
    posLayout->addLayout(makeRow(QString::fromUtf8("目标纬度:"), mTargetLat,
                                 QString::fromUtf8("°"), -90.0, 90.0,
                                 latStep, latDecimals, mDockLat));

    mTargetLon = new QDoubleSpinBox(this);
    posLayout->addLayout(makeRow(QString::fromUtf8("目标经度:"), mTargetLon,
                                 QString::fromUtf8("°"), -180.0, 180.0,
                                 lonStep, lonDecimals, mDockLon));

    mTargetHeight = new QDoubleSpinBox(this);
    mTargetHeight->setRange(2.0, 1500.0);
    mTargetHeight->setDecimals(1);
    mTargetHeight->setSingleStep(1.0);
    mTargetHeight->setValue(70.0);
    mTargetHeight->setMinimumWidth(160);

    mHeightTypeCombo = new QComboBox(this);
    mHeightTypeCombo->addItem(QString::fromUtf8("相对高度 (ALT)"), QStringLiteral("relative"));
    mHeightTypeCombo->addItem(QString::fromUtf8("椭球高 (WGS84)"), QStringLiteral("ellipsoid"));
    mHeightTypeCombo->setCurrentIndex(0);
    mHeightTypeCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px;"
        "padding: 3px 6px; font-size: 12px; background: #fff; }");

    auto* heightUnitLabel = new QLabel(QString::fromUtf8("米 (相对机场)"), this);
    heightUnitLabel->setStyleSheet("font-size: 12px; color: #666; padding-left: 4px;");

    connect(mHeightTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [heightUnitLabel, this](int idx) {
        if (mHeightTypeCombo->itemData(idx).toString() == "relative") {
            heightUnitLabel->setText(QString::fromUtf8("米 (相对机场)"));
        } else {
            heightUnitLabel->setText(QString::fromUtf8("米 (椭球高WGS84)"));
        }
    });

    {
        auto* row = new QHBoxLayout;
        auto* lbl = new QLabel(QString::fromUtf8("目标点高度:"), this);
        lbl->setFixedWidth(120);
        lbl->setStyleSheet("font-size: 13px; color: #333;");
        row->addWidget(lbl);
        row->addWidget(mTargetHeight);
        row->addWidget(heightUnitLabel);
        row->addWidget(mHeightTypeCombo);
        row->addStretch();
        posLayout->addLayout(row);
    }

    mainLayout->addWidget(posGroup);

    // --- 飞行参数 ---
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行参数"), this);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->setSpacing(10);

    mSafeTakeoffHeight = new QDoubleSpinBox(this);
    flightLayout->addLayout(makeRow(QString::fromUtf8("安全起飞高度:"), mSafeTakeoffHeight,
                                    QString::fromUtf8("米"), 1.0, 500.0,
                                    1.0, 1, 70.0));

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

    if (mHeightTypeCombo->currentData().toString() == "relative"
            && mDockAlt == 0.0) {
        QMessageBox::warning(this, QString::fromUtf8("参数校验"),
            QString::fromUtf8("未能获取机场椭球高度，无法使用相对高度模式。\n"
                              "请将高度类型切换为\"椭球高 (WGS84)\"后手动输入椭球高度。"));
        return false;
    }

    return true;
}

QJsonObject TakeoffConfigDialog::takeoffPayload() const {
    QJsonObject data;
    data[QStringLiteral("target_latitude")] = mTargetLat->value();
    data[QStringLiteral("target_longitude")] = mTargetLon->value();

    double targetHeight = mTargetHeight->value();
    if (mHeightTypeCombo->currentData().toString() == "relative") {
        targetHeight = mDockAlt + targetHeight;
    }
    data[QStringLiteral("target_height")] = targetHeight;

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
