## 一键起飞

**Topic:** thing/product/*{gateway_sn}*/services

**Direction:** down

**Method:** takeoff_to_point

**Data:**

| Column                      | Name                                 | Type     | constraint                                                   | Description                                                  |
| --------------------------- | ------------------------------------ | -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| target_latitude             | 目标点纬度                           | double   | {"max":90,"min":-90}                                         | 目标点纬度，角度值，南纬是负，北纬是正，精度到小数点后6位    |
| target_longitude            | 目标点经度                           | double   | {"max":180,"min":-180}                                       | 目标点经度，角度值，东经是正，西经是负，精度到小数点后6位    |
| target_height               | 目标点高度                           | float    | {"max":1500,"min":2,"step":0.1,"unit_name":"米 / m"}         | 目标点高度（椭球高），使用 WGS84 模型，飞行器到点后默认行为：悬停 |
| security_takeoff_height     | 安全起飞高度                         | float    | {"max":1500,"min":20,"step":0.1,"unit_name":"米 / m"}        | 相对(机场)起飞点的高度（ALT），飞行器先升到特定的高度，然后再飞向目标点。 |
| rth_mode                    | 【必填】返航模式设置值               | enum_int | {"0":"智能高度","1":"设定高度"}                              | 智能返航模式下，飞行器将自动规划最佳返航高度。大疆机场当前不支持设置返航高度模式，只能选择'设定高度'模式。当环境，光线不满足视觉系统要求时（譬如傍晚阳光直射、夜间弱光无光），飞行器将使用您设定的返航高度进行直线返航 |
| rth_altitude                | 返航高度                             | int      | {"max":1500,"min":2,"step":1,"unit_name":"米 / m"}           | 相对(机场)起飞点的高度，相对高 ALT                           |
| rc_lost_action              | 遥控器失控动作                       | enum_int | {"0":"悬停","1":"着陆(降落)","2":"返航"}                     | 遥控器失控动作                                               |
| commander_mode_lost_action  | 【必填】指点飞行失控动作             | enum_int | {"0":"继续执行指点飞行任务","1":"退出指点飞行任务，执行普通失控行为"} |                                                              |
| commander_flight_mode       | 【必填】指点飞行模式设置值           | enum_int | {"0":"智能高度飞行","1":"设定高度飞行"}                      |                                                              |
| commander_flight_height     | 【必填】指点飞行高度                 | float    | {"max":3000,"min":2,"step":0.1,"unit_name":"米 / m"}         | 相对(机场)起飞点的高度，相对高 ALT                           |
| flight_id                   | 一键起飞任务 UUID                    | text     |                                                              | 任务 UUID，全局唯一，用于染色，云端区分该值是普通计划任务还是一键起飞任务 |
| max_speed                   | 一键起飞的飞行过程中能达到的最大速度 | int      | {"max":15,"min":1,"unit_name":"米每秒 / m/s"}                |                                                              |
| simulate_mission            | 是否在模拟器中执行任务               | struct   |                                                              | 可选字段，用于在室内进行模拟任务调试。 **注意：进行模拟飞行前，请务必取下桨叶，以防舱盖关闭时夹断桨叶。** |
| »is_enable                  | 是否开启模拟器任务                   | enum_int | {"0":"不开启","1":"开启"}                                    | 当次任务打开或关闭模拟器                                     |
| »latitude                   | 纬度                                 | double   | {"max":"90.0","min":"-90.0"}                                 |                                                              |
| »longitude                  | 经度                                 | double   | {"max":"180.0","min":"-180.0"}                               |                                                              |
| flight_safety_advance_check | 飞行安全预检查                       | bool     | {"0":"关闭","1":"开启"}                                      | 设置一键起飞和航线任务中的飞行安全是否预先检查。此字段为可选，默认为0，值为0表示关闭，1表示开启。飞行安全预先检查表示: 飞行器执行任务前，检查自身作业区文件是否与云端一致，如果不一致则拉取文件更新，如果一致则不处理 |

**Example:**

```json
{
    "bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
    "data": {
        "flight_id": "ABDEAC21DCADDA",
        "max_speed": 12,
        "rc_lost_action": 0,
        "rth_altitude": 100,
        "security_takeoff_height": 100,
        "target_height": 100,
        "target_latitude": 12.23,
        "target_longitude": 12.32,
        "commander_mode_lost_action": 1,
        "commander_flight_height": 80,
        "flight_safety_advance_check": 1
    },
    "tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
    "timestamp": 1654070968655,
	"method": "takeoff_to_point"
}
```