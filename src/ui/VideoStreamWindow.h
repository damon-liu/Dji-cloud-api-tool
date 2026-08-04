#ifndef VIDEOSTREAMWINDOW_H
#define VIDEOSTREAMWINDOW_H

#include <QWidget>
#include <QStringList>
#include <QMap>

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
    // deviceLabel: "飞机" / "机场"
    // defaultSource: 默认视频源 "红外相机" / "机场外视频"
    // switchSources: 可切换的视频源列表
    // switchLabel: 切换按钮文字 "切换镜头" / "切换视频"
    explicit VideoStreamWindow(int index,
                               const QString& deviceLabel,
                               const QString& defaultSource,
                               const QStringList& switchSources,
                               const QString& switchLabel,
                               QWidget* parent = nullptr);

    void setStreamUrl(const QString& url);
    void setStreamUrls(const QMap<QString, QString>& urls);   // 预设所有视频源 URL
    void setVlcInstance(libvlc_instance_t* vlc);              // 注入全局 VLC 实例

    // VLC 事件回调（由静态 C 回调 onVlcEvent 调用）
    void onVlcError();
    void onVlcBuffering(float cache);
    void onVlcPlaying();

    ~VideoStreamWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStart();
    void onStop();
    void onQualitySelected(const QString& quality);
    void onSwitchSource(const QString& source);

private:
    void setupUi();
    void updateTitle();
    void updateButtonStates();
    void updateQualityButtonText();
    void applySourceUrl(const QString& source);   // 切换视频源时更新 URL
    void releaseVlcMedia();                        // 释放当前 VLC media

    int         mIndex;
    QString     mDeviceLabel;
    QString     mCurrentSource;
    QStringList mSwitchSources;
    QString     mSwitchLabel;
    QString     mCurrentQuality;
    bool        mStreaming = false;
    bool        mBuffering = false;

    // 标题栏
    QLabel*  mTitleLabel;

    // 视频区域
    QWidget* mVideoArea;
    QLabel*  mPlaceholderLabel;

    // URL 输入
    QLineEdit* mUrlInput;

    // 控制按钮
    QPushButton* mStartBtn;
    QPushButton* mStopBtn;
    QPushButton* mQualityBtn;
    QMenu*       mQualityMenu;

    // 切换视频源按钮
    QPushButton* mSwitchBtn;
    QMenu*       mSwitchMenu;

    // 视频源 → URL 映射（来自 config.json）
    QMap<QString, QString> mUrlMap;

    // libVLC
    libvlc_instance_t*      mVlcInstance = nullptr;
    libvlc_media_player_t*  mVlcPlayer   = nullptr;
    libvlc_media_t*         mVlcMedia    = nullptr;
};

#endif // VIDEOSTREAMWINDOW_H
