#include "VideoStreamWindow.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

VideoStreamWindow::VideoStreamWindow(int index,
                                     const QString& deviceLabel,
                                     const QString& defaultSource,
                                     const QStringList& switchSources,
                                     const QString& switchLabel,
                                     QWidget* parent)
    : QWidget(parent, Qt::Window)
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

    // 清晰度按钮 + 下拉菜单
    mQualityBtn = new QPushButton(controlBar);
    mQualityBtn->setCursor(Qt::PointingHandCursor);
    mQualityBtn->setFocusPolicy(Qt::NoFocus);
    mQualityBtn->setFixedHeight(30);
    mQualityBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #ccc;"
        "border: 1px solid #555; border-radius: 4px;"
        "padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background: #4a4a4a; border-color: #888; }"
        "QPushButton::menu-indicator { image: none; }");
    updateQualityButtonText();

    mQualityMenu = new QMenu(this);
    mQualityMenu->setStyleSheet(
        "QMenu { background: #2a2a2a; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 0; }"
        "QMenu::item { color: #ccc; padding: 6px 28px 6px 16px; font-size: 12px; }"
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

    // 切换视频源按钮（两个窗口都有）
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

void VideoStreamWindow::updateTitle() {
    setWindowTitle(QString::fromUtf8("直播 - %1").arg(mDeviceLabel));
    mTitleLabel->setText(
        QString::fromUtf8("📷 %1 · %2").arg(mDeviceLabel, mCurrentSource));
}

void VideoStreamWindow::updateQualityButtonText() {
    mQualityBtn->setText(
        QString::fromUtf8("🎛 %1 ▾").arg(mCurrentQuality));
}

void VideoStreamWindow::updateButtonStates() {
    mStartBtn->setEnabled(!mStreaming);
    mStopBtn->setEnabled(mStreaming);
    mQualityBtn->setEnabled(true);
    if (mSwitchBtn)
        mSwitchBtn->setEnabled(true);
}

void VideoStreamWindow::onStart() {
    mStreaming = true;
    mPlaceholderLabel->setText(
        QString::fromUtf8("直播中...\n%1 · %2 · %3")
            .arg(mDeviceLabel, mCurrentSource, mCurrentQuality));
    mPlaceholderLabel->setStyleSheet(
        "color: #4caf50; font-size: 16px; background: transparent;");
    updateButtonStates();
}

void VideoStreamWindow::onStop() {
    mStreaming = false;
    mPlaceholderLabel->setText(
        QString::fromUtf8("等待视频流..."));
    mPlaceholderLabel->setStyleSheet(
        "color: #444; font-size: 16px; background: transparent;");
    updateButtonStates();
}

void VideoStreamWindow::onQualitySelected(const QString& quality) {
    mCurrentQuality = quality;
    updateQualityButtonText();
    if (mStreaming) {
        mPlaceholderLabel->setText(
            QString::fromUtf8("直播中...\n%1 · %2 · %3")
                .arg(mDeviceLabel, mCurrentSource, mCurrentQuality));
    }
}

void VideoStreamWindow::onSwitchSource(const QString& source) {
    mCurrentSource = source;
    updateTitle();
    if (mStreaming) {
        mPlaceholderLabel->setText(
            QString::fromUtf8("直播中...\n%1 · %2 · %3")
                .arg(mDeviceLabel, mCurrentSource, mCurrentQuality));
    }
}

void VideoStreamWindow::setStreamUrl(const QString& url) {
    Q_UNUSED(url);
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
