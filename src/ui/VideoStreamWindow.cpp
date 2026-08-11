#include "VideoStreamWindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#ifdef HAS_VLC
#include <vlc/vlc.h>

// VLC 事件回调（C 回调，需为静态函数）
static void onVlcEvent(const libvlc_event_t* event, void* userData) {
    auto* win = static_cast<VideoStreamWindow*>(userData);
    if (!win) return;

    switch (event->type) {
    case libvlc_MediaPlayerEncounteredError:
        qDebug() << "[VLC event] MediaPlayerEncounteredError";
        QMetaObject::invokeMethod(win, [win]() {
            win->onVlcError();
        }, Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerBuffering:
        qDebug() << "[VLC event] MediaPlayerBuffering cache =" << event->u.media_player_buffering.new_cache;
        QMetaObject::invokeMethod(win, [win, caching = event->u.media_player_buffering.new_cache]() {
            win->onVlcBuffering(caching);
        }, Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerPlaying:
        qDebug() << "[VLC event] MediaPlayerPlaying";
        QMetaObject::invokeMethod(win, [win]() {
            win->onVlcPlaying();
        }, Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerOpening:
        qDebug() << "[VLC event] MediaPlayerOpening";
        break;
    case libvlc_MediaPlayerStopped:
        qDebug() << "[VLC event] MediaPlayerStopped";
        break;
    case libvlc_MediaPlayerEndReached:
        qDebug() << "[VLC event] MediaPlayerEndReached";
        break;
    default:
        qDebug() << "[VLC event] type =" << event->type;
        break;
    }
}
#endif

VideoStreamWindow::VideoStreamWindow(const LiveStatusInfo& info, QWidget* parent)
    : QWidget(parent)
    , mDeviceSn(info.deviceSn)
    , mVideoId(info.videoId)
    , mVideoQuality(info.videoQuality)
    , mVideoType(info.videoType)
    , mLiveStatus(info.status)
    , mErrorStatus(info.errorStatus)
    , mCurrentQuality(QString::fromUtf8("自适应"))
{
    setupUi();
    updateLiveStatus(info);
    updateButtonStates();
}

VideoStreamWindow::~VideoStreamWindow() {
#ifdef HAS_VLC
    if (mVlcPlayer) {
        libvlc_media_player_stop(mVlcPlayer);
    }
    releaseVlcMedia();
    if (mVlcPlayer) {
        libvlc_media_player_release(mVlcPlayer);
        mVlcPlayer = nullptr;
    }
    if (mVlcInstance) {
        libvlc_release(mVlcInstance);
        mVlcInstance = nullptr;
    }
#endif
}

void VideoStreamWindow::setupUi() {
    setMinimumSize(320, 200);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- 信息栏 ---
    auto* infoBar = new QWidget(this);
    infoBar->setStyleSheet("background: #1e1e1e;");
    infoBar->setFixedHeight(36);
    auto* infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(8, 2, 8, 2);
    infoLayout->setSpacing(0);

    mTitleLabel = new QLabel(infoBar);
    mTitleLabel->setStyleSheet("color: #ccc; font-size: 12px; font-weight: bold;");
    infoLayout->addWidget(mTitleLabel);
    infoLayout->addStretch();

    mStatusLabel = new QLabel(infoBar);
    mStatusLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    infoLayout->addWidget(mStatusLabel);

    layout->addWidget(infoBar);

    // --- 视频区域 ---
    mVideoArea = new QWidget(this);
    mVideoArea->setStyleSheet("background: #000;");
    layout->addWidget(mVideoArea, 1);

    mPlaceholderLabel = new QLabel(
        QString::fromUtf8("等待视频流..."), mVideoArea);
    mPlaceholderLabel->setStyleSheet(
        "color: #444; font-size: 16px; background: transparent;");
    mPlaceholderLabel->setAlignment(Qt::AlignCenter);

    auto* areaLayout = new QVBoxLayout(mVideoArea);
    areaLayout->addWidget(mPlaceholderLabel);

    // --- 控制栏 ---
    auto* controlBar = new QWidget(this);
    controlBar->setStyleSheet("background: #2a2a2a;");
    controlBar->setFixedHeight(44);
    auto* controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(8, 6, 8, 6);
    controlLayout->setSpacing(6);

    // URL 输入框（由程序拼接，支持用户手动修改）
    mUrlInput = new QLineEdit(controlBar);
    mUrlInput->setPlaceholderText(QString::fromUtf8("请先在配置中心设置流媒体服务器"));
    mUrlInput->setStyleSheet(
        "QLineEdit { background: #1e1e1e; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 8px; font-size: 12px; }"
        "QLineEdit:focus { border-color: #1a73e8; }");
    mUrlInput->setMinimumWidth(160);
    controlLayout->addWidget(mUrlInput, 1);

    // 开始推流按钮
    mStartBtn = new QPushButton(QString::fromUtf8("▶ 开始推流"), controlBar);
    mStartBtn->setCursor(Qt::PointingHandCursor);
    mStartBtn->setFocusPolicy(Qt::NoFocus);
    mStartBtn->setFixedHeight(30);
    mStartBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #4a4a4a; color: #888; }");
    connect(mStartBtn, &QPushButton::clicked, this, &VideoStreamWindow::onStart);
    controlLayout->addWidget(mStartBtn);

    // 停止推流按钮
    mStopBtn = new QPushButton(QString::fromUtf8("■ 停止推流"), controlBar);
    mStopBtn->setCursor(Qt::PointingHandCursor);
    mStopBtn->setFocusPolicy(Qt::NoFocus);
    mStopBtn->setFixedHeight(30);
    mStopBtn->setStyleSheet(
        "QPushButton { background: #d93025; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background: #b3261e; }"
        "QPushButton:disabled { background: #4a4a4a; color: #888; }");
    connect(mStopBtn, &QPushButton::clicked, this, &VideoStreamWindow::onStop);
    controlLayout->addWidget(mStopBtn);

    // 手动拉流按钮（仅 VLC 拉流，不发送推流指令）
    mPullBtn = new QPushButton(QString::fromUtf8("🔗 拉流"), controlBar);
    mPullBtn->setCursor(Qt::PointingHandCursor);
    mPullBtn->setFocusPolicy(Qt::NoFocus);
    mPullBtn->setFixedHeight(30);
    mPullBtn->setStyleSheet(
        "QPushButton { background: #2e7d32; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background: #1b5e20; }"
        "QPushButton:disabled { background: #4a4a4a; color: #888; }");
    mPullBtn->setToolTip(QString::fromUtf8("仅拉流播放，不发送推流指令"));
    connect(mPullBtn, &QPushButton::clicked, this, &VideoStreamWindow::onPullStream);
    controlLayout->addWidget(mPullBtn);

    // 清晰度按钮
    mQualityBtn = new QPushButton(controlBar);
    mQualityBtn->setCursor(Qt::PointingHandCursor);
    mQualityBtn->setFocusPolicy(Qt::NoFocus);
    mQualityBtn->setFixedHeight(30);
    mQualityBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 10px; font-size: 12px; }"
        "QPushButton:hover { background: #4a4a4a; border-color: #888; }"
        "QPushButton::menu-indicator { image: none; }");
    updateQualityButtonText();

    mQualityMenu = new QMenu(this);
    mQualityMenu->setStyleSheet(
        "QMenu { background: #2a2a2a; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 0; }"
        "QMenu::item { color: #ccc; padding: 6px 28px 6px 16px; font-size: 12px; }"
        "QMenu::item:selected { background: #1a73e8; color: #fff; }");

    const char* qualities[]  = {"自适应", "流畅", "标清", "高清", "超清"};
    const int   qualityVals[] = {0, 1, 2, 3, 4};
    for (int i = 0; i < 5; ++i) {
        QString qStr = QString::fromUtf8(qualities[i]);
        int qVal = qualityVals[i];
        mQualityMenu->addAction(qStr, this, [this, qStr, qVal]() {
            onQualitySelected(qStr, qVal);
        });
    }
    mQualityBtn->setMenu(mQualityMenu);
    controlLayout->addWidget(mQualityBtn);

    // 镜头类型按钮
    mLensBtn = new QPushButton(QString::fromUtf8("🔍 镜头 ▾"), controlBar);
    mLensBtn->setCursor(Qt::PointingHandCursor);
    mLensBtn->setFocusPolicy(Qt::NoFocus);
    mLensBtn->setFixedHeight(30);
    mLensBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 10px; font-size: 12px; }"
        "QPushButton:hover { background: #4a4a4a; border-color: #888; }"
        "QPushButton::menu-indicator { image: none; }");

    mLensMenu = new QMenu(this);
    mLensMenu->setStyleSheet(
        "QMenu { background: #2a2a2a; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 0; }"
        "QMenu::item { color: #ccc; padding: 6px 28px 6px 16px; font-size: 12px; }"
        "QMenu::item:selected { background: #1a73e8; color: #fff; }");

    const char* lensTypes[]  = {"默认", "红外", "变焦", "广角"};
    const char* lensValues[] = {"normal", "ir", "zoom", "wide"};
    for (int i = 0; i < 4; ++i) {
        QString label = QString::fromUtf8(lensTypes[i]);
        QString value = QString::fromUtf8(lensValues[i]);
        mLensMenu->addAction(label, this, [this, value]() {
            onLensSelected(value);
        });
    }
    mLensBtn->setMenu(mLensMenu);
    controlLayout->addWidget(mLensBtn);

    controlLayout->addStretch();
    layout->addWidget(controlBar);
}

void VideoStreamWindow::updateLiveStatus(const LiveStatusInfo& info) {
    mDeviceSn     = info.deviceSn;
    mVideoId      = info.videoId;
    mVideoQuality = info.videoQuality;
    mVideoType    = info.videoType;
    mLiveStatus   = info.status;
    mErrorStatus  = info.errorStatus;

    // 更新标题 — "[机场/飞机]  |  video_id: xxx"
    QString prefix = mDeviceName.isEmpty()
        ? QString()
        : QString("[%1] ").arg(mDeviceName);
    mTitleLabel->setText(QString("%1 |  video_id: %2")
        .arg(prefix, mVideoId));

    refreshStatusLabel();
}

void VideoStreamWindow::refreshStatusLabel() {
    QString statusText;
    if (mStreaming) {
        statusText = QString::fromUtf8("● 在直播");
        mStatusLabel->setStyleSheet("color: #2e7d32; font-size: 11px;");
    } else {
        statusText = QString::fromUtf8("○ 未直播");
        mStatusLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    }

    // 清晰度文本
    const char* qualNames[] = {"自适应", "流畅", "标清", "高清", "超清"};
    if (mVideoQuality >= 0 && mVideoQuality <= 4)
        statusText += QString::fromUtf8("  |  清晰度: %1")
            .arg(QString::fromUtf8(qualNames[mVideoQuality]));

    mStatusLabel->setText(statusText);
}

void VideoStreamWindow::releaseVlcMedia() {
#ifdef HAS_VLC
    if (mVlcMedia) {
        libvlc_media_release(mVlcMedia);
        mVlcMedia = nullptr;
    }
#endif
}

void VideoStreamWindow::updateQualityButtonText() {
    mQualityBtn->setText(
        QString::fromUtf8("🎛 %1 ▾").arg(mCurrentQuality));
}

void VideoStreamWindow::updateButtonStates() {
    bool hasUrl = !mUrlInput->text().trimmed().isEmpty()
        && !mUrlInput->text().contains(QString::fromUtf8("请先在配置中心设置"));
    mStartBtn->setEnabled(!mStreaming && hasUrl);
    mStopBtn->setEnabled(mStreaming);
    mPullBtn->setEnabled(hasUrl);
}

void VideoStreamWindow::onStart() {
    // 检查流媒体服务器是否已配置
    QString url = mUrlInput->text().trimmed();
    if (url.isEmpty()
        || url.contains(QString::fromUtf8("请先在配置中心设置"))) {
        QMessageBox::warning(this,
            QString::fromUtf8("提示"),
            QString::fromUtf8("请先在配置中心设置流媒体服务器"));
        return;
    }

    // 1) 发射信号 → MainWindow → DeviceManager 下发 live_start_push
    emit startPushRequested(mDeviceSn, mVideoId, url, 1, mVideoQuality);

    // 2) VLC 拉流播放（含自动重试）
    tryVlcPlayback(url);
}

void VideoStreamWindow::autoPlay() {
    // 仅 VLC 拉流播放，不发送推流指令
    // 适用场景：设备此前已收到 live_start_push 且持续推流，重启程序后只需重新拉流
    QString url = mUrlInput->text().trimmed();
    qDebug() << "VideoStreamWindow::autoPlay()" << mDeviceSn << mVideoId << "url:" << url;
    if (url.isEmpty()) {
        qDebug() << "  -> skipped: url is empty";
        return;
    }
    tryVlcPlayback(url);
}

void VideoStreamWindow::onPullStream() {
    QString url = mUrlInput->text().trimmed();
    if (url.isEmpty()
        || url.contains(QString::fromUtf8("请先在配置中心设置"))) {
        QMessageBox::warning(this,
            QString::fromUtf8("提示"),
            QString::fromUtf8("请先在配置中心设置流媒体服务器"));
        return;
    }
    qDebug() << "VideoStreamWindow::onPullStream()" << mDeviceSn << mVideoId << "url:" << url;
    tryVlcPlayback(url);
}

void VideoStreamWindow::tryVlcPlayback(const QString& url) {
    mAutoRetryRemaining = 2;  // 最多重试 2 次（共 3 次尝试）
    mRetryUrl = url;
    startVlcPlayback(url);
}

void VideoStreamWindow::scheduleRetry() {
    if (mAutoRetryRemaining <= 0)
        return;

    mAutoRetryRemaining--;
    qDebug() << "VideoStreamWindow::scheduleRetry()" << mDeviceSn << mVideoId
             << "remaining:" << mAutoRetryRemaining;

    mPlaceholderLabel->setText(
        QString::fromUtf8("⏳ 连接失败，%1 秒后重试（剩余 %2 次）...")
            .arg(2).arg(mAutoRetryRemaining + 1));
    mPlaceholderLabel->setStyleSheet(
        "color: #f9ab00; font-size: 14px; background: transparent;");
    mPlaceholderLabel->show();

    // 2 秒后重试
    QTimer::singleShot(2000, this, [this]() {
        if (mRetryUrl.isEmpty()) return;
        qDebug() << "VideoStreamWindow: retrying playback..."
                 << mDeviceSn << mVideoId;
        startVlcPlayback(mRetryUrl);
    });
}

void VideoStreamWindow::startVlcPlayback(const QString& url) {
    qDebug() << "VideoStreamWindow::startVlcPlayback()" << mDeviceSn << mVideoId << "url:" << url;
#ifdef HAS_VLC
    // 懒加载：每个窗口创建自己独立的 VLC 实例，避免单实例多播放器的 RTMP 资源竞争
    if (!mVlcInstance) {
        const char* const vlc_args[] = {
            "--verbose=2",           // 诊断日志：输出 VLC 内部消息
            "--no-video-title-show",
        };
        mVlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
        if (!mVlcInstance) {
            qWarning() << "VideoStreamWindow: libvlc_new failed for" << mDeviceSn << mVideoId;
            if (mAutoRetryRemaining > 0) {
                scheduleRetry();
                return;
            }
            return;
        }
        qDebug() << "  -> VLC instance created for" << mDeviceSn << mVideoId;
    }

    qDebug() << "  -> creating VLC media...";
    // 如果已有播放器，先停再换媒体
    if (mVlcPlayer) {
        libvlc_media_player_stop(mVlcPlayer);
    }
    releaseVlcMedia();

    mVlcMedia = libvlc_media_new_location(mVlcInstance, url.toUtf8().constData());
    if (!mVlcMedia) {
        qDebug() << "  -> FAILED: libvlc_media_new_location returned null";
        if (mAutoRetryRemaining > 0) {
            scheduleRetry();
            return;
        }
        QMessageBox::warning(this,
            QString::fromUtf8("播放失败"),
            QString::fromUtf8("无法创建媒体源，请检查地址是否正确"));
        return;
    }

    qDebug() << "  -> setting VLC options...";
    // RTMP 直播流网络缓冲参数
    libvlc_media_add_option(mVlcMedia, ":network-caching=3000");
    libvlc_media_add_option(mVlcMedia, ":live-caching=3000");
    libvlc_media_add_option(mVlcMedia, ":rtmp-timeout=15");
    libvlc_media_add_option(mVlcMedia, ":no-audio");
    libvlc_media_add_option(mVlcMedia, ":avcodec-hw=none");  // 禁用硬件加速，避免多实例 GPU 资源竞争

    // 首次创建播放器：绑定窗口 + 注册事件
    if (!mVlcPlayer) {
        qDebug() << "  -> creating new VLC player...";
        mVlcPlayer = libvlc_media_player_new_from_media(mVlcMedia);
        libvlc_media_player_set_hwnd(mVlcPlayer, (void*)mVideoArea->winId());

        libvlc_event_manager_t* em = libvlc_media_player_event_manager(mVlcPlayer);
        libvlc_event_attach(em, libvlc_MediaPlayerEncounteredError, onVlcEvent, this);
        libvlc_event_attach(em, libvlc_MediaPlayerBuffering, onVlcEvent, this);
        libvlc_event_attach(em, libvlc_MediaPlayerPlaying, onVlcEvent, this);
    } else {
        qDebug() << "  -> reusing VLC player, setting new media...";
        // 复用播放器，仅替换媒体
        libvlc_media_player_set_media(mVlcPlayer, mVlcMedia);
    }

    qDebug() << "  -> calling libvlc_media_player_play...";
    int ret = libvlc_media_player_play(mVlcPlayer);
    if (ret != 0) {
        qDebug() << "  -> FAILED: libvlc_media_player_play returned" << ret;
        if (mAutoRetryRemaining > 0) {
            releaseVlcMedia();
            scheduleRetry();
            return;
        }
        QMessageBox::warning(this,
            QString::fromUtf8("播放失败"),
            QString::fromUtf8("无法开始播放 (错误码: %1)").arg(ret));
        releaseVlcMedia();
        return;
    }
    qDebug() << "  -> VLC playback started successfully";
#else
    qDebug() << "  -> HAS_VLC not defined, skipping";
#endif

    mStreaming = true;
    mBuffering = false;
    mPlaceholderLabel->setText(
        QString::fromUtf8("⏳ 正在连接视频流..."));
    mPlaceholderLabel->setStyleSheet(
        "color: #f9ab00; font-size: 14px; background: transparent;");
    mPlaceholderLabel->show();
    updateButtonStates();
}

void VideoStreamWindow::onStop() {
#ifdef HAS_VLC
    if (mVlcPlayer) {
        libvlc_media_player_stop(mVlcPlayer);
    }
    releaseVlcMedia();
#endif

    mAutoRetryRemaining = 0;  // 用户主动停止，取消重试
    mStreaming = false;
    mBuffering = false;
    refreshStatusLabel();
    mPlaceholderLabel->setText(QString::fromUtf8("等待视频流..."));
    mPlaceholderLabel->setStyleSheet(
        "color: #444; font-size: 16px; background: transparent;");
    mPlaceholderLabel->show();
    updateButtonStates();

    // 通知 MainWindow 下发 live_stop_push
    emit stopPushRequested(mDeviceSn, mVideoId);
}

void VideoStreamWindow::onQualitySelected(const QString& quality, int qualityVal) {
    mCurrentQuality = quality;
    updateQualityButtonText();
    // 下发清晰度切换指令
    emit setQualityRequested(mDeviceSn, mVideoId, qualityVal);
}

void VideoStreamWindow::onLensSelected(const QString& videoType) {
    emit lensChangeRequested(mDeviceSn, videoType);
}

void VideoStreamWindow::onCameraSelected(int cameraPosition) {
    emit cameraChangeRequested(mDeviceSn, mVideoId, cameraPosition);
}

// ——— VLC 事件回调 ———

void VideoStreamWindow::onVlcError() {
    qDebug() << "[VLC cb]" << mDeviceSn << mVideoId << "onVlcError"
             << "retryRemaining:" << mAutoRetryRemaining;

    mStreaming = false;
    mBuffering = false;

    if (mAutoRetryRemaining > 0) {
        // 还有重试次数，延迟重试
        scheduleRetry();
        return;
    }

    // 重试次数已用完，显示错误
    mPlaceholderLabel->setText(
        QString::fromUtf8("⚠ 播放出错 — 请检查流地址或网络连接"));
    mPlaceholderLabel->setStyleSheet(
        "color: #d93025; font-size: 14px; background: transparent;");
    mPlaceholderLabel->show();
    refreshStatusLabel();
    updateButtonStates();
}

void VideoStreamWindow::onVlcBuffering(float cache) {
    qDebug() << "[VLC cb]" << mDeviceSn << mVideoId << "onVlcBuffering cache =" << cache;
    mBuffering = (cache < 100.0f);
    if (mBuffering && mStreaming) {
        mPlaceholderLabel->setText(
            QString::fromUtf8("⏳ 缓冲中 %1%...").arg(static_cast<int>(cache)));
        mPlaceholderLabel->setStyleSheet(
            "color: #f9ab00; font-size: 14px; background: transparent;");
        mPlaceholderLabel->show();
    }
}

void VideoStreamWindow::onVlcPlaying() {
    qDebug() << "[VLC cb]" << mDeviceSn << mVideoId << "onVlcPlaying — hiding placeholder";
    mAutoRetryRemaining = 0;  // 播放成功，清零重试计数
    mBuffering = false;
    refreshStatusLabel();
    mPlaceholderLabel->hide();
}

void VideoStreamWindow::setStreamUrl(const QString& url) {
    mUrlInput->setText(url);
    updateButtonStates();
}

void VideoStreamWindow::setDeviceName(const QString& name) {
    mDeviceName = name;
    // 立即刷新标题
    QString prefix = mDeviceName.isEmpty()
        ? QString()
        : QString("[%1] ").arg(mDeviceName);
    mTitleLabel->setText(QString("%1 |  video_id: %2")
        .arg(prefix, mVideoId));
}

void VideoStreamWindow::setDeviceType(DeviceType type) {
    // 镜头切换按钮仅飞机显示
    bool isAircraft = (type == DeviceType::Aircraft);
    mLensBtn->setVisible(isAircraft);
}

void VideoStreamWindow::closeEvent(QCloseEvent* event) {
    event->accept();
}
