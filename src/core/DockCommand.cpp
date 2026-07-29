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
    case DockCommandType::ReturnHomeCancel: return QStringLiteral("return_home_cancel");
    case DockCommandType::EmergencyStop:   return QStringLiteral("drone_emergency_stop");
    case DockCommandType::FillLightOpen:          return QStringLiteral("fill_light_open");
    case DockCommandType::FillLightClose:         return QStringLiteral("fill_light_close");
    case DockCommandType::FlightAuthorityGrab:    return QStringLiteral("flight_authority_grab");
    case DockCommandType::FlightAuthorityRelease: return QStringLiteral("drc_mode_exit");
    case DockCommandType::PayloadAuthorityGrab:   return QStringLiteral("payload_authority_grab");
    case DockCommandType::PayloadAuthorityRelease: return QStringLiteral("drc_mode_exit");
    case DockCommandType::CameraPhotoTake:        return QStringLiteral("camera_photo_take");
    case DockCommandType::CameraRecordStart:      return QStringLiteral("camera_recording_start");
    case DockCommandType::CameraRecordStop:       return QStringLiteral("camera_recording_stop");
    case DockCommandType::GimbalReset:            return QStringLiteral("gimbal_reset");
    case DockCommandType::SpeakerTtsPlay:   return QStringLiteral("speaker_tts_play_start");
    case DockCommandType::SpeakerAudioPlay: return QStringLiteral("speaker_audio_play_start");
    case DockCommandType::SpeakerVolumeSet: return QStringLiteral("speaker_play_volume_set");
    case DockCommandType::SpeakerModeSet:   return QStringLiteral("speaker_play_mode_set");
    case DockCommandType::SpeakerStop:      return QStringLiteral("speaker_play_stop");
    case DockCommandType::SpeakerReplay:    return QStringLiteral("speaker_replay");
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
    case DockCommandType::ReturnHomeCancel: return QString::fromUtf8("取消返航");
    case DockCommandType::EmergencyStop:   return QString::fromUtf8("急停");
    case DockCommandType::FillLightOpen:  return QString::fromUtf8("补光灯开启");
    case DockCommandType::FillLightClose: return QString::fromUtf8("补光灯关闭");
    case DockCommandType::FlightAuthorityGrab:    return QString::fromUtf8("获取飞行控制权");
    case DockCommandType::FlightAuthorityRelease: return QString::fromUtf8("释放飞行控制权");
    case DockCommandType::PayloadAuthorityGrab:   return QString::fromUtf8("获取负载控制权");
    case DockCommandType::PayloadAuthorityRelease: return QString::fromUtf8("释放负载控制权");
    case DockCommandType::CameraPhotoTake:        return QString::fromUtf8("拍照");
    case DockCommandType::CameraRecordStart:      return QString::fromUtf8("开始录像");
    case DockCommandType::CameraRecordStop:       return QString::fromUtf8("结束录像");
    case DockCommandType::GimbalReset:            return QString::fromUtf8("云台复位");
    case DockCommandType::SpeakerTtsPlay:   return QString::fromUtf8("TTS文本喊话");
    case DockCommandType::SpeakerAudioPlay: return QString::fromUtf8("音频文件喊话");
    case DockCommandType::SpeakerVolumeSet: return QString::fromUtf8("设置音量");
    case DockCommandType::SpeakerModeSet:   return QString::fromUtf8("设置播放模式");
    case DockCommandType::SpeakerStop:      return QString::fromUtf8("停止播放");
    case DockCommandType::SpeakerReplay:    return QString::fromUtf8("重新播放");
    }
    return {};
}

bool DockCommandBuilder::requiresDebugMode(DockCommandType type) {
    // 喊话器指令不需要调试模式（通过 PSDK 通信）
    if (type == DockCommandType::SpeakerTtsPlay
        || type == DockCommandType::SpeakerAudioPlay
        || type == DockCommandType::SpeakerVolumeSet
        || type == DockCommandType::SpeakerModeSet
        || type == DockCommandType::SpeakerStop
        || type == DockCommandType::SpeakerReplay)
        return false;

    return type != DockCommandType::DebugModeOpen
        && type != DockCommandType::DebugModeClose
        && type != DockCommandType::Takeoff
        && type != DockCommandType::Return
        && type != DockCommandType::ReturnHomeCancel
        && type != DockCommandType::EmergencyStop
        && type != DockCommandType::FlightAuthorityGrab
        && type != DockCommandType::FlightAuthorityRelease
        && type != DockCommandType::PayloadAuthorityGrab
        && type != DockCommandType::PayloadAuthorityRelease
        && type != DockCommandType::CameraPhotoTake
        && type != DockCommandType::CameraRecordStart
        && type != DockCommandType::CameraRecordStop
        && type != DockCommandType::GimbalReset;
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
