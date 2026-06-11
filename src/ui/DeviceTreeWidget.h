#ifndef DEVICETREEWIDGET_H
#define DEVICETREEWIDGET_H

#include <QTreeWidget>
#include <QMouseEvent>
#include <QMap>
#include "DeviceInfo.h"

class DeviceTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit DeviceTreeWidget(QWidget* parent = nullptr);

    // 重建整棵树（通常在初始化和设备变更后调用）
    void rebuild(const QVector<DeviceInfo*>& topLevelDevices,
                 const QVector<DeviceInfo*>& allDevices);
    // 获取当前选中的设备 SN
    QString selectedDeviceSn() const;

signals:
    void deviceSelected(const QString& sn);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    // SN -> TreeItem 映射（用于快速查找）
    QMap<QString, QTreeWidgetItem*> mItemMap;
};

#endif // DEVICETREEWIDGET_H
