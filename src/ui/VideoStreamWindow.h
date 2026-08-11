#ifndef VIDEOSTREAMWINDOW_H
#define VIDEOSTREAMWINDOW_H

#include <QWidget>
#include <QStringList>
#include <QMap>
#include "DeviceInfo.h"  // LiveStatusInfo

class QLabel;
class QPushButton;
class QMenu;
class QLineEdit;

// libVLC 前向声明
struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;

class VideoStreamWindow : public QWidget {
    Q_OBJECT
public:
    // 新构造：由 LiveStatusInfo 驱动
    explicit VideoStreamWindow(const LiveStatusInfo& info,
                               QWidget* parent = nullptr);

    void setStreamUrl(const QString& url);
    void setDeviceName(const QString& name);
    void setDeviceType(DeviceType type);

    // 增量更新：OSD 高频推送时复用窗口，只更新标签不重建 VLC
    void updateLiveStatus(const LiveStatusInfo& info);

    // 自动拉流播放（仅 VLC 播放，不发送推流指令）— 用于已保存推流地址的窗口初始化
    void autoPlay();

    // 手动拉流播放（不发送推流指令）
    void onPullStream();

    // VLC 事件回调
    void onVlcError();
    void onVlcBuffering(float cache);
    void onVlcPlaying();

    ~VideoStreamWindow() override;

signals:
    void startPushRequested(const QString& gatewaySn, const QString& videoId,
                            const QString& url, int urlType, int videoQuality);
    void stopPushRequested(const QString& gatewaySn, const QString& videoId);
    void setQualityRequested(const QString& gatewaySn, const QString& videoId,
                             int quality);
    void lensChangeRequested(const QString& gatewaySn, const QString& videoType);
    void cameraChangeRequested(const QString& gatewaySn, const QString& videoId,
                               int cameraPosition);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStart();
    void onStop();
    void onQualitySelected(const QString& quality, int qualityVal);
    void onLensSelected(const QString& videoType);
    void onCameraSelected(int cameraPosition);

private:
    void setupUi();
    void updateButtonStates();
    void updateQualityButtonText();
    void releaseVlcMedia();
    void startVlcPlayback(const QString& url);  // VLC 拉流播放（底层实现）
    void tryVlcPlayback(const QString& url);     // VLC 拉流播放（含自动重试）
    void scheduleRetry();                        // 延迟重试
    void refreshStatusLabel();  // 根据拉流状态刷新状态标签

    // 当前直播状态
    QString     mDeviceSn;
    QString     mDeviceName;         // "[机场]" / "[飞机]" 设备名
    QString     mVideoId;
    int         mVideoQuality = 0;
    QString     mVideoType;
    int         mLiveStatus   = 0;   // 0=未直播, 1=在直播
    int         mErrorStatus  = 0;
    QString     mCurrentQuality;
    bool        mStreaming    = false;
    bool        mBuffering    = false;

    // 视频区域
    QWidget*    mVideoArea;
    QLabel*     mPlaceholderLabel;

    // 信息标签
    QLabel*     mTitleLabel;         // "设备SN | video_id: xxx"
    QLabel*     mStatusLabel;        // "状态: 在直播 | 清晰度: 超清"

    // URL 输入
    QLineEdit*  mUrlInput;

    // 控制按钮
    QPushButton* mStartBtn;
    QPushButton* mStopBtn;
    QPushButton* mPullBtn;         // 手动拉流（不推流）
    QPushButton* mQualityBtn;
    QMenu*       mQualityMenu;
    QPushButton* mLensBtn;          // 镜头类型切换
    QMenu*       mLensMenu;

    // libVLC
    libvlc_instance_t*      mVlcInstance = nullptr;
    libvlc_media_player_t*  mVlcPlayer   = nullptr;
    libvlc_media_t*         mVlcMedia    = nullptr;

    // 自动重试
    int     mAutoRetryRemaining = 0;   // 剩余重试次数（0=不重试）
    QString mRetryUrl;                 // 重试使用的 URL
};

#endif // VIDEOSTREAMWINDOW_H
