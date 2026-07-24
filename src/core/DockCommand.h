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
    CoverForceClose,  // 强制关舱盖 cover_force_close
    ChargeOpen,
    ChargeClose,
    DeviceReboot,  // 机场重启 device_reboot
    Takeoff,               // 一键起飞 takeoff_to_point
    Return,                // 一键返航 return_home
    ReturnHomeCancel,      // 取消返航 return_home_cancel
    EmergencyStop,         // 急停 drone_emergency_stop
    FillLightOpen,         // 补光灯开启
    FillLightClose,        // 补光灯关闭
    FlightAuthorityGrab,   // 飞行控制权抢夺 flight_authority_grab
    FlightAuthorityRelease,// 退出指令飞行控制模式 drc_mode_exit
    PayloadAuthorityGrab,  // 负载控制权抢夺 payload_authority_grab
    PayloadAuthorityRelease,// 退出指令飞行控制模式 drc_mode_exit
    CameraPhotoTake,       // 拍照 camera_photo_take
    CameraRecordStart,     // 开始录像 camera_recording_start
    CameraRecordStop,      // 结束录像 camera_recording_stop
    GimbalReset            // 云台回中/向下 gimbal_reset
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
    static DockCommandRequest build(const QString& gatewaySn, DockCommandType type,
                                    const QJsonObject& data = {});
    static QString method(DockCommandType type);
    static QString displayName(DockCommandType type);
    static bool requiresDebugMode(DockCommandType type);
    static DockCommandReply parseReply(const QByteArray& payload);
};

Q_DECLARE_METATYPE(DockCommandType)
Q_DECLARE_METATYPE(DockCommandResult)

#endif // DOCKCOMMAND_H
