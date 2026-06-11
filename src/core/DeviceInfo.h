#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>
#include <QStringList>
#include <QJsonObject>

// 设备类型
enum class DeviceType {
    Dock,       // 机场/机库
    Aircraft    // 飞机（手飞 + 库内）
};

// 设备描述数据结构
struct DeviceInfo {
    QString     sn;             // 设备序列号（唯一标识）
    QString     name;           // 用户自定义名称
    DeviceType  type;           // 设备类型
    QString     parentSn;       // 父设备 SN（库内飞机指向机场，手飞为空）
    bool        online = false; // 在线状态

    // 序列化为 JSON（保存配置用）
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["sn"] = sn;
        obj["name"] = name;
        obj["type"] = (type == DeviceType::Dock) ? "dock" : "aircraft";
        if (!parentSn.isEmpty())
            obj["parent_sn"] = parentSn;
        return obj;
    }

    // 从 JSON 反序列化
    static DeviceInfo fromJson(const QJsonObject& obj) {
        DeviceInfo info;
        info.sn = obj["sn"].toString();
        info.name = obj["name"].toString();
        QString typeStr = obj["type"].toString();
        info.type = (typeStr == "dock") ? DeviceType::Dock : DeviceType::Aircraft;
        info.parentSn = obj.value("parent_sn").toString();
        return info;
    }

    // 是否属于某个机场（parentSn 不为空即为库内飞机）
    bool isChild() const { return !parentSn.isEmpty(); }
};

#endif // DEVICEINFO_H
