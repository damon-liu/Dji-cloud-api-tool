#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QMap>
#include <QSet>
#include "DeviceInfo.h"

// 视频直播流 URL 配置
struct StreamUrlConfig {
    QMap<QString, QString> aircraft;   // 飞机摄像头 -> URL ("红外相机" -> "rtmp://...")
    QMap<QString, QString> dock;       // 机场视频源 -> URL ("机场外视频" -> "rtmp://...")
};

// 流媒体推流服务器配置
struct StreamMediaConfig {
    QString ip        = "";       // 流媒体服务器 IP
    int     port      = 1935;     // 端口 (RTMP 默认 1935)
    int     protocol  = 1;        // 协议类型: 1=RTMP, 3=GB28181, 4=WebRTC
    QString streamKey = "";       // 流名称（用户自定义），为空时自动使用 video_id
};

// MQTT 连接配置（含流媒体推流服务器）
struct MqttConfig {
    QString host     = "192.168.1.100";
    int     port     = 8883;
    QString username = "admin";
    QString password = "";
    QString clientId = "";   // 为空时使用默认值
    StreamMediaConfig streamMedia;  // 流媒体推流服务器配置
};

// 单个 Profile 的完整数据
struct ProfileData {
    QString                     name;
    MqttConfig                  mqtt;               // MQTT 连接 + 流媒体推流
    StreamUrlConfig             streamUrls;         // 视频直播流 URL
    QVector<DeviceInfo>         devices;
    QMap<QString, QStringList>  deviceTopics;       // SN -> topics (有序)
    QMap<QString, QSet<QString>> disabledTopics;    // SN -> disabled topics
    // 设备推流地址持久化: deviceSn -> {videoId -> pushUrl}
    // 用户点击"开始推流"后写入，下次优先使用已保存地址
    QMap<QString, QMap<QString, QString>> devicePushUrls;
};

// ConfigStore: JSON 配置文件读写，支持多 Profile
class ConfigStore : public QObject {
    Q_OBJECT
public:
    explicit ConfigStore(QObject* parent = nullptr);

    // 加载配置（文件不存在则创建默认）
    bool load(const QString& filePath);
    // 保存配置
    bool save(const QString& filePath);

    // —— Profile 管理 ——
    QStringList profileNames() const;
    QString currentProfileName() const;
    bool setCurrentProfile(const QString& name, bool emitSignal = true);   // 切换到指定 profile（内存中）
    bool addProfile(const QString& name, const MqttConfig& mqtt);
    bool removeProfile(const QString& name, bool emitSignal = true);
    bool renameProfile(const QString& oldName, const QString& newName);

    // —— 当前 Profile 的 MQTT / 设备操作 ——
    MqttConfig mqttConfig() const;
    MqttConfig mqttConfigForProfile(const QString& name) const;
    void setMqttConfig(const MqttConfig& config);
    void setMqttConfigForProfile(const QString& name, const MqttConfig& config);

    QVector<DeviceInfo> devices() const;
    void setDevices(const QVector<DeviceInfo>& devices);
    bool renameDevice(const QString& sn, const QString& newName);

    StreamUrlConfig streamUrls() const;
    void setStreamUrls(const StreamUrlConfig& urls);

    // 设备推流地址持久化
    QString devicePushUrl(const QString& sn, const QString& videoId) const;
    void setDevicePushUrl(const QString& sn, const QString& videoId, const QString& url);

    QStringList topicsForDevice(const QString& sn) const;
    void setTopicsForDevice(const QString& sn, const QStringList& topics);

    QStringList disabledTopicsForDevice(const QString& sn) const;
    void setDisabledTopicsForDevice(const QString& sn, const QStringList& topics);

signals:
    void configChanged();
    void profileSwitched(const QString& profileName);

private:
    QString defaultConfigPath() const;
    ProfileData& currentProfileData();
    const ProfileData& currentProfileData() const;

    QString                     mConfigPath;
    QMap<QString, ProfileData>  mProfiles;
    QString                     mCurrentProfile;
};

#endif // CONFIGSTORE_H
