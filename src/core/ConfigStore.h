#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QMap>
#include <QSet>
#include "DeviceInfo.h"

// MQTT 连接配置
struct MqttConfig {
    QString host     = "192.168.1.100";
    int     port     = 8883;
    QString username = "admin";
    QString password = "";
};

// 单个 Profile 的完整数据
struct ProfileData {
    QString                     name;
    MqttConfig                  mqtt;
    QVector<DeviceInfo>         devices;
    QMap<QString, QStringList>  deviceTopics;      // SN -> topics (有序)
    QMap<QString, QSet<QString>> disabledTopics;    // SN -> disabled topics
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
    bool setCurrentProfile(const QString& name);   // 切换到指定 profile（内存中）
    bool addProfile(const QString& name, const MqttConfig& mqtt);
    bool removeProfile(const QString& name);
    bool renameProfile(const QString& oldName, const QString& newName);

    // —— 当前 Profile 的 MQTT / 设备操作 ——
    MqttConfig mqttConfig() const;
    MqttConfig mqttConfigForProfile(const QString& name) const;
    void setMqttConfig(const MqttConfig& config);
    void setMqttConfigForProfile(const QString& name, const MqttConfig& config);

    QVector<DeviceInfo> devices() const;
    void setDevices(const QVector<DeviceInfo>& devices);
    bool renameDevice(const QString& sn, const QString& newName);

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
