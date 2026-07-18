#ifndef DOCKCONTROLPANEL_H
#define DOCKCONTROLPANEL_H

#include <QWidget>
#include "DockCommand.h"

class QLabel;
class QPushButton;
class QPlainTextEdit;

class DockControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit DockControlPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);

public slots:
    void onCommandStateChanged(const DockCommandResult& result);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type);

private:
    enum class DebugModeState { Unknown, Disabled, Enabled };

    void setupUi();
    void requestCommand(DockCommandType type);
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);
    void appendHistory(const DockCommandResult& result);

    QLabel* mDeviceLabel = nullptr;
    QLabel* mDebugModeLabel = nullptr;
    QLabel* mStatusLabel = nullptr;
    QPushButton* mDebugOpenBtn = nullptr;
    QPushButton* mDebugCloseBtn = nullptr;
    QPushButton* mDroneOpenBtn = nullptr;
    QPushButton* mDroneCloseBtn = nullptr;
    QPushButton* mCoverOpenBtn = nullptr;
    QPushButton* mCoverCloseBtn = nullptr;
    QPushButton* mChargeOpenBtn = nullptr;
    QPushButton* mChargeCloseBtn = nullptr;
    QPlainTextEdit* mHistoryEdit = nullptr;

    QString mDisplayName;
    QString mGatewaySn;
    bool mConnected = false;
    bool mOnline = false;
    bool mPending = false;
    DebugModeState mDebugModeState = DebugModeState::Unknown;
};

#endif // DOCKCONTROLPANEL_H
