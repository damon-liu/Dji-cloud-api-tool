#ifndef MQTTCLIENTMANAGER_H
#define MQTTCLIENTMANAGER_H

#include <QObject>
#include <QMqttClient>
#include <QTimer>
#include "ConfigStore.h"

// MqttClientManager: 封装 QMqttClient，管理连接和断线重连
class MqttClientManager : public QObject {
    Q_OBJECT
public:
    explicit MqttClientManager(QObject* parent = nullptr);
    ~MqttClientManager();

    // 连接/断开
    void connectToBroker(const MqttConfig& config);
    void disconnectFromBroker();
    bool isConnected() const;

    // 订阅/取消订阅 topic 列表
    void subscribeTopics(const QStringList& topics);
    void unsubscribeTopics(const QStringList& topics);

    // 全量替换所有订阅
    void replaceSubscriptions(const QStringList& addTopics, const QStringList& removeTopics);

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& errorMsg);
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QMqttClient::ClientError error);
    void onMessageReceived(const QByteArray& message, const QMqttTopicName& topic);
    void onReconnectTimer();

private:
    void startReconnect();
    void stopReconnect();

    QMqttClient*  mClient;
    QTimer*       mReconnectTimer;
    MqttConfig    mConfig;
    QStringList   mSubscribedTopics;
    int           mReconnectDelayMs;   // 当前重连延迟
    static constexpr int MAX_RECONNECT_MS = 30000;  // 最大重连间隔 30s
    static constexpr int BASE_RECONNECT_MS = 1000;  // 基础重连间隔 1s
};

#endif // MQTTCLIENTMANAGER_H
