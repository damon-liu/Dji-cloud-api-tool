#include "TopicManager.h"
#include <QDebug>

TopicManager::TopicManager(QObject* parent)
    : QObject(parent) {}

void TopicManager::setDeviceTopics(const QString& deviceSn, const QStringList& topics) {
    // 移除旧 topic
    removeDevice(deviceSn);

    QSet<QString> topicSet;
    for (const auto& t : topics) {
        topicSet.insert(t);
        mTopicToDevice[t] = deviceSn;
    }
    mDeviceTopics[deviceSn] = topicSet;

    // 通知变更（新增所有 topic）
    emit topicsChanged(topics, {});
}

void TopicManager::addTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        mDeviceTopics[deviceSn] = {};

    // 新添加的 topic 始终从启用状态开始，清理可能残留的禁用记录
    if (mDisabledTopics.contains(deviceSn)) {
        mDisabledTopics[deviceSn].remove(topic);
        if (mDisabledTopics[deviceSn].isEmpty())
            mDisabledTopics.remove(deviceSn);
    }
    mDeviceTopics[deviceSn].insert(topic);
    mTopicToDevice[topic] = deviceSn;
    emit topicsChanged({topic}, {});
}

void TopicManager::removeTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    mDeviceTopics[deviceSn].remove(topic);
    mTopicToDevice.remove(topic);
    // 同步清理禁用记录
    if (mDisabledTopics.contains(deviceSn)) {
        mDisabledTopics[deviceSn].remove(topic);
        if (mDisabledTopics[deviceSn].isEmpty())
            mDisabledTopics.remove(deviceSn);
    }
    emit topicsChanged({}, {topic});
}

void TopicManager::updateTopic(const QString& deviceSn,
                                const QString& oldTopic,
                                const QString& newTopic) {
    removeTopic(deviceSn, oldTopic);
    addTopic(deviceSn, newTopic);
}

QStringList TopicManager::topicsForDevice(const QString& deviceSn) const {
    return mDeviceTopics.value(deviceSn).values();
}

QStringList TopicManager::allTopics() const {
    QSet<QString> result;
    for (const auto& topics : mDeviceTopics)
        result.unite(topics);
    return result.values();
}

QString TopicManager::deviceForTopic(const QString& topic) const {
    return mTopicToDevice.value(topic);
}

void TopicManager::removeDevice(const QString& deviceSn) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    QStringList removed = mDeviceTopics[deviceSn].values();
    for (const auto& t : removed)
        mTopicToDevice.remove(t);
    mDeviceTopics.remove(deviceSn);
    mDisabledTopics.remove(deviceSn);
    emit topicsChanged({}, removed);
}

void TopicManager::clear() {
    QStringList removed = allTopics();
    mDeviceTopics.clear();
    mTopicToDevice.clear();
    mDisabledTopics.clear();
    emit topicsChanged({}, removed);
}

QStringList TopicManager::allEnabledTopics() const {
    QSet<QString> result;
    for (auto it = mDeviceTopics.begin(); it != mDeviceTopics.end(); ++it) {
        const QString& sn = it.key();
        const QSet<QString>& deviceTopics = it.value();
        const QSet<QString> disabled = mDisabledTopics.value(sn);
        for (const auto& t : deviceTopics) {
            if (!disabled.contains(t))
                result.insert(t);
        }
    }
    return result.values();
}

void TopicManager::setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled) {
    if (!mDeviceTopics.contains(deviceSn) || !mDeviceTopics[deviceSn].contains(topic))
        return;

    bool currentlyEnabled = !mDisabledTopics.value(deviceSn).contains(topic);

    if (enabled && !currentlyEnabled) {
        // 启用：从禁用集合中移除
        mDisabledTopics[deviceSn].remove(topic);
        if (mDisabledTopics[deviceSn].isEmpty())
            mDisabledTopics.remove(deviceSn);
        emit topicsChanged({topic}, {});
    } else if (!enabled && currentlyEnabled) {
        // 禁用：加入禁用集合
        mDisabledTopics[deviceSn].insert(topic);
        emit topicsChanged({}, {topic});
    }
    // 状态未变则不操作
}

bool TopicManager::isTopicEnabled(const QString& deviceSn, const QString& topic) const {
    return !mDisabledTopics.value(deviceSn).contains(topic);
}

QStringList TopicManager::enabledTopicsForDevice(const QString& deviceSn) const {
    QSet<QString> deviceTopics = mDeviceTopics.value(deviceSn);
    QSet<QString> disabled = mDisabledTopics.value(deviceSn);
    QStringList result;
    for (const auto& t : deviceTopics) {
        if (!disabled.contains(t))
            result.append(t);
    }
    return result;
}

QSet<QString> TopicManager::disabledTopicsForDevice(const QString& deviceSn) const {
    return mDisabledTopics.value(deviceSn);
}

void TopicManager::setDisabledTopicsForDevice(const QString& deviceSn, const QSet<QString>& topics) {
    if (topics.isEmpty())
        mDisabledTopics.remove(deviceSn);
    else
        mDisabledTopics[deviceSn] = topics;
}
