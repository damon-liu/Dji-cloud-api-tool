#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QMap>
#include "DeviceInfo.h"

// MQTT 连接配置
struct MqttConfig {
    QString host     = "192.168.1.100";
    int     port     = 8883;
    QString username = "admin";
    QString password = "";
};

// ConfigStore: JSON 配置文件读写
class ConfigStore : public QObject {
    Q_OBJECT
public:
    explicit ConfigStore(QObject* parent = nullptr);

    // 加载配置（文件不存在则创建默认）
    bool load(const QString& filePath);

    // 保存配置
    bool save(const QString& filePath);

    // MQTT 连接参数
    MqttConfig mqttConfig() const;
    void setMqttConfig(const MqttConfig& config);

    // 设备列表
    QVector<DeviceInfo> devices() const;
    void setDevices(const QVector<DeviceInfo>& devices);

    // 获取设备的所有 topic（通过 SN）
    QStringList topicsForDevice(const QString& sn) const;
    void setTopicsForDevice(const QString& sn, const QStringList& topics);

signals:
    void configChanged();

private:
    QString defaultConfigPath() const;

    MqttConfig           mMqttConfig;
    QVector<DeviceInfo>  mDevices;
    QMap<QString, QStringList> mDeviceTopics;  // SN -> topics
};

#endif // CONFIGSTORE_H
