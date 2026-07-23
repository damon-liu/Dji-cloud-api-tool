#ifndef TAKEOFFCONFIGDIALOG_H
#define TAKEOFFCONFIGDIALOG_H

#include <QDialog>
#include <QJsonObject>

class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;

class TakeoffConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit TakeoffConfigDialog(double dockLat, double dockLon,
                                  QWidget* parent = nullptr);

    QJsonObject takeoffPayload() const;

private:
    void setupUi();
    void updateConfirmButton();
    bool validateInputs();

    double mDockLat;
    double mDockLon;

    QLabel*         mDockInfoLabel = nullptr;
    QDoubleSpinBox* mTargetLat = nullptr;
    QDoubleSpinBox* mTargetLon = nullptr;
    QDoubleSpinBox* mTargetHeight = nullptr;
    QDoubleSpinBox* mSafeTakeoffHeight = nullptr;
    QDoubleSpinBox* mRthAltitude = nullptr;

    QCheckBox*  mSafetyConfirm = nullptr;
    QPushButton* mConfirmBtn = nullptr;
};

#endif // TAKEOFFCONFIGDIALOG_H
