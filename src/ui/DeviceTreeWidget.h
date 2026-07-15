#ifndef DEVICETREEWIDGET_H
#define DEVICETREEWIDGET_H

#include <QTreeWidget>
#include <QMouseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QMap>
#include "DeviceInfo.h"

class DeviceTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit DeviceTreeWidget(QWidget* parent = nullptr);

    void rebuild(const QVector<DeviceInfo*>& topLevelDevices,
                 const QVector<DeviceInfo*>& allDevices);
    QString selectedDeviceSn() const;
    void selectDevice(const QString& sn);

signals:
    void deviceSelected(const QString& sn);
    void deviceRenameRequested(const QString& sn, const QString& newName);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    void showContextMenu(const QPoint& pos);
    QMap<QString, QTreeWidgetItem*> mItemMap;
};

#endif // DEVICETREEWIDGET_H
