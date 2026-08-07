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

// 视频直播状态信息（从 OSD live_status[] 解析）
struct LiveStatusInfo {
    QString videoId;       // "1ZNDH1D0010098/39-0-7/normal-0"
    int     videoQuality = 0;  // 0=自适应, 1=流畅, 2=标清, 3=高清, 4=超清
    QString videoType;     // "normal", "wide", "zoom", "ir"
    int     status = 0;        // 0=未直播, 1=在直播
    int     errorStatus = 0;   // 0=正常, 非0=错误
    QString deviceSn;      // 所属设备 SN（从 topic 路径解析）

    bool operator==(const LiveStatusInfo& other) const {
        return videoId == other.videoId
            && videoQuality == other.videoQuality
            && videoType == other.videoType
            && status == other.status
            && errorStatus == other.errorStatus
            && deviceSn == other.deviceSn;
    }

    bool operator!=(const LiveStatusInfo& other) const {
        return !(*this == other);
    }
};

// 用于 QVector<LiveStatusInfo> 的相等比较
inline bool operator==(const QVector<LiveStatusInfo>& a, const QVector<LiveStatusInfo>& b) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

inline bool operator!=(const QVector<LiveStatusInfo>& a, const QVector<LiveStatusInfo>& b) {
    return !(a == b);
}

#endif // DEVICEINFO_H
