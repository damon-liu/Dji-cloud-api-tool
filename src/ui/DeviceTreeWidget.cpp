#include "DeviceTreeWidget.h"
#include <QHeaderView>

DeviceTreeWidget::DeviceTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabels({"设备"});
    header()->setStretchLastSection(true);
    setRootIsDecorated(true);
    setAnimated(true);
    setIndentation(18);
    setFocusPolicy(Qt::StrongFocus);

    connect(this, &QTreeWidget::itemClicked,
            this, &DeviceTreeWidget::onItemClicked);
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
        QString icon = (dev->type == DeviceType::Dock)
            ? QString::fromUtf8("🏢 ") : QString::fromUtf8("✈ ");
        item->setText(0, icon + dev->name);
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
                childItem->setText(0, QString::fromUtf8("✈ ") + child->name);
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
