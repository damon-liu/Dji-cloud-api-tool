# DJI Cloud API 监控客户端

基于 Qt 6 C++ 的桌面应用，连接 DJI Cloud API MQTT Broker，订阅无人机/机场的遥测主题，实时展示 OSD（机载系统数据）并自动翻译 JSON 字段为中文。

## 功能

- **MQTT 连接管理** — 支持 SSL/TLS，可配置 Broker 地址/端口/账号密码，连接状态实时显示
- **设备列表** — 树形展示机场 + 手飞无人机，支持新增/删除设备，在线状态指示
- **Topic 订阅管理** — 启用/禁用/新增/删除 Topic，点击即可切换原始 JSON 过滤
- **OSD 遥测面板** — 设备基本信息 + 机场数据（舱盖、风速、温度、搜星等）实时刷新
- **JSON 字段翻译** — 自动将 `thing/product/{sn}/osd` 主题的 JSON key 翻译为中文，分组表格展示，点击字段名可复制 key
- **原始 JSON 面板** — 按 Topic 过滤、暂停滚动、一键复制
- **Topic 下发** — 支持向设备下发自定义 MQTT 消息（v1.1）
- **可配置刷新间隔** — 适合不同网络环境和数据量

## 快速开始

### Windows

直接运行 `deploy/DjiCloudApi.exe`，无需安装任何依赖。

首次启动会自动生成默认配置文件 `config.json`，修改 Broker 连接信息后点击「连接」即可。

### Linux

```bash
# 从 deploy 目录运行（需安装 Qt 6 运行时）
./deploy/main
```

### Docker

```bash
cp build_linux/main deploy/main
docker build -t dji-cloud-api:latest -f deploy/Dockerfile .
docker run --rm dji-cloud-api:latest
```

## 配置文件

应用启动后自动在可执行文件同目录生成 `config.json`：

```json
{
    "mqtt": {
        "host": "your-broker.example.com",
        "port": 8883,
        "username": "admin",
        "password": ""
    },
    "devices": [
        {
            "type": "dock",
            "sn": "dock_001",
            "name": "机场1号",
            "aircraft_sn": "drone_001",
            "topics": [
                "thing/product/dock_001/osd",
                "thing/product/drone_001/osd"
            ]
        }
    ]
}
```

主题字符串支持 `{sn}` 占位符，运行时会自动替换为设备 SN。

## 界面截图

![主界面](screenshots/main.png)

## 架构

三层设计，单线程运行，通过 Qt 信号/槽实现异步 I/O：

```
UI 层 (src/ui/)         — MainWindow、DeviceTreeWidget、OsdPanel、RawJsonPanel、ConfigDialog、TopicEditDialog
核心层 (src/core/)       — DeviceManager（中心调度器）、ConfigStore、TopicManager、DeviceInfo/OsdData 数据结构
通信层 (src/mqtt/)       — MqttClientManager（QMqttClient 封装，指数退避自动重连）
```

**数据流：**

```
MQTT 消息 → MqttClientManager::messageReceived
  → DeviceManager::parseAndRoute()
    → TopicManager 匹配主题到设备
    → 解析 DJI 格式 JSON（{"tid":..., "data":...}）
    → 缓存 OSD，发出 deviceOsdUpdated(sn, rawJson)
      → UI 面板响应式更新
```

**线程模型：** 一切运行在 Qt 主线程。`QMqttClient` 是异步的——无需工作线程。

### 关键类

| 类 | 职责 |
|---|---|
| `DeviceManager` | 中心调度器——持有 ConfigStore、TopicManager、MqttClientManager；设备增删改查；消息路由；OSD 缓存 |
| `ConfigStore` | JSON 配置持久化。处理设备列表中机场→子飞机的拆分/合并逻辑 |
| `TopicManager` | 主题到设备 SN 的映射及反向索引；发出 `topicsChanged` 信号触发 MQTT 重新订阅 |
| `MqttClientManager` | `QMqttClient` 封装；指数退避自动重连（基数 1s，上限 30s）；去重订阅管理 |
| `DeviceInfo` / `OsdData` | 纯头文件数据结构。`AircraftOsd` 和 `DockOsd` 继承 `OsdBase` |
| `MainWindow` | 顶层窗口（1280×760），水平分割器：左侧设备树 + Topic 列表，右侧 OSD 详情 + 原始 JSON |

## 开发

### 构建（Windows MinGW + Qt 6）

```bash
cmake -B build_mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_mingw

# 部署 Qt DLL（可选）
cmake --build build_mingw --target deploy
```

### 原生 Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 从 Windows 交叉编译到 Linux（通过 Zig）

```bash
export ZIG_PATH=/path/to/zig/zig.exe
cmake -B build_linux -G "MinGW Makefiles" \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x64.cmake \
      -DZIG_PATH="$ZIG_PATH" \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build_linux
# 产物：build_linux/main
```

### 环境要求

- C++17
- Qt 6（Core、Widgets、Mqtt 模块）
- CMake ≥ 3.10
- 编译器：MSVC 2022 / MinGW-w64 / GCC / Clang（Zig 交叉编译）

## License

MIT
