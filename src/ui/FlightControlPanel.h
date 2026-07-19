#ifndef FLIGHTCONTROLPANEL_H
#define FLIGHTCONTROLPANEL_H

#include <QWidget>
#include <QComboBox>
#include "DockCommand.h"
#include "DeviceInfo.h"

class QLabel;
class QPushButton;
class QPlainTextEdit;

class FlightControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit FlightControlPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);
    void setAvailableDocks(const QVector<DeviceInfo>& docks, const QString& currentSn,
                           double dockLat, double dockLon);
    QString currentGatewaySn() const { return mGatewaySn; }

public slots:
    void onCommandStateChanged(const DockCommandResult& result);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});

private:
    void setupUi();
    void requestCommand(DockCommandType type, const QJsonObject& data = {});
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);
    void appendHistory(const DockCommandResult& result);

    // --- top row ---
    QComboBox*    mDockCombo = nullptr;
    QLabel*       mOnlineLabel = nullptr;

    // --- status ---
    QLabel*       mStatusLabel = nullptr;

    // --- flight controls ---
    QPushButton*  mTakeoffBtn = nullptr;

    // --- return home controls (后续版本完善) ---
    QPushButton*  mReturnHomeBtn = nullptr;
    QPushButton*  mCancelReturnBtn = nullptr;

    QPlainTextEdit* mHistoryEdit = nullptr;

    // --- data ---
    QVector<DeviceInfo> mAvailableDocks;
    QString mDisplayName;
    QString mGatewaySn;
    double  mDockLat = 0.0;
    double  mDockLon = 0.0;
    bool    mConnected = false;
    bool    mOnline = false;
    bool    mPending = false;
    bool    mUpdatingCombo = false;
};

#endif // FLIGHTCONTROLPANEL_H
