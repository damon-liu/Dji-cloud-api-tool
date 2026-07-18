#ifndef TAKEOFFDIALOG_H
#define TAKEOFFDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
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
        setFixedSize(420, 340);

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(10);

        // 提示标签
        auto* hint = new QLabel(
            QString::fromUtf8("请输入目标位置和飞行参数，目标位置默认为机场当前位置"));
        hint->setWordWrap(true);
        hint->setStyleSheet("color: #5f6368; font-size: 12px; margin-bottom: 4px;");
        layout->addWidget(hint);

        auto* form = new QFormLayout;
        form->setSpacing(8);
        form->setContentsMargins(8, 0, 8, 0);

        mLatSpin = new QDoubleSpinBox;
        mLatSpin->setRange(-90.0, 90.0);
        mLatSpin->setDecimals(6);
        mLatSpin->setValue(defaultLat);
        mLatSpin->setSuffix(QString::fromUtf8("\xc2\xb0"));
        form->addRow(QString::fromUtf8("目标纬度:"), mLatSpin);

        mLonSpin = new QDoubleSpinBox;
        mLonSpin->setRange(-180.0, 180.0);
        mLonSpin->setDecimals(6);
        mLonSpin->setValue(defaultLon);
        mLonSpin->setSuffix(QString::fromUtf8("\xc2\xb0"));
        form->addRow(QString::fromUtf8("目标经度:"), mLonSpin);

        mHeightSpin = new QDoubleSpinBox;
        mHeightSpin->setRange(20.0, 500.0);
        mHeightSpin->setDecimals(1);
        mHeightSpin->setValue(100.0);
        mHeightSpin->setSuffix(QString::fromUtf8(" m"));
        form->addRow(QString::fromUtf8("飞行高度:"), mHeightSpin);

        mSpeedSpin = new QDoubleSpinBox;
        mSpeedSpin->setRange(0.0, 15.0);
        mSpeedSpin->setDecimals(1);
        mSpeedSpin->setValue(0.0);
        mSpeedSpin->setSpecialValueText(QString::fromUtf8("默认"));
        mSpeedSpin->setSuffix(QString::fromUtf8(" m/s"));
        form->addRow(QString::fromUtf8("飞行速度:"), mSpeedSpin);

        layout->addLayout(form);

        // 分隔线
        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setFrameShadow(QFrame::Sunken);
        layout->addWidget(sep);

        // 环境确认
        mConfirmCheck = new QCheckBox(
            QString::fromUtf8("我确认当前环境适合起飞，已完成飞行前检查"));
        mConfirmCheck->setStyleSheet("font-weight: bold; color: #333;");
        layout->addWidget(mConfirmCheck);

        // 确定/取消按钮
        auto* btnRow = new QHBoxLayout;
        btnRow->addStretch();

        mOkBtn = new QPushButton(QString::fromUtf8("确定起飞"));
        mOkBtn->setStyleSheet(
            "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
            "border: none; border-radius: 4px; padding: 8px 24px; }"
            "QPushButton:hover { background: #1557b0; }"
            "QPushButton:disabled { background: #dadce0; color: #80868b; }");
        mOkBtn->setEnabled(false);
        mOkBtn->setCursor(Qt::PointingHandCursor);

        auto* cancelBtn = new QPushButton(QString::fromUtf8("取消"));
        cancelBtn->setStyleSheet(
            "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
            "padding: 8px 20px; background: #fff; color: #5f6368; }"
            "QPushButton:hover { background: #f1f3f4; }");
        cancelBtn->setCursor(Qt::PointingHandCursor);

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
