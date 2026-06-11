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

    mDeviceTopics[deviceSn].insert(topic);
    mTopicToDevice[topic] = deviceSn;
    emit topicsChanged({topic}, {});
}

void TopicManager::removeTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    mDeviceTopics[deviceSn].remove(topic);
    mTopicToDevice.remove(topic);
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
    emit topicsChanged({}, removed);
}

void TopicManager::clear() {
    QStringList removed = allTopics();
    mDeviceTopics.clear();
    mTopicToDevice.clear();
    emit topicsChanged({}, removed);
}
