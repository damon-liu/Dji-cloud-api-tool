#include "TopicListWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

TopicListWidget::TopicListWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // 标题 + 操作按钮
    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(4);

    mTitleLabel = new QLabel("Topic 列表", this);
    mTitleLabel->setObjectName("sectionTitle");
    titleRow->addWidget(mTitleLabel);
    titleRow->addStretch();

    mAddBtn = new QPushButton("＋", this);
    mAddBtn->setCursor(Qt::PointingHandCursor);
    mAddBtn->setFixedSize(28, 28);
    mAddBtn->setToolTip("添加 Topic");
    mAddBtn->setStyleSheet(
        "QPushButton { background: #e8f5e9; color: #2e7d32; border: 1px solid #a5d6a7; "
        "border-radius: 4px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #c8e6c9; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    titleRow->addWidget(mAddBtn);

    mToggleBtn = new QPushButton("◎", this);
    mToggleBtn->setCursor(Qt::PointingHandCursor);
    mToggleBtn->setFixedSize(28, 28);
    mToggleBtn->setToolTip("启用/禁用 Topic");
    mToggleBtn->setEnabled(false);
    mToggleBtn->setStyleSheet(
        "QPushButton { background: #fff3e0; color: #e65100; border: 1px solid #ffcc80; "
        "border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background: #ffe0b2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    titleRow->addWidget(mToggleBtn);

    mRemoveBtn = new QPushButton("✕", this);
    mRemoveBtn->setCursor(Qt::PointingHandCursor);
    mRemoveBtn->setFixedSize(28, 28);
    mRemoveBtn->setToolTip("删除 Topic");
    mRemoveBtn->setEnabled(false);
    mRemoveBtn->setStyleSheet(
        "QPushButton { background: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
        "border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #ffcdd2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    titleRow->addWidget(mRemoveBtn);

    layout->addLayout(titleRow);

    // Topic 列表（占满宽度）
    mTopicList = new QListWidget(this);
    mTopicList->setMinimumWidth(310);
    mTopicList->setMaximumWidth(440);
    mTopicList->setMaximumHeight(640);
    mTopicList->setStyleSheet(
        "QListWidget { background: #ffffff; border: 1px solid #e0e0e0; border-radius: 4px; "
        "font-size: 12px; }"
        "QListWidget::item { padding: 3px 6px; }"
        "QListWidget::item:selected { background: #e8f0fe; color: #1a73e8; }");
    layout->addWidget(mTopicList, 1);

    // 信号连接
    connect(mAddBtn, &QPushButton::clicked, this, &TopicListWidget::onAddTopic);
    connect(mToggleBtn, &QPushButton::clicked, this, &TopicListWidget::onToggleTopic);
    connect(mRemoveBtn, &QPushButton::clicked, this, &TopicListWidget::onRemoveTopic);
    connect(mTopicList, &QListWidget::itemSelectionChanged,
            this, &TopicListWidget::onTopicSelectionChanged);

    // 初始状态：无设备选中
    clearTopics();
}

void TopicListWidget::setTopics(const QString& sn,
                                const QStringList& topics,
                                const QSet<QString>& disabledTopics) {
    mCurrentSn = sn;
    mAllTopics = topics;
    mDisabledTopics = disabledTopics;
    refreshList();
}

void TopicListWidget::clearTopics() {
    mCurrentSn.clear();
    mAllTopics.clear();
    mDisabledTopics.clear();
    mTopicList->clear();
    mAddBtn->setEnabled(false);
    mToggleBtn->setEnabled(false);
    mRemoveBtn->setEnabled(false);

    if (mAllTopics.isEmpty() && mCurrentSn.isEmpty()) {
        mTopicList->addItem("（请选择设备）");
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
    }
}

QString TopicListWidget::selectedTopic() const {
    auto* item = mTopicList->currentItem();
    if (!item) return {};
    // 跳过 placeholder 行
    QString text = item->text();
    if (text.startsWith("（"))
        return {};
    // 移除 ●/○ 前缀（前缀 emoji + space = 2 个 UTF-16 字符）
    return text.mid(2);
}

void TopicListWidget::refreshList() {
    // 保存当前选中（重建前），跳过 placeholder 项
    QString saved = selectedTopic();
    mTopicList->clear();
    mAddBtn->setEnabled(true);

    if (mCurrentSn.isEmpty()) {
        mTopicList->addItem("（请选择设备）");
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
        mAddBtn->setEnabled(false);
        return;
    }

    if (mAllTopics.isEmpty()) {
        mTopicList->addItem("（无 Topic）");
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
        return;
    }

    for (const auto& t : mAllTopics) {
        bool enabled = !mDisabledTopics.contains(t);
        QString prefix = enabled ? QString::fromUtf8("● ") : QString::fromUtf8("○ ");
        auto* item = new QListWidgetItem(prefix + t);
        item->setData(Qt::UserRole, t);  // 存储原始 topic 字符串
        if (!enabled)
            item->setForeground(QColor(180, 180, 180));
        mTopicList->addItem(item);
    }

    // 恢复之前用户选中的 topic，若不存在则选第一项
    if (!saved.isEmpty()) {
        for (int i = 0; i < mTopicList->count(); ++i) {
            if (mTopicList->item(i)->data(Qt::UserRole).toString() == saved) {
                mTopicList->setCurrentRow(i);
                return;
            }
        }
    }
    if (mTopicList->count() > 0)
        mTopicList->setCurrentRow(0);
}

void TopicListWidget::onAddTopic() {
    if (mCurrentSn.isEmpty()) return;

    QString defaultTopic = QString("thing/product/%1/osd").arg(mCurrentSn);
    QString topic = QInputDialog::getText(this, "添加 Topic",
        "输入 MQTT Topic 字符串:", QLineEdit::Normal, defaultTopic);
    if (topic.trimmed().isEmpty()) return;

    // 检查重复
    QString finalTopic = topic.trimmed();
    if (mAllTopics.contains(finalTopic)) {
        QMessageBox::information(this, "提示", "该 Topic 已存在。");
        return;
    }

    emit topicAdded(mCurrentSn, finalTopic);
}

void TopicListWidget::onToggleTopic() {
    QString topic = selectedTopic();
    if (topic.isEmpty() || mCurrentSn.isEmpty()) return;
    emit topicToggled(mCurrentSn, topic);
}

void TopicListWidget::onRemoveTopic() {
    QString topic = selectedTopic();
    if (topic.isEmpty() || mCurrentSn.isEmpty()) return;

    auto ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除 Topic「%1」吗？").arg(topic),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes)
        emit topicRemoved(mCurrentSn, topic);
}

void TopicListWidget::onTopicSelectionChanged() {
    bool hasSelection = !selectedTopic().isEmpty();
    mToggleBtn->setEnabled(hasSelection);
    mRemoveBtn->setEnabled(hasSelection);
    emit topicSelectionChanged(hasSelection ? selectedTopic() : QString());
}
