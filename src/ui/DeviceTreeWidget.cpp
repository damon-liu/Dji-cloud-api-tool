#include "DeviceTreeWidget.h"
#include <QClipboard>
#include <QApplication>

DeviceTreeWidget::DeviceTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setAnimated(true);
    setIndentation(18);
    setFocusPolicy(Qt::StrongFocus);

    connect(this, &QTreeWidget::itemClicked,
            this, &DeviceTreeWidget::onItemClicked);

    connect(this, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem* item, int) {
        QString sn = item->data(0, Qt::UserRole).toString();
        if (!sn.isEmpty())
            QApplication::clipboard()->setText(sn);
    });
}

void DeviceTreeWidget::rebuild(const QVector<DeviceInfo*>& topLevelDevices,
                                 const QVector<DeviceInfo*>& allDevices) {
    clear();
    mItemMap.clear();

    if (topLevelDevices.isEmpty()) {
        auto* placeholder = new QTreeWidgetItem(this);
        placeholder->setText(0, "（无设备）");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsSelectable);
        placeholder->setForeground(0, QColor(180, 180, 180));
        return;
    }

    for (auto* dev : topLevelDevices) {
        auto* item = new QTreeWidgetItem(this);
        QString status = dev->online ? QString::fromUtf8("\xf0\x9f\x9f\xa2 ")  // 🟢
                                     : QString::fromUtf8("\xf0\x9f\x94\xb4 "); // 🔴
        QString icon = (dev->type == DeviceType::Dock)
            ? QString::fromUtf8("\xf0\x9f\x8f\xa2 ") : QString::fromUtf8("\xe2\x9c\x88 ");
        item->setText(0, status + icon + dev->name + "  " + dev->sn);
        item->setData(0, Qt::UserRole, dev->sn);
        item->setData(0, Qt::UserRole + 1, static_cast<int>(dev->type));
        item->setToolTip(0, dev->sn);

        if (!dev->online)
            item->setForeground(0, QColor(180, 180, 180));

        mItemMap[dev->sn] = item;

        // 子设备
        for (auto* child : allDevices) {
            if (child->parentSn == dev->sn) {
                auto* childItem = new QTreeWidgetItem(item);
                QString childStatus = child->online ? QString::fromUtf8("\xf0\x9f\x9f\xa2 ")
                                                     : QString::fromUtf8("\xf0\x9f\x94\xb4 ");
                childItem->setText(0, childStatus + QString::fromUtf8("\xe2\x9c\x88 ")
                                   + child->name + "  " + child->sn);
                childItem->setData(0, Qt::UserRole, child->sn);
                childItem->setData(0, Qt::UserRole + 1, static_cast<int>(child->type));
                childItem->setToolTip(0, child->sn);

                if (!child->online)
                    childItem->setForeground(0, QColor(180, 180, 180));

                mItemMap[child->sn] = childItem;
            }
        }
    }

    expandAll();
}

QString DeviceTreeWidget::selectedDeviceSn() const {
    auto* item = currentItem();
    if (!item) return {};
    return item->data(0, Qt::UserRole).toString();
}

void DeviceTreeWidget::mousePressEvent(QMouseEvent* event) {
    QTreeWidgetItem* item = itemAt(event->pos());
    if (!item) {
        // 点击空白区域：取消选中
        clearSelection();
        setCurrentItem(nullptr);
        emit deviceSelected("");
    }
    QTreeWidget::mousePressEvent(event);
}

void DeviceTreeWidget::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    QString sn = item->data(0, Qt::UserRole).toString();
    if (!sn.isEmpty())
        emit deviceSelected(sn);
}
