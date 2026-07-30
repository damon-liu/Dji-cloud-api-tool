#include "PsdkSpeakerPanel.h"

#include <QAbstractItemView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSlider>
#include <QTextCursor>
#include <QTime>
#include <QVBoxLayout>
#include <QTimer>
#include <QCryptographicHash>
#include <QGridLayout>

// 表单行标签统一宽度，保证对齐
static const int LABEL_WIDTH = 72;

static QLabel* makeLabel(const QString& text, QWidget* parent) {
    auto* lbl = new QLabel(text, parent);
    lbl->setFixedWidth(LABEL_WIDTH);
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lbl->setStyleSheet("font-weight: bold; color: #333; font-size: 13px;");
    return lbl;
}

PsdkSpeakerPanel::PsdkSpeakerPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateButtonStates();
}

void PsdkSpeakerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ===== top row: dock selector + online + status =====
    auto* topRow = new QHBoxLayout;

    auto* dockLabel = new QLabel(QString::fromUtf8("控制机场:"), this);
    dockLabel->setStyleSheet("font-weight: bold; color: #333;");
    topRow->addWidget(dockLabel);

    mDockCombo = new QComboBox(this);
    mDockCombo->setEditable(true);
    mDockCombo->setInsertPolicy(QComboBox::NoInsert);
    mDockCombo->setMinimumWidth(280);
    mDockCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    mDockCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    mDockCombo->lineEdit()->setAlignment(Qt::AlignLeft);
    mDockCombo->lineEdit()->setReadOnly(true);
    mDockCombo->view()->setMinimumWidth(320);
    mDockCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 8px;"
        "font-size: 13px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }"
        "QComboBox QAbstractItemView { border: 1px solid #dadce0;"
        "selection-background-color: #e8f0fe; selection-color: #1a73e8; }");
    connect(mDockCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (mUpdatingCombo || idx < 0 || mAvailableDocks.isEmpty()) return;
        const auto& dock = mAvailableDocks[idx];
        setDevice(dock.name, dock.sn, true);
    });
    topRow->addWidget(mDockCombo);

    topRow->addSpacing(16);

    // PSDK 负载索引：选择 E-Port 物理挂载位置
    auto* psdkIndexLabel = new QLabel(QString::fromUtf8("负载索引:"), this);
    psdkIndexLabel->setStyleSheet("color: #5f6368; font-size: 12px;");
    topRow->addWidget(psdkIndexLabel);

    mPsdkIndexCombo = new QComboBox(this);
    mPsdkIndexCombo->addItems({"0", "1", "2", "3"});
    mPsdkIndexCombo->setCurrentIndex(2);
    mPsdkIndexCombo->setFixedWidth(56);
    mPsdkIndexCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 3px; padding: 2px 6px;"
        "font-size: 12px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }");
    topRow->addWidget(mPsdkIndexCombo);

    // auto* psdkHint = new QLabel(
    //     QString::fromUtf8("挂载位置"), this);
    // psdkHint->setStyleSheet("color: #9aa0a6; font-size: 11px;");
    // topRow->addWidget(psdkHint);

    topRow->addSpacing(16);

    mOnlineLabel = new QLabel(this);
    topRow->addWidget(mOnlineLabel);

    topRow->addStretch();

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(false);
    setStatus(QString::fromUtf8("连接机场后可使用PSDK喊话器"));
    topRow->addWidget(mStatusLabel);

    mainLayout->addLayout(topRow);

    // ===== 可滚动内容区 =====
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    // --- 喊话器控制 ---
    auto* ctrlGroup = new QGroupBox(QString::fromUtf8("喊话器控制"), scrollContent);
    auto* ctrlOuterLayout = new QVBoxLayout(ctrlGroup);
    ctrlOuterLayout->setSpacing(8);

    // 2×2 控制卡片网格
    auto* ctrlGrid = new QGridLayout;
    ctrlGrid->setSpacing(12);

    // (0,0) 音量控制
    auto* volCard = new QWidget(ctrlGroup);
    auto* volLayout = new QVBoxLayout(volCard);
    volLayout->setContentsMargins(8, 4, 8, 4);
    auto* volTitle = new QLabel(QString::fromUtf8("音量控制"), volCard);
    volTitle->setStyleSheet("font-weight: bold; color: #5f6368; font-size: 12px;");
    volLayout->addWidget(volTitle);
    auto* volRow = new QHBoxLayout;
    mVolumeSlider = new QSlider(Qt::Horizontal, volCard);
    mVolumeSlider->setRange(0, 100);
    mVolumeSlider->setValue(50);
    mVolumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 6px; background: #e0e0e0; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #1a73e8; width: 14px; height: 14px;"
        "margin: -4px 0; border-radius: 7px; }"
        "QSlider::sub-page:horizontal { background: #1a73e8; border-radius: 3px; }");
    volRow->addWidget(mVolumeSlider, 1);
    mVolumeLabel = new QLabel("50", volCard);
    mVolumeLabel->setFixedWidth(36);
    mVolumeLabel->setAlignment(Qt::AlignCenter);
    mVolumeLabel->setStyleSheet("font-weight: bold; color: #1a73e8; font-size: 14px;");
    volRow->addWidget(mVolumeLabel);
    volLayout->addLayout(volRow);
    // 300ms debounce 定时器：键盘/滚轮操作 slider 不会触发 sliderReleased，
    // 用定时器兜底确保最终值总能下发
    mVolumeDebounceTimer = new QTimer(this);
    mVolumeDebounceTimer->setSingleShot(true);
    mVolumeDebounceTimer->setInterval(300);
    connect(mVolumeDebounceTimer, &QTimer::timeout, this, [this]() {
        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        data["play_volume"] = mVolumeSlider->value();
        requestCommand(DockCommandType::SpeakerVolumeSet, data);
    });
    connect(mVolumeSlider, &QSlider::valueChanged, this, [this](int v) {
        mVolumeLabel->setText(QString::number(v));
        mVolumeDebounceTimer->start();
    });
    connect(mVolumeSlider, &QSlider::sliderReleased, this, [this]() {
        // 鼠标拖拽/点击松开时立即下发，停止定时器避免重复
        mVolumeDebounceTimer->stop();
        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        data["play_volume"] = mVolumeSlider->value();
        requestCommand(DockCommandType::SpeakerVolumeSet, data);
    });
    ctrlGrid->addWidget(volCard, 0, 0);

    // (0,1) 播放模式
    auto* modeCard = new QWidget(ctrlGroup);
    auto* modeLayout = new QVBoxLayout(modeCard);
    modeLayout->setContentsMargins(8, 4, 8, 4);
    auto* modeTitle = new QLabel(QString::fromUtf8("播放模式"), modeCard);
    modeTitle->setStyleSheet("font-weight: bold; color: #5f6368; font-size: 12px;");
    modeLayout->addWidget(modeTitle);
    mModeCombo = new QComboBox(modeCard);
    mModeCombo->addItem(QString::fromUtf8("单次播放"), 0);
    mModeCombo->addItem(QString::fromUtf8("循环播放"), 1);
    mModeCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 8px;"
        "font-size: 13px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }");
    modeLayout->addWidget(mModeCombo);
    modeLayout->addStretch();
    connect(mModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (mUpdatingCombo) return;
        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        data["play_mode"] = mModeCombo->currentData().toInt();
        requestCommand(DockCommandType::SpeakerModeSet, data);
    });
    ctrlGrid->addWidget(modeCard, 0, 1);

    // (1,0) 播放控制按钮
    auto* actionCard = new QWidget(ctrlGroup);
    auto* actionLayout = new QVBoxLayout(actionCard);
    actionLayout->setContentsMargins(8, 4, 8, 4);
    auto* actionTitle = new QLabel(QString::fromUtf8("播放控制"), actionCard);
    actionTitle->setStyleSheet("font-weight: bold; color: #5f6368; font-size: 12px;");
    actionLayout->addWidget(actionTitle);
    auto* actionBtnRow = new QHBoxLayout;
    mStopBtn = new QPushButton(QString::fromUtf8("⏹ 停止"), actionCard);
    mStopBtn->setCursor(Qt::PointingHandCursor);
    mStopBtn->setStyleSheet(
        "QPushButton { background: #ea4335; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background: #c5221f; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    mReplayBtn = new QPushButton(QString::fromUtf8("🔄 重播"), actionCard);
    mReplayBtn->setCursor(Qt::PointingHandCursor);
    mReplayBtn->setStyleSheet(
        "QPushButton { background: #f29900; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background: #e37400; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    actionBtnRow->addWidget(mStopBtn);
    actionBtnRow->addWidget(mReplayBtn);
    actionLayout->addLayout(actionBtnRow);
    actionLayout->addStretch();
    connect(mStopBtn, &QPushButton::clicked, this, [this]() {
        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        requestCommand(DockCommandType::SpeakerStop, data);
    });
    connect(mReplayBtn, &QPushButton::clicked, this, [this]() {
        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        requestCommand(DockCommandType::SpeakerReplay, data);
    });
    ctrlGrid->addWidget(actionCard, 1, 0);

    // (1,1) 播放进度
    auto* progCard = new QWidget(ctrlGroup);
    auto* progLayout = new QVBoxLayout(progCard);
    progLayout->setContentsMargins(8, 4, 8, 4);
    auto* progTitle = new QLabel(QString::fromUtf8("播放进度"), progCard);
    progTitle->setStyleSheet("font-weight: bold; color: #5f6368; font-size: 12px;");
    progLayout->addWidget(progTitle);
    mProgressLabel = new QLabel(progCard);
    mProgressLabel->setWordWrap(true);
    mProgressLabel->setStyleSheet("color: #80868b; font-size: 13px; padding: 4px 0;");
    mProgressLabel->setText(QString::fromUtf8("等待指令..."));
    progLayout->addWidget(mProgressLabel, 1);
    ctrlGrid->addWidget(progCard, 1, 1);

    ctrlGrid->setColumnStretch(0, 1);
    ctrlGrid->setColumnStretch(1, 1);
    ctrlOuterLayout->addLayout(ctrlGrid);

    contentLayout->addWidget(ctrlGroup);

    // --- TTS 文本喊话 ---
    auto* ttsGroup = new QGroupBox(QString::fromUtf8("TTS 文本喊话"), scrollContent);
    auto* ttsLayout = new QVBoxLayout(ttsGroup);
    ttsLayout->setSpacing(6);

    auto* ttsNameRow = new QHBoxLayout;
    ttsNameRow->addWidget(makeLabel(QString::fromUtf8("文件名:"), ttsGroup));
    mTtsNameEdit = new QLineEdit(ttsGroup);
    mTtsNameEdit->setPlaceholderText(QString::fromUtf8("用于机场侧标识，如：安全提醒（留空则自动生成）"));
    mTtsNameEdit->setMaximumWidth(240);
    mTtsNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 10px;"
        "font-size: 13px; } QLineEdit:focus { border-color: #1a73e8; }");
    ttsNameRow->addWidget(mTtsNameEdit);
    ttsNameRow->addStretch();
    ttsLayout->addLayout(ttsNameRow);

    // 文本标签 + 字符计数同行
    auto* ttsLabelRow = new QHBoxLayout;
    ttsLabelRow->addWidget(makeLabel(QString::fromUtf8("文本内容:"), ttsGroup));
    mTtsCharCount = new QLabel("0 / 1000", ttsGroup);
    mTtsCharCount->setStyleSheet("color: #80868b; font-size: 12px;");
    ttsLabelRow->addStretch();
    ttsLabelRow->addWidget(mTtsCharCount);
    ttsLayout->addLayout(ttsLabelRow);

    mTtsTextEdit = new QPlainTextEdit(ttsGroup);
    mTtsTextEdit->setPlaceholderText(QString::fromUtf8("输入 TTS 喊话文本内容（最大 1000 字符）..."));
    mTtsTextEdit->setFixedHeight(80);
    mTtsTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mTtsTextEdit->setStyleSheet(
        "QPlainTextEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 8px 10px;"
        "font-size: 13px; background: #fff; color: #202124; line-height: 1.5; }"
        "QPlainTextEdit:focus { border-color: #1a73e8; }");
    connect(mTtsTextEdit, &QPlainTextEdit::textChanged, this, [this]() {
        QString text = mTtsTextEdit->toPlainText();
        mTtsCharCount->setText(QString("%1 / 1000").arg(text.length()));
        if (!text.isEmpty())
            mTtsMd5Label->setText(QString::fromUtf8("MD5: ") + computeMd5(text));
        else
            mTtsMd5Label->setText(QString::fromUtf8("MD5: （输入文本后自动计算）"));
    });
    ttsLayout->addWidget(mTtsTextEdit);

    // 底部：MD5 + 发送按钮
    auto* ttsBottomRow = new QHBoxLayout;
    mTtsMd5Label = new QLabel(QString::fromUtf8("MD5: （输入文本后自动计算）"), ttsGroup);
    mTtsMd5Label->setStyleSheet("color: #80868b; font-size: 12px;");
    mTtsMd5Label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ttsBottomRow->addWidget(mTtsMd5Label, 1);

    mTtsSendBtn = new QPushButton(QString::fromUtf8("📢 发送 TTS 喊话"), ttsGroup);
    mTtsSendBtn->setCursor(Qt::PointingHandCursor);
    mTtsSendBtn->setFixedHeight(36);
    mTtsSendBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 8px 24px; font-size: 13px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    ttsBottomRow->addWidget(mTtsSendBtn);
    ttsLayout->addLayout(ttsBottomRow);

    connect(mTtsSendBtn, &QPushButton::clicked, this, [this]() {
        QString text = mTtsTextEdit->toPlainText().trimmed();
        QString name = mTtsNameEdit->text().trimmed();
        if (text.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("TTS 喊话"),
                                 QString::fromUtf8("请输入 TTS 文本内容"));
            return;
        }
        if (name.isEmpty())
            name = QString::fromUtf8("tts_%1").arg(QTime::currentTime().toString("HHmmss"));

        QJsonObject tts;
        tts["name"] = name;
        tts["text"] = text;
        tts["md5"]  = computeMd5(text);

        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        data["tts"] = tts;
        requestCommand(DockCommandType::SpeakerTtsPlay, data);
    });

    contentLayout->addWidget(ttsGroup);

    // --- 音频文件喊话 ---
    auto* audioGroup = new QGroupBox(QString::fromUtf8("音频文件喊话"), scrollContent);
    auto* audioLayout = new QVBoxLayout(audioGroup);
    audioLayout->setSpacing(6);

    // 文件名
    auto* audioNameRow = new QHBoxLayout;
    audioNameRow->addWidget(makeLabel(QString::fromUtf8("文件名:"), audioGroup));
    mAudioNameEdit = new QLineEdit(audioGroup);
    mAudioNameEdit->setPlaceholderText(QString::fromUtf8("如：alert_20230720（留空则自动生成）"));
    mAudioNameEdit->setMaximumWidth(240);
    mAudioNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 10px;"
        "font-size: 13px; } QLineEdit:focus { border-color: #1a73e8; }");
    audioNameRow->addWidget(mAudioNameEdit);
    audioNameRow->addStretch();
    audioLayout->addLayout(audioNameRow);

    // URL 独占一行（长链接需要足够宽度）
    auto* audioUrlRow = new QHBoxLayout;
    audioUrlRow->addWidget(makeLabel(QString::fromUtf8("音频URL:"), audioGroup));
    mAudioUrlEdit = new QLineEdit(audioGroup);
    mAudioUrlEdit->setPlaceholderText(QString::fromUtf8("https://your-cdn.example.com/audio/alert.pcm"));
    mAudioUrlEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 10px;"
        "font-size: 13px; } QLineEdit:focus { border-color: #1a73e8; }");
    audioUrlRow->addWidget(mAudioUrlEdit, 1);
    audioLayout->addLayout(audioUrlRow);

    // 格式说明 + MD5 分行
    auto* audioFmtRow = new QHBoxLayout;
    audioFmtRow->addWidget(makeLabel(QString::fromUtf8("格式:"), audioGroup));
    mAudioFormatLabel = new QLabel(
        QString::fromUtf8("PCM（16kHz 采样率 / 单声道 / 16bit 位深）— 无需手动选择"), audioGroup);
    mAudioFormatLabel->setStyleSheet("color: #5f6368; font-size: 12px;");
    audioFmtRow->addWidget(mAudioFormatLabel, 1);
    audioLayout->addLayout(audioFmtRow);

    auto* audioMd5Row = new QHBoxLayout;
    audioMd5Row->addWidget(makeLabel(QString::fromUtf8("MD5校验:"), audioGroup));
    mAudioMd5Edit = new QLineEdit(audioGroup);
    mAudioMd5Edit->setPlaceholderText(QString::fromUtf8("输入音频文件的 MD5 校验和（32位十六进制）"));
    mAudioMd5Edit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 10px;"
        "font-size: 13px; font-family: 'Consolas', monospace; }"
        "QLineEdit:focus { border-color: #1a73e8; }");
    audioMd5Row->addWidget(mAudioMd5Edit, 1);
    audioLayout->addLayout(audioMd5Row);

    // 发送按钮右对齐
    auto* audioBottomRow = new QHBoxLayout;
    audioBottomRow->addStretch();
    mAudioSendBtn = new QPushButton(QString::fromUtf8("🔊 发送音频喊话"), audioGroup);
    mAudioSendBtn->setCursor(Qt::PointingHandCursor);
    mAudioSendBtn->setFixedHeight(36);
    mAudioSendBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 8px 24px; font-size: 13px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    audioBottomRow->addWidget(mAudioSendBtn);
    audioLayout->addLayout(audioBottomRow);

    connect(mAudioSendBtn, &QPushButton::clicked, this, [this]() {
        QString url  = mAudioUrlEdit->text().trimmed();
        QString name = mAudioNameEdit->text().trimmed();
        QString md5  = mAudioMd5Edit->text().trimmed();
        if (url.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("音频喊话"),
                                 QString::fromUtf8("请输入音频文件下载链接"));
            return;
        }
        if (name.isEmpty())
            name = QString::fromUtf8("audio_%1").arg(QTime::currentTime().toString("HHmmss"));
        if (md5.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("音频喊话"),
                                 QString::fromUtf8("请输入音频文件的 MD5 校验和"));
            return;
        }

        QJsonObject file;
        file["name"]   = name;
        file["url"]    = url;
        file["md5"]    = md5;
        file["format"] = QStringLiteral("pcm");

        QJsonObject data;
        data["psdk_index"] = mPsdkIndexCombo->currentText().toInt();
        data["file"] = file;
        requestCommand(DockCommandType::SpeakerAudioPlay, data);
    });

    contentLayout->addWidget(audioGroup);
    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // ===== 下发记录（默认收起，▶/◢ 按钮切换） =====
    mHistoryEdit = new QPlainTextEdit(this);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setMinimumHeight(100);
    mHistoryEdit->setMaximumHeight(180);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    mHistoryEdit->setVisible(false);  // 默认收起
    mainLayout->addWidget(mHistoryEdit);

    mToggleHistoryBtn = new QPushButton(QString::fromUtf8("▶ 下发记录"), this);
    mToggleHistoryBtn->setCheckable(true);
    mToggleHistoryBtn->setCursor(Qt::PointingHandCursor);
    mToggleHistoryBtn->setStyleSheet(
        "QPushButton { border: none; background: #f1f3f4; color: #5f6368;"
        "font-size: 13px; font-weight: bold; padding: 4px 8px; border-radius: 4px; }"
        "QPushButton:hover { background: #e8eaed; }");
    mainLayout->addWidget(mToggleHistoryBtn);

    connect(mToggleHistoryBtn, &QPushButton::toggled, this, [this](bool checked) {
        mHistoryEdit->setVisible(checked);
        mToggleHistoryBtn->setText(checked
            ? QString::fromUtf8("◢ 下发记录")
            : QString::fromUtf8("▶ 下发记录"));
    });

    // 按钮最小高度
    mStopBtn->setMinimumHeight(34);
    mReplayBtn->setMinimumHeight(34);
}

void PsdkSpeakerPanel::setDevice(const QString& displayName, const QString& gatewaySn, bool online) {
    if (mGatewaySn != gatewaySn || mOnline != online) {
        mPending = false;
        mProgressLabel->setText(QString::fromUtf8("等待指令..."));
        mProgressLabel->setStyleSheet("color: #80868b; font-size: 13px; padding: 4px 0;");
    }
    mDisplayName = displayName;
    mGatewaySn = gatewaySn;
    mOnline = online;

    if (mGatewaySn.isEmpty()) {
        clearDevice();
        return;
    }

    if (mOnline) {
        mOnlineLabel->setText(QString::fromUtf8("🟢 在线"));
        mOnlineLabel->setStyleSheet("color: #1e8e3e; font-weight: bold; padding: 0 8px;");
    } else {
        mOnlineLabel->setText(QString::fromUtf8("🔴 离线"));
        mOnlineLabel->setStyleSheet("color: #d93025; font-weight: bold; padding: 0 8px;");
    }

    if (!mOnline)
        setStatus(QString::fromUtf8("机场离线，无法发送喊话器指令"), true);
    else
        setStatus(QString::fromUtf8("已就绪，可控制PSDK喊话器"));
    updateButtonStates();
}

void PsdkSpeakerPanel::clearDevice() {
    mDisplayName.clear();
    mGatewaySn.clear();
    mOnline = false;
    mPending = false;
    mStepProgress.clear();
    mCurrentProgressMethod.clear();
    mProgressLabel->setText(QString::fromUtf8("等待指令..."));
    mProgressLabel->setStyleSheet("color: #80868b; font-size: 13px; padding: 4px 0;");
    mOnlineLabel->setText(QString());
    mOnlineLabel->setStyleSheet(QString());
    setStatus(QString::fromUtf8("未选择可控制的机场设备"));
    updateButtonStates();
}

void PsdkSpeakerPanel::setConnected(bool connected) {
    mConnected = connected;
    if (!connected) {
        mPending = false;
        mStepProgress.clear();
        mCurrentProgressMethod.clear();
        mProgressLabel->setText(QString::fromUtf8("等待指令..."));
        mProgressLabel->setStyleSheet("color: #80868b; font-size: 13px; padding: 4px 0;");
        setStatus(QString::fromUtf8("MQTT 未连接"), true);
    }
    updateButtonStates();
}

void PsdkSpeakerPanel::setAvailableDocks(const QVector<DeviceInfo>& docks,
                                          const QString& currentSn,
                                          double dockLat, double dockLon) {
    mAvailableDocks = docks;
    mDockLat = dockLat;
    mDockLon = dockLon;

    mUpdatingCombo = true;
    mDockCombo->clear();

    int selectIdx = -1;
    for (int i = 0; i < docks.size(); ++i) {
        const auto& d = docks[i];
        mDockCombo->addItem(
            QString::fromUtf8("%1 - %2").arg(d.name, d.sn), d.sn);
        if (d.sn == currentSn)
            selectIdx = i;
    }

    if (selectIdx >= 0)
        mDockCombo->setCurrentIndex(selectIdx);
    else if (!docks.isEmpty())
        mDockCombo->setCurrentIndex(0);

    mUpdatingCombo = false;

    if (docks.isEmpty()) {
        clearDevice();
    } else {
        int idx = mDockCombo->currentIndex();
        if (idx >= 0 && idx < docks.size()) {
            const auto& d = docks[idx];
            setDevice(d.name, d.sn, true);
        }
    }
}

void PsdkSpeakerPanel::requestCommand(DockCommandType type, const QJsonObject& data) {
    if (mGatewaySn.isEmpty() || mPending)
        return;

    mPending = true;
    setStatus(QString::fromUtf8("正在执行：%1").arg(DockCommandBuilder::displayName(type)));
    updateButtonStates();
    emit commandRequested(mGatewaySn, type, data);
}

void PsdkSpeakerPanel::onCommandStateChanged(const DockCommandResult& result) {
    if (!mGatewaySn.isEmpty() && result.gatewaySn != mGatewaySn)
        return;

    // 只处理喊话器相关指令
    if (result.type != DockCommandType::SpeakerTtsPlay
        && result.type != DockCommandType::SpeakerAudioPlay
        && result.type != DockCommandType::SpeakerVolumeSet
        && result.type != DockCommandType::SpeakerModeSet
        && result.type != DockCommandType::SpeakerStop
        && result.type != DockCommandType::SpeakerReplay)
        return;

    const QString action = DockCommandBuilder::displayName(result.type);
    if (result.state == DockCommandState::Publishing
            || result.state == DockCommandState::WaitingReply) {
        mPending = true;
        setStatus(QString::fromUtf8("%1：%2").arg(action, result.message));
        // 新的播放指令开始时清空进度步骤
        if (result.state == DockCommandState::Publishing
            && (result.type == DockCommandType::SpeakerTtsPlay
                || result.type == DockCommandType::SpeakerAudioPlay)) {
            mStepProgress.clear();
            mCurrentProgressMethod.clear();
            mProgressLabel->clear();
        }
        updateButtonStates();
        return;
    }

    mPending = false;
    if (result.state == DockCommandState::Succeeded) {
        setStatus(QString::fromUtf8("%1成功").arg(action));
    } else {
        setStatus(QString::fromUtf8("%1失败：%2").arg(action, result.message), true);
    }
    appendHistory(result);
    updateButtonStates();
}

void PsdkSpeakerPanel::onSpeakerProgress(const SpeakerProgress& progress) {
    if (!mGatewaySn.isEmpty() && progress.gatewaySn != mGatewaySn)
        return;

    // 步骤管线定义
    struct StepDef { QString key; QString label; };
    const bool isTts = progress.method.contains("tts");
    const QVector<StepDef> pipeline = isTts
        ? QVector<StepDef>{
            {QStringLiteral("change_work_mode"), QString::fromUtf8("切换工作模式")},
            {QStringLiteral("upload"),           QString::fromUtf8("上传音频到PSDK")},
            {QStringLiteral("encoding"),         QString::fromUtf8("编码PCM为Opus")},
            {QStringLiteral("play"),             QString::fromUtf8("播放中")}}
        : QVector<StepDef>{
            {QStringLiteral("change_work_mode"), QString::fromUtf8("切换工作模式")},
            {QStringLiteral("download"),         QString::fromUtf8("下载音频文件")},
            {QStringLiteral("encoding"),         QString::fromUtf8("编码PCM为Opus")},
            {QStringLiteral("play"),             QString::fromUtf8("播放中")}};

    mCurrentProgressMethod = progress.method;

    // 找到当前 stepKey 在管线中的位置
    int currentIdx = -1;
    for (int i = 0; i < pipeline.size(); ++i) {
        if (pipeline[i].key == progress.stepKey) {
            currentIdx = i;
            break;
        }
    }
    if (currentIdx < 0) {
        // 未知步骤，回退到简单显示
        mProgressLabel->setText(QString::fromUtf8("[%1] %2 — %3%")
            .arg(isTts ? QString::fromUtf8("TTS") : QString::fromUtf8("音频"),
                 progress.stepKey)
            .arg(progress.percent));
        return;
    }

    // 更新所有步骤状态
    for (int i = 0; i < pipeline.size(); ++i) {
        if (i < currentIdx) {
            // 已过的步骤标记为完成
            mStepProgress[pipeline[i].key] = 100;
        } else if (i == currentIdx) {
            // 当前步骤
            mStepProgress[pipeline[i].key] = (progress.status == QStringLiteral("ok")) ? 100 : progress.percent;
        } else {
            // 未到达的步骤
            mStepProgress[pipeline[i].key] = -1;
        }
    }

    renderProgress();

    // 只有最终步骤 "play" 完成才复位 pending / 更新状态
    if (progress.stepKey == QStringLiteral("play") && progress.status == QStringLiteral("ok")) {
        mPending = false;
        setStatus(QString::fromUtf8("喊话播放完成"));
        updateButtonStates();
    }
}

void PsdkSpeakerPanel::renderProgress() {
    // 根据 mCurrentProgressMethod 选择管线
    const bool isTts = mCurrentProgressMethod.contains("tts");
    const QVector<QPair<QString, QString>> pipeline = isTts
        ? QVector<QPair<QString, QString>>{
            {QStringLiteral("change_work_mode"), QString::fromUtf8("切换工作模式")},
            {QStringLiteral("upload"),           QString::fromUtf8("上传音频到PSDK")},
            {QStringLiteral("encoding"),         QString::fromUtf8("编码PCM为Opus")},
            {QStringLiteral("play"),             QString::fromUtf8("播放中")}}
        : QVector<QPair<QString, QString>>{
            {QStringLiteral("change_work_mode"), QString::fromUtf8("切换工作模式")},
            {QStringLiteral("download"),         QString::fromUtf8("下载音频文件")},
            {QStringLiteral("encoding"),         QString::fromUtf8("编码PCM为Opus")},
            {QStringLiteral("play"),             QString::fromUtf8("播放中")}};

    QStringList lines;
    for (const auto& step : pipeline) {
        int pct = mStepProgress.value(step.first, -1);
        QString line;
        if (pct == 100) {
            // 已完成
            line = QString::fromUtf8("<span style='color:#1e8e3e;'>✓ %1</span>").arg(step.second);
        } else if (pct >= 0) {
            // 进行中
            line = QString::fromUtf8("<span style='color:#1a73e8;'>◉ %1  %2%</span>").arg(step.second).arg(pct);
        } else {
            // 未开始
            line = QString::fromUtf8("<span style='color:#80868b;'>◻ %1</span>").arg(step.second);
        }
        lines.append(line);
    }

    mProgressLabel->setText(lines.join("<br>"));
    mProgressLabel->setStyleSheet("font-size: 13px; padding: 4px 0;");
}

void PsdkSpeakerPanel::updateButtonStates() {
    const bool available = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mStopBtn->setEnabled(available);
    mReplayBtn->setEnabled(available);
    mTtsSendBtn->setEnabled(available);
    mAudioSendBtn->setEnabled(available);
}

void PsdkSpeakerPanel::setStatus(const QString& text, bool error) {
    mStatusLabel->setText(text);
    mStatusLabel->setStyleSheet(error
        ? QStringLiteral("color: #d93025; font-weight: bold; padding: 4px;")
        : QStringLiteral("color: #e8710a; font-weight: bold; padding: 4px;"));
}

void PsdkSpeakerPanel::appendHistory(const DockCommandResult& result) {
    const QString action = DockCommandBuilder::displayName(result.type);
    QString verdict;
    switch (result.state) {
    case DockCommandState::Succeeded:
        verdict = QString::fromUtf8("✅ 成功 (result=0)");
        break;
    case DockCommandState::TimedOut:
        verdict = QString::fromUtf8("❌ 超时");
        break;
    default:
        verdict = QString::fromUtf8("❌ 失败（%1）").arg(result.message);
        break;
    }

    QString block;
    block += QStringLiteral("[%1] %2  %3\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action, verdict);
    block += QString::fromUtf8("Topic: thing/product/%1/services\n").arg(result.gatewaySn);
    block += QString::fromUtf8("下发:\n%1\n").arg(result.requestJson.trimmed());
    block += QString::fromUtf8("响应:\n%1\n").arg(result.replyJson.isEmpty()
        ? QString::fromUtf8("（无响应）") : result.replyJson.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    mHistoryEdit->moveCursor(QTextCursor::Start);
    mHistoryEdit->insertPlainText(block);
    mHistoryEdit->moveCursor(QTextCursor::Start);
}

QString PsdkSpeakerPanel::computeMd5(const QString& text) const {
    QByteArray hash = QCryptographicHash::hash(
        text.toUtf8(), QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
}
