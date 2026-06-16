#ifndef TOPICMANAGER_H
#define TOPICMANAGER_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QSet>

// TopicManager: 管理所有设备订阅的 topic
// 职责：记录 topic 归属关系、生成去重的全量订阅列表、topic 增删改
class TopicManager : public QObject {
    Q_OBJECT
public:
    explicit TopicManager(QObject* parent = nullptr);

    // 设置设备的所有 topic（覆盖写入）
    void setDeviceTopics(const QString& deviceSn, const QStringList& topics);

    // 添加单个 topic
    void addTopic(const QString& deviceSn, const QString& topic);

    // 删除单个 topic
    void removeTopic(const QString& deviceSn, const QString& topic);

    // 修改 topic
    void updateTopic(const QString& deviceSn, const QString& oldTopic, const QString& newTopic);

    // 获取某个设备的所有 topic
    QStringList topicsForDevice(const QString& deviceSn) const;

    // 获取所有设备的去重 topic 合集（供 MQTT 订阅用）
    QStringList allTopics() const;

    // 根据 topic 反查设备 SN
    QString deviceForTopic(const QString& topic) const;

    // 移除某个设备的所有 topic
    void removeDevice(const QString& deviceSn);

    // 获取所有启用的 topic（过滤掉禁用的）——供 MQTT 订阅用
    QStringList allEnabledTopics() const;

    // 设置/查询某个 topic 的启用/禁用状态
    void setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled);
    bool isTopicEnabled(const QString& deviceSn, const QString& topic) const;

    // 获取某个设备所有启用的 topic
    QStringList enabledTopicsForDevice(const QString& deviceSn) const;

    // 获取某个设备所有禁用的 topic（供 ConfigStore 持久化用）
    QSet<QString> disabledTopicsForDevice(const QString& deviceSn) const;

    // 批量设置禁用 topic（供 ConfigStore 加载用）
    void setDisabledTopicsForDevice(const QString& deviceSn, const QSet<QString>& topics);

    // 重排某个设备的 topic 顺序（不改变集合内容，不发射 topicsChanged）
    void reorderTopics(const QString& deviceSn, const QStringList& orderedTopics);

    // 清空所有 topic
    void clear();

    // 取消订阅指定 topic 列表（用于初始禁用后取消订阅）
    void unsubscribeTopics(const QStringList& topics) { emit topicsChanged({}, topics); }

signals:
    // topic 变更（需要重新订阅）
    void topicsChanged(const QStringList& newTopics, const QStringList& removedTopics);

private:
    QMap<QString, QStringList>   mDeviceTopics;     // deviceSn -> ordered topic list
    QMap<QString, QString>       mTopicToDevice;     // topic -> deviceSn (反向索引)
    QMap<QString, QSet<QString>> mDisabledTopics;    // deviceSn -> set of disabled topics
};

#endif // TOPICMANAGER_H
