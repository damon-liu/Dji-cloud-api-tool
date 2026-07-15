# DJI Cloud API 监控客户端



基于 Qt 6 C++ 的桌面应用，连接 DJI Cloud API MQTT Broker，订阅无人机/机场的遥测主题，实时展示 OSD（机载系统数据）并自动翻译 JSON 字段为中文。

[![GitHub Release](https://img.shields.io/github/v/release/damon-liu/Dji-cloud-api-tool?style=flat-square&color=blue)](https://github.com/damon-liu/Dji-cloud-api-tool/releases/latest)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey?style=flat-square)](https://github.com/damon-liu/Dji-cloud-api-tool/releases/latest)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)

## 📥 下载

前往 [Releases](https://github.com/damon-liu/Dji-cloud-api-tool/releases) 页面下载最新版本：

- **`DjiCloudApiTool-v1.0.1.zip`** — 完整包（exe + Qt 运行库 + 配置模板），解压即用

> 无需安装 Qt 或其他依赖，Windows 10/11 x64 下直接运行。首次启动自动生成 `config.json`。

## ✨ 功能

- **MQTT 连接管理** — 支持 SSL/TLS，可配置 Broker 地址/端口/账号密码，连接状态实时显示
- **多环境配置切换** — 支持多个 Connection 配置，一键切换生产/测试环境，互不干扰
- **设备树形管理** — 机场 + 无人机层级展示，支持新增/删除/重命名，在线状态实时指示
- **机场飞机自动发现** — 连接机场后自动识别并添加关联的无人机（从机场 OSD 中提取）
- **OSD 遥测面板** — 机场信息与飞机信息并排显示，刷新间隔可调（1s/2s/5s/10s）
- **JSON 字段自动翻译** — 英文字段自动翻译为中文，按分组展示，点击字段名即可复制原始 key
- **原始 JSON 面板** — 按 Topic 过滤 MQTT 报文，支持暂停滚动、一键复制、清除历史记录
- **Topic 订阅管理** — 每个设备独立管理 Topic，支持新增/删除/启用/禁用/拖拽排序；添加机场自动创建 7 个常用 Topic
- **Topic 下发** — 预设常用下发 Topic，自动匹配参数模板（从 `topic-send-construct.md` 解析），支持 JSON 编辑、发送历史、双击恢复
- **📦 抓包导出** — 一键抓包，数据实时写入 `captures/` 文件夹，停止后弹窗显示路径和记录条数
- **自动重连** — 指数退避（1s → 2s → 4s → … → 最长 30s），恢复后自动刷新
- **断线保护** — 断线自动暂停面板刷新，手动暂停优先级更高，不打断数据查看
- **免安装绿色运行** — 单 zip 包，解压即用，零依赖

> 🔜 v1.1 预告：JSON 解析优化、暗色模式支持、批量设备管理。详见 [用户指南](docs/user-guide.md)。

## 🚶 快速开始

### Windows

从 [Releases](https://github.com/damon-liu/Dji-cloud-api-tool/releases/latest) 下载 `DjiCloudApiTool-v1.0.1.zip`，解压后双击 `DjiCloudApi.exe` 即可运行。

**三步上手：**

1. **配置 MQTT** — 点击「⚙ 配置」→ 填写 Broker 地址/端口/用户名/密码 → 「Test」测试 → 「OK」保存
2. **添加设备** — 点击「＋」→ 选择 Dock（机场）或 Pilot（无人机）→ 输入设备 SN 和名称
3. **连接监控** — 点击「● 连接」，OSD 面板和 JSON 面板开始实时刷新

详细教程见 [📖 用户使用指南](docs/user-guide.md)。

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

## ⚙️ 配置文件

应用启动后自动在可执行文件同目录生成 `config.json`：

```json
{
    "current_profile": "默认",
    "profiles": [
        {
            "name": "默认",
            "mqtt": {
                "host": "your-broker.example.com",
                "port": 8883,
                "username": "admin",
                "password": "YOUR_PASSWORD"
            },
            "devices": [
                {
                    "type": "dock",
                    "sn": "YOUR_DOCK_SN",
                    "name": "示例机场",
                    "aircraft_sn": "",
                    "topics": [
                        "thing/product/{sn}/osd",
                        "thing/product/{sn}/state"
                    ]
                }
            ]
        }
    ]
}
```

- 支持多个 Profile（Connection），每个 Profile 有独立的 MQTT 参数和设备列表
- 主题字符串支持 `{sn}` 占位符，运行时会自动替换为设备 SN
- ⚠️ 此文件包含 MQTT 密码，请勿分享或提交到公开仓库

## 📸 界面截图

![image-20260710165818685](https://damon-siyuan.oss-cn-wuhan-lr.aliyuncs.com/markdown/image-20260710165818685.png)

## 🏗 架构

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
| `ConfigStore` | JSON 配置持久化（多 Profile）。处理设备列表中机场→子飞机的拆分/合并逻辑 |
| `TopicManager` | 主题到设备 SN 的映射及反向索引；发出 `topicsChanged` 信号触发 MQTT 重新订阅 |
| `MqttClientManager` | `QMqttClient` 封装；指数退避自动重连（基数 1s，上限 30s）；去重订阅管理 |
| `DeviceInfo` / `OsdData` | 纯头文件数据结构。`AircraftOsd` 和 `DockOsd` 继承 `OsdBase` |
| `MainWindow` | 顶层窗口（1280×760），水平分割器：左侧设备树 + Topic 列表（可拖拽），右侧 OSD + JSON 解析 + 原始 JSON |
| `PublishPanel` | Topic 下发面板：预设常用 Topic、MD 模板自动匹配、JSON 编辑、发送历史 |

## 📁 项目结构

```
.
├── src/                          # 源代码
│   ├── main.cpp                  # 入口点
│   ├── core/                     # 核心层 — 业务逻辑与数据结构
│   │   ├── DeviceManager.h/cpp   #   中心调度器：设备增删、消息路由、OSD 缓存
│   │   ├── ConfigStore.h/cpp     #   配置文件读写（多 Profile）、设备列表管理
│   │   ├── TopicManager.h/cpp    #   Topic ↔ 设备 SN 映射、订阅管理
│   │   ├── DeviceInfo.h          #   设备信息数据结构（纯头文件）
│   │   ├── OsdData.h             #   OSD 数据结构：AircraftOsd / DockOsd
│   │   └── TopicMapping.h        #   JSON key → 中文映射配置
│   ├── mqtt/                     # 通信层 — MQTT 客户端封装
│   │   └── MqttClientManager.h/cpp  QMqttClient 封装、自动重连、去重订阅
│   ├── ui/                       # UI 层 — 面板与对话框
│   │   ├── MainWindow.h/cpp      #   主窗口：工具栏、布局、信号连接
│   │   ├── DeviceTreeWidget.h/cpp #  设备树（机场/飞机层级）
│   │   ├── TopicListWidget.h/cpp  #  Topic 列表：启用/禁用/新增/删除/排序
│   │   ├── TopicEditDialog.h/cpp  #  Topic 编辑对话框
│   │   ├── OsdPanel.h/cpp         #  OSD 设备信息面板
│   │   ├── TopicParsePanel.h/cpp  #  JSON 字段 → 中文翻译面板
│   │   ├── RawJsonPanel.h/cpp     #  原始 JSON 显示（暂停/复制/抓包）
│   │   ├── PublishPanel.h/cpp     #  Topic 下发面板（预设模板、JSON 编辑、发送历史）
│   │   └── ConfigDialog.h/cpp     #  MQTT 连接配置对话框（多 Profile）
│   └── resources/
│       └── config.json           # 默认配置文件模板（不含真实凭证）
├── config/
│   ├── topic_mappings.json       # JSON key → 中文翻译映射表
│   └── topic-send-construct/
│       └── topic-send-construct.md  # Topic 下发参数模板定义
├── cmake/
│   └── toolchains/
│       └── linux-x64.cmake       # 交叉编译工具链（Zig）
├── deploy/
│   ├── DjiCloudApi.exe           # Windows 预编译可执行文件
│   ├── config.example.json       # 配置模板（含示例值）
│   ├── topic_mappings.json       # 翻译映射表
│   └── Dockerfile                # Docker 镜像构建
├── docs/
│   ├── user-guide.md             # 小白使用指南
│   ├── prd.md                    # 产品需求文档
│   └── RELEASE-v1.0.md           # Release 发布说明
├── package.sh                    # 一键打包脚本（编译 → 部署 → 清除凭证 → 打包）
├── CMakeLists.txt                # CMake 构建配置
└── CLAUDE.md                     # AI 辅助开发指引
```

## 🛠 开发

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

# 部署 Qt DLL（用于分发）
cmake --build build_mingw --target deploy

# === 一键打包（编译 + 部署 DLL + 清除凭证 + 打包 zip） ===
bash package.sh v1.0.1

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

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| C++ 标准 | 17 | `CMAKE_CXX_STANDARD 17` |
| Qt 6 | 6.x | Core、Widgets、Mqtt 模块 |
| CMake | ≥ 3.10 | 构建系统 |
| 编译器 | MSVC 2022 / MinGW-w64 / GCC / Clang | Zig 交叉编译使用 Clang |

关注我，后期版本更新均在公众号上通知！！

<img width="430" height="430" alt="img_v3_0213k_6b03e120-c73b-43f4-812a-c9fc2a9ca33g" src="https://github.com/user-attachments/assets/626fc73a-3a74-4514-b5c4-1829dd6f04c6" />
## 📄 License

MIT
