#include "DeviceTreeWidget.h"
#include <QClipboard>
#include <QApplication>
#include <QContextMenuEvent>

DeviceTreeWidget::DeviceTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setAnimated(true);
    setIndentation(18);
    setFocusPolicy(Qt::StrongFocus);
    setContextMenuPolicy(Qt::DefaultContextMenu);

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
        placeholder->setText(0, QString::fromUtf8("\xef\xbc\x88\xe6\x97\xa0\xe8\xae\xbe\xe5\xa4\x87\xef\xbc\x89"));
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsSelectable);
        placeholder->setForeground(0, QColor(180, 180, 180));
        return;
    }

    for (auto* dev : topLevelDevices) {
        auto* item = new QTreeWidgetItem(this);
        QString status = dev->online ? QString::fromUtf8("\xf0\x9f\x9f\xa2 ")  // 🟢
                                     : QString::fromUtf8("\xf0\x9f\x94\xb4 "); // 🔴
        item->setText(0, status + dev->name + "-" + dev->sn);
        item->setData(0, Qt::UserRole, dev->sn);
        item->setData(0, Qt::UserRole + 1, static_cast<int>(dev->type));
        item->setData(0, Qt::UserRole + 2, dev->name);
        item->setToolTip(0, dev->sn);

        if (!dev->online)
            item->setForeground(0, QColor(180, 180, 180));

        mItemMap[dev->sn] = item;

        for (auto* child : allDevices) {
            if (child->parentSn == dev->sn) {
                auto* childItem = new QTreeWidgetItem(item);
                QString childStatus = child->online ? QString::fromUtf8("\xf0\x9f\x9f\xa2 ")
                                                     : QString::fromUtf8("\xf0\x9f\x94\xb4 ");
                childItem->setText(0, childStatus + child->name + "-" + child->sn);
                childItem->setData(0, Qt::UserRole, child->sn);
                childItem->setData(0, Qt::UserRole + 1, static_cast<int>(child->type));
                childItem->setData(0, Qt::UserRole + 2, child->name);
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
        clearSelection();
        setCurrentItem(nullptr);
        emit deviceSelected("");
    }
    QTreeWidget::mousePressEvent(event);
}

void DeviceTreeWidget::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidgetItem* item = itemAt(event->pos());
    if (!item) return;
    QString sn = item->data(0, Qt::UserRole).toString();
    if (sn.isEmpty()) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #fff; border: 1px solid #e0e0e0; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #e8f0fe; }");

    QAction* renameAct = menu.addAction(QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"));
    QAction* copyAct   = menu.addAction(QString::fromUtf8("\xe5\xa4\x8d\xe5\x88\xb6 SN"));

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == renameAct) {
        QString oldName = item->data(0, Qt::UserRole + 2).toString();
        bool ok;
        QString newName = QInputDialog::getText(this,
            QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d\xe8\xae\xbe\xe5\xa4\x87"),
            QString::fromUtf8("\xe6\x96\xb0\xe5\x90\x8d\xe7\xa7\xb0:"),
            QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.trimmed().isEmpty())
            emit deviceRenameRequested(sn, newName.trimmed());
    } else if (chosen == copyAct) {
        QApplication::clipboard()->setText(sn);
    }
}

void DeviceTreeWidget::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    QString sn = item->data(0, Qt::UserRole).toString();
    if (!sn.isEmpty())
        emit deviceSelected(sn);
}
