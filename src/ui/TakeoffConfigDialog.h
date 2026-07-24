#ifndef TAKEOFFCONFIGDIALOG_H
#define TAKEOFFCONFIGDIALOG_H

#include <QDialog>
#include <QJsonObject>

class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

class TakeoffConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit TakeoffConfigDialog(double dockLat, double dockLon, double dockAlt,
                                  const QString& dockLatStr,
                                  const QString& dockLonStr,
                                  const QString& dockAltStr,
                                  QWidget* parent = nullptr);

    QJsonObject takeoffPayload() const;

private:
    void setupUi();
    void updateConfirmButton();
    bool validateInputs();

    double mDockLat;
    double mDockLon;
    double mDockAlt;

    // 原始字符串（保留 OSD 上报的小数位数）
    QString mDockLatStr;
    QString mDockLonStr;
    QString mDockAltStr;

    QDoubleSpinBox* mTargetLat = nullptr;
    QDoubleSpinBox* mTargetLon = nullptr;
    QDoubleSpinBox* mTargetHeight = nullptr;
    QComboBox*      mHeightTypeCombo = nullptr;
    QDoubleSpinBox* mSafeTakeoffHeight = nullptr;
    QDoubleSpinBox* mRthAltitude = nullptr;

    QCheckBox*  mSafetyConfirm = nullptr;
    QPushButton* mConfirmBtn = nullptr;
};

#endif // TAKEOFFCONFIGDIALOG_H
