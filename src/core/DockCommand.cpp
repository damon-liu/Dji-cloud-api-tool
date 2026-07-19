#include "DockCommand.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUuid>

QString DockCommandBuilder::method(DockCommandType type) {
    switch (type) {
    case DockCommandType::DebugModeOpen:  return QStringLiteral("debug_mode_open");
    case DockCommandType::DebugModeClose: return QStringLiteral("debug_mode_close");
    case DockCommandType::DroneOpen:      return QStringLiteral("drone_open");
    case DockCommandType::DroneClose:     return QStringLiteral("drone_close");
    case DockCommandType::CoverOpen:      return QStringLiteral("cover_open");
    case DockCommandType::CoverClose:     return QStringLiteral("cover_close");
    case DockCommandType::CoverForceClose: return QStringLiteral("cover_force_close");
    case DockCommandType::ChargeOpen:     return QStringLiteral("charge_open");
    case DockCommandType::ChargeClose:    return QStringLiteral("charge_close");
    case DockCommandType::DeviceReboot:   return QStringLiteral("device_reboot");
    case DockCommandType::Takeoff:       return QStringLiteral("takeoff_to_point");
    case DockCommandType::Return:        return QStringLiteral("return_home");
    case DockCommandType::FillLightOpen:  return QStringLiteral("fill_light_open");
    case DockCommandType::FillLightClose: return QStringLiteral("fill_light_close");
    }
    return {};
}

QString DockCommandBuilder::displayName(DockCommandType type) {
    switch (type) {
    case DockCommandType::DebugModeOpen:  return QString::fromUtf8("进入远程调试");
    case DockCommandType::DebugModeClose: return QString::fromUtf8("退出远程调试");
    case DockCommandType::DroneOpen:      return QString::fromUtf8("飞机开机");
    case DockCommandType::DroneClose:     return QString::fromUtf8("飞机关机");
    case DockCommandType::CoverOpen:      return QString::fromUtf8("打开舱盖");
    case DockCommandType::CoverClose:     return QString::fromUtf8("关闭舱盖");
    case DockCommandType::CoverForceClose: return QString::fromUtf8("强制关舱盖");
    case DockCommandType::ChargeOpen:     return QString::fromUtf8("开启充电");
    case DockCommandType::ChargeClose:    return QString::fromUtf8("关闭充电");
    case DockCommandType::DeviceReboot:   return QString::fromUtf8("机场重启");
    case DockCommandType::Takeoff:       return QString::fromUtf8("一键起飞");
    case DockCommandType::Return:        return QString::fromUtf8("一键返航");
    case DockCommandType::FillLightOpen:  return QString::fromUtf8("补光灯开启");
    case DockCommandType::FillLightClose: return QString::fromUtf8("补光灯关闭");
    }
    return {};
}

bool DockCommandBuilder::requiresDebugMode(DockCommandType type) {
    return type != DockCommandType::DebugModeOpen
        && type != DockCommandType::DebugModeClose
        && type != DockCommandType::Takeoff
        && type != DockCommandType::Return;
}

DockCommandRequest DockCommandBuilder::build(const QString& gatewaySn, DockCommandType type,
                                             const QJsonObject& data) {
    DockCommandRequest request;
    request.type = type;
    request.gatewaySn = gatewaySn.trimmed();
    request.topic = QStringLiteral("thing/product/%1/services").arg(request.gatewaySn);
    request.tid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.bid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.method = method(type);

    request.payload[QStringLiteral("tid")] = request.tid;
    request.payload[QStringLiteral("bid")] = request.bid;
    request.payload[QStringLiteral("timestamp")] = QDateTime::currentMSecsSinceEpoch();
    request.payload[QStringLiteral("gateway")] = request.gatewaySn;
    request.payload[QStringLiteral("method")] = request.method;
    request.payload[QStringLiteral("data")] = data.isEmpty() ? QJsonObject{} : data;
    return request;
}

DockCommandReply DockCommandBuilder::parseReply(const QByteArray& payload) {
    DockCommandReply reply;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return reply;

    const QJsonObject root = document.object();
    reply.tid = root.value(QStringLiteral("tid")).toString();
    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    if (reply.tid.isEmpty() || !data.contains(QStringLiteral("result")))
        return reply;

    reply.resultCode = data.value(QStringLiteral("result")).toInt(-1);
    reply.valid = true;
    return reply;
}
