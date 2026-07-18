#ifndef DOCKCOMMAND_H
#define DOCKCOMMAND_H

#include <QByteArray>
#include <QJsonObject>
#include <QMetaType>
#include <QString>

enum class DockCommandType {
    DebugModeOpen,
    DebugModeClose,
    DroneOpen,
    DroneClose,
    CoverOpen,
    CoverClose,
    ChargeOpen,
    ChargeClose
};

enum class DockCommandState {
    Publishing,
    WaitingReply,
    Succeeded,
    Failed,
    TimedOut
};

struct DockCommandRequest {
    DockCommandType type = DockCommandType::DebugModeOpen;
    QString gatewaySn;
    QString topic;
    QString tid;
    QString bid;
    QString method;
    QJsonObject payload;
};

struct DockCommandReply {
    bool valid = false;
    QString tid;
    int resultCode = -1;
};

struct DockCommandResult {
    DockCommandType type = DockCommandType::DebugModeOpen;
    DockCommandState state = DockCommandState::Failed;
    QString gatewaySn;
    QString tid;
    QString method;
    int resultCode = -1;
    QString message;
    QString requestJson;   // 下发报文（缩进格式化）
    QString replyJson;     // 响应报文（缩进格式化），无回复时为空
};

class DockCommandBuilder {
public:
    static DockCommandRequest build(const QString& gatewaySn, DockCommandType type);
    static QString method(DockCommandType type);
    static QString displayName(DockCommandType type);
    static bool requiresDebugMode(DockCommandType type);
    static DockCommandReply parseReply(const QByteArray& payload);
};

Q_DECLARE_METATYPE(DockCommandType)
Q_DECLARE_METATYPE(DockCommandResult)

#endif // DOCKCOMMAND_H
