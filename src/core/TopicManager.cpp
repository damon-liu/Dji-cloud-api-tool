#include "TopicManager.h"
#include <QDebug>

TopicManager::TopicManager(QObject* parent)
    : QObject(parent) {}

void TopicManager::setDeviceTopics(const QString& deviceSn, const QStringList& topics) {
    // 移除旧 topic
    removeDevice(deviceSn);

    QStringList orderedList;
    for (const auto& t : topics) {
        if (!orderedList.contains(t)) {
            orderedList.append(t);
            mTopicToDevice[t] = deviceSn;
        }
    }
    mDeviceTopics[deviceSn] = orderedList;

    // 通知变更（新增所有 topic）
    emit topicsChanged(orderedList, {});
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
    // 去重：已存在则不追加
    if (!mDeviceTopics[deviceSn].contains(topic)) {
        mDeviceTopics[deviceSn].append(topic);
        mTopicToDevice[topic] = deviceSn;
    }
    emit topicsChanged({topic}, {});
}

void TopicManager::removeTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    mDeviceTopics[deviceSn].removeAll(topic);
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
    return mDeviceTopics.value(deviceSn);
}

QStringList TopicManager::allTopics() const {
    QStringList result;
    for (const auto& topics : mDeviceTopics) {
        for (const auto& t : topics) {
            if (!result.contains(t))
                result.append(t);
        }
    }
    return result;
}

QString TopicManager::deviceForTopic(const QString& topic) const {
    return mTopicToDevice.value(topic);
}

void TopicManager::removeDevice(const QString& deviceSn) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    QStringList removed = mDeviceTopics[deviceSn];
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
    QStringList result;
    for (auto it = mDeviceTopics.begin(); it != mDeviceTopics.end(); ++it) {
        const QString& sn = it.key();
        const QStringList& deviceTopics = it.value();
        const QSet<QString> disabled = mDisabledTopics.value(sn);
        for (const auto& t : deviceTopics) {
            if (!disabled.contains(t) && !result.contains(t))
                result.append(t);
        }
    }
    return result;
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
    QStringList deviceTopics = mDeviceTopics.value(deviceSn);
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

void TopicManager::reorderTopics(const QString& deviceSn, const QStringList& orderedTopics) {
    if (!mDeviceTopics.contains(deviceSn))
        return;

    // 安全检查：orderedTopics 的集合必须与当前列表一致
    QStringList current = mDeviceTopics[deviceSn];
    if (current.size() != orderedTopics.size())
        return;

    QSet<QString> currentSet(current.begin(), current.end());
    QSet<QString> newSet(orderedTopics.begin(), orderedTopics.end());
    if (currentSet != newSet)
        return;

    // 仅当顺序确实改变时更新
    if (current == orderedTopics)
        return;

    mDeviceTopics[deviceSn] = orderedTopics;
    // 不发射 topicsChanged — 集合内容未变，无需重新订阅 MQTT
}
