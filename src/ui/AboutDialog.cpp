#include "AboutDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QFrame>
#include <QApplication>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("关于 DJI-CLOUD-API-TOOL 监控客户端"));
    setFixedSize(480, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUi();
}

void AboutDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 16);
    mainLayout->setSpacing(10);

    // ── Logo ──
    auto* logoLabel = new QLabel(this);
    QPixmap logo(":/logo.jpg");
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel);

    // ── App Name ──
    auto* nameLabel = new QLabel(QString::fromUtf8("DJI-CLOUD-API-TOOL 监控客户端"), this);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #212121;");
    mainLayout->addWidget(nameLabel);

    // ── Version ──
    auto* versionLabel = new QLabel(QString::fromUtf8("版本: v1.0.4"), this);
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet("font-size: 13px; color: #616161; margin-top: 2px;");
    mainLayout->addWidget(versionLabel);

    // ── Separator ──
    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #e0e0e0;");
    mainLayout->addWidget(sep1);

    // ── Author ──
    auto* authorLabel = new QLabel(QString::fromUtf8("作者: damon.liu"), this);
    authorLabel->setAlignment(Qt::AlignCenter);
    authorLabel->setStyleSheet("font-size: 13px; color: #424242;");
    mainLayout->addWidget(authorLabel);

    // ── Project Link ──
    auto* linkLabel = new QLabel(this);
    linkLabel->setText(QString::fromUtf8(
        "<a href='https://github.com/damon-liu/Dji-cloud-api-tool' "
        "style='color: #1565C0; text-decoration: none;'>"
        "github.com/damon-liu/Dji-cloud-api-tool</a>"));
    linkLabel->setAlignment(Qt::AlignCenter);
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setStyleSheet("font-size: 12px;");
    mainLayout->addWidget(linkLabel);

    mainLayout->addSpacing(4);

    // ── WeChat Official Account Section ──
    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #e0e0e0;");
    mainLayout->addWidget(sep2);

    auto* followLabel = new QLabel(QString::fromUtf8("📱 关注公众号，获取更多技术分享"), this);
    followLabel->setAlignment(Qt::AlignCenter);
    followLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #333333; margin-top: 4px;");
    mainLayout->addWidget(followLabel);

    // ── QR / Official Account Image ──
    auto* qrLabel = new QLabel(this);
    QPixmap qrPixmap(":/gongzhonghao.png");
    if (!qrPixmap.isNull()) {
        qrLabel->setPixmap(qrPixmap.scaledToWidth(440, Qt::SmoothTransformation));
    } else {
        qrLabel->setText(QString::fromUtf8("（公众号图片加载失败）"));
        qrLabel->setStyleSheet("color: #9e9e9e; font-size: 12px;");
    }
    qrLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addSpacing(28);
    mainLayout->addWidget(qrLabel);

    mainLayout->addStretch();
}
