#include "DockCommandExecutor.h"
#include "MqttClientManager.h"
#include <QJsonDocument>
#include <QDebug>

DockCommandExecutor::DockCommandExecutor(MqttClientManager* mqtt, QObject* parent)
    : QObject(parent)
    , mMqtt(mqtt)
    , mTimeoutTimer(new QTimer(this))
{
    mTimeoutTimer->setSingleShot(true);
    mTimeoutTimer->setInterval(REPLY_TIMEOUT_MS);
    connect(mTimeoutTimer, &QTimer::timeout, this, &DockCommandExecutor::onTimeout);
}

bool DockCommandExecutor::execute(const QString& gatewaySn, DockCommandType type) {
    if (mHasPending) {
        qWarning() << "DockCommandExecutor: command already pending, ignored";
        return false;
    }

    mPending = DockCommandBuilder::build(gatewaySn, type);
    mReplyTopic = QStringLiteral("thing/product/%1/services_reply").arg(mPending.gatewaySn);
    mHasPending = true;

    emitState(DockCommandState::Publishing, -1, QString::fromUtf8("正在发布指令"));

    // 确保已订阅 reply topic（subscribeTopics 内部去重，
    // 防止用户禁用了该 topic 导致收不到回复）
    mMqtt->subscribeTopics({mReplyTopic});

    QJsonDocument doc(mPending.payload);
    mMqtt->publish(mPending.topic, doc.toJson(QJsonDocument::Compact));

    emitState(DockCommandState::WaitingReply, -1, QString::fromUtf8("等待机场响应"));
    mTimeoutTimer->start();
    return true;
}

void DockCommandExecutor::onMqttMessage(const QString& topic, const QByteArray& payload) {
    if (!mHasPending || topic != mReplyTopic)
        return;

    DockCommandReply reply = DockCommandBuilder::parseReply(payload);
    if (!reply.valid || reply.tid != mPending.tid)
        return;  // 非本指令的回复（可能来自其他客户端下发）

    mTimeoutTimer->stop();
    mHasPending = false;

    if (reply.resultCode == 0)
        emitState(DockCommandState::Succeeded, 0, QString::fromUtf8("result = 0"));
    else
        emitState(DockCommandState::Failed, reply.resultCode,
                  QString::fromUtf8("错误码 %1").arg(reply.resultCode));
}

void DockCommandExecutor::onTimeout() {
    if (!mHasPending)
        return;
    mHasPending = false;
    emitState(DockCommandState::TimedOut, -1,
              QString::fromUtf8("10 秒未收到回复，请检查机场在线状态及 services_reply 订阅"));
}

void DockCommandExecutor::emitState(DockCommandState state, int resultCode,
                                    const QString& message) {
    DockCommandResult result;
    result.type       = mPending.type;
    result.state      = state;
    result.gatewaySn  = mPending.gatewaySn;
    result.tid        = mPending.tid;
    result.method     = mPending.method;
    result.resultCode = resultCode;
    result.message    = message;
    emit commandStateChanged(result);
}
