#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QTimer>
#include "DeviceInfo.h"
#include "OsdData.h"
#include "ConfigStore.h"
#include "TopicManager.h"
#include "MqttClientManager.h"
#include "DockCommandExecutor.h"

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
    void renameDevice(const QString& sn, const QString& newName);
    DeviceInfo* device(const QString& sn);
    QVector<DeviceInfo*> allDevices();
    QVector<DeviceInfo*> topLevelDevices();  // 顶级设备（机场 + 独立手飞）

    // Topic 管理
    QStringList topicsForDevice(const QString& sn) const;
    // 根据 topic 反查所属设备 SN
    QString deviceForTopic(const QString& topic) const;
    void addTopic(const QString& deviceSn, const QString& topic);
    void removeTopic(const QString& deviceSn, const QString& topic);
    void updateTopic(const QString& deviceSn, const QString& oldTopic, const QString& newTopic);

    // 重排设备 topic 顺序
    void reorderTopics(const QString& deviceSn, const QStringList& orderedTopics);

    // Topic 启用/禁用控制
    void setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled);
    void setAllTopicsEnabled(const QString& deviceSn, bool enabled);
    bool isTopicEnabled(const QString& deviceSn, const QString& topic) const;

    // OSD 缓存

    // OSD 缓存
    const AircraftOsd* latestAircraftOsd(const QString& sn) const;
    const DockOsd* latestDockOsd(const QString& sn) const;
    QString latestRawJson(const QString& sn, const QString& topic = QString()) const;

    // OSD 合并数据（字段级合并，解决 DJI 分消息推送问题）
    QString mergedOsdJson(const QString& sn, const QString& topic) const;
    void clearMergedOsdData(const QString& sn, const QString& topic = QString());

    // JSON 历史数据
    QString jsonHistory(const QString& sn, const QString& topic = {}) const;
    void clearJsonHistory(const QString& sn, const QString& topic = {});

    // 配置访问
    MqttConfig mqttConfig() const;
    MqttConfig mqttConfigForProfile(const QString& name) const;
    void setMqttConfig(const MqttConfig& cfg);
    void setMqttConfigForProfile(const QString& name, const MqttConfig& cfg);

    // 保存配置
    bool saveConfig(const QString& path);

    // Profile 管理（代理到 ConfigStore）
    QStringList profileNames() const;
    QString currentProfileName() const;
    void switchToProfile(const QString& name);
    bool addProfile(const QString& name, const MqttConfig& mqtt);
    bool removeProfile(const QString& name);
    bool renameProfile(const QString& oldName, const QString& newName);

public slots:
    void publishMessage(const QString& topic, const QString& json);
    void executeDockCommand(const QString& gatewaySn, DockCommandType type,
                            const QJsonObject& data = {});

signals:
    void brokerConnected();
    void brokerDisconnected();
    void brokerError(const QString& error);
    void deviceAdded(const QString& sn);
    void deviceRemoved(const QString& sn);
    void deviceOsdUpdated(const QString& sn, const QString& topic, const QString& rawJson);
    void deviceOnlineChanged(const QString& sn, bool online);
    void profileSwitched(const QString& name);
    void profileListChanged();
    void publishResult(const QString& topic, bool success, const QString& message);
    void dockCommandStateChanged(const DockCommandResult& result);

private slots:
    void onPublishCompleted(const QString& topic, bool success, const QString& message);
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttMessage(const QString& topic, const QByteArray& payload);
    void onTopicsChanged(const QStringList& add, const QStringList& remove);
    void checkDeviceOffline();

private:
    void parseAndRoute(const QString& topic, const QByteArray& payload);

    ConfigStore*               mConfigStore;
    TopicManager*              mTopicManager;
    MqttClientManager*         mMqttManager;
    DockCommandExecutor*       mDockCmdExecutor;
    QMap<QString, DeviceInfo>  mDevices;
    QMap<QString, AircraftOsd> mAircraftOsdCache;
    QMap<QString, DockOsd>     mDockOsdCache;
    QMap<QString, QMap<QString, QString>> mRawJsonCache;  // sn → topic → json
    QMap<QString, QMap<QString, QJsonObject>> mMergedOsdData;  // sn → topic → merged data fields
    QMap<QString, QMap<QString, QStringList>> mJsonHistory;  // SN → topic → history[]
    QMap<QString, qint64>       mLastMessageTime;  // SN → 最后收到消息的时间戳
    QTimer*                     mOfflineTimer = nullptr;
    QString                     mConfigPath;
    static constexpr int MAX_JSON_HISTORY = 500;
    static constexpr qint64 OFFLINE_TIMEOUT_MS = 5000;   // 5s 无消息判定离线
};

#endif // DEVICEMANAGER_H
