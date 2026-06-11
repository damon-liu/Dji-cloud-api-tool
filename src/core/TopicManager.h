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

    // 清空所有 topic
    void clear();

signals:
    // topic 变更（需要重新订阅）
    void topicsChanged(const QStringList& newTopics, const QStringList& removedTopics);

private:
    QMap<QString, QSet<QString>> mDeviceTopics;  // deviceSn -> set of topics
    QMap<QString, QString>       mTopicToDevice;  // topic -> deviceSn (反向索引)
};

#endif // TOPICMANAGER_H
