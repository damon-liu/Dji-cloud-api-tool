# DJI Cloud API 监控客户端



基于 Qt 6 C++ 的桌面应用，连接 DJI Cloud API MQTT Broker，订阅无人机/机场的遥测主题，实时展示 OSD（机载系统数据）并自动翻译 JSON 字段为中文。

[![GitHub Release](https://img.shields.io/github/v/release/damon-liu/Dji-cloud-api-tool?style=flat-square&color=blue)](https://github.com/damon-liu/Dji-cloud-api-tool/releases/latest)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey?style=flat-square)](https://github.com/damon-liu/Dji-cloud-api-tool/releases/latest)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)

## 📥 下载

前往 [Releases](https://github.com/damon-liu/Dji-cloud-api-tool/releases) 页面下载最新版本：

- **`DjiCloudApiTool-v1.0.4.zip`** — 完整包（exe + Qt 运行库 + 配置模板），解压即用

> 无需安装 Qt 或其他依赖，Windows 10/11 x64 下直接运行。首次启动自动生成 `config.json` 在 `config/` 子目录下。

## ✨ 核心功能

- **多环境一键切换**：支持多个 Connection 配置，生产/测试环境互不干扰
- **设备树形管理**：机场 + 无人机层级展示，在线状态实时指示，支持新增/删除/重命名
- **机场飞机自动订阅**：连接机场后自动识别并添加关联的无人机
- **设备 OSD 面板**：机场信息与飞机信息并排显示，刷新间隔可调，鼠标悬停显示字段 key、点击可复制
- **Topic 订阅管理**：每个设备独立管理 Topic，自动订阅大疆机场 7 个常用 Topic，支持新增/删除/启用/禁用/拖拽排序
- **上报JSON管理**：按设备 Topic 过滤报文，支持暂停滚动、一键复制、清除历史
- **上报JSON抓包导出**：一键抓包，数据实时写入 `captures/` 文件夹，停止后弹窗显示路径和记录条数
- **上报JSON 字段自动翻译**：根据大疆上云官网自动翻译 Topic 上报 json 参数 key，按分组展示，支持网格和列表分组查看
- **Topic 下发**：自动预设常用下发 Topic，每个 Topic 提供官方下发参数 JSON 模板
- **Topic 下发记录**：自动记录每次下发与响应，成功/失败一目了然
- **控制中心-机场控制**：集成远程调试、飞机电源、机场舱盖、飞机充电、机场维护、强制关舱门等快捷控制，实时显示发送及响应消息记录
- **控制中心-飞行控制**：支持飞行控制权限抢夺/释放、一键起飞（参数可配置）/返航/取消返航/紧急停机等飞行指令
- **控制中心-负载控制**：支持负载控制权限抢夺/释放、拍照/录像/云台回中等负载控制；支持多机场下拉切换
- **控制中心-PSDK 喊话器**：支持 TTS 文字转语音播报和音频文件 URL 播报，音量可调，播报模式可切换，实时进度管线展示
- **视频直播**：飞机/机场双窗口独立直播，支持镜头切换（红外/变焦/广角）、视频源切换（内外视频）、5档清晰度调节
- **嵌入应用图标**：exe 文件嵌入品牌图标，任务栏和资源管理器正确显示应用标识
- **自动重连**：指数退避（1s → 2s → … → 最长 30s），恢复后自动刷新
- **断线保护**：断线自动暂停面板刷新，手动暂停优先级更高，不打断数据查看

## 🆚 vs MQTTX

很多开发者使用 [MQTTX](https://mqttx.app/) 进行 MQTT 调试，但针对 DJI Cloud API 场景，本工具有以下明显优势：

| 对比维度 | MQTTX | DjiCloudApiTool |
|----------|-------|-----------------|
| 设备识别 | 只能看到 Topic 字符串，需人工判断是哪个设备 | 自动识别设备 SN，树形展示机场与子飞机的层级关系 |
| 设备管理 | 所有 Topic 混在一起，无层级结构 | 每个设备独立管理 Topic，支持启用/禁用/拖拽排序 |
| 自动关联子设备 | 无 | 机场添加以后发现并订阅子飞机 |
| 自动订阅 | 无（需手动逐个添加 Topic） | 添加机场自动订阅 7 个Topic |
| 上报JSON管理 | 无分类筛选能力 | 支持原始 JSON 实时滚动、暂停/恢复、一键复制、清除历史 |
| JSON抓包导出 | 需手动复制粘贴或配置外部工具 | 一键抓包，数据实时写入 `captures/` 文件夹，按设备/Topic 分类保存 |
| JSON自动翻译 | 原始 JSON，字段名全英文（如 `drone_in_dock`） | 自动翻译为中文，按分组展示，一目了然 |
| Topic 指令下发 | 手动输入 Topic 字符串 + 手写 JSON 参数 | 自动预设常用6个下发 Topic，每个Topic提供官方下发参数 JSON模板 |
| Topic下发记录 | 需自行保存或外部记录 | 自动记录每次下发与响应，成功/失败一目了然 |
| 机场控制 | 需手动拼装远程调试/电源/舱盖/充电等指令 JSON | 点击按钮一键控制，进入远程调试后可用，实时反馈执行结果 |
| 飞行控制 | 需手动拼装起飞/返航/降落指令 JSON | 独立飞行控制窗口，一键起飞/返航/降落，含飞行器状态反馈 |
| 负载控制 | 需手动拼装拍照/录像指令 JSON | 独立飞行控制窗口，一键拍照/录像 |
| PSDK 喊话器 | 需手动拼装指令 JSON | 独立 PSDK 喊话器面板，可视化配置播报参数，实时进度管线展示 |
| 视频直播 | 无视频直播能力 | 飞机/机场双窗口独立直播，支持飞机多镜头、机场内外视频源切换、清晰度调节 |
| 环境依赖 | 需安装 Node.js 或下载桌面版 | 可执行文件 exe，无需任何依赖 |
| 学习成本 | 通用 MQTT 工具，需自行理解 DJI 协议 | 专为 DJI Cloud API 设计，开箱即用 |
| 行业定制 | 无                                            | 专门针对大疆上云定制版本，后续持续更新内容 |

> 💡 总结：MQTTX 是优秀的通用 MQTT 调试利器，但 DjiCloudApiTool 在 DJI 行业设备场景下效率更高——**设备层级树展示、自动关联订阅机场无人机、默认集成设备上下行的所有 Topic、Topic 抓包导出、JSON 自动翻译、Topic指令下发及下发记录、机场快捷控制、飞行控制、负载控制、PSDK 喊话器、视频直播**等功能都是针对大疆设备的专属优化。

## 🚶 快速开始

### Windows

从 [Releases](https://github.com/damon-liu/Dji-cloud-api-tool/releases/latest) 下载 `DjiCloudApiTool-v1.0.4.zip`，解压后双击 `DjiCloudApi.exe` 即可运行。

**三步上手：**

1. **配置 MQTT** — 点击左上角「⚙ 配置中心」→ 填写 Broker 地址/端口/用户名/密码 → 保存
2. **添加设备** — 点击左侧「＋」→ 选择 Dock（机场）或 Pilot（无人机）→ 输入设备 SN 和名称
3. **连接监控** — 点击「● 连接」，OSD 面板和 JSON 面板开始实时刷新

详细教程见 [📖 用户使用指南](docs/guide/user-guide.md)。

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

应用启动后自动在可执行文件同目录 `config/` 子目录下生成 `config.json`：

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

<img width="1903" height="1039" alt="img_v3_0213s_b73b7dde-8feb-4a0f-92b9-bebbf5c0f7fg" src="https://github.com/user-attachments/assets/abf55dfa-4c5f-41f4-9719-a57c262ed70f" />

## 🏗 架构

三层设计，单线程运行，通过 Qt 信号/槽实现异步 I/O，VLC 集成实现视频拉流播放：

```
UI 层 (src/ui/)         — MainWindow、DeviceTreeWidget、OsdPanel、RawJsonPanel、ConfigDialog、各种控制面板
核心层 (src/core/)       — DeviceManager（中心调度器）、ConfigStore、TopicManager、DockCommand/DockCommandExecutor
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
| `DockCommandExecutor` | 串行指令执行器：发布至 `services` 主题 → 订阅 `services_reply` → 10s 超时匹配，同时仅允许一个待处理指令 |
| `DeviceInfo` / `OsdData` | 纯头文件数据结构。`AircraftOsd` 和 `DockOsd` 继承 `OsdBase` |
| `MainWindow` | 顶层窗口（1280×760），水平分割器：左侧设备树 + Topic 列表，右侧 OSD + JSON 解析 + 原始 JSON；工具栏集成配置中心、功能中心菜单 |
| `PublishPanel` | Topic 下发面板：预设常用 Topic、模板自动匹配、JSON 编辑、发送历史 |

## 📁 项目结构

```
.
├── src/                          # 源代码
│   ├── main.cpp                  # 入口点
│   ├── core/                     # 核心层 — 业务逻辑与数据结构
│   │   ├── DeviceManager.h/cpp   #   中心调度器：设备增删、消息路由、OSD 缓存
│   │   ├── ConfigStore.h/cpp     #   配置文件读写（多 Profile）、设备列表管理
│   │   ├── TopicManager.h/cpp    #   Topic ↔ 设备 SN 映射、订阅管理
│   │   ├── DockCommand.h/cpp      #   机场控制指令数据结构与构建器
│   │   ├── DockCommandExecutor.h/cpp  # 串行指令执行器
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
│   │   ├── RawJsonPanel.h         #  原始 JSON 显示（暂停/复制/抓包，header-only）
│   │   ├── PublishPanel.h/cpp     #  Topic 下发面板（预设模板、JSON 编辑、发送历史）
│   │   ├── ConfigDialog.h/cpp     #  连接配置对话框（多 Profile + 流媒体配置）
│   │   ├── DockControlPanel.h/cpp  # 机场控制面板
│   │   ├── FlightControlPanel.h/cpp # 飞行控制面板
│   │   ├── MaintenancePanel.h/cpp #  运维模式面板
│   │   ├── PsdkSpeakerPanel.h/cpp #  PSDK 喊话器控制面板
│   │   ├── VideoStreamWindow.h/cpp # 视频直播窗口
│   │   ├── TakeoffConfigDialog.h/cpp # 一键起飞参数配置对话框
│   │   ├── CommandHistoryDialog.h/cpp # 下发记录对话框
│   │   ├── AboutDialog.h/cpp      #  关于本软件对话框
│   │   ├── StreamMediaDialog.h/cpp # 流媒体服务配置对话框
│   │   └── FlowLayout.h           #  Qt 流式布局辅助类
│   └── resources/
│       ├── config.json           # 默认配置文件模板（不含真实凭证）
│       ├── logo.jpg              # 应用图标源文件
│       ├── logo3.ico             # 应用图标
│       └── app_icon.rc           # Windows 资源文件
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
│   ├── guide/
│   │   └── user-guide.md           # 用户使用指南
│   ├── releases/
│   │   ├── RELEASE-v1.0.md         # v1.0 发布说明
│   │   ├── RELEASE-v1.0.1.md       # v1.0.1 发布说明
│   │   ├── RELEASE-v1.0.2.md       # v1.0.2 发布说明
│   │   ├── RELEASE-v1.0.3.md       # v1.0.3 发布说明
│   │   └── RELEASE-v1.0.4.md       # v1.0.4 发布说明
│   └── product/
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
bash package.sh v1.0.4

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
| VLC SDK | 3.x | 视频直播功能依赖，需安装 VLC 并配置 SDK 路径 |
| Python | 3.x | 图标自动生成（PIL/Pillow），可选 |
| 编译器 | MSVC 2022 / MinGW-w64 / GCC / Clang | Zig 交叉编译使用 Clang |


## 📄 License

MIT

关注我，后期版本更新均在公众号上通知！！

<img width="430" height="430" alt="公众号接入" src="https://github.com/user-attachments/assets/d78e1266-a5ae-4a7b-817e-e3dcdb3a243f" />

