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
    int     mode_code            = -1;   // 飞行模式码
    double  battery_temperature  = -273; // 电池温度 ℃
    double  height               = 0;    // 相对起飞点高度 m
    int     gps_number           = 0;    // GPS搜星数
    double  wind_speed           = -1;   // 风速 m/s

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
        if (data.contains("mode_code"))
            mode_code = data["mode_code"].toInt();
        if (data.contains("battery_temperature"))
            battery_temperature = data["battery_temperature"].toDouble();
        if (data.contains("height"))
            height = data["height"].toDouble();
        if (data.contains("gps_number"))
            gps_number = data["gps_number"].toInt();
        if (data.contains("wind_speed"))
            wind_speed = data["wind_speed"].toDouble();
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
    double  alternate_land_lat     = 0;    // 备降点纬度
    double  alternate_land_lon     = 0;    // 备降点经度
    double  dock_inside_temp       = -273; // 舱内温度 ℃, -273 表示无数据
    double  rainfall               = -1;   // 降雨量 mm, -1 表示无数据
    int     putter_state           = -1;   // 推杆状态: 0=收回, 1=推出, -1=未知

    void parse(const QJsonObject& data) {
        parseCommon(data);
        cover_state            = data.value("cover_state").toString();
        drone_in_dock          = data.value("drone_in_dock").toVariant().toInt() != 0;
        working_voltage        = data.value("working_voltage").toDouble();
        working_current        = data.value("working_current").toDouble();
        backup_battery_voltage = data.value("backup_battery_voltage").toDouble();

        if (data.contains("wind_speed"))
            wind_speed = data["wind_speed"].toDouble();
        if (data.contains("environment_temperature"))
            environment_temp = data["environment_temperature"].toDouble();
        if (data.contains("environment_humidity"))
            environment_humidity = data["environment_humidity"].toDouble();
        if (data.contains("alternate_land_point")) {
            QJsonObject alp = data["alternate_land_point"].toObject();
            if (alp.contains("latitude"))
                alternate_land_lat = alp["latitude"].toDouble();
            if (alp.contains("longitude"))
                alternate_land_lon = alp["longitude"].toDouble();
        }
        if (data.contains("putter_state"))
            putter_state = data["putter_state"].toInt();
        if (data.contains("dock_inside_temperature"))
            dock_inside_temp = data["dock_inside_temperature"].toDouble();
        if (data.contains("rainfall"))
            rainfall = data["rainfall"].toDouble();
    }

    static DockOsd fromJson(const QJsonObject& data) {
        DockOsd osd;
        osd.parse(data);
        return osd;
    }
};

#endif // OSDDATA_H
