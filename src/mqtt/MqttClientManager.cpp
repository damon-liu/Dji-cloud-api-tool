#include "MqttClientManager.h"
#include <QDebug>
#include <QCoreApplication>

MqttClientManager::MqttClientManager(QObject* parent)
    : QObject(parent)
    , mClient(new QMqttClient(this))
    , mReconnectTimer(new QTimer(this))
    , mReconnectDelayMs(BASE_RECONNECT_MS)
{
    mReconnectTimer->setSingleShot(true);

    connect(mClient, &QMqttClient::connected,
            this, &MqttClientManager::onConnected);
    connect(mClient, &QMqttClient::disconnected,
            this, &MqttClientManager::onDisconnected);
    connect(mClient, &QMqttClient::errorChanged,
            this, &MqttClientManager::onError);
    connect(mClient, &QMqttClient::messageReceived,
            this, &MqttClientManager::onMessageReceived);
    connect(mClient, &QMqttClient::messageSent,
            this, &MqttClientManager::onMessageSent);
    connect(mReconnectTimer, &QTimer::timeout,
            this, &MqttClientManager::onReconnectTimer);
}

MqttClientManager::~MqttClientManager() {
    disconnectFromBroker();
}

void MqttClientManager::connectToBroker(const MqttConfig& config) {
    mConfig = config;
    mReconnectDelayMs = BASE_RECONNECT_MS;
    mIntentionalDisconnect = false;

    mClient->setHostname(config.host);
    mClient->setPort(static_cast<quint16>(config.port));
    mClient->setUsername(config.username);
    mClient->setPassword(config.password);
    if (!config.clientId.isEmpty())
        mClient->setClientId(config.clientId);
    else
        mClient->setClientId("DjiCloudApi_" + QString::number(QCoreApplication::applicationPid()));

    qDebug() << "MQTT: connecting to" << config.host << ":" << config.port;
    mClient->connectToHost();
}

void MqttClientManager::disconnectFromBroker() {
    mIntentionalDisconnect = true;
    stopReconnect();
    mSubscribedTopics.clear();
    mClient->disconnectFromHost();
}

bool MqttClientManager::isConnected() const {
    return mClient->state() == QMqttClient::Connected;
}

void MqttClientManager::subscribeTopics(const QStringList& topics) {
    if (!isConnected() || topics.isEmpty()) return;
    for (const auto& t : topics) {
        if (mSubscribedTopics.contains(t)) continue;
        auto* sub = mClient->subscribe(t, 1);  // QoS 1
        if (sub) {
            mSubscribedTopics.append(t);
            qDebug() << "MQTT: subscribed" << t;
        } else {
            qWarning() << "MQTT: subscribe failed" << t;
        }
    }
}

void MqttClientManager::unsubscribeTopics(const QStringList& topics) {
    if (!isConnected() || topics.isEmpty()) return;
    for (const auto& t : topics) {
        mClient->unsubscribe(t);
        mSubscribedTopics.removeAll(t);
        qDebug() << "MQTT: unsubscribed" << t;
    }
}

void MqttClientManager::replaceSubscriptions(const QStringList& addTopics,
                                               const QStringList& removeTopics) {
    unsubscribeTopics(removeTopics);
    subscribeTopics(addTopics);
}

void MqttClientManager::publish(const QString& topic, const QByteArray& payload) {
    if (!isConnected()) {
        emit publishCompleted(topic, false, QStringLiteral("MQTT not connected"));
        return;
    }

    qint32 msgId = mClient->publish(QMqttTopicName(topic), payload, 1);  // QoS 1
    if (msgId < 0) {
        emit publishCompleted(topic, false, QStringLiteral("publish() returned error"));
        return;
    }

    mPendingPublishes[msgId] = topic;
}

void MqttClientManager::onMessageSent(qint32 id) {
    if (mPendingPublishes.contains(id)) {
        QString topic = mPendingPublishes.take(id);
        emit publishCompleted(topic, true, QString());
    }
}

void MqttClientManager::onConnected() {
    qDebug() << "MQTT: connected";
    stopReconnect();
    emit connected();
}

void MqttClientManager::onDisconnected() {
    qDebug() << "MQTT: disconnected";
    emit disconnected();
    if (mIntentionalDisconnect) {
        mIntentionalDisconnect = false;
        return;
    }
    startReconnect();
}

void MqttClientManager::onError(QMqttClient::ClientError error) {
    QString errStr;
    switch (error) {
    case QMqttClient::NoError:               errStr = QStringLiteral("No error"); break;
    case QMqttClient::InvalidProtocolVersion: errStr = QStringLiteral("Invalid protocol version"); break;
    case QMqttClient::IdRejected:             errStr = QStringLiteral("Id rejected"); break;
    case QMqttClient::ServerUnavailable:     errStr = QStringLiteral("Server unavailable"); break;
    case QMqttClient::BadUsernameOrPassword: errStr = QStringLiteral("Bad username or password"); break;
    case QMqttClient::NotAuthorized:         errStr = QStringLiteral("Not authorized"); break;
    case QMqttClient::TransportInvalid:      errStr = QStringLiteral("Transport invalid"); break;
    case QMqttClient::ProtocolViolation:     errStr = QStringLiteral("Protocol violation"); break;
    case QMqttClient::Mqtt5SpecificError:    errStr = QStringLiteral("MQTT 5 specific error"); break;
    case QMqttClient::UnknownError:
    default:                                 errStr = QStringLiteral("Unknown error"); break;
    }
    qWarning() << "MQTT error:" << errStr
               << "| host:" << mConfig.host << "port:" << mConfig.port;
    emit connectionError(errStr);
}

void MqttClientManager::onMessageReceived(const QByteArray& message,
                                           const QMqttTopicName& topic) {
    emit messageReceived(topic.name(), message);
}

void MqttClientManager::onReconnectTimer() {
    qDebug() << "MQTT: reconnecting...";
    // 重连前先断开，确保内部传输层回到干净状态
    // 避免前次连接失败（如 TransportInvalid）导致的残留错误状态
    mClient->disconnectFromHost();
    mClient->connectToHost();
}

void MqttClientManager::startReconnect() {
    if (mReconnectTimer->isActive()) return;
    mReconnectDelayMs = qMin(mReconnectDelayMs * 2, MAX_RECONNECT_MS);
    qDebug() << "MQTT: retry in" << mReconnectDelayMs << "ms";
    mReconnectTimer->start(mReconnectDelayMs);
}

void MqttClientManager::stopReconnect() {
    mReconnectTimer->stop();
    mReconnectDelayMs = BASE_RECONNECT_MS;
}
