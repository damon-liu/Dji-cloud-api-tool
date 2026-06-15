#include "DeviceManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent)
    , mConfigStore(new ConfigStore(this))
    , mTopicManager(new TopicManager(this))
    , mMqttManager(new MqttClientManager(this))
{
    // 转发 MQTT 信号
    connect(mMqttManager, &MqttClientManager::connected,
            this, &DeviceManager::onMqttConnected);
    connect(mMqttManager, &MqttClientManager::disconnected,
            this, &DeviceManager::brokerDisconnected);
    connect(mMqttManager, &MqttClientManager::connectionError,
            this, &DeviceManager::brokerError);
    connect(mMqttManager, &MqttClientManager::messageReceived,
            this, &DeviceManager::onMqttMessage);

    // Topic 变更 → MQTT 重新订阅
    connect(mTopicManager, &TopicManager::topicsChanged,
            this, &DeviceManager::onTopicsChanged);
}

bool DeviceManager::initialize(const QString& configPath) {
    mConfigPath = configPath;
    if (!mConfigStore->load(configPath))
        return false;

    // 加载设备到内存
    for (const auto& info : mConfigStore->devices()) {
        mDevices[info.sn] = info;
        QStringList topics = mConfigStore->topicsForDevice(info.sn);
        mTopicManager->setDeviceTopics(info.sn, topics);

        // 加载禁用 topic 状态
        QStringList disabled = mConfigStore->disabledTopicsForDevice(info.sn);
        if (!disabled.isEmpty()) {
            mTopicManager->setDisabledTopicsForDevice(
                info.sn,
                QSet<QString>(disabled.begin(), disabled.end()));
        }
    }

    qDebug() << "DeviceManager: initialized with" << mDevices.size() << "devices";
    return true;
}

void DeviceManager::connectBroker() {
    mMqttManager->connectToBroker(mConfigStore->mqttConfig());
}

void DeviceManager::disconnectBroker() {
    mMqttManager->disconnectFromBroker();
}

bool DeviceManager::isConnected() const {
    return mMqttManager->isConnected();
}

void DeviceManager::addDevice(const DeviceInfo& info, const QStringList& topics) {
    mDevices[info.sn] = info;
    mTopicManager->setDeviceTopics(info.sn, topics);

    // 持久化
    QVector<DeviceInfo> devs;
    for (const auto& d : mDevices)
        devs.append(d);
    mConfigStore->setDevices(devs);
    saveConfig(mConfigPath);

    emit deviceAdded(info.sn);
}

void DeviceManager::removeDevice(const QString& sn) {
    mDevices.remove(sn);
    mTopicManager->removeDevice(sn);
    mAircraftOsdCache.remove(sn);
    mDockOsdCache.remove(sn);
    mRawJsonCache.remove(sn);

    // 如果该设备是机场，同时删除子飞机
    QList<QString> childSns;
    for (const auto& d : mDevices) {
        if (d.parentSn == sn)
            childSns.append(d.sn);
    }
    for (const auto& child : childSns) {
        mDevices.remove(child);
        mTopicManager->removeDevice(child);
        mAircraftOsdCache.remove(child);
        mDockOsdCache.remove(child);
        mRawJsonCache.remove(child);
    }

    // 持久化
    QVector<DeviceInfo> devs;
    for (const auto& d : mDevices)
        devs.append(d);
    mConfigStore->setDevices(devs);
    saveConfig(mConfigPath);

    emit deviceRemoved(sn);
}

DeviceInfo* DeviceManager::device(const QString& sn) {
    if (mDevices.contains(sn))
        return &mDevices[sn];
    return nullptr;
}

QVector<DeviceInfo*> DeviceManager::allDevices() {
    QVector<DeviceInfo*> result;
    for (auto& d : mDevices)
        result.append(&d);
    return result;
}

QVector<DeviceInfo*> DeviceManager::topLevelDevices() {
    QVector<DeviceInfo*> result;
    for (auto& d : mDevices) {
        if (!d.isChild())
            result.append(&d);
    }
    return result;
}

QStringList DeviceManager::topicsForDevice(const QString& sn) const {
    return mTopicManager->topicsForDevice(sn);
}

void DeviceManager::addTopic(const QString& deviceSn, const QString& topic) {
    mTopicManager->addTopic(deviceSn, topic);
    // 同步到 ConfigStore
    mConfigStore->setTopicsForDevice(deviceSn, mTopicManager->topicsForDevice(deviceSn));
    saveConfig(mConfigPath);
}

void DeviceManager::removeTopic(const QString& deviceSn, const QString& topic) {
    mTopicManager->removeTopic(deviceSn, topic);
    mConfigStore->setTopicsForDevice(deviceSn, mTopicManager->topicsForDevice(deviceSn));
    saveConfig(mConfigPath);
}

void DeviceManager::updateTopic(const QString& deviceSn,
                                  const QString& oldTopic,
                                  const QString& newTopic) {
    mTopicManager->updateTopic(deviceSn, oldTopic, newTopic);
    mConfigStore->setTopicsForDevice(deviceSn, mTopicManager->topicsForDevice(deviceSn));
    saveConfig(mConfigPath);
}

const AircraftOsd* DeviceManager::latestAircraftOsd(const QString& sn) const {
    auto it = mAircraftOsdCache.find(sn);
    if (it != mAircraftOsdCache.end())
        return &*it;
    return nullptr;
}

const DockOsd* DeviceManager::latestDockOsd(const QString& sn) const {
    auto it = mDockOsdCache.find(sn);
    if (it != mDockOsdCache.end())
        return &*it;
    return nullptr;
}

QString DeviceManager::latestRawJson(const QString& sn, const QString& topic) const {
    if (!mRawJsonCache.contains(sn))
        return {};
    const auto& topicMap = mRawJsonCache[sn];
    if (!topic.isEmpty() && topicMap.contains(topic))
        return topicMap[topic];
    // topic 为空时返回该设备任意一条缓存（兼容旧调用）
    if (!topicMap.isEmpty())
        return topicMap.first();
    return {};
}

QString DeviceManager::jsonHistory(const QString& sn, const QString& topic) const {
    if (!mJsonHistory.contains(sn))
        return {};
    if (topic.isEmpty()) {
        // 空 topic → 返回所有 topic 的合并历史
        QStringList all;
        const auto& topicMap = mJsonHistory[sn];
        for (auto it = topicMap.begin(); it != topicMap.end(); ++it)
            all.append(it.value());
        return all.join("\n---\n");
    }
    return mJsonHistory[sn].value(topic).join("\n---\n");
}

void DeviceManager::clearJsonHistory(const QString& sn, const QString& topic) {
    if (topic.isEmpty())
        mJsonHistory.remove(sn);
    else if (mJsonHistory.contains(sn))
        mJsonHistory[sn].remove(topic);
}

MqttConfig DeviceManager::mqttConfig() const {
    return mConfigStore->mqttConfig();
}

void DeviceManager::setMqttConfig(const MqttConfig& cfg) {
    mConfigStore->setMqttConfig(cfg);
    saveConfig(mConfigPath);
}

bool DeviceManager::saveConfig(const QString& path) {
    // 同步设备列表和 topics 到 ConfigStore
    QVector<DeviceInfo> devs;
    for (const auto& d : mDevices)
        devs.append(d);
    mConfigStore->setDevices(devs);

    for (const auto& d : mDevices) {
        mConfigStore->setTopicsForDevice(d.sn, mTopicManager->topicsForDevice(d.sn));
    }

    return mConfigStore->save(path);
}

void DeviceManager::setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled) {
    mTopicManager->setTopicEnabled(deviceSn, topic, enabled);
    // 持久化到 ConfigStore
    QStringList disabled = mTopicManager->disabledTopicsForDevice(deviceSn).values();
    mConfigStore->setDisabledTopicsForDevice(deviceSn, disabled);
    saveConfig(mConfigPath);
}

bool DeviceManager::isTopicEnabled(const QString& deviceSn, const QString& topic) const {
    return mTopicManager->isTopicEnabled(deviceSn, topic);
}

// ——— 私有槽 ———

void DeviceManager::onMqttConnected() {
    // 连接成功后订阅所有启用的 topic
    QStringList all = mTopicManager->allEnabledTopics();
    mMqttManager->subscribeTopics(all);
    emit brokerConnected();
}

void DeviceManager::onMqttMessage(const QString& topic, const QByteArray& payload) {
    parseAndRoute(topic, payload);
}

void DeviceManager::onTopicsChanged(const QStringList& add, const QStringList& remove) {
    mMqttManager->replaceSubscriptions(add, remove);
}

void DeviceManager::parseAndRoute(const QString& topic, const QByteArray& payload) {
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        qWarning() << "DeviceManager: invalid JSON on topic" << topic;
        return;
    }

    QJsonObject root = doc.object();
    // DJI 消息体: {"tid": "...", "timestamp": ..., "gateway": "...", "data": {...}}
    QJsonObject data = root.value("data").toObject();

    // 根据 topic 找到设备
    QString sn = mTopicManager->deviceForTopic(topic);
    if (!mDevices.contains(sn)) {
        qWarning() << "DeviceManager: unknown device for topic" << topic;
        return;
    }

    // 保存原始 JSON（最新一条）
    QString formatted = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    mRawJsonCache[sn][topic] = formatted;

    // 累积 JSON 历史（最多 500 条）
    mJsonHistory[sn][topic].append(formatted);
    while (mJsonHistory[sn][topic].size() > MAX_JSON_HISTORY)
        mJsonHistory[sn][topic].removeFirst();

    // 解析 OSD 数据
    DeviceInfo& info = mDevices[sn];
    if (topic.contains("/osd")) {
        if (info.type == DeviceType::Aircraft) {
            AircraftOsd osd = AircraftOsd::fromJson(data);
            mAircraftOsdCache[sn] = osd;
        } else if (info.type == DeviceType::Dock) {
            DockOsd osd = DockOsd::fromJson(data);
            mDockOsdCache[sn] = osd;
        }
    }

    // 如果之前是 offline，切换为 online
    if (!info.online) {
        info.online = true;
        emit deviceOnlineChanged(sn, true);
    }

    emit deviceOsdUpdated(sn, topic, formatted);
}
