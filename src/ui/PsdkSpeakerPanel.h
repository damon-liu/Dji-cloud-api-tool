#ifndef PSDKSPEAKERPANEL_H
#define PSDKSPEAKERPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QMap>
#include "DockCommand.h"
#include "DeviceInfo.h"

class QLabel;
class QPushButton;
class QPlainTextEdit;
class QSlider;
class QLineEdit;
class QTimer;

// PSDK 喊话器控制面板：内嵌于 PsdkSpeakerDialog，提供完整的喊话器控制 UI
class PsdkSpeakerPanel : public QWidget {
    Q_OBJECT
public:
    explicit PsdkSpeakerPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);
    void setAvailableDocks(const QVector<DeviceInfo>& docks, const QString& currentSn,
                           double dockLat, double dockLon);
    QString currentGatewaySn() const { return mGatewaySn; }

public slots:
    void onCommandStateChanged(const DockCommandResult& result);
    void onSpeakerProgress(const SpeakerProgress& progress);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});

private:
    void setupUi();
    void requestCommand(DockCommandType type, const QJsonObject& data = {});
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);
    void appendHistory(const DockCommandResult& result);
    void renderProgress();
    QString computeMd5(const QString& text) const;

    // --- top row ---
    QComboBox*    mDockCombo = nullptr;
    QLabel*       mOnlineLabel = nullptr;
    QLabel*       mStatusLabel = nullptr;

    // --- PSDK device config ---
    QComboBox*    mPsdkIndexCombo = nullptr;

    // --- speaker control ---
    QSlider*      mVolumeSlider = nullptr;
    QLabel*       mVolumeLabel = nullptr;
    QTimer*       mVolumeDebounceTimer = nullptr;
    QComboBox*    mModeCombo = nullptr;
    QPushButton*  mStopBtn = nullptr;
    QPushButton*  mReplayBtn = nullptr;

    // --- TTS text broadcast ---
    QLineEdit*    mTtsNameEdit = nullptr;
    QPlainTextEdit* mTtsTextEdit = nullptr;
    QLabel*       mTtsMd5Label = nullptr;
    QLabel*       mTtsCharCount = nullptr;
    QPushButton*  mTtsSendBtn = nullptr;

    // --- audio file broadcast ---
    QLineEdit*    mAudioNameEdit = nullptr;
    QLineEdit*    mAudioUrlEdit = nullptr;
    QLineEdit*    mAudioMd5Edit = nullptr;
    QLabel*       mAudioFormatLabel = nullptr;
    QPushButton*  mAudioSendBtn = nullptr;

    // --- progress ---
    QLabel*       mProgressLabel = nullptr;
    QMap<QString, int> mStepProgress;   // stepKey → percent (-1=未开始, 0-99=进行中, 100=已完成)
    QString       mCurrentProgressMethod; // 当前进度对应的 method（区分 TTS/音频）

    // --- history ---
    QPlainTextEdit* mHistoryEdit = nullptr;
    QPushButton*    mToggleHistoryBtn = nullptr;

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

#endif // PSDKSPEAKERPANEL_H
