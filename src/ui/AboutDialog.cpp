#include "AboutDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <cstring>

// ── 编译时构建信息 ──

// 解析 __DATE__ "Mmm dd yyyy" → "yyyy-MM-dd"
static QString buildDate() {
    const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"};
    char month[4]; int day, year;
    sscanf(__DATE__, "%3s %d %d", month, &day, &year);
    int mon = 0;
    for (int i = 0; i < 12; ++i) {
        if (strcmp(month, months[i]) == 0) { mon = i + 1; break; }
    }
    return QString("%1-%2-%3")
        .arg(year, 4, 10, QChar('0'))
        .arg(mon,  2, 10, QChar('0'))
        .arg(day,  2, 10, QChar('0'));
}

static QString buildEnv() {
#ifdef __VERSION__
    // GCC / MinGW
    return QString::fromUtf8("MinGW / Qt %1 / C++17").arg(QT_VERSION_STR);
#elif defined(_MSC_VER)
    return QString::fromUtf8("MSVC / Qt %1 / C++17").arg(QT_VERSION_STR);
#else
    return QString::fromUtf8("Qt %1 / C++17").arg(QT_VERSION_STR);
#endif
}

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("关于本软件"));
    setFixedSize(470, 590);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setStyleSheet(
        "QDialog { background: #f5f6fa; }"
        "QFrame#infoCard { background: #ffffff; border: 1px solid #e8e8e8; border-radius: 10px; }"
        "QFrame#qrCard { background: #ffffff; border: 1px solid #e8e8e8; border-radius: 10px; }"
        "QLabel#appTitle { font-size: 17px; font-weight: bold; color: #1a1a1a; }"
        "QLabel#appVersion { font-size: 13px; color: #999; }"
        "QLabel#appDesc { font-size: 12px; color: #888; }"
        "QLabel#infoLabel { font-size: 13px; color: #444; }"
        "QLabel#infoLink { font-size: 13px; }"
        "QLabel#copyright { font-size: 11px; color: #bbb; }"
        "QLabel#qrTitle { font-size: 14px; font-weight: bold; color: #333; }"
        "QLabel#qrHint { font-size: 12px; color: #999; }");
    setupUi();
}

void AboutDialog::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 20);
    layout->setSpacing(12);

    // ── Logo ──
    auto* logoLabel = new QLabel(this);
    logoLabel->setAlignment(Qt::AlignCenter);
    QPixmap logoPixmap(":/logo.jpg");
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    layout->addWidget(logoLabel);

    // ── 标题 ──
    auto* titleLabel = new QLabel(QString::fromUtf8("DJI-CLOUD-API-TOOL 监控客户端"), this);
    titleLabel->setObjectName("appTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // ── 简短描述 ──
    auto* descLabel = new QLabel(QString::fromUtf8("轻量级大疆上云 API 调试与监控工具"), this);
    descLabel->setObjectName("appDesc");
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);

    layout->addSpacing(4);

    // ── 信息卡片 ──
    auto* infoCard = new QFrame(this);
    infoCard->setObjectName("infoCard");
    auto* cardLayout = new QVBoxLayout(infoCard);
    cardLayout->setContentsMargins(20, 16, 20, 14);
    cardLayout->setSpacing(8);

    // 信息行：左标签右值，使用 QHBoxLayout
    auto addInfoRow = [&](const QString& icon, const QString& label, const QString& value,
                          const QString& linkUrl = QString()) {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);

        auto* labelWidget = new QLabel(QString("%1 %2").arg(icon, label), infoCard);
        labelWidget->setObjectName("infoLabel");
        labelWidget->setFixedWidth(115);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(labelWidget);

        if (!linkUrl.isEmpty()) {
            auto* linkLabel = new QLabel(infoCard);
            linkLabel->setObjectName("infoLink");
            linkLabel->setText(QString::fromUtf8(
                "<a href='%1' style='color:#1a73e8; text-decoration:none;'>%2</a>")
                .arg(linkUrl, value));
            linkLabel->setOpenExternalLinks(true);
            linkLabel->setCursor(Qt::PointingHandCursor);
            row->addWidget(linkLabel);
        } else {
            auto* valueLabel = new QLabel(value, infoCard);
            valueLabel->setObjectName("infoLabel");
            row->addWidget(valueLabel);
        }
        row->addStretch();
        cardLayout->addLayout(row);
    };

    addInfoRow(QString::fromUtf8("👤"), QString::fromUtf8("作者"),
               QString::fromUtf8("damon.liu"));
    addInfoRow(QString::fromUtf8("🏷️"), QString::fromUtf8("版本"),
               QString::fromUtf8("v1.0.4"));
    addInfoRow(QString::fromUtf8("📅"), QString::fromUtf8("构建时间"),
               buildDate());
    addInfoRow(QString::fromUtf8("🔧"), QString::fromUtf8("构建环境"),
               buildEnv());
    addInfoRow(QString::fromUtf8("🔗"), QString::fromUtf8("项目地址"),
               QString::fromUtf8("github.com/damon-liu/Dji-cloud-api-tool"),
               QString::fromUtf8("https://github.com/damon-liu/Dji-cloud-api-tool"));

    // 版权行
    cardLayout->addSpacing(2);
    auto* copyrightLabel = new QLabel(QString::fromUtf8("© 2026 damon.liu"), infoCard);
    copyrightLabel->setObjectName("copyright");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(copyrightLabel);

    layout->addWidget(infoCard);

    // ── 公众号卡片 ──
    auto* qrCard = new QFrame(this);
    qrCard->setObjectName("qrCard");
    auto* qrCardLayout = new QVBoxLayout(qrCard);
    qrCardLayout->setContentsMargins(16, 16, 16, 12);
    qrCardLayout->setSpacing(10);
    qrCardLayout->setAlignment(Qt::AlignCenter);

    // auto* qrTitleLabel = new QLabel(QString::fromUtf8("📱 关注公众号"), qrCard);
    // qrTitleLabel->setObjectName("qrTitle");
    // qrTitleLabel->setAlignment(Qt::AlignCenter);
    // qrCardLayout->addWidget(qrTitleLabel);

    auto* imgLabel = new QLabel(qrCard);
    imgLabel->setAlignment(Qt::AlignCenter);
    QPixmap pixmap(":/gongzhonghao.png");
    if (!pixmap.isNull()) {
        imgLabel->setPixmap(pixmap.scaledToWidth(370, Qt::SmoothTransformation));
    }
    qrCardLayout->addWidget(imgLabel);

    auto* qrHintLabel = new QLabel(QString::fromUtf8("扫码获取更多技术分享"), qrCard);
    qrHintLabel->setObjectName("qrHint");
    qrHintLabel->setAlignment(Qt::AlignCenter);
    qrCardLayout->addWidget(qrHintLabel);

    layout->addWidget(qrCard);
}
