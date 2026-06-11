#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include "DeviceInfo.h"
#include "OsdData.h"
#include "ConfigStore.h"
#include "TopicManager.h"
#include "MqttClientManager.h"

// DeviceManager: 系统核心，协调所有模块
class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(QObject* parent = nullptr);

    // 初始化（加载配置）
    bool initialize(const QString& configPath);

    // MQTT 连接控制
    void connectBroker();
    void disconnectBroker();
    bool isConnected() const;

    // 设备管理
    void addDevice(const DeviceInfo& info, const QStringList& topics);
    void removeDevice(const QString& sn);
    DeviceInfo* device(const QString& sn);
    QVector<DeviceInfo*> allDevices();
    QVector<DeviceInfo*> topLevelDevices();  // 顶级设备（机场 + 独立手飞）

    // Topic 管理
    QStringList topicsForDevice(const QString& sn) const;
    void addTopic(const QString& deviceSn, const QString& topic);
    void removeTopic(const QString& deviceSn, const QString& topic);
    void updateTopic(const QString& deviceSn, const QString& oldTopic, const QString& newTopic);

    // OSD 缓存
    const AircraftOsd* latestAircraftOsd(const QString& sn) const;
    const DockOsd* latestDockOsd(const QString& sn) const;
    QString latestRawJson(const QString& sn) const;

    // 配置访问
    MqttConfig mqttConfig() const;
    void setMqttConfig(const MqttConfig& cfg);

    // 保存配置
    bool saveConfig(const QString& path);

signals:
    void brokerConnected();
    void brokerDisconnected();
    void brokerError(const QString& error);
    void deviceAdded(const QString& sn);
    void deviceRemoved(const QString& sn);
    void deviceOsdUpdated(const QString& sn, const QString& rawJson);
    void deviceOnlineChanged(const QString& sn, bool online);

private slots:
    void onMqttConnected();
    void onMqttMessage(const QString& topic, const QByteArray& payload);
    void onTopicsChanged(const QStringList& add, const QStringList& remove);

private:
    void parseAndRoute(const QString& topic, const QByteArray& payload);

    ConfigStore*               mConfigStore;
    TopicManager*              mTopicManager;
    MqttClientManager*         mMqttManager;
    QMap<QString, DeviceInfo>  mDevices;
    QMap<QString, AircraftOsd> mAircraftOsdCache;
    QMap<QString, DockOsd>     mDockOsdCache;
    QMap<QString, QString>     mRawJsonCache;
    QString                    mConfigPath;
};

#endif // DEVICEMANAGER_H
