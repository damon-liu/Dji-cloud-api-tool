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
    GimbalReset,           // 云台回中/向下 gimbal_reset
    // 喊话器控制（PSDK）
    SpeakerTtsPlay,      // TTS 文本喊话 speaker_tts_play_start
    SpeakerAudioPlay,    // 音频文件喊话 speaker_audio_play_start
    SpeakerVolumeSet,    // 设置音量 speaker_play_volume_set
    SpeakerModeSet,      // 设置播放模式 speaker_play_mode_set
    SpeakerStop,         // 停止播放 speaker_play_stop
    SpeakerReplay,        // 重新播放 speaker_replay
    // 视频直播推流控制
    LiveStartPush,       // 开始推流 live_start_push
    LiveStopPush,        // 停止推流 live_stop_push
    LiveSetQuality,      // 设置清晰度 live_set_quality
    LiveLensChange,      // 切换镜头类型 live_lens_change
    LiveCameraChange,    // 切换相机 live_camera_change
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

// 喊话器播放进度（上行 events 事件专用）
struct SpeakerProgress {
    QString gatewaySn;     // 机场 SN
    int     psdkIndex = 0; // PSDK 负载设备索引
    QString status;        // "in_progress" | "ok"
    int     percent = 0;   // 进度百分比 0–100
    QString stepKey;       // 当前步骤：change_work_mode / upload / download / encoding / play
    QString md5;           // 文件 MD5
    QString method;        // speaker_tts_play_start_progress | speaker_audio_play_start_progress
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
Q_DECLARE_METATYPE(SpeakerProgress)

#endif // DOCKCOMMAND_H
