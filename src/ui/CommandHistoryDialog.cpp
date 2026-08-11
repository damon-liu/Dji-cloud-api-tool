#include "CommandHistoryDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTime>
#include <QCloseEvent>
#include <QJsonDocument>

// ——— 构造函数 ———
CommandHistoryDialog::CommandHistoryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("下发记录"));
    setWindowFlags(windowFlags()
                   | Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, false);
    setSizeGripEnabled(true);
    setMinimumSize(600, 400);
    resize(800, 560);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ——— 顶部工具栏 ———
    auto* toolbarRow = new QHBoxLayout;
    toolbarRow->setSpacing(8);

    // 来源筛选
    auto* filterLabel = new QLabel(QString::fromUtf8("来源:"), this);
    filterLabel->setStyleSheet("font-size: 12px; color: #5f6368;");
    toolbarRow->addWidget(filterLabel);

    mSourceFilter = new QComboBox(this);
    mSourceFilter->addItems({
        QString::fromUtf8("全部"),
        QString::fromUtf8("远程调试"),
        QString::fromUtf8("飞行控制"),
        QString::fromUtf8("PSDK功能"),
        QString::fromUtf8("运维工具"),
        QString::fromUtf8("Topic下发"),
        QString::fromUtf8("视频直播")
    });
    mSourceFilter->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 3px 6px;"
        "font-size: 12px; background: #fff; }");
    toolbarRow->addWidget(mSourceFilter);

    toolbarRow->addStretch();

    // 搜索框
    auto* searchLabel = new QLabel(QString::fromUtf8("搜索:"), this);
    searchLabel->setStyleSheet("font-size: 12px; color: #5f6368;");
    toolbarRow->addWidget(searchLabel);

    mSearchEdit = new QLineEdit(this);
    mSearchEdit->setPlaceholderText(QString::fromUtf8("输入关键词..."));
    mSearchEdit->setClearButtonEnabled(true);
    mSearchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 3px 8px;"
        "font-size: 12px; background: #fff; }");
    mSearchEdit->setFixedWidth(180);
    toolbarRow->addWidget(mSearchEdit);

    // 导出按钮
    mExportBtn = new QPushButton(QString::fromUtf8("导出"), this);
    mExportBtn->setCursor(Qt::PointingHandCursor);
    mExportBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
        "padding: 4px 12px; font-size: 12px; background: #fff; color: #1a73e8; }"
        "QPushButton:hover { background: #e8f0fe; }");
    toolbarRow->addWidget(mExportBtn);

    // 清空按钮
    mClearBtn = new QPushButton(QString::fromUtf8("清空"), this);
    mClearBtn->setCursor(Qt::PointingHandCursor);
    mClearBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
        "padding: 4px 12px; font-size: 12px; background: #fff; color: #d93025; }"
        "QPushButton:hover { background: #fce8e6; }");
    toolbarRow->addWidget(mClearBtn);

    // 计数标签
    mCountLabel = new QLabel(QString::fromUtf8("共 0 条"), this);
    mCountLabel->setStyleSheet("font-size: 12px; color: #80868b;");
    toolbarRow->addWidget(mCountLabel);

    mainLayout->addLayout(toolbarRow);

    // ——— 历史记录显示区 ———
    mHistoryEdit = new QPlainTextEdit(this);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setFont(QFont("Consolas", 11));
    mHistoryEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    mHistoryEdit->setMaximumBlockCount(2000);
    mHistoryEdit->setStyleSheet(
        "QPlainTextEdit { background: #1e1e1e; color: #d4d4d4;"
        "border: 1px solid #d0d0d0; border-radius: 6px;"
        "font-family: 'Consolas', 'Courier New', monospace;"
        "font-size: 12px; padding: 8px; }");
    mainLayout->addWidget(mHistoryEdit, 1);

    // ——— 信号连接 ———
    connect(mSourceFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CommandHistoryDialog::onFilterChanged);
    connect(mSearchEdit, &QLineEdit::textChanged,
            this, &CommandHistoryDialog::onSearchChanged);
    connect(mClearBtn, &QPushButton::clicked,
            this, &CommandHistoryDialog::onClearClicked);
    connect(mExportBtn, &QPushButton::clicked,
            this, &CommandHistoryDialog::onExportClicked);
}

// ——— 关闭事件：隐藏而非销毁 ———
void CommandHistoryDialog::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

// ——— 追加 DockCommand 记录 ———
void CommandHistoryDialog::appendDockCommand(HistorySource source, const DockCommandResult& result) {
    // 过滤中间状态，只记录最终结果
    if (result.state == DockCommandState::Publishing
        || result.state == DockCommandState::WaitingReply)
        return;

    const QString action = DockCommandBuilder::displayName(result.type);
    const QString verdict = formatVerdict(result);

    QString block;
    block += QStringLiteral("[%1] %2  %3\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action, verdict);
    block += QString::fromUtf8("Topic: thing/product/%1/services\n").arg(result.gatewaySn);
    block += QString::fromUtf8("下发:\n%1\n").arg(result.requestJson.trimmed());
    block += QString::fromUtf8("响应:\n%1\n").arg(result.replyJson.isEmpty()
        ? QString::fromUtf8("（无响应）") : result.replyJson.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    // 存储
    mEntries.prepend({source, block});
    while (mEntries.size() > MAX_ENTRIES)
        mEntries.removeLast();

    // 如果当前筛选条件匹配，追加到显示
    int filterIdx = mSourceFilter->currentIndex();
    bool sourceMatch = (filterIdx == 0) || (static_cast<int>(source) == filterIdx);
    bool searchMatch = mSearchEdit->text().isEmpty()
                       || block.contains(mSearchEdit->text(), Qt::CaseInsensitive);
    if (sourceMatch && searchMatch) {
        mHistoryEdit->moveCursor(QTextCursor::Start);
        mHistoryEdit->insertPlainText(block);
        mHistoryEdit->moveCursor(QTextCursor::Start);
    }

    mCountLabel->setText(QString::fromUtf8("共 %1 条").arg(mEntries.size()));
}

// ——— 追加 Topic 下发记录 ———
void CommandHistoryDialog::appendTopicPublish(const QString& topic, const QString& json,
                                               bool success, const QString& message) {
    QString timeStr = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    QString icon = success ? QString::fromUtf8("✅") : QString::fromUtf8("❌");

    QString block;
    block += QStringLiteral("[%1] %2 %3\n").arg(timeStr, icon, message);
    block += QString::fromUtf8("Topic: %1\n").arg(topic);
    block += QString::fromUtf8("下发:\n%1\n").arg(json.trimmed());
    block += QString::fromUtf8("────────────────────────────\n");

    mEntries.prepend({HistorySource::Publish, block});
    while (mEntries.size() > MAX_ENTRIES)
        mEntries.removeLast();

    int filterIdx = mSourceFilter->currentIndex();
    bool sourceMatch = (filterIdx == 0) || (filterIdx == static_cast<int>(HistorySource::Publish));
    bool searchMatch = mSearchEdit->text().isEmpty()
                       || block.contains(mSearchEdit->text(), Qt::CaseInsensitive);
    if (sourceMatch && searchMatch) {
        mHistoryEdit->moveCursor(QTextCursor::Start);
        mHistoryEdit->insertPlainText(block);
        mHistoryEdit->moveCursor(QTextCursor::Start);
    }

    mCountLabel->setText(QString::fromUtf8("共 %1 条").arg(mEntries.size()));
}

// ——— 追加运维操作记录 ———
void CommandHistoryDialog::appendMaintenanceAction(const QString& action) {
    QString block;
    block += QStringLiteral("[%1] 🔧 %2\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action);
    block += QString::fromUtf8("状态: 功能预览（未实现）\n");
    block += QString::fromUtf8("────────────────────────────\n");

    mEntries.prepend({HistorySource::Maintenance, block});
    while (mEntries.size() > MAX_ENTRIES)
        mEntries.removeLast();

    int filterIdx = mSourceFilter->currentIndex();
    bool sourceMatch = (filterIdx == 0) || (filterIdx == static_cast<int>(HistorySource::Maintenance));
    bool searchMatch = mSearchEdit->text().isEmpty()
                       || block.contains(mSearchEdit->text(), Qt::CaseInsensitive);
    if (sourceMatch && searchMatch) {
        mHistoryEdit->moveCursor(QTextCursor::Start);
        mHistoryEdit->insertPlainText(block);
        mHistoryEdit->moveCursor(QTextCursor::Start);
    }

    mCountLabel->setText(QString::fromUtf8("共 %1 条").arg(mEntries.size()));
}

// ——— 追加视频直播推流控制记录 ———
void CommandHistoryDialog::appendLivePushCommand(const QString& gatewaySn,
                                                  const QString& method,
                                                  const QString& json) {
    // 中文方法名映射
    static const QMap<QString, QString> METHOD_NAMES = {
        {"live_start_push",   QString::fromUtf8("开始推流")},
        {"live_stop_push",    QString::fromUtf8("停止推流")},
        {"live_set_quality",  QString::fromUtf8("设置清晰度")},
        {"live_lens_change",  QString::fromUtf8("切换镜头类型")},
    };

    QString action = METHOD_NAMES.value(method, method);

    // 将 compact JSON 格式化为缩进显示
    QString formattedJson = json.trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isObject()) {
        formattedJson = QString::fromUtf8(
            doc.toJson(QJsonDocument::Indented));
    }

    QString block;
    block += QStringLiteral("[%1] %2  📡 已下发\n")
        .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), action);
    block += QString::fromUtf8("Topic: thing/product/%1/services\n").arg(gatewaySn);
    block += QString::fromUtf8("下发:\n%1\n").arg(formattedJson);
    block += QString::fromUtf8("响应:\n%1\n").arg(QString::fromUtf8("（无响应）"));
    block += QString::fromUtf8("────────────────────────────\n");

    mEntries.prepend({HistorySource::Video, block});
    while (mEntries.size() > MAX_ENTRIES)
        mEntries.removeLast();

    int filterIdx = mSourceFilter->currentIndex();
    bool sourceMatch = (filterIdx == 0) || (filterIdx == static_cast<int>(HistorySource::Video));
    bool searchMatch = mSearchEdit->text().isEmpty()
                       || block.contains(mSearchEdit->text(), Qt::CaseInsensitive);
    if (sourceMatch && searchMatch) {
        mHistoryEdit->moveCursor(QTextCursor::Start);
        mHistoryEdit->insertPlainText(block);
        mHistoryEdit->moveCursor(QTextCursor::Start);
    }

    mCountLabel->setText(QString::fromUtf8("共 %1 条").arg(mEntries.size()));
}

// ——— 格式化判定结果 ———
QString CommandHistoryDialog::formatVerdict(const DockCommandResult& result) const {
    switch (result.state) {
    case DockCommandState::Succeeded:
        return QString::fromUtf8("✅ 成功 (result=0)");
    case DockCommandState::TimedOut:
        return QString::fromUtf8("❌ 超时");
    default:
        return QString::fromUtf8("❌ 失败（%1）").arg(result.message);
    }
}

// ——— 筛选变更 ———
void CommandHistoryDialog::onFilterChanged(int /*index*/) {
    refreshDisplay();
}

// ——— 搜索文本变更 ———
void CommandHistoryDialog::onSearchChanged(const QString& /*text*/) {
    refreshDisplay();
}

// ——— 清空 ———
void CommandHistoryDialog::onClearClicked() {
    if (mEntries.isEmpty()) return;

    auto ret = QMessageBox::question(this,
        QString::fromUtf8("确认清空"),
        QString::fromUtf8("确定要清空所有下发记录吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    mEntries.clear();
    mHistoryEdit->clear();
    mCountLabel->setText(QString::fromUtf8("共 0 条"));
}

// ——— 根据筛选条件重建显示 ———
void CommandHistoryDialog::refreshDisplay() {
    mHistoryEdit->clear();

    int filterIdx = mSourceFilter->currentIndex();
    QString searchText = mSearchEdit->text();

    int visibleCount = 0;
    for (const auto& entry : mEntries) {
        // 来源筛选
        if (filterIdx != 0 && static_cast<int>(entry.source) != filterIdx)
            continue;
        // 搜索筛选
        if (!searchText.isEmpty()
            && !entry.formattedText.contains(searchText, Qt::CaseInsensitive))
            continue;

        mHistoryEdit->moveCursor(QTextCursor::End);
        mHistoryEdit->insertPlainText(entry.formattedText);
        ++visibleCount;
    }

    // 光标回到顶部
    mHistoryEdit->moveCursor(QTextCursor::Start);
    mCountLabel->setText(QString::fromUtf8("共 %1 条").arg(mEntries.size()));
}

// ——— 导出 ———
void CommandHistoryDialog::onExportClicked() {
    if (mEntries.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("导出"),
            QString::fromUtf8("没有可导出的记录"));
        return;
    }

    QString defaultName = QString::fromUtf8("控制记录_%1.txt")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString filePath = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("导出控制记录"),
        defaultName,
        QString::fromUtf8("文本文件 (*.txt);;所有文件 (*)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("导出失败"),
            QString::fromUtf8("无法写入文件: %1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);

    // 写入头部
    out << QString::fromUtf8("DJI Cloud API 控制记录导出\n");
    out << QString::fromUtf8("导出时间: ") << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << QString::fromUtf8("总记录数: ") << mEntries.size() << "\n";
    out << QString::fromUtf8("============================================================\n\n");

    // 按筛选条件导出（当前过滤 & 搜索）
    int filterIdx = mSourceFilter->currentIndex();
    QString searchText = mSearchEdit->text();

    int exportedCount = 0;
    for (const auto& entry : mEntries) {
        if (filterIdx != 0 && static_cast<int>(entry.source) != filterIdx)
            continue;
        if (!searchText.isEmpty()
            && !entry.formattedText.contains(searchText, Qt::CaseInsensitive))
            continue;

        out << entry.formattedText;
        ++exportedCount;
    }

    file.close();

    QMessageBox::information(this, QString::fromUtf8("导出成功"),
        QString::fromUtf8("已导出 %1 条记录到:\n%2").arg(exportedCount).arg(filePath));
}
