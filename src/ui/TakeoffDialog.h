#ifndef TAKEOFFDIALOG_H
#define TAKEOFFDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

struct TakeoffParams {
    double latitude  = 0.0;
    double longitude = 0.0;
    double height    = 100.0;   // 飞行高度 (m)
    double speed     = 0.0;     // 飞行速度 (m/s)，0 表示使用默认值
};

class TakeoffDialog : public QDialog {
    Q_OBJECT
public:
    explicit TakeoffDialog(double defaultLat, double defaultLon, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QString::fromUtf8("一键起飞 - 飞行参数设置"));

        setStyleSheet(
            "QGroupBox { font-weight: bold; color: #333; border: 1px solid #e0e0e0;"
            "border-radius: 6px; margin-top: 10px; background: #fafbfc; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QDoubleSpinBox { border: 1px solid #dadce0; border-radius: 4px;"
            "padding: 5px 8px; background: #fff; font-size: 13px; font-weight: normal; }"
            "QDoubleSpinBox:focus { border: 1px solid #1a73e8; }");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(10);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        // --- 标题区 ---
        auto* titleLabel = new QLabel(QString::fromUtf8("🛫 设置飞行参数"), this);
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #202124;");
        layout->addWidget(titleLabel);

        auto* hint = new QLabel(
            QString::fromUtf8("目标位置默认为机场当前位置，确认环境安全后方可起飞"), this);
        hint->setWordWrap(true);
        hint->setStyleSheet("color: #5f6368; font-size: 12px;");
        hint->setMinimumWidth(428);
        layout->addWidget(hint);

        auto makeFieldLabel = [](const QString& text, QWidget* parent) {
            auto* label = new QLabel(text, parent);
            label->setStyleSheet("color: #5f6368; font-size: 12px; font-weight: normal;");
            return label;
        };

        // --- 目标位置卡片：纬度 | 经度 ---
        auto* posGroup = new QGroupBox(QString::fromUtf8("目标位置"), this);
        auto* posGrid = new QGridLayout(posGroup);
        posGrid->setContentsMargins(12, 16, 12, 12);
        posGrid->setHorizontalSpacing(16);
        posGrid->setVerticalSpacing(4);

        mLatSpin = new QDoubleSpinBox(posGroup);
        mLatSpin->setRange(-90.0, 90.0);
        mLatSpin->setDecimals(6);
        mLatSpin->setValue(defaultLat);
        mLatSpin->setSuffix(QString::fromUtf8("\xc2\xb0"));

        mLonSpin = new QDoubleSpinBox(posGroup);
        mLonSpin->setRange(-180.0, 180.0);
        mLonSpin->setDecimals(6);
        mLonSpin->setValue(defaultLon);
        mLonSpin->setSuffix(QString::fromUtf8("\xc2\xb0"));

        posGrid->addWidget(makeFieldLabel(QString::fromUtf8("纬度"), posGroup), 0, 0);
        posGrid->addWidget(makeFieldLabel(QString::fromUtf8("经度"), posGroup), 0, 1);
        posGrid->addWidget(mLatSpin, 1, 0);
        posGrid->addWidget(mLonSpin, 1, 1);
        posGrid->setColumnStretch(0, 1);
        posGrid->setColumnStretch(1, 1);
        layout->addWidget(posGroup);

        // --- 飞行参数卡片：高度 | 速度 ---
        auto* paramGroup = new QGroupBox(QString::fromUtf8("飞行参数"), this);
        auto* paramGrid = new QGridLayout(paramGroup);
        paramGrid->setContentsMargins(12, 16, 12, 12);
        paramGrid->setHorizontalSpacing(16);
        paramGrid->setVerticalSpacing(4);

        mHeightSpin = new QDoubleSpinBox(paramGroup);
        mHeightSpin->setRange(20.0, 500.0);
        mHeightSpin->setDecimals(1);
        mHeightSpin->setValue(100.0);
        mHeightSpin->setSuffix(QString::fromUtf8(" m"));

        mSpeedSpin = new QDoubleSpinBox(paramGroup);
        mSpeedSpin->setRange(0.0, 15.0);
        mSpeedSpin->setDecimals(1);
        mSpeedSpin->setValue(0.0);
        mSpeedSpin->setSpecialValueText(QString::fromUtf8("默认"));
        mSpeedSpin->setSuffix(QString::fromUtf8(" m/s"));

        paramGrid->addWidget(makeFieldLabel(QString::fromUtf8("飞行高度（20 ~ 500 m）"), paramGroup), 0, 0);
        paramGrid->addWidget(makeFieldLabel(QString::fromUtf8("飞行速度（0 ~ 15 m/s）"), paramGroup), 0, 1);
        paramGrid->addWidget(mHeightSpin, 1, 0);
        paramGrid->addWidget(mSpeedSpin, 1, 1);
        paramGrid->setColumnStretch(0, 1);
        paramGrid->setColumnStretch(1, 1);
        layout->addWidget(paramGroup);

        // --- 起飞前确认（琥珀色警示条） ---
        auto* confirmFrame = new QFrame(this);
        confirmFrame->setStyleSheet(
            "QFrame { background: #fef7e0; border: 1px solid #f9ab00; border-radius: 6px; }");
        auto* confirmLayout = new QVBoxLayout(confirmFrame);
        confirmLayout->setContentsMargins(10, 8, 10, 8);
        mConfirmCheck = new QCheckBox(
            QString::fromUtf8("⚠️ 我确认当前环境适合起飞，已完成飞行前检查"), confirmFrame);
        mConfirmCheck->setStyleSheet(
            "QCheckBox { font-weight: bold; color: #b06000; background: transparent; border: none; }");
        confirmLayout->addWidget(mConfirmCheck);
        layout->addWidget(confirmFrame);

        // --- 确定/取消按钮 ---
        auto* btnRow = new QHBoxLayout;
        btnRow->setSpacing(8);
        btnRow->addStretch();

        auto* cancelBtn = new QPushButton(QString::fromUtf8("取消"), this);
        cancelBtn->setStyleSheet(
            "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
            "padding: 8px 20px; background: #fff; color: #5f6368; }"
            "QPushButton:hover { background: #f1f3f4; }");
        cancelBtn->setCursor(Qt::PointingHandCursor);

        mOkBtn = new QPushButton(QString::fromUtf8("确定起飞"), this);
        mOkBtn->setStyleSheet(
            "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
            "border: none; border-radius: 4px; padding: 8px 24px; }"
            "QPushButton:hover { background: #1557b0; }"
            "QPushButton:disabled { background: #dadce0; color: #80868b; }");
        mOkBtn->setEnabled(false);
        mOkBtn->setCursor(Qt::PointingHandCursor);

        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(mOkBtn);
        layout->addLayout(btnRow);

        connect(mConfirmCheck, &QCheckBox::toggled, mOkBtn, &QPushButton::setEnabled);
        connect(mOkBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }

    TakeoffParams params() const {
        TakeoffParams p;
        p.latitude  = mLatSpin->value();
        p.longitude = mLonSpin->value();
        p.height    = mHeightSpin->value();
        p.speed     = mSpeedSpin->value();
        return p;
    }

private:
    QDoubleSpinBox* mLatSpin;
    QDoubleSpinBox* mLonSpin;
    QDoubleSpinBox* mHeightSpin;
    QDoubleSpinBox* mSpeedSpin;
    QCheckBox*      mConfirmCheck;
    QPushButton*    mOkBtn;
};

#endif // TAKEOFFDIALOG_H
