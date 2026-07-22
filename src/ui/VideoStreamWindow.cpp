#include "VideoStreamWindow.h"

#include <QCloseEvent>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

VideoStreamWindow::VideoStreamWindow(int index, QWidget* parent)
    : QWidget(parent, Qt::Window), mIndex(index) {
    setWindowTitle(QString::fromUtf8("视频流 %1").arg(index + 1));
    setMinimumSize(480, 270);
    resize(480, 270);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    mTitleLabel = new QLabel(QString::fromUtf8("视频流 %1").arg(index + 1), this);
    mTitleLabel->setStyleSheet(
        "background: #1e1e1e; color: #ccc; font-size: 12px; padding: 4px 8px; font-weight: bold;");
    mTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(mTitleLabel);

    mVideoArea = new QWidget(this);
    mVideoArea->setStyleSheet("background: #000;");
    layout->addWidget(mVideoArea, 1);

    mPlaceholderLabel = new QLabel(
        QString::fromUtf8("等待视频流..."), mVideoArea);
    mPlaceholderLabel->setStyleSheet("color: #444; font-size: 14px; background: transparent;");
    mPlaceholderLabel->setAlignment(Qt::AlignCenter);

    auto* areaLayout = new QVBoxLayout(mVideoArea);
    areaLayout->addWidget(mPlaceholderLabel);
}

void VideoStreamWindow::setStreamUrl(const QString& url) {
    Q_UNUSED(url);
}

void VideoStreamWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}
