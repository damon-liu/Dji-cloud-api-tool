#ifndef DOCKCOMMANDEXECUTOR_H
#define DOCKCOMMANDEXECUTOR_H

#include <QObject>
#include <QTimer>
#include "DockCommand.h"

class MqttClientManager;

// DockCommandExecutor: 机场控制指令执行器
// 发布 services 指令 → 订阅 services_reply → 按 tid 匹配结果 → 超时处理
// 同一时刻仅允许一个进行中指令
class DockCommandExecutor : public QObject {
    Q_OBJECT
public:
    explicit DockCommandExecutor(MqttClientManager* mqtt, QObject* parent = nullptr);

    // 发起指令；已有进行中指令时返回 false
    bool execute(const QString& gatewaySn, DockCommandType type);

    // 由 DeviceManager 转发所有 MQTT 消息，内部过滤 reply topic
    void onMqttMessage(const QString& topic, const QByteArray& payload);

signals:
    void commandStateChanged(const DockCommandResult& result);

private slots:
    void onTimeout();

private:
    void emitState(DockCommandState state, int resultCode, const QString& message);

    MqttClientManager* mMqtt;
    QTimer*            mTimeoutTimer;
    DockCommandRequest mPending;
    QString            mReplyTopic;
    bool               mHasPending = false;

    static constexpr int REPLY_TIMEOUT_MS = 10000;
};

#endif // DOCKCOMMANDEXECUTOR_H
