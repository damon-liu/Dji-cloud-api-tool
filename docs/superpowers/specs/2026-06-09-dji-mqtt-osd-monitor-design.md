# DJI-CLOUD-API 监控客户端 — 设计方案

> 日期: 2026-06-09
> 版本: v1.0
> 状态: 待评审

## 一、项目概述

基于 Qt C++ 实现的 DJI 上云 API MQTT 监控客户端。连接单个 MQTT Broker，订阅多个机场（Dock）和手飞飞机（Pilot）的 OSD 遥测数据，以树形结构展示设备层级，右侧面板实时刷新 OSD 属性。

**参考 API 文档：**
- 机场（Dock-to-Cloud）topic 定义: https://developer.dji.com/doc/cloud-api-tutorial/cn/api-reference/dock-to-cloud/mqtt/topic-definition.html
- 飞机（Pilot-to-Cloud）topic 定义: https://developer.dji.com/doc/cloud-api-tutorial/cn/api-reference/pilot-to-cloud/mqtt/topic-definition.html

## 二、关键决策

| # | 决策项 | 选择 |
|---|--------|------|
| 1 | MQTT 拓扑 | 单 Broker，Topic 区分设备 |
| 2 | UI 设备组织 | 左侧树形结构（机场→子飞机 + 独立手飞飞机） |
| 3 | 配置持久化 | JSON 配置文件，启动自动加载，支持运行时修改 |
| 4 | 设备规模 | 5-20 个机场，10-100 架飞机（中等规模） |
| 5 | MQTT 库 | Qt MQTT（QMqttClient） |
| 6 | OSD 展示 | 右侧属性面板实时刷新，数值变化高亮，同时展示原始 JSON |
| 7 | 预留功能 | 为 Topic 下发配置（发布 JSON 指令）预留界面区域 |
| 8 | 部署平台 | Ubuntu |

## 三、整体架构

```
┌──────────────────────────────────────────────────────────────┐
│                       Qt 主线程 (UI)                         │
│  ┌──────────┐  ┌──────────────┐  ┌─────────────────────────┐│
│  │ 设备树    │  │ OSD属性面板   │  │  配置/Topic管理对话框    ││
│  │ (左侧)   │  │ (右侧)       │  │  (连接配置/Topic增删改)  ││
│  └────┬─────┘  └──────▲───────┘  └──────────┬──────────────┘│
│       │               │                     │                │
│  ┌────▼───────────────▼─────────────────────▼─────────────┐  │
│  │                  DeviceManager (核心)                    │  │
│  │  · 设备树管理(机场/飞机)  · OSD缓存  · Topic路由         │  │
│  │  · 配置读写(连接+Topic)  · 连接状态汇总                  │  │
│  └────────────────────────┬───────────────────────────────┘  │
│                           │                                  │
│  ┌────────────────────────▼───────────────────────────────┐  │
│  │              MqttClientManager                           │  │
│  │  · 单一 QMqttClient 实例                                 │  │
│  │  · Topic 订阅/取消/管理                                  │  │
│  │  · 断线自动重连                                          │  │
│  └────────────────────────┬───────────────────────────────┘  │
└───────────────────────────┼─────────────────────────────────┘
                            │
                    ┌───────▼────────┐
                    │   MQTT Broker  │
                    │  (ip:port,     │
                    │   user, pass)  │
                    └───────▲────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
   ┌────▼────┐        ┌────▼────┐        ┌─────▼────┐
   │ 机场 A   │        │ 机场 B   │        │ 手飞飞机 C │
   │ (Dock)  │        │ (Dock)  │        │ (Pilot)  │
   │ topic:  │        │ topic:  │        │ topic:   │
   │ /sn_a/  │        │ /sn_b/  │        │ /sn_c/   │
   └─────────┘        └─────────┘        └──────────┘
```

**分层职责：**

| 层 | 职责 | 关键类 |
|----|------|--------|
| UI 层 | 界面展示与交互 | `MainWindow`, `DeviceTreeWidget`, `OsdPanel`, `ConfigDialog`, `TopicEditDialog` |
| 核心层 | 设备管理、OSD 数据缓存、topic 路由、配置管理 | `DeviceManager`, `ConfigStore` |
| 通信层 | MQTT 连接、订阅、断线重连 | `MqttClientManager` |
| 数据层 | 配置读写、OSD 数据解析、数据模型 | `ConfigStore`, `OsdData`, `DeviceInfo` |

## 四、模块划分与文件结构

```
src/
├── main.cpp                    # 入口
├── core/
│   ├── DeviceManager.h/cpp     # 设备树管理，OSD数据路由
│   ├── DeviceInfo.h            # 设备数据结构（SN、类型、名称等）
│   ├── OsdData.h               # OSD数据模型（基类+AircraftOsd+DockOsd）
│   └── ConfigStore.h/cpp       # JSON配置文件读写
├── mqtt/
│   ├── MqttClientManager.h/cpp # QMqttClient封装，断线重连
│   └── TopicManager.h/cpp      # Topic增删改，消息路由
├── ui/
│   ├── MainWindow.h/cpp        # 主窗口布局编排
│   ├── DeviceTreeWidget.h/cpp  # 左侧设备树
│   ├── OsdPanel.h/cpp          # 右侧OSD属性面板（上部）
│   ├── RawJsonPanel.h/cpp      # 原始JSON展示面板（中部，只读）
│   ├── PublishPanel.h/cpp      # Topic下发面板（下部，预留，默认隐藏）
│   ├── ConfigDialog.h/cpp      # MQTT连接配置对话框
│   └── TopicEditDialog.h/cpp   # Topic增删改对话框
└── resources/
    └── config.json             # 默认配置模板
```

## 五、数据流

```
MQTT Broker
    │  MQTT message (topic + JSON payload)
    ▼
MqttClientManager
    │  signal: messageReceived(topic, payload)
    ▼
DeviceManager
    │  1. 根据 topic 中的 SN 定位设备
    │  2. 解析 JSON → OsdData 结构体
    │  3. 缓存原始 JSON 字符串和结构化 OSD
    │  4. 发出 signal: deviceOsdUpdated(deviceSn, rawJson, osdData)
    ▼
┌──────────────────┬───────────────────┐
│ OsdPanel (UI)    │ RawJsonPanel (UI) │
│ 刷新属性列表      │ 更新原始 JSON 显示  │
│ 变化项高亮        │                   │
└──────────────────┴───────────────────┘
```

## 六、OSD 数据模型

DJI 机场（Dock）和飞机（Aircraft）上报的 OSD 字段不同，共用基类各自扩展：

```
OsdBase (公共字段)
    timestamp:      时间戳
    longitude:      经度
    latitude:       纬度
    altitude:       海拔高度

AircraftOsd : OsdBase (飞机特有字段)
    battery_percent:     电量百分比 (0-100)
    battery_voltage:     电压 (mV)
    speed_horizontal:    水平速度 (m/s)
    speed_vertical:      垂直速度 (m/s)
    heading:             航向角 (度)
    pitch:               俯仰角 (度)
    roll:                横滚角 (度)
    yaw:                 偏航角 (度)
    home_distance:       距home点距离 (m)
    flight_time_sec:     已飞行时间 (秒)
    rc_signal_strength:  遥控信号强度 (0-100)

DockOsd : OsdBase (机场特有字段)
    cover_state:           舱盖状态 (open/closed)
    drone_in_dock:         飞机是否在库内 (true/false)
    working_voltage:       工作电压 (mV)
    working_current:       工作电流 (mA)
    backup_battery_voltage: 备用电池电压 (mV)
    wind_speed:            风速 (m/s, 可选)
    environment_temp:      环境温度 (℃, 可选)
    environment_humidity:  环境湿度 (%, 可选)
```

## 七、MQTT Topic 定义

### 机场 (Dock-to-Cloud)

| Topic | 方向 | 用途 |
|-------|------|------|
| `thing/product/{dock_sn}/osd` | 设备→云 | 机场 OSD 遥测 |
| `thing/product/{drone_sn}/osd` | 设备→云 | 库内飞机 OSD 遥测 |
| `sys/product/{dock_sn}/status` | 设备→云 | 机场在线/离线状态 |

### 手飞飞机 (Pilot-to-Cloud)

| Topic | 方向 | 用途 |
|-------|------|------|
| `thing/product/{pilot_sn}/osd` | 设备→云 | 飞机 OSD 遥测 |
| `sys/product/{pilot_sn}/status` | 设备→云 | 飞机在线/离线状态 |

## 八、UI 布局

```
┌──────────────────────────────────────────────────────────────────────┐
│  菜单栏: [连接] [设备] [帮助]                                          │
├──────────┬───────────────────────────────────────────────────────────┤
│ 设备树   │  ◆ OSD 属性面板                                             │
│          │  ┌─────────────────┬────────────────┐                     │
│ 🏢 机场A │  │ 设备: 飞机1      │ 类型: Aircraft │                     │
│  └ ✈️飞机1│  ├─────────────────┼────────────────┤                     │
│ 🏢 机场B │  │ 在线: 🟢在线     │ 更新: 0.8s    │                     │
│  └ ✈️飞机2│  └─────────────────┴────────────────┘                     │
│ ✈ 手飞X  │  经度:113.412  纬度:23.058  高度:120.5m                   │
│ ✈ 手飞Y  │  电量:85%  速度:12m/s  航向:270°                          │
│          │  Pitch:3°  Roll:2°  Yaw:270°                              │
│ 右键菜单 ├───────────────────────────────────────────────────────────┤
│ 添加设备 │  ◆ 原始 JSON (只读)                       [📋一键复制]     │
│ 删除设备 │  ┌───────────────────────────────────────────────────────┐│
│ 编辑Topic│  │ {                                                     ││
│          │  │   "tid": "...",                                       ││
│ ~200px   │  │   "data": { ... }                                     ││
│          │  │ }                                                     ││
│          │  └───────────────────────────────────────────────────────┘│
│          ├───────────────────────────────────────────────────────────┤
│          │  [▶ Topic下发]  ← 点击展开（默认隐藏）                     │
├──────────┴───────────────────────────────────────────────────────────┤
│  状态栏: 🔗已连接 | Broker: 192.168.1.100:8883 | 设备: 8               │
└──────────────────────────────────────────────────────────────────────┘
```

**右侧面板三个区域（垂直排列，可通过分割器调整高度）：**

| 区域 | 说明 | v1.0 |
|------|------|------|
| OSD 属性面板（上部） | 结构化展示 OSD 字段，按分组排列 | ✅ 实现 |
| 原始 JSON（中部） | 实时显示当前选中设备最新一条 OSD JSON，只读，右上角**一键复制按钮** | ✅ 实现 |
| Topic 下发（下部） | 目标 topic 选择框 + JSON 编辑框 + 发送按钮，**默认隐藏**，点击按钮展开 | 📋 仅 UI 占位 |

**交互说明：**

| 操作 | 方式 |
|------|------|
| 连接/断开 Broker | 菜单栏 → 连接 → 配置 → 连接 |
| 添加设备 | 设备树空白处右键 → 添加设备（选择 Dock/Pilot 类型，填写 SN 和名称） |
| 删除设备 | 右键设备节点 → 删除 |
| 编辑 Topic | 右键设备节点 → 编辑 Topic → 弹出对话框增删改 |
| 查看 OSD | 左侧单击设备节点，右侧 OSD 面板刷新 |
| 查看/复制原始 JSON | 选中设备后 JSON 区域自动显示，点 📋 按钮复制到剪贴板 |
| 打开 Topic 下发 | 点击 [▶ Topic下发] 按钮展开面板（默认隐藏），展开后可编辑和发送 |
| 状态栏 | 实时显示连接状态和设备统计 |

## 九、配置文件格式

```json
{
    "mqtt": {
        "host": "192.168.1.100",
        "port": 8883,
        "username": "admin",
        "password": ""
    },
    "devices": [
        {
            "type": "dock",
            "name": "机场A",
            "sn": "dock_sn_001",
            "aircraft_sn": "drone_sn_001",
            "topics": [
                "thing/product/dock_sn_001/osd",
                "thing/product/drone_sn_001/osd"
            ]
        },
        {
            "type": "pilot",
            "name": "手飞飞机X",
            "sn": "pilot_sn_001",
            "topics": [
                "thing/product/pilot_sn_001/osd"
            ]
        }
    ]
}
```

## 十、技术实现要点

### 10.1 MQTT 连接
- 使用 Qt MQTT 模块的 `QMqttClient`
- 连接参数：host, port, username, password
- 连接成功自动订阅所有已配置 topic
- 断线自动重连（指数退避，最大间隔 30 秒）
- 连接/断开状态通过 signal 通知 UI

### 10.2 Topic 管理
- 启动时从配置文件加载所有 topic
- 支持运行时增删改 topic（对话框操作 + 立即生效）
- topic 变更后持久化到配置文件
- topic 按设备 SN 分组管理

### 10.3 OSD 数据解析
- 使用 Qt 的 `QJsonDocument` 解析 JSON payload
- 根据 topic 中的 SN 判断设备类型，选择对应解析器
- 解析失败记录日志，不影响其他消息处理
- OSD 数据缓存最新值，供 UI 面板读取

### 10.4 线程模型
- 全部运行在主线程，基于 Qt 信号/槽事件循环
- `QMqttClient` 内部异步处理网络 I/O，不阻塞 UI

### 10.5 错误处理
- MQTT 连接失败 → 状态栏提示 + 自动重试
- Topic 订阅失败 → 弹窗警告，记录失败 topic
- JSON 解析异常 → 日志记录原始 payload
- 配置文件不存在 → 首次启动生成默认模板

## 十一、v1.0 范围与后续扩展

**当前实现（v1.0）：**
- ✅ MQTT 连接/断线重连
- ✅ 设备树展示（机场+飞机 + 手飞飞机）
- ✅ Topic 订阅增删改
- ✅ 机场 OSD 数据接收与展示
- ✅ 飞机 OSD 数据接收与展示
- ✅ 原始 JSON 实时展示（只读）
- ✅ JSON 配置文件持久化
- 📋 Topic 下发界面占位（仅 UI，发送功能后续实现）

**不在 v1.0 范围（后续扩展）：**
- ❌ 地图集成（设备位置可视化）
- ❌ Topic 下发（publish JSON 指令到设备）
- ❌ 飞行任务指令下发（services topic）
- ❌ 历史数据曲线图
- ❌ 多 Broker 连接
- ❌ DRC 远程控制

---

> 🤖 Generated with [Claude Code](https://claude.com/claude-code)
