#include "MaintenancePanel.h"

#include <QMessageBox>

MaintenancePanel::MaintenancePanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

QGroupBox* MaintenancePanel::makeToggleCard(const QString& title,
                                              const QString& leftText, const QString& rightText,
                                              QPushButton*& leftBtn, QPushButton*& rightBtn) {
    auto* group = new QGroupBox(title, this);
    auto* v = new QVBoxLayout(group);
    v->addStretch();
    auto* row = new QHBoxLayout;
    leftBtn  = new QPushButton(leftText, group);
    rightBtn = new QPushButton(rightText, group);
    row->addWidget(leftBtn);
    row->addWidget(rightBtn);
    v->addLayout(row);
    return group;
}

QGroupBox* MaintenancePanel::makeActionCard(const QString& title, const QString& btnText,
                                              QPushButton*& btn) {
    auto* group = new QGroupBox(title, this);
    auto* v = new QVBoxLayout(group);
    v->addStretch();
    btn = new QPushButton(btnText, group);
    v->addWidget(btn);
    return group;
}

void MaintenancePanel::showComingSoon(const QString& featureName) {
    QMessageBox::information(this,
        QString::fromUtf8("功能预览"),
        QString::fromUtf8("「%1」功能将在后续版本中完善，敬请期待！").arg(featureName));
    appendHistory(featureName);
}

void MaintenancePanel::appendHistory(const QString& action) {
    QString block;
    block += QStringLiteral("[%1] 🔧 %2\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action);
    block += QString::fromUtf8("状态: 功能预览（未实现）\n");
    block += QString::fromUtf8("────────────────────────────\n");

    mHistoryEdit->moveCursor(QTextCursor::Start);
    mHistoryEdit->insertPlainText(block);
    mHistoryEdit->moveCursor(QTextCursor::Start);
}

void MaintenancePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 滚动区域：包裹提示 + 卡片 + 展开按钮 ---
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { width: 8px; background: #f5f5f5; }"
        "QScrollBar::handle:vertical { background: #c4c4c4; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: #a0a0a0; }");

    auto* scrollContent = new QWidget(scrollArea);
    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(8, 8, 8, 8);
    scrollLayout->setSpacing(6);

    // --- 状态提示 ---
    mHintLabel = new QLabel(
        QString::fromUtf8("⚠️ 运维模式功能将在后续版本中完善，当前仅供预览"), scrollContent);
    mHintLabel->setStyleSheet(
        "color: #b06000; font-weight: bold; padding: 6px 12px;"
        "background: #fff8e1; border: 1px solid #ffcc02; border-radius: 4px;");
    mHintLabel->setWordWrap(true);
    scrollLayout->addWidget(mHintLabel);

    // --- 卡片网格：第一行 3 列默认可见 ---
    auto* cardGrid = new QGridLayout;
    cardGrid->setSpacing(10);

    // 第1行：补光灯开关 | 电池保养状态 | 电池运行模式
    auto* fillLightGroup = makeToggleCard(
        QString::fromUtf8("补光灯开关"),
        QString::fromUtf8("开启"), QString::fromUtf8("关闭"),
        mFillLightOnBtn, mFillLightOffBtn);
    auto* batteryMaintGroup = makeToggleCard(
        QString::fromUtf8("电池保养状态"),
        QString::fromUtf8("普通"), QString::fromUtf8("保养"),
        mBatteryNormalBtn, mBatteryMaintainBtn);
    auto* batteryModeGroup = makeToggleCard(
        QString::fromUtf8("电池运行模式"),
        QString::fromUtf8("标准"), QString::fromUtf8("静音"),
        mBatteryStandardBtn, mBatterySilentBtn);
    cardGrid->addWidget(fillLightGroup,    0, 0);
    cardGrid->addWidget(batteryMaintGroup, 0, 1);
    cardGrid->addWidget(batteryModeGroup,  0, 2);

    for (int col = 0; col < 3; ++col)
        cardGrid->setColumnStretch(col, 1);

    scrollLayout->addLayout(cardGrid);

    // --- 展开/收起按钮 ---
    auto* expandBar = new QHBoxLayout;
    expandBar->addStretch();
    mExpandBtn = new QPushButton(QString::fromUtf8("展开更多  ▼"), scrollContent);
    mExpandBtn->setFlat(true);
    mExpandBtn->setCursor(Qt::PointingHandCursor);
    mExpandBtn->setStyleSheet(
        "QPushButton { color: #1a73e8; font-size: 12px; padding: 2px 8px; border: none; }"
        "QPushButton:hover { color: #1557b0; }");
    expandBar->addWidget(mExpandBtn);
    expandBar->addStretch();
    scrollLayout->addLayout(expandBar);

    // 展开区域（默认隐藏）：每行 3 列
    mExpandRow = new QWidget(scrollContent);
    auto* expandGrid = new QGridLayout(mExpandRow);
    expandGrid->setContentsMargins(0, 0, 0, 0);
    expandGrid->setSpacing(10);

    // 机场空调：3个按钮（制冷/制热/送风）
    auto* acGroup = new QGroupBox(QString::fromUtf8("机场空调工作模式"), mExpandRow);
    auto* acLayout = new QVBoxLayout(acGroup);
    acLayout->addStretch();
    auto* acBtnRow = new QHBoxLayout;
    mAcCoolBtn = new QPushButton(QString::fromUtf8("制冷"), acGroup);
    mAcHeatBtn = new QPushButton(QString::fromUtf8("制热"), acGroup);
    mAcFanBtn  = new QPushButton(QString::fromUtf8("送风"), acGroup);
    acBtnRow->addWidget(mAcCoolBtn);
    acBtnRow->addWidget(mAcHeatBtn);
    acBtnRow->addWidget(mAcFanBtn);
    acLayout->addLayout(acBtnRow);

    auto* alarmGroup = makeToggleCard(
        QString::fromUtf8("机场声光报警"),
        QString::fromUtf8("开启"), QString::fromUtf8("关闭"),
        mAlarmOnBtn, mAlarmOffBtn);
    auto* aircraftFormatGroup = makeActionCard(
        QString::fromUtf8("飞行器数据格式化"),
        QString::fromUtf8("格式化"), mAircraftFormatBtn);
    expandGrid->addWidget(acGroup,             0, 0);
    expandGrid->addWidget(alarmGroup,          0, 1);
    expandGrid->addWidget(aircraftFormatGroup, 0, 2);

    // 机场数据格式化 | eSIM 激活 | SIM 切换
    auto* dockFormatGroup = makeActionCard(
        QString::fromUtf8("机场数据格式化"),
        QString::fromUtf8("格式化"), mDockFormatBtn);
    auto* esimGroup = makeActionCard(
        QString::fromUtf8("eSIM 激活"),
        QString::fromUtf8("激活"), mEsimActivateBtn);
    auto* simGroup = makeToggleCard(
        QString::fromUtf8("SIM 切换"),
        QString::fromUtf8("SIM 1"), QString::fromUtf8("SIM 2"),
        mSim1Btn, mSim2Btn);
    expandGrid->addWidget(dockFormatGroup, 1, 0);
    expandGrid->addWidget(esimGroup,       1, 1);
    expandGrid->addWidget(simGroup,        1, 2);

    // 运营商切换：3个按钮
    auto* carrierGroup = new QGroupBox(QString::fromUtf8("运营商切换"), mExpandRow);
    auto* carrierLayout = new QVBoxLayout(carrierGroup);
    carrierLayout->addStretch();
    auto* carrierBtnRow = new QHBoxLayout;
    mCarrierMobileBtn  = new QPushButton(QString::fromUtf8("移动"), carrierGroup);
    mCarrierUnicomBtn  = new QPushButton(QString::fromUtf8("联通"), carrierGroup);
    mCarrierTelecomBtn = new QPushButton(QString::fromUtf8("电信"), carrierGroup);
    carrierBtnRow->addWidget(mCarrierMobileBtn);
    carrierBtnRow->addWidget(mCarrierUnicomBtn);
    carrierBtnRow->addWidget(mCarrierTelecomBtn);
    carrierLayout->addLayout(carrierBtnRow);

    expandGrid->addWidget(carrierGroup, 2, 0);

    for (int col = 0; col < 3; ++col)
        expandGrid->setColumnStretch(col, 1);
    mExpandRow->setVisible(false);
    scrollLayout->addWidget(mExpandRow);

    // scrollLayout 末尾加弹簧，防止内容不足时卡片被拉伸
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    connect(mExpandBtn, &QPushButton::clicked, this, [this]() {
        bool visible = !mExpandRow->isVisible();
        mExpandRow->setVisible(visible);
        mExpandBtn->setText(visible
            ? QString::fromUtf8("收起更多  ▲")
            : QString::fromUtf8("展开更多  ▼"));
    });

    // --- 统一样式所有按钮 ---
    const QList<QPushButton*> buttons = {
        mFillLightOnBtn, mFillLightOffBtn,
        mBatteryNormalBtn, mBatteryMaintainBtn,
        mBatteryStandardBtn, mBatterySilentBtn,
        mAcCoolBtn, mAcHeatBtn, mAcFanBtn,
        mAlarmOnBtn, mAlarmOffBtn,
        mAircraftFormatBtn, mDockFormatBtn, mEsimActivateBtn,
        mSim1Btn, mSim2Btn,
        mCarrierMobileBtn, mCarrierUnicomBtn, mCarrierTelecomBtn
    };
    for (auto* button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(30);
        button->setStyleSheet(
            "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
            "background: #fff; font-size: 12px; padding: 6px 12px; color: #333; }"
            "QPushButton:hover { background: #e8f0fe; border-color: #1a73e8; color: #1a73e8; }");
    }

    // --- 连接信号 ---
    connect(mFillLightOnBtn,  &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("补光灯开启")); });
    connect(mFillLightOffBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("补光灯关闭")); });
    connect(mBatteryNormalBtn,   &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("电池普通状态")); });
    connect(mBatteryMaintainBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("电池保养状态")); });
    connect(mBatteryStandardBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("电池标准模式")); });
    connect(mBatterySilentBtn,   &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("电池静音模式")); });
    connect(mAcCoolBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("空调制冷模式")); });
    connect(mAcHeatBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("空调制热模式")); });
    connect(mAcFanBtn,  &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("空调送风模式")); });
    connect(mAlarmOnBtn,  &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("声光报警开启")); });
    connect(mAlarmOffBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("声光报警关闭")); });
    connect(mAircraftFormatBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("飞行器数据格式化")); });
    connect(mDockFormatBtn,     &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("机场数据格式化")); });
    connect(mEsimActivateBtn,   &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("eSIM 激活")); });
    connect(mSim1Btn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("切换 SIM 1")); });
    connect(mSim2Btn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("切换 SIM 2")); });
    connect(mCarrierMobileBtn,  &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("切换移动运营商")); });
    connect(mCarrierUnicomBtn,  &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("切换联通运营商")); });
    connect(mCarrierTelecomBtn, &QPushButton::clicked, this, [this]() { showComingSoon(QString::fromUtf8("切换电信运营商")); });

    // --- 下发记录 ---
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);
}