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
        // VLC 异步播放错误 — 通过 QMetaObject::invokeMethod 切回主线程通知
        QMetaObject::invokeMethod(win, [win]() {
            win->onVlcError();
        }, Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerBuffering:
        // 缓冲状态 — 可用于显示"加载中"提示
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

VideoStreamWindow::VideoStreamWindow(int index,
                                     const QString& deviceLabel,
                                     const QString& defaultSource,
                                     const QStringList& switchSources,
                                     const QString& switchLabel,
                                     QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint)
    , mIndex(index)
    , mDeviceLabel(deviceLabel)
    , mCurrentSource(defaultSource)
    , mSwitchSources(switchSources)
    , mSwitchLabel(switchLabel)
    , mCurrentQuality(QString::fromUtf8("自适应"))
{
    setupUi();
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
    setMinimumSize(640, 400);
    resize(640, 400);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- 标题栏 ---
    mTitleLabel = new QLabel(this);
    mTitleLabel->setStyleSheet(
        "background: #1e1e1e; color: #ccc; font-size: 12px;"
        "padding: 6px 10px; font-weight: bold;");
    mTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    updateTitle();
    layout->addWidget(mTitleLabel);

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
    controlLayout->setSpacing(8);

    // URL 输入框
    mUrlInput = new QLineEdit(controlBar);
    mUrlInput->setPlaceholderText(QString::fromUtf8("输入视频流地址，如 rtmp://..."));
    mUrlInput->setStyleSheet(
        "QLineEdit { background: #1e1e1e; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 8px; font-size: 12px; }"
        "QLineEdit:focus { border-color: #1a73e8; }");
    mUrlInput->setMinimumWidth(200);
    controlLayout->addWidget(mUrlInput, 1);

    // 开始按钮
    mStartBtn = new QPushButton(
        QString::fromUtf8("▶ 开始"), controlBar);
    mStartBtn->setCursor(Qt::PointingHandCursor);
    mStartBtn->setFocusPolicy(Qt::NoFocus);
    mStartBtn->setFixedHeight(30);
    mStartBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #4a4a4a; color: #888; }");
    connect(mStartBtn, &QPushButton::clicked, this, &VideoStreamWindow::onStart);
    controlLayout->addWidget(mStartBtn);

    // 结束按钮
    mStopBtn = new QPushButton(
        QString::fromUtf8("■ 结束"), controlBar);
    mStopBtn->setCursor(Qt::PointingHandCursor);
    mStopBtn->setFocusPolicy(Qt::NoFocus);
    mStopBtn->setFixedHeight(30);
    mStopBtn->setStyleSheet(
        "QPushButton { background: #d93025; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
        "QPushButton:hover { background: #b3261e; }"
        "QPushButton:disabled { background: #4a4a4a; color: #888; }");
    connect(mStopBtn, &QPushButton::clicked, this, &VideoStreamWindow::onStop);
    controlLayout->addWidget(mStopBtn);

    // 清晰度按钮（RTMP 流清晰度由推流端控制，按钮置灰）
    mQualityBtn = new QPushButton(controlBar);
    mQualityBtn->setCursor(Qt::PointingHandCursor);
    mQualityBtn->setFocusPolicy(Qt::NoFocus);
    mQualityBtn->setFixedHeight(30);
    mQualityBtn->setEnabled(false);
    mQualityBtn->setToolTip(QString::fromUtf8("清晰度由推流端控制"));
    mQualityBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #666;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 12px; font-size: 12px; }");
    updateQualityButtonText();

    mQualityMenu = new QMenu(this);
    mQualityMenu->setStyleSheet(
        "QMenu { background: #2a2a2a; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 0; }"
        "QMenu::item { color: #666; padding: 6px 28px 6px 16px; font-size: 12px; }"
        "QMenu::item:selected { background: #1a73e8; color: #fff; }");

    const char* qualities[] = {"自适应", "流畅", "标清", "高清", "超清"};
    for (const auto& q : qualities) {
        QString qStr = QString::fromUtf8(q);
        mQualityMenu->addAction(qStr, this, [this, qStr]() {
            onQualitySelected(qStr);
        });
    }
    mQualityBtn->setMenu(mQualityMenu);
    controlLayout->addWidget(mQualityBtn);

    // 切换视频源按钮
    mSwitchBtn = new QPushButton(controlBar);
    mSwitchBtn->setCursor(Qt::PointingHandCursor);
    mSwitchBtn->setFocusPolicy(Qt::NoFocus);
    mSwitchBtn->setFixedHeight(30);
    mSwitchBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background: #4a4a4a; border-color: #888; }"
        "QPushButton::menu-indicator { image: none; }");
    mSwitchBtn->setText(
        QString::fromUtf8("📷 %1 ▾").arg(mSwitchLabel));

    mSwitchMenu = new QMenu(this);
    mSwitchMenu->setStyleSheet(
        "QMenu { background: #2a2a2a; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 0; }"
        "QMenu::item { color: #ccc; padding: 6px 28px 6px 16px;"
        "font-size: 12px; }"
        "QMenu::item:selected { background: #1a73e8; color: #fff; }");

    for (const auto& src : mSwitchSources) {
        mSwitchMenu->addAction(src, this, [this, src]() {
            onSwitchSource(src);
        });
    }
    mSwitchBtn->setMenu(mSwitchMenu);
    controlLayout->addWidget(mSwitchBtn);

    controlLayout->addStretch();
    layout->addWidget(controlBar);
}

void VideoStreamWindow::applySourceUrl(const QString& source) {
    // 从 mUrlMap 查找该视频源的预设 URL
    if (mUrlMap.contains(source) && !mUrlMap.value(source).isEmpty()) {
        mUrlInput->setText(mUrlMap.value(source));
    }
}

void VideoStreamWindow::releaseVlcMedia() {
#ifdef HAS_VLC
    if (mVlcMedia) {
        libvlc_media_release(mVlcMedia);
        mVlcMedia = nullptr;
    }
#endif
}

void VideoStreamWindow::updateTitle() {
    setWindowTitle(QString::fromUtf8("直播 - %1").arg(mDeviceLabel));
    mTitleLabel->setText(
        QString::fromUtf8("📷 %1 · %2").arg(mDeviceLabel, mCurrentSource));
}

void VideoStreamWindow::updateQualityButtonText() {
    mQualityBtn->setText(
        QString::fromUtf8("🎛 %1").arg(mCurrentQuality));
}

void VideoStreamWindow::updateButtonStates() {
    mStartBtn->setEnabled(!mStreaming);
    mStopBtn->setEnabled(mStreaming);
    if (mSwitchBtn)
        mSwitchBtn->setEnabled(true);
}

void VideoStreamWindow::onStart() {
    QString url = mUrlInput->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this,
            QString::fromUtf8("提示"),
            QString::fromUtf8("请先输入视频流地址"));
        return;
    }

#ifdef HAS_VLC
    if (!mVlcInstance) {
        QMessageBox::warning(this,
            QString::fromUtf8("提示"),
            QString::fromUtf8("VLC 未初始化"));
        return;
    }

    // 如果已有播放器，先停
    if (mVlcPlayer) {
        libvlc_media_player_stop(mVlcPlayer);
    }
    releaseVlcMedia();

    mVlcMedia = libvlc_media_new_location(mVlcInstance, url.toUtf8().constData());
    if (!mVlcMedia) {
        QMessageBox::warning(this,
            QString::fromUtf8("播放失败"),
            QString::fromUtf8("无法创建媒体源，请检查地址是否正确"));
        return;
    }

    // RTMP 直播流需要更大的网络缓冲和直播缓存
    libvlc_media_add_option(mVlcMedia, ":network-caching=3000");
    libvlc_media_add_option(mVlcMedia, ":live-caching=3000");
    libvlc_media_add_option(mVlcMedia, ":rtmp-timeout=15");
    libvlc_media_add_option(mVlcMedia, ":no-audio");

    if (!mVlcPlayer) {
        mVlcPlayer = libvlc_media_player_new_from_media(mVlcMedia);
        libvlc_media_player_set_hwnd(mVlcPlayer, (void*)mVideoArea->winId());

        // 注册 VLC 事件回调 — 用于错误提示和缓冲状态反馈
        libvlc_event_manager_t* em = libvlc_media_player_event_manager(mVlcPlayer);
        libvlc_event_attach(em, libvlc_MediaPlayerEncounteredError, onVlcEvent, this);
        libvlc_event_attach(em, libvlc_MediaPlayerBuffering, onVlcEvent, this);
        libvlc_event_attach(em, libvlc_MediaPlayerPlaying, onVlcEvent, this);
    } else {
        libvlc_media_player_set_media(mVlcPlayer, mVlcMedia);
    }

    int ret = libvlc_media_player_play(mVlcPlayer);
    if (ret != 0) {
        QMessageBox::warning(this,
            QString::fromUtf8("播放失败"),
            QString::fromUtf8("无法开始播放 (错误码: %1)").arg(ret));
        releaseVlcMedia();
        return;
    }
#endif

    mStreaming = true;
    // 不立即隐藏占位符 — 等待 onVlcPlaying 回调确认第一帧渲染后再隐藏
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
        // 不分离事件回调 — 播放器复用期间保持事件监听
        // 事件在 ~VideoStreamWindow() 中 libvlc_media_player_release 时自动清理
    }
    releaseVlcMedia();
#endif

    mStreaming = false;
    mBuffering = false;
    mPlaceholderLabel->setText(
        QString::fromUtf8("等待视频流..."));
    mPlaceholderLabel->setStyleSheet(
        "color: #444; font-size: 16px; background: transparent;");
    mPlaceholderLabel->show();
    updateButtonStates();
}

void VideoStreamWindow::onQualitySelected(const QString& quality) {
    mCurrentQuality = quality;
    updateQualityButtonText();
    // RTMP 流清晰度由推流端控制，本地不做处理
}

void VideoStreamWindow::onSwitchSource(const QString& source) {
    mCurrentSource = source;
    updateTitle();
    applySourceUrl(source);

    // 如果正在播放，自动切换到新视频源
    if (mStreaming) {
        onStop();
        onStart();
    }
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

void VideoStreamWindow::setStreamUrls(const QMap<QString, QString>& urls) {
    mUrlMap = urls;
    // 默认填入当前视频源的预设 URL
    applySourceUrl(mCurrentSource);
}

void VideoStreamWindow::setVlcInstance(libvlc_instance_t* vlc) {
#ifdef HAS_VLC
    mVlcInstance = vlc;
#else
    Q_UNUSED(vlc);
#endif
}

void VideoStreamWindow::closeEvent(QCloseEvent* event) {
    if (event->spontaneous()) {
        // 用户点击 X → 仅隐藏，不销毁
        hide();
        event->ignore();
    } else {
        // 程序关闭（主窗口关闭级联）→ 真正关闭
        event->accept();
    }
}
