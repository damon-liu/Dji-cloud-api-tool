#ifndef MAINTENANCEPANEL_H
#define MAINTENANCEPANEL_H

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTime>

// 运维模式面板：放置不常用的维护功能按钮，后续版本实现具体功能
class MaintenancePanel : public QWidget {
    Q_OBJECT
public:
    explicit MaintenancePanel(QWidget* parent = nullptr);

private:
    void setupUi();
    void appendHistory(const QString& action);
    void showComingSoon(const QString& featureName);

    // 帮助函数：创建含两个按钮的卡片
    QGroupBox* makeToggleCard(const QString& title,
                               const QString& leftText, const QString& rightText,
                               QPushButton*& leftBtn, QPushButton*& rightBtn);
    // 帮助函数：创建单按钮卡片
    QGroupBox* makeActionCard(const QString& title, const QString& btnText,
                               QPushButton*& btn);

    // --- 补光灯 ---
    QPushButton* mFillLightOnBtn  = nullptr;
    QPushButton* mFillLightOffBtn = nullptr;

    // --- 电池保养状态 ---
    QPushButton* mBatteryNormalBtn = nullptr;
    QPushButton* mBatteryMaintainBtn = nullptr;

    // --- 电池运行模式 ---
    QPushButton* mBatteryStandardBtn = nullptr;
    QPushButton* mBatterySilentBtn = nullptr;

    // --- 机场空调 ---
    QPushButton* mAcCoolBtn = nullptr;
    QPushButton* mAcHeatBtn = nullptr;
    QPushButton* mAcFanBtn  = nullptr;

    // --- 声光报警 ---
    QPushButton* mAlarmOnBtn  = nullptr;
    QPushButton* mAlarmOffBtn = nullptr;

    // --- 飞行器数据格式化 ---
    QPushButton* mAircraftFormatBtn = nullptr;

    // --- 机场数据格式化 ---
    QPushButton* mDockFormatBtn = nullptr;

    // --- eSIM 激活 ---
    QPushButton* mEsimActivateBtn = nullptr;

    // --- SIM 切换 ---
    QPushButton* mSim1Btn = nullptr;
    QPushButton* mSim2Btn = nullptr;

    // --- 运营商切换 ---
    QPushButton* mCarrierMobileBtn = nullptr;
    QPushButton* mCarrierUnicomBtn = nullptr;
    QPushButton* mCarrierTelecomBtn = nullptr;

    // --- 状态提示 ---
    QLabel* mHintLabel = nullptr;

    // --- 展开/收起 ---
    QPushButton* mExpandBtn = nullptr;
    QWidget*     mExpandRow = nullptr;

    // --- 下发记录 ---
    QPlainTextEdit* mHistoryEdit = nullptr;
};

#endif // MAINTENANCEPANEL_H
