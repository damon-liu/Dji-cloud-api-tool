#ifndef OSDDATA_H
#define OSDDATA_H

#include <QString>
#include <QJsonObject>
#include <QJsonValue>

// OSD 公共基类
struct OsdBase {
    qint64   timestamp = 0;    // 毫秒时间戳
    double   longitude = 0.0;  // 经度
    double   latitude  = 0.0;  // 纬度
    double   altitude  = 0.0;  // 海拔高度 (m)
    bool     valid     = false;

    virtual ~OsdBase() = default;

    // 解析公共字段
    void parseCommon(const QJsonObject& data) {
        timestamp = data.value("timestamp").toVariant().toLongLong();
        if (data.contains("longitude"))
            longitude = data["longitude"].toDouble();
        if (data.contains("latitude"))
            latitude = data["latitude"].toDouble();
        if (data.contains("altitude"))
            altitude = data["altitude"].toDouble();
        valid = true;
    }
};

// 飞机 OSD 数据
struct AircraftOsd : public OsdBase {
    int     battery_percent      = -1;   // 电量百分比
    double  battery_voltage      = 0;    // 电压 mV
    double  speed_horizontal     = 0;    // 水平速度 m/s
    double  speed_vertical       = 0;    // 垂直速度 m/s
    double  heading              = 0;    // 航向角 度
    double  pitch                = 0;    // 俯仰角 度
    double  roll                 = 0;    // 横滚角 度
    double  yaw                  = 0;    // 偏航角 度
    double  home_distance        = 0;    // 距home点距离 m
    int     flight_time_sec      = 0;    // 已飞行时间 秒
    int     rc_signal_strength   = 0;    // 遥控信号强度 0-100

    void parse(const QJsonObject& data) {
        parseCommon(data);
        battery_percent    = data.value("battery_percent").toInt(-1);
        battery_voltage    = data.value("battery_voltage").toDouble();
        speed_horizontal   = data.value("speed_horizontal").toDouble();
        speed_vertical     = data.value("speed_vertical").toDouble();
        heading            = data.value("heading").toDouble();
        pitch              = data.value("pitch").toDouble();
        roll               = data.value("roll").toDouble();
        yaw                = data.value("yaw").toDouble();
        home_distance      = data.value("home_distance").toDouble();
        flight_time_sec    = data.value("flight_time_sec").toInt();
        rc_signal_strength = data.value("rc_signal_strength").toInt();
    }

    static AircraftOsd fromJson(const QJsonObject& data) {
        AircraftOsd osd;
        osd.parse(data);
        return osd;
    }
};

// 机场 OSD 数据
struct DockOsd : public OsdBase {
    QString cover_state            = "";   // open/closed
    bool    drone_in_dock          = false;
    double  working_voltage        = 0;    // mV
    double  working_current        = 0;    // mA
    double  backup_battery_voltage = 0;    // mV
    double  wind_speed             = -1;   // m/s, -1 表示无数据
    double  environment_temp       = -273; // ℃, -273 表示无数据
    double  environment_humidity   = -1;   // %, -1 表示无数据

    void parse(const QJsonObject& data) {
        parseCommon(data);
        cover_state            = data.value("cover_state").toString();
        drone_in_dock          = data.value("drone_in_dock").toBool();
        working_voltage        = data.value("working_voltage").toDouble();
        working_current        = data.value("working_current").toDouble();
        backup_battery_voltage = data.value("backup_battery_voltage").toDouble();

        if (data.contains("wind_speed"))
            wind_speed = data["wind_speed"].toDouble();
        if (data.contains("environment_temp"))
            environment_temp = data["environment_temp"].toDouble();
        if (data.contains("environment_humidity"))
            environment_humidity = data["environment_humidity"].toDouble();
    }

    static DockOsd fromJson(const QJsonObject& data) {
        DockOsd osd;
        osd.parse(data);
        return osd;
    }
};

#endif // OSDDATA_H
