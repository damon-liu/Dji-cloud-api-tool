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

![image-20260612144649206](https://damon-siyuan.oss-cn-wuhan-lr.aliyuncs.com/markdown/image-20260612144649206.png)

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

## 项目结构

```
.
├── src/                          # 源代码
│   ├── main.cpp                  # 入口点
│   ├── core/                     # 核心层 — 业务逻辑与数据结构
│   │   ├── DeviceManager.h/cpp   #   中心调度器：设备增删、消息路由、OSD 缓存
│   │   ├── ConfigStore.h/cpp     #   配置文件读写、设备列表拆分/合并
│   │   ├── TopicManager.h/cpp    #   Topic ↔ 设备 SN 映射、订阅管理
│   │   ├── DeviceInfo.h          #   设备信息数据结构（纯头文件）
│   │   ├── OsdData.h             #   OSD 数据结构：AircraftOsd / DockOsd
│   │   └── TopicMapping.h        #   JSON key → 中文映射配置
│   ├── mqtt/                     # 通信层 — MQTT 客户端封装
│   │   └── MqttClientManager.h/cpp  QMqttClient 封装、自动重连、去重订阅
│   ├── ui/                       # UI 层 — 面板与对话框
│   │   ├── MainWindow.h/cpp      #   主窗口：工具栏、布局、信号连接
│   │   ├── DeviceTreeWidget.h/cpp #  设备树（机场/飞机）
│   │   ├── TopicListWidget.h/cpp  #  Topic 列表：启用/禁用/新增/删除
│   │   ├── TopicEditDialog.h/cpp  #  Topic 编辑对话框
│   │   ├── OsdPanel.h/cpp         #  OSD 设备信息面板（GroupBox）
│   │   ├── OsdParsePanel.h/cpp    #  JSON 字段 → 中文翻译面板
│   │   ├── RawJsonPanel.h/cpp     #  原始 JSON 显示（暂停/复制）
│   │   ├── PublishPanel.h/cpp     #  Topic 下发面板
│   │   └── ConfigDialog.h/cpp     #  MQTT 连接配置对话框
│   └── resources/
│       └── config.json           # 默认配置文件模板
├── config/
│   └── topic_mappings.json       # JSON key → 中文 翻译映射表
├── cmake/
│   └── toolchains/
│       └── linux-x64.cmake       # 交叉编译工具链（Zig）
├── deploy/
│   ├── DjiCloudApi.exe           # Windows 预编译包
│   ├── config.json               #   + 默认配置
│   ├── topic_mappings.json       #   + 翻译映射
│   └── Dockerfile                # Docker 镜像构建
├── tests/                        # 测试（待补充）
├── CMakeLists.txt                # CMake 构建配置
└── CLAUDE.md                     # AI 辅助开发指引
```

## 开发

### 环境准备

**Windows 本地开发：**

1. 安装 [Qt 6.x](https://www.qt.io/download)（选择 MinGW 或 MSVC 版本，确保勾选 `Qt MQTT` 模块）
2. 安装 [CMake](https://cmake.org/download/) ≥ 3.10
3. 将 Qt 和 MinGW/MSVC 的 `bin` 目录加入系统 `PATH`

**Linux 本地开发：**

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake qt6-base-dev qt6-mqtt-dev

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmqtt-devel

# Arch
sudo pacman -S gcc cmake qt6-base qt6-mqtt
```

**Windows → Linux 交叉编译（Zig）：**

下载 [Zig](https://ziglang.org/download/) 并设置 `ZIG_PATH` 环境变量，Zig 同时提供 C/C++ 编译器，无需额外安装交叉编译工具链。

### 构建

```bash
# 克隆仓库
git clone https://github.com/damon-liu/Dji-cloud-api-tool.git
cd Dji-cloud-api-tool

# === Windows (MinGW) ===
cmake -B build_mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_mingw

# 部署 Qt DLL 到构建目录（可选，便于分发）
cmake --build build_mingw --target deploy

# === 原生 Linux ===
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# === Windows → Linux 交叉编译（Zig） ===
export ZIG_PATH=/path/to/zig/zig.exe
cmake -B build_linux -G "MinGW Makefiles" \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x64.cmake \
      -DZIG_PATH="$ZIG_PATH" \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build_linux
# 产物：build_linux/main
```

### 运行（开发调试）

```bash
# Windows — 直接运行
./build_mingw/DjiCloudApi.exe

# Linux — 直接运行
./build/main
```

构建目录下会自动复制 `config.json` 和 `topic_mappings.json`，可直接修改后运行。

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| C++ 标准 | 17 | `CMAKE_CXX_STANDARD 17` |
| Qt 6 | 6.x | Core、Widgets、Mqtt 模块 |
| CMake | ≥ 3.10 | 构建系统 |
| 编译器 | MSVC 2022 / MinGW-w64 / GCC / Clang | Zig 交叉编译使用 Clang |

## License

MIT
