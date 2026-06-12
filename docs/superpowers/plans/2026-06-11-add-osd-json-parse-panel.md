# OSD JSON 解析面板 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在设备信息下方新增 JSON 解析面板，定时获取最新 OSD 数据，按 topic 映射配置将 key 翻译为中文后分组展示

**Architecture:** 新增 `TopicMapping`（核心层，加载 JSON 映射文件并提供查询）和 `OsdParsePanel`（UI 层，定时刷新 + 分组表格渲染 + 值变化高亮）。修改 `MainWindow` 布局将 OsdPanel 和 OsdParsePanel 垂直堆叠在水平分割器左半区

**Tech Stack:** Qt 6 (Core, Widgets), C++17, QJsonDocument, QTimer, QRegularExpression

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `config/topic_mappings.json` | 新建 | 外部映射配置文件，按 topic 模式组织 key→中文、分组、枚举值 |
| `src/core/TopicMapping.h` | 新建 | 加载/解析映射文件，提供 topic 模式匹配和字段查询 |
| `src/ui/OsdParsePanel.h` | 新建 | JSON 解析面板头文件 |
| `src/ui/OsdParsePanel.cpp` | 新建 | 面板实现：定时刷新、分组表格渲染、暂停恢复、值变化高亮 |
| `src/ui/MainWindow.h` | 修改 | 添加 OsdParsePanel 成员变量 |
| `src/ui/MainWindow.cpp` | 修改 | 布局调整 + 信号连接 |
| `CMakeLists.txt` | 修改 | 添加新源文件 + 部署映射文件 |

---

### Task 1: 创建 TopicMapping 核心类

**Files:**
- Create: `src/core/TopicMapping.h`

- [ ] **Step 1: 创建 `src/core/TopicMapping.h`**

```cpp
#ifndef TOPICMAPPING_H
#define TOPICMAPPING_H

#include <QString>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QRegularExpression>
#include <QDebug>

// 单个字段的映射定义
struct FieldMapping {
    QString zh;                         // 中文名称
    QString unit;                       // 单位 (如 "V", "m/s", "℃", 可为空)
    QMap<QString, QString> values;      // 枚举值翻译 (如 "0" → "待机")
};

// 分组定义
struct GroupDef {
    QString id;                         // 分组 ID (如 "power")
    QString label;                      // 分组标签 (如 "🔋 电源")
    QStringList keys;                   // 该分组包含的字段 key (支持点号分隔的嵌套路径)
};

// 一个 topic 的完整映射配置
struct TopicMappingConfig {
    QString description;                         // topic 描述
    QMap<QString, FieldMapping> fields;          // key → FieldMapping
    QList<GroupDef> groups;                      // 分组定义 (有序)
};

// TopicMapping: 加载 topic_mappings.json，提供映射查询
class TopicMapping {
public:
    TopicMapping() = default;

    // 从 JSON 文件加载映射配置
    bool load(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "TopicMapping: cannot open" << path;
            return false;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();

        if (err.error != QJsonParseError::NoError) {
            qWarning() << "TopicMapping: JSON parse error:" << err.errorString();
            return false;
        }

        QJsonObject root = doc.object();
        QJsonObject topics = root.value("topics").toObject();

        for (auto tit = topics.begin(); tit != topics.end(); ++tit) {
            TopicMappingConfig cfg;
            QJsonObject topicObj = tit.value().toObject();
            cfg.description = topicObj.value("description").toString();

            // 解析 fields
            QJsonObject fieldsObj = topicObj.value("fields").toObject();
            for (auto fit = fieldsObj.begin(); fit != fieldsObj.end(); ++fit) {
                FieldMapping fm;
                QJsonObject fieldObj = fit.value().toObject();
                fm.zh   = fieldObj.value("zh").toString();
                fm.unit = fieldObj.value("unit").toString();

                QJsonObject valuesObj = fieldObj.value("values").toObject();
                for (auto vit = valuesObj.begin(); vit != valuesObj.end(); ++vit) {
                    fm.values[vit.key()] = vit.value().toString();
                }
                cfg.fields[fit.key()] = fm;
            }

            // 解析 groups
            QJsonArray groupsArr = topicObj.value("groups").toArray();
            for (const auto& gv : groupsArr) {
                QJsonObject gobj = gv.toObject();
                GroupDef gd;
                gd.id    = gobj.value("id").toString();
                gd.label = gobj.value("label").toString();
                QJsonArray keysArr = gobj.value("keys").toArray();
                for (const auto& kv : keysArr)
                    gd.keys.append(kv.toString());
                cfg.groups.append(gd);
            }

            mConfigs[tit.key()] = cfg;
        }

        qDebug() << "TopicMapping: loaded" << mConfigs.size() << "topic mappings from" << path;
        return true;
    }

    // 加载内置默认映射 JSON 字符串（文件缺失时降级使用）
    bool loadFromString(const QString& json) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "TopicMapping: built-in JSON parse error:" << err.errorString();
            return false;
        }
        // 复用 load 的逻辑但接受 QJsonDocument
        // 简化：直接调用 load 的核心解析逻辑
        QJsonObject root = doc.object();
        QJsonObject topics = root.value("topics").toObject();
        for (auto tit = topics.begin(); tit != topics.end(); ++tit) {
            TopicMappingConfig cfg;
            QJsonObject topicObj = tit.value().toObject();
            cfg.description = topicObj.value("description").toString();
            QJsonObject fieldsObj = topicObj.value("fields").toObject();
            for (auto fit = fieldsObj.begin(); fit != fieldsObj.end(); ++fit) {
                FieldMapping fm;
                QJsonObject fieldObj = fit.value().toObject();
                fm.zh   = fieldObj.value("zh").toString();
                fm.unit = fieldObj.value("unit").toString();
                QJsonObject valuesObj = fieldObj.value("values").toObject();
                for (auto vit = valuesObj.begin(); vit != valuesObj.end(); ++vit)
                    fm.values[vit.key()] = vit.value().toString();
                cfg.fields[fit.key()] = fm;
            }
            QJsonArray groupsArr = topicObj.value("groups").toArray();
            for (const auto& gv : groupsArr) {
                QJsonObject gobj = gv.toObject();
                GroupDef gd;
                gd.id    = gobj.value("id").toString();
                gd.label = gobj.value("label").toString();
                QJsonArray keysArr = gobj.value("keys").toArray();
                for (const auto& kv : keysArr)
                    gd.keys.append(kv.toString());
                cfg.groups.append(gd);
            }
            mConfigs[tit.key()] = cfg;
        }
        return true;
    }

    // 根据实际 topic 查找映射配置（支持 {sn} 通配符模式匹配）
    TopicMappingConfig mappingForTopic(const QString& topic) const {
        // 1. 精确匹配
        if (mConfigs.contains(topic))
            return mConfigs[topic];

        // 2. 模式匹配：将配置中的 {sn} 替换为 [^/]+ 做正则匹配
        for (auto it = mConfigs.begin(); it != mConfigs.end(); ++it) {
            QString pattern = it.key();
            if (!pattern.contains("{sn}"))
                continue;
            QString escaped = QRegularExpression::escape(pattern);
            escaped.replace("\\{sn\\}", "[^/]+");
            QRegularExpression re("^" + escaped + "$");
            if (re.match(topic).hasMatch())
                return it.value();
        }

        return TopicMappingConfig{}; // 空配置
    }

    // 检查是否有可用的映射
    bool isEmpty() const { return mConfigs.isEmpty(); }

private:
    QMap<QString, TopicMappingConfig> mConfigs; // topic 模式 → 配置
};

#endif // TOPICMAPPING_H
```

- [ ] **Step 2: 验证语法正确性**

```bash
cd build_mingw && cmake --build . --target DjiCloudApi 2>&1 | head -20
```

期望: 编译通过（TopicMapping 暂未被引用，无新增编译单元，仅作为头文件存在）

- [ ] **Step 3: Commit**

```bash
git add src/core/TopicMapping.h
git commit -m "feat: 添加 TopicMapping 核心类 — JSON 映射配置加载与 topic 模式匹配"
```

---

### Task 2: 创建 topic_mappings.json 配置文件

**Files:**
- Create: `config/topic_mappings.json`

- [ ] **Step 1: 创建 `config/topic_mappings.json`**

```json
{
    "topics": {
        "thing/product/{sn}/osd": {
            "description": "机场/无人机 OSD 遥测数据 (0.5Hz 定时上报)",
            "fields": {
                "job_number": { "zh": "累计作业次数", "unit": "次" },
                "acc_time": { "zh": "累计运行时间", "unit": "秒" },
                "electric_supply_voltage": { "zh": "供电电压", "unit": "mV" },
                "working_voltage": { "zh": "工作电压", "unit": "mV" },
                "working_current": { "zh": "工作电流", "unit": "mA" },
                "backup_battery.voltage": { "zh": "备用电池电压", "unit": "mV" },
                "backup_battery.temperature": { "zh": "备用电池温度", "unit": "℃" },
                "backup_battery.switch": {
                    "zh": "备用电池开关",
                    "unit": "",
                    "values": { "0": "关闭", "1": "开启" }
                },
                "drone_charge_state.state": {
                    "zh": "充电状态",
                    "unit": "",
                    "values": { "0": "未充电", "1": "充电中", "2": "充电完成" }
                },
                "drone_charge_state.capacity_percent": { "zh": "充电百分比", "unit": "%" },
                "drone_in_dock": {
                    "zh": "飞机在舱内",
                    "unit": "",
                    "values": { "0": "否", "1": "是" }
                },
                "rainfall": { "zh": "降雨量", "unit": "mm" },
                "wind_speed": { "zh": "风速", "unit": "m/s" },
                "environment_temperature": { "zh": "环境温度", "unit": "℃" },
                "temperature": { "zh": "机舱温度", "unit": "℃" },
                "humidity": { "zh": "湿度", "unit": "%" },
                "latitude": { "zh": "纬度", "unit": "°" },
                "longitude": { "zh": "经度", "unit": "°" },
                "height": { "zh": "海拔高度", "unit": "m" },
                "position_state.is_fixed": {
                    "zh": "定位收敛状态",
                    "unit": "",
                    "values": { "0": "未开始", "1": "收敛中", "2": "已收敛", "3": "失败" }
                },
                "position_state.quality": {
                    "zh": "搜星质量档位",
                    "unit": "",
                    "values": { "1": "1档", "2": "2档", "3": "3档", "4": "4档", "5": "5档" }
                },
                "position_state.gps_number": { "zh": "GPS 搜星数", "unit": "" },
                "position_state.rtk_number": { "zh": "RTK 搜星数", "unit": "" },
                "cover_state": {
                    "zh": "舱盖状态",
                    "unit": "",
                    "values": { "0": "关闭", "1": "打开" }
                },
                "supplement_light_state": {
                    "zh": "补光灯状态",
                    "unit": "",
                    "values": { "0": "关闭", "1": "打开" }
                },
                "emergency_stop_state": {
                    "zh": "急停按钮",
                    "unit": "",
                    "values": { "0": "正常", "1": "按下" }
                },
                "air_conditioner_mode": {
                    "zh": "空调模式",
                    "unit": "",
                    "values": { "0": "关闭", "1": "制冷", "2": "制热", "3": "通风" }
                },
                "battery_store_mode": {
                    "zh": "电池存储模式",
                    "unit": "",
                    "values": { "0": "关闭", "1": "开启" }
                },
                "alarm_state": {
                    "zh": "告警状态",
                    "unit": "",
                    "values": { "0": "正常", "1": "告警" }
                },
                "putter_state": {
                    "zh": "推杆状态",
                    "unit": "",
                    "values": { "0": "收回", "1": "推出" }
                },
                "mode_code": {
                    "zh": "模式码",
                    "unit": "",
                    "values": {
                        "0": "待机", "4": "自动起飞", "5": "航线飞行",
                        "9": "自动返航", "10": "自动降落"
                    }
                },
                "network_state.type": {
                    "zh": "网络类型",
                    "unit": "",
                    "values": { "1": "有线", "2": "4G", "3": "5G" }
                },
                "network_state.quality": { "zh": "网络质量", "unit": "" },
                "network_state.rate": { "zh": "网络速率", "unit": "KB/s" },
                "storage.total": { "zh": "存储总容量", "unit": "KB" },
                "storage.used": { "zh": "已用容量", "unit": "KB" },
                "alternate_land_point.latitude": { "zh": "备降点纬度", "unit": "°" },
                "alternate_land_point.longitude": { "zh": "备降点经度", "unit": "°" },
                "alternate_land_point.is_configured": {
                    "zh": "备降点已配置",
                    "unit": "",
                    "values": { "0": "否", "1": "是" }
                },
                "drone_battery_maintenance_info.maintenance_state": {
                    "zh": "电池保养状态",
                    "unit": "",
                    "values": { "0": "无需保养", "1": "需要保养" }
                },
                "drone_battery_maintenance_info.maintenance_time_left": { "zh": "剩余保养时间", "unit": "秒" },
                "battery.capacity_percent": { "zh": "总电池电量", "unit": "%" },
                "battery.remain_flight_time": { "zh": "剩余飞行时间", "unit": "秒" },
                "battery.return_home_power": { "zh": "返航所需电量", "unit": "%" },
                "battery.landing_power": { "zh": "强制降落电量", "unit": "%" },
                "battery.batteries[0].voltage": { "zh": "左电池电压", "unit": "mV" },
                "battery.batteries[0].temperature": { "zh": "左电池温度", "unit": "℃" },
                "battery.batteries[0].loop_times": { "zh": "左电池循环次数", "unit": "次" },
                "battery.batteries[1].voltage": { "zh": "右电池电压", "unit": "mV" },
                "battery.batteries[1].temperature": { "zh": "右电池温度", "unit": "℃" },
                "battery.batteries[1].loop_times": { "zh": "右电池循环次数", "unit": "次" },
                "horizontal_speed": { "zh": "水平速度", "unit": "m/s" },
                "vertical_speed": { "zh": "垂直速度", "unit": "m/s" },
                "elevation": { "zh": "相对起飞点高度", "unit": "m" },
                "attitude_head": { "zh": "航向角", "unit": "°" },
                "attitude_pitch": { "zh": "俯仰角", "unit": "°" },
                "attitude_roll": { "zh": "横滚角", "unit": "°" },
                "gear": {
                    "zh": "档位",
                    "unit": "",
                    "values": { "0": "A档", "1": "P档", "2": "NAV", "3": "FPV", "5": "S档", "6": "F档", "7": "M档", "9": "T档" }
                },
                "home_distance": { "zh": "距 Home 点距离", "unit": "m" },
                "total_flight_distance": { "zh": "累计飞行里程", "unit": "m" },
                "total_flight_time": { "zh": "累计飞行时间", "unit": "秒" },
                "height_limit": { "zh": "限高", "unit": "m" },
                "wind_direction": {
                    "zh": "风向",
                    "unit": "",
                    "values": { "1": "正北", "2": "东北", "3": "正东", "4": "东南", "5": "正南", "6": "西南", "7": "正西", "8": "西北" }
                },
                "track_id": { "zh": "航迹ID", "unit": "" },
                "firmware_version": { "zh": "固件版本", "unit": "" }
            },
            "groups": [
                {
                    "id": "basic",
                    "label": "📋 基础信息",
                    "keys": ["job_number", "acc_time", "mode_code", "alarm_state",
                             "firmware_version", "gear", "track_id"]
                },
                {
                    "id": "power",
                    "label": "🔋 电源",
                    "keys": ["electric_supply_voltage", "working_voltage", "working_current",
                             "backup_battery.voltage", "backup_battery.temperature", "backup_battery.switch",
                             "battery.capacity_percent", "battery.remain_flight_time",
                             "battery.return_home_power", "battery.landing_power",
                             "battery.batteries[0].voltage", "battery.batteries[0].temperature", "battery.batteries[0].loop_times",
                             "battery.batteries[1].voltage", "battery.batteries[1].temperature", "battery.batteries[1].loop_times"]
                },
                {
                    "id": "charge",
                    "label": "⚡ 充电状态",
                    "keys": ["drone_charge_state.state", "drone_charge_state.capacity_percent"]
                },
                {
                    "id": "flight",
                    "label": "✈ 飞行数据",
                    "keys": ["horizontal_speed", "vertical_speed", "elevation", "height",
                             "home_distance", "total_flight_distance", "total_flight_time",
                             "height_limit", "attitude_head", "attitude_pitch", "attitude_roll"]
                },
                {
                    "id": "position",
                    "label": "📍 定位",
                    "keys": ["latitude", "longitude", "position_state.is_fixed",
                             "position_state.quality", "position_state.gps_number",
                             "position_state.rtk_number"]
                },
                {
                    "id": "environment",
                    "label": "🌡 环境",
                    "keys": ["environment_temperature", "temperature", "humidity",
                             "wind_speed", "rainfall", "wind_direction"]
                },
                {
                    "id": "device",
                    "label": "🔧 设备状态",
                    "keys": ["cover_state", "drone_in_dock", "supplement_light_state",
                             "emergency_stop_state", "air_conditioner_mode",
                             "battery_store_mode", "putter_state"]
                },
                {
                    "id": "network",
                    "label": "📶 网络",
                    "keys": ["network_state.type", "network_state.quality", "network_state.rate"]
                },
                {
                    "id": "storage",
                    "label": "💾 存储",
                    "keys": ["storage.total", "storage.used"]
                },
                {
                    "id": "alternate_land",
                    "label": "🛬 备降点",
                    "keys": ["alternate_land_point.latitude", "alternate_land_point.longitude",
                             "alternate_land_point.is_configured"]
                },
                {
                    "id": "battery_maintenance",
                    "label": "🔧 电池保养",
                    "keys": ["drone_battery_maintenance_info.maintenance_state",
                             "drone_battery_maintenance_info.maintenance_time_left"]
                }
            ]
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add config/topic_mappings.json
git commit -m "feat: 添加 topic_mappings.json — osd topic 完整 key→中文映射配置"
```

---

### Task 3: 实现 OsdParsePanel 面板

**Files:**
- Create: `src/ui/OsdParsePanel.h`
- Create: `src/ui/OsdParsePanel.cpp`

- [ ] **Step 1: 创建 `src/ui/OsdParsePanel.h` 头文件**

```cpp
#ifndef OSDPARSEPANEL_H
#define OSDPARSEPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QTimer>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include "TopicMapping.h"

class DeviceManager;

// OsdParsePanel: 定时解析 OSD JSON，翻译为中文分组展示
class OsdParsePanel : public QWidget {
    Q_OBJECT
public:
    explicit OsdParsePanel(QWidget* parent = nullptr);

    // 设置 DeviceManager 指针（供数据获取）
    void setDeviceManager(DeviceManager* mgr) { mDevMgr = mgr; }

    // 设置当前 topic（用户切换 topic 时调用）
    void setTopic(const QString& deviceSn, const QString& topic);

    // 设置 TopicMapping（加载后传入）
    void setTopicMapping(TopicMapping* mapping) { mMapping = mapping; }

    // 清空面板
    void clear();

public slots:
    void refresh();         // 手动/定时刷新
    void togglePause();     // 暂停/恢复

private:
    void setupUi();
    void renderGroups(const QJsonObject& data);
    QMap<QString, QString> flattenJson(const QJsonObject& obj, const QString& prefix = {}) const;
    void setFieldValue(QLabel* label, const QString& value, bool highlight);

    DeviceManager*      mDevMgr    = nullptr;
    TopicMapping*       mMapping   = nullptr;
    QString             mDeviceSn;
    QString             mTopic;
    bool                mPaused    = false;
    int                 mIntervalMs = 2000;

    // UI 组件
    QLabel*             mTitleLabel;
    QLabel*             mTopicLabel;
    QComboBox*          mIntervalCombo;
    QPushButton*        mPauseBtn;
    QVBoxLayout*        mContentLayout;
    QScrollArea*        mScrollArea;
    QWidget*            mContentWidget;
    QTimer*             mRefreshTimer;

    // 值缓存（用于变化高亮）
    QMap<QString, QString> mPrevValues;
};

#endif // OSDPARSEPANEL_H
```

- [ ] **Step 2: 创建 `src/ui/OsdParsePanel.cpp` 实现文件**

```cpp
#include "OsdParsePanel.h"
#include "DeviceManager.h"
#include <QDateTime>
#include <QFrame>

OsdParsePanel::OsdParsePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();

    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(mIntervalMs);
    connect(mRefreshTimer, &QTimer::timeout, this, &OsdParsePanel::refresh);
    mRefreshTimer->start();
}

void OsdParsePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // ——— 标题栏 ———
    auto* header = new QHBoxLayout;
    header->setSpacing(6);

    mTitleLabel = new QLabel("🔍 JSON 解析");
    mTitleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #1a73e8;");

    mTopicLabel = new QLabel("");
    mTopicLabel->setStyleSheet("color: #80868b; font-size: 10px;");

    header->addWidget(mTitleLabel);
    header->addWidget(mTopicLabel, 1);

    // 刷新间隔选择
    auto* intervalLabel = new QLabel("刷新间隔:");
    intervalLabel->setStyleSheet("color: #80868b; font-size: 11px;");

    mIntervalCombo = new QComboBox;
    mIntervalCombo->addItems({"1s", "2s", "5s", "10s"});
    mIntervalCombo->setCurrentIndex(1); // 默认 2s
    mIntervalCombo->setFixedWidth(60);
    mIntervalCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 3px; padding: 2px 4px; font-size: 11px; }");
    connect(mIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        int intervals[] = {1000, 2000, 5000, 10000};
        mIntervalMs = intervals[idx];
        if (!mPaused)
            mRefreshTimer->start(mIntervalMs);
    });

    mPauseBtn = new QPushButton("⏸ 暂停");
    mPauseBtn->setCursor(Qt::PointingHandCursor);
    mPauseBtn->setFixedWidth(80);
    mPauseBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 12px; "
        "font-size: 12px; background: #fff; color: #5f6368; }"
        "QPushButton:hover { background: #f1f3f4; }");
    connect(mPauseBtn, &QPushButton::clicked, this, &OsdParsePanel::togglePause);

    header->addWidget(intervalLabel);
    header->addWidget(mIntervalCombo);
    header->addWidget(mPauseBtn);
    mainLayout->addLayout(header);

    // 分隔线
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #e0e0e0;");
    mainLayout->addWidget(sep);

    // ——— 内容区域 ———
    mScrollArea = new QScrollArea;
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setFrameShape(QFrame::NoFrame);
    mScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    mContentWidget = new QWidget;
    mContentLayout = new QVBoxLayout(mContentWidget);
    mContentLayout->setContentsMargins(0, 0, 0, 0);
    mContentLayout->setSpacing(4);
    mContentLayout->addStretch();

    mScrollArea->setWidget(mContentWidget);
    mainLayout->addWidget(mScrollArea, 1);
}

void OsdParsePanel::setTopic(const QString& deviceSn, const QString& topic) {
    mDeviceSn = deviceSn;
    mTopic    = topic;
    mPrevValues.clear();
    mTopicLabel->setText(topic.isEmpty() ? "" : topic);

    if (deviceSn.isEmpty() || topic.isEmpty()) {
        clear();
        return;
    }

    // 立即刷新一次
    refresh();
}

void OsdParsePanel::clear() {
    // 清空内容区域中的所有分组 widget
    QLayoutItem* item;
    while ((item = mContentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    mContentLayout->addStretch();
    mPrevValues.clear();
    mTopicLabel->setText("");
}

void OsdParsePanel::refresh() {
    if (!mDevMgr || mDeviceSn.isEmpty() || mTopic.isEmpty())
        return;

    QString rawJson = mDevMgr->latestRawJson(mDeviceSn);
    if (rawJson.isEmpty())
        return;

    QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8());
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();
    // DJI 消息体: {"data": {...}}
    QJsonObject data = root.value("data").toObject();
    if (data.isEmpty())
        return;

    renderGroups(data);
}

void OsdParsePanel::togglePause() {
    mPaused = !mPaused;
    if (mPaused) {
        mRefreshTimer->stop();
        mPauseBtn->setText("▶ 继续");
    } else {
        mRefreshTimer->start(mIntervalMs);
        mPauseBtn->setText("⏸ 暂停");
        refresh(); // 恢复时立即刷新
    }
}

QMap<QString, QString> OsdParsePanel::flattenJson(const QJsonObject& obj, const QString& prefix) const {
    QMap<QString, QString> result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
        QJsonValue val = it.value();

        if (val.isObject()) {
            result.insert(flattenJson(val.toObject(), key));
        } else if (val.isArray()) {
            QJsonArray arr = val.toArray();
            for (int i = 0; i < arr.size(); ++i) {
                if (arr[i].isObject()) {
                    result.insert(flattenJson(arr[i].toObject(), key + "[" + QString::number(i) + "]"));
                } else {
                    result[key + "[" + QString::number(i) + "]"] = valToString(arr[i]);
                }
            }
        } else {
            result[key] = valToString(val);
        }
    }
    return result;
}

static QString valToString(const QJsonValue& val) {
    if (val.isBool())
        return val.toBool() ? "true" : "false";
    if (val.isDouble()) {
        double d = val.toDouble();
        // 避免无意义的小数位
        if (d == static_cast<qint64>(d))
            return QString::number(static_cast<qint64>(d));
        return QString::number(d, 'f', 2);
    }
    if (val.isString())
        return val.toString();
    return val.toVariant().toString();
}

void OsdParsePanel::renderGroups(const QJsonObject& data) {
    // 展平 JSON
    QMap<QString, QString> flatData = flattenJson(data);

    // 获取映射配置
    TopicMappingConfig cfg;
    if (mMapping)
        cfg = mMapping->mappingForTopic(mTopic);

    // 跟踪已渲染的 key
    QSet<QString> renderedKeys;

    // 存储新的值缓存
    QMap<QString, QString> newValues;

    // 清除旧内容
    clear();

    if (cfg.fields.isEmpty()) {
        // 无映射配置
        auto* noMapLabel = new QLabel("该 topic 暂无映射配置");
        noMapLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 16px;");
        noMapLabel->setAlignment(Qt::AlignCenter);
        mContentLayout->insertWidget(mContentLayout->count() - 1, noMapLabel);
        return;
    }

    // 按分组渲染
    for (const auto& group : cfg.groups) {
        auto* groupBox = new QGroupBox(group.label);
        groupBox->setStyleSheet(
            "QGroupBox { font-weight: bold; color: #333; border: 1px solid #e0e0e0; "
            "border-radius: 4px; margin-top: 8px; padding: 12px 8px 8px 8px; background: #ffffff; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");

        auto* formLayout = new QFormLayout(groupBox);
        formLayout->setSpacing(2);
        formLayout->setContentsMargins(4, 4, 4, 4);

        bool groupHasContent = false;

        for (const auto& key : group.keys) {
            FieldMapping fm = cfg.fields.value(key);
            QString zhName = fm.zh.isEmpty() ? key : fm.zh;

            QString rawValue = flatData.value(key, "");
            QString displayValue;

            if (rawValue.isEmpty() && !flatData.contains(key)) {
                displayValue = "-";
            } else if (!fm.values.isEmpty() && fm.values.contains(rawValue)) {
                displayValue = fm.values[rawValue];
            } else {
                displayValue = rawValue;
            }

            if (!fm.unit.isEmpty())
                displayValue += " " + fm.unit;

            auto* nameLabel = new QLabel(zhName);
            nameLabel->setStyleSheet("color: #5f6368; font-size: 11px;");

            auto* valueLabel = new QLabel(displayValue);
            valueLabel->setStyleSheet("font-size: 11px; font-weight: 500;");

            formLayout->addRow(nameLabel, valueLabel);
            renderedKeys.insert(key);
            newValues[key] = displayValue;
            groupHasContent = true;
        }

        if (groupHasContent) {
            mContentLayout->insertWidget(mContentLayout->count() - 1, groupBox);
        } else {
            delete groupBox;
        }
    }

    // 未映射字段（灰色显示在底部）
    QStringList unmappedKeys;
    for (auto it = flatData.begin(); it != flatData.end(); ++it) {
        if (!renderedKeys.contains(it.key()))
            unmappedKeys.append(it.key());
    }

    if (!unmappedKeys.isEmpty()) {
        auto* unmappedGroup = new QGroupBox("未映射字段");
        unmappedGroup->setStyleSheet(
            "QGroupBox { font-weight: bold; color: #9e9e9e; border: 1px solid #e0e0e0; "
            "border-radius: 4px; margin-top: 8px; padding: 12px 8px 8px 8px; background: #fafafa; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");

        auto* unmappedLayout = new QFormLayout(unmappedGroup);
        unmappedLayout->setSpacing(2);
        unmappedLayout->setContentsMargins(4, 4, 4, 4);

        for (const auto& key : unmappedKeys) {
            auto* keyLabel = new QLabel(key);
            keyLabel->setStyleSheet("color: #b0b0b0; font-size: 10px;");

            auto* valLabel = new QLabel(flatData[key]);
            valLabel->setStyleSheet("color: #b0b0b0; font-size: 10px;");

            unmappedLayout->addRow(keyLabel, valLabel);
            newValues[key] = flatData[key];
        }

        mContentLayout->insertWidget(mContentLayout->count() - 1, unmappedGroup);
    }

    // 值变化高亮
    for (auto it = newValues.begin(); it != newValues.end(); ++it) {
        QString oldVal = mPrevValues.value(it.key());
        bool changed = !oldVal.isEmpty() && oldVal != it.value();
        // 寻找对应的 valueLabel 并高亮
        if (changed) {
            // 遍历找到对应的 label（简化：对已映射的分组中的 label 做高亮）
            // 通过遍历所有 QGroupBox 中的 QFormLayout 来更新
            for (int i = 0; i < mContentLayout->count(); ++i) {
                QLayoutItem* item = mContentLayout->itemAt(i);
                if (!item || !item->widget()) continue;
                QGroupBox* gb = qobject_cast<QGroupBox*>(item->widget());
                if (!gb) continue;
                QFormLayout* fl = qobject_cast<QFormLayout*>(gb->layout());
                if (!fl) continue;
                for (int r = 0; r < fl->rowCount(); ++r) {
                    QLayoutItem* labelItem = fl->itemAt(r, QFormLayout::LabelRole);
                    QLayoutItem* fieldItem = fl->itemAt(r, QFormLayout::FieldRole);
                    if (!labelItem || !fieldItem) continue;
                    QLabel* nameLbl = qobject_cast<QLabel*>(labelItem->widget());
                    QLabel* valLbl  = qobject_cast<QLabel*>(fieldItem->widget());
                    if (!nameLbl || !valLbl) continue;
                    // 通过中文名反查 key
                    for (auto fit = cfg.fields.begin(); fit != cfg.fields.end(); ++fit) {
                        if (fit.value().zh == nameLbl->text() && fit.key() == it.key()) {
                            setFieldValue(valLbl, it.value(), true);
                            break;
                        }
                    }
                }
            }
        }
    }

    mPrevValues = newValues;
}

void OsdParsePanel::setFieldValue(QLabel* label, const QString& value, bool highlight) {
    label->setText(value);
    if (highlight) {
        label->setStyleSheet("color: #1a73e8; font-weight: bold; font-size: 11px;");
        QTimer::singleShot(1200, this, [label]() {
            label->setStyleSheet("font-size: 11px; font-weight: 500;");
        });
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add src/ui/OsdParsePanel.h src/ui/OsdParsePanel.cpp
git commit -m "feat: 实现 OsdParsePanel — 定时解析 JSON 分组表格展示"
```

---

### Task 4: 集成到 MainWindow

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: 修改 `src/ui/MainWindow.h` — 添加 OsdParsePanel 成员**

在现有的 include 块末尾（第 16 行 `#include "TopicListWidget.h"` 之后）添加：

```cpp
#include "OsdParsePanel.h"
```

在现有的 `TopicListWidget*   mTopicListWidget;` 成员之后（第 58 行）添加：

```cpp
    OsdParsePanel*    mOsdParsePanel;
```

- [ ] **Step 2: 修改 `src/ui/MainWindow.cpp` — setupLayout()**

找到 `setupLayout()` 中的右半区布局（第 263-291 行）。原来的结构是：

```cpp
// === 右侧：OSD + JSON 水平分割 ===
auto* rightPanel = new QWidget(this);
...
mOsdPanel = new OsdPanel(this);
mRawJsonPanel = new RawJsonPanel(this);
auto* osdScroll = new QScrollArea(this);
osdScroll->setWidget(mOsdPanel);
...
mRightSplitter = new QSplitter(Qt::Horizontal, this);
mRightSplitter->addWidget(osdScroll);
mRightSplitter->addWidget(mRawJsonPanel);
```

替换为（在 `mOsdPanel` 创建后，`mRightSplitter` 创建前插入新的左半区面板）：

```cpp
    // === 右侧：OSD + JSON 水平分割 ===
    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 8, 8, 8);
    rightLayout->setSpacing(4);

    // OSD | JSON
    mOsdPanel = new OsdPanel(this);
    mRawJsonPanel = new RawJsonPanel(this);
    mOsdParsePanel = new OsdParsePanel(this);

    auto* osdScroll = new QScrollArea(this);
    osdScroll->setWidget(mOsdPanel);
    osdScroll->setWidgetResizable(true);
    osdScroll->setFrameShape(QFrame::NoFrame);

    // 左半区：OSD 面板 + JSON 解析面板 垂直堆叠
    auto* leftHalf = new QWidget(this);
    auto* leftHalfLayout = new QVBoxLayout(leftHalf);
    leftHalfLayout->setContentsMargins(0, 0, 0, 0);
    leftHalfLayout->setSpacing(4);
    leftHalfLayout->addWidget(osdScroll, 3);      // OSD 占 3 份
    leftHalfLayout->addWidget(mOsdParsePanel, 2);  // 解析占 2 份

    mRightSplitter = new QSplitter(Qt::Horizontal, this);
    mRightSplitter->addWidget(leftHalf);
    mRightSplitter->addWidget(mRawJsonPanel);
    mRightSplitter->setStretchFactor(0, 2);
    mRightSplitter->setStretchFactor(1, 3);
    mRightSplitter->setSizes({400, 600});

    rightLayout->addWidget(mRightSplitter, 1);
```

- [ ] **Step 3: 修改 `src/ui/MainWindow.cpp` — connectSignals()**

在 `connectSignals()` 末尾（`updateStatusBar();` 之前，第 413 行附近）添加信号连接：

```cpp
    // OsdParsePanel: topic 选中变化 → 更新解析面板
    connect(mTopicListWidget, &TopicListWidget::topicSelectionChanged,
            mOsdParsePanel, [this](const QString& selectedTopic) {
        QString sn = mDeviceTree->selectedDeviceSn();
        mOsdParsePanel->setTopic(sn, selectedTopic);
    });
```

在 `connectSignals()` 中，`mDeviceTree->rebuild(...)` 调用之后（第 412 行附近），添加 TopicMapping 初始化：

```cpp
    mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
```

替换为：

```cpp
    // 加载 topic 映射配置
    TopicMapping* mapping = new TopicMapping(this);
    QString mappingPath = QApplication::applicationDirPath() + "/topic_mappings.json";
    if (!mapping->load(mappingPath)) {
        // 文件不存在或损坏：使用内置默认映射，并自动生成文件
        qWarning() << "MainWindow: failed to load topic_mappings.json, using built-in fallback";
        mapping->loadFromString(TOPIC_MAPPINGS_BUILTIN);
        // 自动生成默认映射文件（首次启动）
        QFile outFile(mappingPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(TOPIC_MAPPINGS_BUILTIN);
            outFile.close();
            qDebug() << "MainWindow: auto-generated default topic_mappings.json";
        }
    }
    mOsdParsePanel->setTopicMapping(mapping);
    mOsdParsePanel->setDeviceManager(mDevMgr);

    mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
```

注意：`TOPIC_MAPPINGS_BUILTIN` 需要定义为内置的默认 JSON 字符串。在 `MainWindow.cpp` 文件中 `#include` 块之后、构造函数之前添加：

```cpp
// 内置默认 topic 映射 JSON（文件缺失时降级使用）
static const char* TOPIC_MAPPINGS_BUILTIN = R"(
{
    "topics": {
        "thing/product/{sn}/osd": {
            "description": "OSD 遥测数据",
            "fields": {
                "job_number": {"zh":"累计作业次数","unit":"次"},
                "electric_supply_voltage": {"zh":"供电电压","unit":"mV"},
                "working_voltage": {"zh":"工作电压","unit":"mV"},
                "wind_speed": {"zh":"风速","unit":"m/s"},
                "environment_temperature": {"zh":"环境温度","unit":"℃"},
                "humidity": {"zh":"湿度","unit":"%"},
                "latitude": {"zh":"纬度","unit":"°"},
                "longitude": {"zh":"经度","unit":"°"},
                "height": {"zh":"海拔高度","unit":"m"},
                "battery.capacity_percent": {"zh":"电池电量","unit":"%"},
                "horizontal_speed": {"zh":"水平速度","unit":"m/s"},
                "vertical_speed": {"zh":"垂直速度","unit":"m/s"},
                "attitude_head": {"zh":"航向角","unit":"°"},
                "attitude_pitch": {"zh":"俯仰角","unit":"°"},
                "attitude_roll": {"zh":"横滚角","unit":"°"},
                "home_distance": {"zh":"距Home距离","unit":"m"},
                "mode_code": {"zh":"模式码","unit":"","values":{"0":"待机","4":"自动起飞","5":"航线飞行","9":"自动返航","10":"自动降落"}},
                "drone_in_dock": {"zh":"飞机在舱","unit":"","values":{"0":"否","1":"是"}},
                "cover_state": {"zh":"舱盖","unit":"","values":{"0":"关闭","1":"打开"}},
                "position_state.gps_number": {"zh":"GPS搜星","unit":""},
                "position_state.rtk_number": {"zh":"RTK搜星","unit":""}
            },
            "groups": [
                {"id":"basic","label":"📋 基础信息","keys":["job_number","mode_code","drone_in_dock","cover_state"]},
                {"id":"power","label":"🔋 电源","keys":["electric_supply_voltage","working_voltage","battery.capacity_percent"]},
                {"id":"flight","label":"✈ 飞行","keys":["horizontal_speed","vertical_speed","attitude_head","attitude_pitch","attitude_roll","home_distance"]},
                {"id":"position","label":"📍 定位","keys":["latitude","longitude","height","position_state.gps_number","position_state.rtk_number"]},
                {"id":"environment","label":"🌡 环境","keys":["wind_speed","environment_temperature","humidity"]}
            ]
        }
    }
}
)";
```

并在 `MainWindow.cpp` 文件顶部 `#include` 区域添加：

```cpp
#include "TopicMapping.h"
```

- [ ] **Step 4: 修改 `src/ui/MainWindow.cpp` — onDeviceSelected()**

在 `onDeviceSelected()` 末尾添加（在函数 `return` 之前 / `mAddDeviceBtn->setEnabled` 之后）：

```cpp
    // 更新 OsdParsePanel
    QString selectedTopic = mTopicListWidget->selectedTopic();
    mOsdParsePanel->setTopic(sn, selectedTopic);
```

- [ ] **Step 5: 修改 `src/ui/MainWindow.cpp` — 取消选中**

在 `onDeviceSelected()` 中 `sn.isEmpty()` 分支，已清空面板，但还需清空 OsdParsePanel。找到：

```cpp
        mTopicListWidget->clearTopics();
```

在下一行添加：

```cpp
        mOsdParsePanel->clear();
```

- [ ] **Step 6: Commit**

```bash
git add src/ui/MainWindow.h src/ui/MainWindow.cpp
git commit -m "feat: MainWindow 集成 OsdParsePanel — 布局调整与信号连接"
```

---

### Task 5: 更新 CMakeLists.txt 并部署映射文件

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 修改 `CMakeLists.txt` 添加新文件**

在 `SOURCES` 列表中添加（第 30 行 `TopicListWidget.cpp` 之后）：

```cmake
    src/ui/OsdParsePanel.cpp
```

在 `HEADERS` 列表中添加（第 47 行类似位置）：

```cmake
    src/core/TopicMapping.h
    src/ui/OsdParsePanel.h
```

在 `configure_file` 小节（第 69-74 行）后添加 topic_mappings.json 的部署：

```cmake
# 复制 topic 映射配置文件到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/config/topic_mappings.json
    ${CMAKE_BINARY_DIR}/topic_mappings.json
    COPYONLY
)
```

同时更新 Windows 部署 target 中的文件复制（第 93-99 行 deploy target 中），在 `config.json` 行后添加：

```cmake
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config/topic_mappings.json"
            "${CMAKE_BINARY_DIR}/topic_mappings.json"
```

完整的 deploy target 变为：

```cmake
if(WIN32)
    add_custom_target(deploy
        COMMAND ${CMAKE_COMMAND} -E echo "=== Running windeployqt ==="
        COMMAND windeployqt
            --no-translations
            "$<TARGET_FILE:DjiCloudApi>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/resources/config.json"
            "${CMAKE_BINARY_DIR}/config.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config/topic_mappings.json"
            "${CMAKE_BINARY_DIR}/topic_mappings.json"
        DEPENDS DjiCloudApi
        COMMENT "Deploying Qt DLLs and config files for Windows distribution"
    )
endif()
```

- [ ] **Step 2: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: 添加 OsdParsePanel/TopicMapping 源文件及 topic_mappings.json 部署"
```

---

### Task 6: 编译与验证

- [ ] **Step 1: 编译项目**

```bash
cd build_mingw && cmake --build . 2>&1
```

期望: 无编译错误、无警告。

- [ ] **Step 2: 修复编译问题**

常见问题：
- `valToString` 函数放在了匿名 namespace 外 → 添加 `static` 或移到 namespace 中
- 缺少 `QApplication` include → 确保 `MainWindow.cpp` include 了 `<QApplication>`
- `QSet` 未 include → 确保 `OsdParsePanel.cpp` include 了 `<QSet>`

在 `OsdParsePanel.cpp` 顶部添加缺失的 include：

```cpp
#include <QSet>
```

**如果编译失败，修复后重复 Step 1，直到编译通过。**

- [ ] **Step 3: 启动应用手动验证**

```bash
cd build_mingw && ./DjiCloudApi.exe
```

验证步骤：
1. 应用启动 → 检查 `config/topic_mappings.json` 是否自动生成（首次）
2. 配置 MQTT 连接 → 连接 Broker
3. 添加设备 → 选中设备 → 选中 osd topic
4. 确认 JSON 解析面板显示分组表格数据：
   - 已映射字段显示中文名 + 值 + 单位
   - 值变化时短暂蓝闪
   - 未映射字段在底部灰色显示
5. 切换刷新间隔 → 确认间隔变化生效
6. 点击暂停 → 面板冻结 → 点击继续 → 立即刷新
7. 取消设备选中 → 面板清空

- [ ] **Step 4: Commit（如有修复）**

```bash
git add -A && git commit -m "fix: 编译修复与验证调整"
```

---

### Task 7: 最终构建产物

- [ ] **Step 1: 编译 Release 版本并部署 exe**

```bash
cd build_mingw && cmake --build . 2>&1 && cmake --build . --target deploy 2>&1
```

- [ ] **Step 2: 复制 exe 到 deploy 目录**

```bash
cp build_mingw/DjiCloudApi.exe deploy/DjiCloudApi.exe
```

- [ ] **Step 3: 提交最终产物**

```bash
git add deploy/DjiCloudApi.exe
git commit -m "chore: 更新编译产物 DjiCloudApi.exe (含 OSD JSON 解析面板)"
```

---

## Self-Review Checklist

- [x] Spec coverage: 外部映射配置文件 (Task 1,2), 定时解析翻译 (Task 3,4), 分组表格展示 (Task 3), 刷新间隔 (Task 3), 暂停恢复 (Task 3), 值变化高亮 (Task 3) — all 6 requirements covered
- [x] Placeholder scan: No TBD/TODO, no vague "add error handling", all code blocks are concrete
- [x] Type consistency: `TopicMappingConfig`, `FieldMapping`, `GroupDef` used consistently across Task 1 (definition) and Task 3 (usage)
- [x] Integration: MainWindow adjustments (Task 4) correctly reference classes defined in Task 1 and Task 3
