#include "DeviceManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>

static const QStringList DEFAULT_DOCK_TOPICS = {
    "thing/product/{sn}/state",
    "thing/product/{sn}/requests",
    "thing/product/{sn}/events",
    "thing/product/{sn}/services_reply",
    "thing/product/{sn}/property/set_reply",
    "sys/product/{sn}/status",
    "thing/product/{sn}/drc/up",
};

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
            this, &DeviceManager::onMqttDisconnected);
    connect(mMqttManager, &MqttClientManager::connectionError,
            this, &DeviceManager::brokerError);
    connect(mMqttManager, &MqttClientManager::messageReceived,
            this, &DeviceManager::onMqttMessage);

    // Topic 变更 → MQTT 重新订阅
    connect(mTopicManager, &TopicManager::topicsChanged,
            this, &DeviceManager::onTopicsChanged);

    // Profile 切换信号转发
    connect(mConfigStore, &ConfigStore::profileSwitched,
            this, &DeviceManager::profileSwitched);

    // Publish 结果转发（过滤 DockCommandExecutor 发起的 topic，
    // 避免同一条指令在 Topic 下发记录中重复显示）
    connect(mMqttManager, &MqttClientManager::publishCompleted,
            this, &DeviceManager::onPublishCompleted);

    // 机场控制指令执行器
    mDockCmdExecutor = new DockCommandExecutor(mMqttManager, this);
    connect(mDockCmdExecutor, &DockCommandExecutor::commandStateChanged,
            this, &DeviceManager::dockCommandStateChanged);

    // 设备离线检测：每 2 秒检查一次
    mOfflineTimer = new QTimer(this);
    mOfflineTimer->setInterval(2000);
    connect(mOfflineTimer, &QTimer::timeout, this, &DeviceManager::checkDeviceOffline);
    mOfflineTimer->start();
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

        // 加载禁用 topic 状态，并取消订阅已禁用的 topic
        QStringList disabled = mConfigStore->disabledTopicsForDevice(info.sn);
        if (!disabled.isEmpty()) {
            mTopicManager->setDisabledTopicsForDevice(
                info.sn,
                QSet<QString>(disabled.begin(), disabled.end()));
            // 取消订阅禁用 topic（setDeviceTopics 已全部订阅）
            mTopicManager->unsubscribeTopics(disabled);
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

    if (info.type == DeviceType::Dock) {
        // 机场设备：追加 7 个默认 topic，默认禁用
        QStringList extendedTopics = topics;
        QSet<QString> newDisabled;
        for (const auto& tpl : DEFAULT_DOCK_TOPICS) {
            QString topic = QString(tpl).replace("{sn}", info.sn);
            if (!extendedTopics.contains(topic)) {
                extendedTopics.append(topic);
                newDisabled.insert(topic);
            }
        }
        mTopicManager->setDeviceTopics(info.sn, extendedTopics);
        // 将已有禁用记录与新增默认禁用的 topic 合并
        QSet<QString> existingDisabled = mTopicManager->disabledTopicsForDevice(info.sn);
        existingDisabled.unite(newDisabled);
        mTopicManager->setDisabledTopicsForDevice(info.sn, existingDisabled);
        // 立即取消订阅新增的禁用 topic（setDeviceTopics 已将其全部订阅）
        QStringList disabledList = newDisabled.values();
        if (!disabledList.isEmpty())
            mTopicManager->unsubscribeTopics(disabledList);
    } else {
        mTopicManager->setDeviceTopics(info.sn, topics);
    }

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
    mMergedOsdData.remove(sn);

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
        mMergedOsdData.remove(child);
    }

    // 持久化
    QVector<DeviceInfo> devs;
    for (const auto& d : mDevices)
        devs.append(d);
    mConfigStore->setDevices(devs);
    saveConfig(mConfigPath);

    emit deviceRemoved(sn);
}

void DeviceManager::renameDevice(const QString& sn, const QString& newName) {
    if (!mDevices.contains(sn)) return;
    mDevices[sn].name = newName;
    mConfigStore->renameDevice(sn, newName);
    saveConfig(mConfigPath);
    emit deviceAdded(sn);  // 复用 deviceAdded 信号触发 UI 重建
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

QString DeviceManager::deviceForTopic(const QString& topic) const {
    return mTopicManager->deviceForTopic(topic);
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
    if (topic.isEmpty()) {
        // topic 为空时返回该设备任意一条缓存（兼容旧调用）
        if (!topicMap.isEmpty())
            return topicMap.first();
        return {};
    }
    return topicMap.value(topic);
}

QString DeviceManager::mergedOsdJson(const QString& sn, const QString& topic) const {
    if (!mMergedOsdData.contains(sn))
        return {};
    const auto& topicMap = mMergedOsdData[sn];
    if (!topicMap.contains(topic))
        return {};
    const QJsonObject& merged = topicMap[topic];
    if (merged.isEmpty())
        return {};
    QJsonDocument doc(merged);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

void DeviceManager::clearMergedOsdData(const QString& sn, const QString& topic) {
    if (topic.isEmpty())
        mMergedOsdData.remove(sn);
    else if (mMergedOsdData.contains(sn))
        mMergedOsdData[sn].remove(topic);
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

MqttConfig DeviceManager::mqttConfigForProfile(const QString& name) const {
    return mConfigStore->mqttConfigForProfile(name);
}

void DeviceManager::setMqttConfigForProfile(const QString& name, const MqttConfig& cfg) {
    mConfigStore->setMqttConfigForProfile(name, cfg);
}

StreamUrlConfig DeviceManager::streamUrls() const {
    return mConfigStore->streamUrls();
}

void DeviceManager::setStreamUrls(const StreamUrlConfig& urls) {
    mConfigStore->setStreamUrls(urls);
}

StreamMediaConfig DeviceManager::streamMediaConfig() const {
    return mConfigStore->mqttConfig().streamMedia;
}

void DeviceManager::setStreamMediaConfig(const StreamMediaConfig& config) {
    MqttConfig mqtt = mConfigStore->mqttConfig();
    mqtt.streamMedia = config;
    mConfigStore->setMqttConfig(mqtt);
}

QString DeviceManager::devicePushUrl(const QString& sn, const QString& videoId) const {
    return mConfigStore->devicePushUrl(sn, videoId);
}

void DeviceManager::setDevicePushUrl(const QString& sn, const QString& videoId, const QString& url) {
    mConfigStore->setDevicePushUrl(sn, videoId, url);
}

QVector<LiveStatusInfo> DeviceManager::latestLiveStatus(const QString& sn) const {
    return mLiveStatusCache.value(sn);
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

void DeviceManager::setAllTopicsEnabled(const QString& deviceSn, bool enabled) {
    mTopicManager->setAllTopicsEnabled(deviceSn, enabled);

    // 持久化禁用状态
    QSet<QString> disabledSet = mTopicManager->disabledTopicsForDevice(deviceSn);
    mConfigStore->setDisabledTopicsForDevice(deviceSn,
        QStringList(disabledSet.begin(), disabledSet.end()));
    saveConfig(mConfigPath);
}

bool DeviceManager::isTopicEnabled(const QString& deviceSn, const QString& topic) const {
    return mTopicManager->isTopicEnabled(deviceSn, topic);
}

void DeviceManager::reorderTopics(const QString& deviceSn, const QStringList& orderedTopics) {
    mTopicManager->reorderTopics(deviceSn, orderedTopics);
    // 持久化顺序变更
    mConfigStore->setTopicsForDevice(deviceSn, orderedTopics);
    saveConfig(mConfigPath);
}

// ——— Profile 管理 ———

QStringList DeviceManager::profileNames() const {
    return mConfigStore->profileNames();
}

QString DeviceManager::currentProfileName() const {
    return mConfigStore->currentProfileName();
}

void DeviceManager::switchToProfile(const QString& name) {
    if (mConfigStore->currentProfileName() == name)
        return;

    bool wasConnected = isConnected();
    if (wasConnected)
        disconnectBroker();

    // 清空运行时状态
    mDevices.clear();
    mAircraftOsdCache.clear();
    mDockOsdCache.clear();
    mRawJsonCache.clear();
    mJsonHistory.clear();
    mLiveStatusCache.clear();
    mTopicManager->clear();

    // 切换到新 profile（抑制信号，设备加载完后再通知 UI）
    mConfigStore->setCurrentProfile(name, false);

    // 重新加载设备
    for (const auto& info : mConfigStore->devices()) {
        mDevices[info.sn] = info;
        QStringList topics = mConfigStore->topicsForDevice(info.sn);
        mTopicManager->setDeviceTopics(info.sn, topics);

        QStringList disabled = mConfigStore->disabledTopicsForDevice(info.sn);
        if (!disabled.isEmpty()) {
            mTopicManager->setDisabledTopicsForDevice(
                info.sn,
                QSet<QString>(disabled.begin(), disabled.end()));
            mTopicManager->unsubscribeTopics(disabled);
        }
    }

    qDebug() << "DeviceManager: switched to profile" << name
             << "with" << mDevices.size() << "devices";

    // 设备加载完毕，通知 UI 刷新
    emit profileSwitched(name);

    if (wasConnected)
        connectBroker();
}

bool DeviceManager::addProfile(const QString& name, const MqttConfig& mqtt) {
    bool ok = mConfigStore->addProfile(name, mqtt);
    if (ok) emit profileListChanged();
    return ok;
}

bool DeviceManager::removeProfile(const QString& name) {
    bool wasCurrent = (mConfigStore->currentProfileName() == name);
    // 抑制 ConfigStore 的 profileSwitched 信号，等设备重载完后再发
    bool ok = mConfigStore->removeProfile(name, false);
    if (ok) {
        if (wasCurrent) {
            // 清空运行时状态并重新加载新当前 profile 的设备
            mDevices.clear();
            mAircraftOsdCache.clear();
            mDockOsdCache.clear();
            mRawJsonCache.clear();
            mJsonHistory.clear();
            mTopicManager->clear();

            for (const auto& info : mConfigStore->devices()) {
                mDevices[info.sn] = info;
                QStringList topics = mConfigStore->topicsForDevice(info.sn);
                mTopicManager->setDeviceTopics(info.sn, topics);

                QStringList disabled = mConfigStore->disabledTopicsForDevice(info.sn);
                if (!disabled.isEmpty()) {
                    mTopicManager->setDisabledTopicsForDevice(
                        info.sn,
                        QSet<QString>(disabled.begin(), disabled.end()));
                    mTopicManager->unsubscribeTopics(disabled);
                }
            }

            qDebug() << "DeviceManager: profile" << name
                     << "removed, switched to" << mConfigStore->currentProfileName()
                     << "with" << mDevices.size() << "devices";
        }

        emit profileListChanged();
        if (wasCurrent)
            emit profileSwitched(mConfigStore->currentProfileName());
    }
    return ok;
}

bool DeviceManager::renameProfile(const QString& oldName, const QString& newName) {
    bool ok = mConfigStore->renameProfile(oldName, newName);
    if (ok) emit profileListChanged();
    return ok;
}

// ——— 私有槽 ———

void DeviceManager::onMqttConnected() {
    // 连接成功后订阅所有启用的 topic
    QStringList all = mTopicManager->allEnabledTopics();
    mMqttManager->subscribeTopics(all);
    emit brokerConnected();
}

void DeviceManager::onMqttDisconnected() {
    // MQTT 断连 → 所有设备标为离线
    for (auto it = mDevices.begin(); it != mDevices.end(); ++it) {
        if (it->online) {
            it->online = false;
            emit deviceOnlineChanged(it.key(), false);
        }
    }
    emit brokerDisconnected();
}

void DeviceManager::onMqttMessage(const QString& topic, const QByteArray& payload) {
    mDockCmdExecutor->onMqttMessage(topic, payload);
    parseAndRoute(topic, payload);
}

void DeviceManager::onTopicsChanged(const QStringList& add, const QStringList& remove) {
    mMqttManager->replaceSubscriptions(add, remove);
}

// 从原始 JSON payload 中提取数值字段的原始字符串，保留 OSD 上报的原始小数位数
static QString extractJsonNumberString(const QByteArray& payload, const QString& key) {
    QRegularExpression re(
        QString(R"("%1"\s*:\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?))").arg(key));
    QRegularExpressionMatch m = re.match(QString::fromUtf8(payload));
    if (m.hasMatch())
        return m.captured(1);
    return QString();
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

    // 记录最后消息时间（用于离线检测）
    mLastMessageTime[sn] = QDateTime::currentMSecsSinceEpoch();

    // 保存原始 JSON（最新一条）
    QString formatted = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    mRawJsonCache[sn][topic] = formatted;

    // 累积 JSON 历史（最多 500 条）
    mJsonHistory[sn][topic].append(formatted);
    while (mJsonHistory[sn][topic].size() > MAX_JSON_HISTORY)
        mJsonHistory[sn][topic].removeFirst();

    // 字段级合并：DJI 机场 OSD 数据按字段子集分消息推送，
    // 将每条消息的 data 字段增量合并到累积对象中，确保面板始终显示所有已知字段
    if (!data.isEmpty()) {
        QJsonObject& merged = mMergedOsdData[sn][topic];
        for (auto it = data.begin(); it != data.end(); ++it) {
            merged[it.key()] = it.value();
        }
    }

    // 解析 OSD 数据 — 字段级合并：从已有缓存拷贝，再 parse 当前消息的数据，
    // 确保 DJI 分消息推送的字段子集不会互相覆盖
    DeviceInfo& info = mDevices[sn];

    // 解析 PSDK 喊话器事件（上行 progress 通知）
    QString method = root.value("method").toString();
    if (method == QStringLiteral("speaker_tts_play_start_progress")
        || method == QStringLiteral("speaker_audio_play_start_progress")) {
        QJsonObject output = data.value("output").toObject();
        if (!output.isEmpty()) {
            SpeakerProgress progress;
            progress.gatewaySn = sn;
            progress.method    = method;
            progress.psdkIndex = output.value("psdk_index").toInt();
            progress.status    = output.value("status").toString();
            progress.md5       = output.value("md5").toString();
            QJsonObject progObj = output.value("progress").toObject();
            progress.percent   = progObj.value("percent").toInt();
            progress.stepKey   = progObj.value("step_key").toString();
            emit speakerProgressUpdated(progress);
        }
    }
    if (topic.contains("/osd")) {
        if (info.type == DeviceType::Aircraft) {
            AircraftOsd osd = mAircraftOsdCache.value(sn);
            osd.parse(data);
            // 从原始 payload 提取坐标原始字符串，保留 OSD 上报小数位数
            if (osd.latitudeStr.isEmpty())
                osd.latitudeStr = extractJsonNumberString(payload, QStringLiteral("latitude"));
            if (osd.longitudeStr.isEmpty())
                osd.longitudeStr = extractJsonNumberString(payload, QStringLiteral("longitude"));
            if (osd.altitudeStr.isEmpty())
                osd.altitudeStr = extractJsonNumberString(payload, QStringLiteral("altitude"));
            mAircraftOsdCache[sn] = osd;
        } else if (info.type == DeviceType::Dock) {
            DockOsd osd = mDockOsdCache.value(sn);
            osd.parse(data);
            // 从原始 payload 提取坐标原始字符串，保留 OSD 上报小数位数
            if (osd.latitudeStr.isEmpty())
                osd.latitudeStr = extractJsonNumberString(payload, QStringLiteral("latitude"));
            if (osd.longitudeStr.isEmpty())
                osd.longitudeStr = extractJsonNumberString(payload, QStringLiteral("longitude"));
            if (osd.altitudeStr.isEmpty())
                osd.altitudeStr = extractJsonNumberString(payload, QStringLiteral("altitude"));
            if (osd.heightStr.isEmpty())
                osd.heightStr = extractJsonNumberString(payload, QStringLiteral("height"));
            mDockOsdCache[sn] = osd;

            // 自动检测库内飞机：从 OSD 的 sub_device.device_sn 提取飞机 SN，
            // 若尚未加入设备列表则自动创建并持久化到 config.json
            if (data.contains("sub_device")) {
                QJsonObject subDevice = data["sub_device"].toObject();
                QString detectedSn = subDevice["device_sn"].toString();
                if (!detectedSn.isEmpty() && !mDevices.contains(detectedSn)) {
                    DeviceInfo child;
                    child.sn       = detectedSn;
                    child.name     = QString::fromUtf8("\xe9\xa3\x9e\xe6\x9c\xba-") + detectedSn.right(4);
                    child.type     = DeviceType::Aircraft;
                    child.parentSn = sn;

                    QStringList childTopics;
                    childTopics << QString("thing/product/%1/osd").arg(detectedSn);
                    mDevices[detectedSn] = child;
                    mTopicManager->setDeviceTopics(detectedSn, childTopics);

                    saveConfig(mConfigPath);
                    emit deviceAdded(detectedSn);

                    qDebug() << "DeviceManager: auto-detected child aircraft"
                             << detectedSn << "for dock" << sn;
                }
            }
        }

        // 解析 live_status[] 数组（视频直播状态）— 适用于所有设备类型
        if (data.contains("live_status")) {
            QJsonArray liveArr = data["live_status"].toArray();
            QVector<LiveStatusInfo> liveList;
            for (const auto& item : liveArr) {
                QJsonObject obj = item.toObject();
                LiveStatusInfo info;
                info.videoId      = obj["video_id"].toString();
                info.videoQuality = obj["video_quality"].toInt(0);
                info.videoType    = obj["video_type"].toString();
                info.status       = obj["status"].toInt(0);
                info.errorStatus  = obj["error_status"].toInt(0);
                // video_id 格式 "{deviceSn}/...", 提取真实设备 SN（飞机摄像头经机场 topic 上报时 sn 为机场 SN）
                {
                    QString vidSn = info.videoId.section('/', 0, 0);
                    info.deviceSn = vidSn.isEmpty() ? sn : vidSn;
                }
                liveList.append(info);
            }

            // 反闪烁：内容未变化则不发射信号
            if (mLiveStatusCache.value(sn) != liveList) {
                mLiveStatusCache[sn] = liveList;
                emit deviceLiveStatusChanged(sn, liveList);
            }
        }
    }

    // 如果之前是 offline，切换为 online
    if (!info.online) {
        info.online = true;
        emit deviceOnlineChanged(sn, true);

    }

    emit deviceOsdUpdated(sn, topic, formatted);
}

void DeviceManager::checkDeviceOffline() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = mDevices.begin(); it != mDevices.end(); ++it) {
        if (!it->online)
            continue;
        qint64 last = mLastMessageTime.value(it.key(), 0);
        if (last > 0 && (now - last) > OFFLINE_TIMEOUT_MS) {
            it->online = false;
            emit deviceOnlineChanged(it.key(), false);
            qDebug() << "DeviceManager: device offline" << it.key()
                     << "(no message for" << (now - last) / 1000 << "s)";

            // 父设备离线时，子飞机也一并离线
            for (auto child = mDevices.begin(); child != mDevices.end(); ++child) {
                if (child->parentSn == it.key() && child->online) {
                    child->online = false;
                    emit deviceOnlineChanged(child.key(), false);
                }
            }
        }
    }
}

void DeviceManager::onPublishCompleted(const QString& topic, bool success, const QString& message) {
    // 过滤 DockCommandExecutor 发起的 topic，避免同一条指令在 Topic 下发记录中重复
    if (mDockCmdExecutor->pendingTopic() == topic)
        return;
    emit publishResult(topic, success, message);
}

void DeviceManager::publishMessage(const QString& topic, const QString& json) {
    QByteArray payload = json.toUtf8();
    mMqttManager->publish(topic, payload);
}

void DeviceManager::executeDockCommand(const QString& gatewaySn, DockCommandType type,
                                       const QJsonObject& data) {
    mDockCmdExecutor->execute(gatewaySn, type, data);
}

void DeviceManager::liveStartPush(const QString& gatewaySn, const QString& videoId,
                                   const QString& url, int urlType, int videoQuality) {
    QJsonObject data;
    data["url_type"]      = urlType;
    data["url"]           = url;
    data["video_id"]      = videoId;
    data["video_quality"] = videoQuality;
    executeDockCommand(gatewaySn, DockCommandType::LiveStartPush, data);
}

void DeviceManager::liveStopPush(const QString& gatewaySn, const QString& videoId) {
    QJsonObject data;
    data["video_id"] = videoId;
    executeDockCommand(gatewaySn, DockCommandType::LiveStopPush, data);
}

void DeviceManager::liveSetQuality(const QString& gatewaySn, const QString& videoId,
                                    int quality) {
    QJsonObject data;
    data["video_id"]      = videoId;
    data["video_quality"] = quality;
    executeDockCommand(gatewaySn, DockCommandType::LiveSetQuality, data);
}

void DeviceManager::liveLensChange(const QString& gatewaySn, const QString& videoType) {
    QJsonObject data;
    data["video_type"] = videoType;
    executeDockCommand(gatewaySn, DockCommandType::LiveLensChange, data);
}

