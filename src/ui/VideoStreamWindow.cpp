#include "VideoStreamWindow.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#ifdef HAS_VLC
#include <vlc/vlc.h>

// VLC 事件回调（C 回调，需为静态函数）
static void onVlcEvent(const libvlc_event_t* event, void* userData) {
    auto* win = static_cast<VideoStreamWindow*>(userData);
    if (!win) return;

    switch (event->type) {
    case libvlc_MediaPlayerEncounteredError:
        QMetaObject::invokeMethod(win, [win]() {
            win->onVlcError();
        }, Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerBuffering:
        QMetaObject::invokeMethod(win, [win, caching = event->u.media_player_buffering.new_cache]() {
            win->onVlcBuffering(caching);
        }, Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerPlaying:
        QMetaObject::invokeMethod(win, [win]() {
            win->onVlcPlaying();
        }, Qt::QueuedConnection);
        break;
    default:
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

    // URL 输入框（只读，由程序拼接）
    mUrlInput = new QLineEdit(controlBar);
    mUrlInput->setPlaceholderText(QString::fromUtf8("请先在配置中心设置流媒体服务器"));
    mUrlInput->setStyleSheet(
        "QLineEdit { background: #1e1e1e; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 8px; font-size: 12px; }"
        "QLineEdit:focus { border-color: #1a73e8; }");
    mUrlInput->setReadOnly(true);
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

    // 相机切换按钮
    mCameraBtn = new QPushButton(QString::fromUtf8("📷 相机 ▾"), controlBar);
    mCameraBtn->setCursor(Qt::PointingHandCursor);
    mCameraBtn->setFocusPolicy(Qt::NoFocus);
    mCameraBtn->setFixedHeight(30);
    mCameraBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 10px; font-size: 12px; }"
        "QPushButton:hover { background: #4a4a4a; border-color: #888; }"
        "QPushButton::menu-indicator { image: none; }");

    mCameraMenu = new QMenu(this);
    mCameraMenu->setStyleSheet(
        "QMenu { background: #2a2a2a; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 0; }"
        "QMenu::item { color: #ccc; padding: 6px 28px 6px 16px; font-size: 12px; }"
        "QMenu::item:selected { background: #1a73e8; color: #fff; }");

    mCameraMenu->addAction(QString::fromUtf8("舱内 (FPV)"), this, [this]() {
        onCameraSelected(0);
    });
    mCameraMenu->addAction(QString::fromUtf8("舱外"), this, [this]() {
        onCameraSelected(1);
    });
    mCameraBtn->setMenu(mCameraMenu);
    controlLayout->addWidget(mCameraBtn);

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

    // 更新标题 — "设备SN | video_id: xxx"
    mTitleLabel->setText(QString("%1  |  video_id: %2")
        .arg(mDeviceSn, mVideoId));

    // 更新状态标签
    QString statusText;
    if (mErrorStatus != 0) {
        statusText = QString::fromUtf8("⚠ 错误: %1").arg(mErrorStatus);
        mStatusLabel->setStyleSheet("color: #d93025; font-size: 11px;");
    } else if (mLiveStatus == 1) {
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
    mStartBtn->setEnabled(!mStreaming);
    mStopBtn->setEnabled(mStreaming);
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

    // 发射信号 → MainWindow → DeviceManager 下发 live_start_push
    emit startPushRequested(mDeviceSn, mVideoId, url, 1, mVideoQuality);
}

void VideoStreamWindow::onStop() {
#ifdef HAS_VLC
    if (mVlcPlayer) {
        libvlc_media_player_stop(mVlcPlayer);
    }
    releaseVlcMedia();
#endif

    mStreaming = false;
    mBuffering = false;
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
    mPlaceholderLabel->setText(
        QString::fromUtf8("⚠ 播放出错 — 请检查流地址或网络连接"));
    mPlaceholderLabel->setStyleSheet(
        "color: #d93025; font-size: 14px; background: transparent;");
    mPlaceholderLabel->show();
    mStreaming = false;
    mBuffering = false;
    updateButtonStates();
}

void VideoStreamWindow::onVlcBuffering(float cache) {
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
    mBuffering = false;
    mPlaceholderLabel->hide();
}

void VideoStreamWindow::setStreamUrl(const QString& url) {
    mUrlInput->setText(url);
}

void VideoStreamWindow::setVlcInstance(libvlc_instance_t* vlc) {
#ifdef HAS_VLC
    mVlcInstance = vlc;
#else
    Q_UNUSED(vlc);
#endif
}

void VideoStreamWindow::closeEvent(QCloseEvent* event) {
    event->accept();
}
