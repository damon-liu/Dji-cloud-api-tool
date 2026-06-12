# CLAUDE.md

本文件为 Claude Code（claude.ai/code）在此仓库中工作时提供指导。

## 项目概述

DJI Cloud API MQTT监控客户端 — 基于 Qt 6 C++ 的桌面应用，连接 MQTT Broker，订阅 DJI 无人机/机场的遥测主题，以树形结构实时展示 OSD（机载系统数据）。v1.0 仅支持监控；指令下发功能计划在 v1.1 实现。

## 构建命令

### Windows（MinGW + Qt 6）

```bash
# 配置
cmake -B build_mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# 构建
cmake --build build_mingw

# 部署 Qt DLL（可选）
cmake --build build_mingw --target deploy
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

### 原生 Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Docker

```bash
cp build_linux/main deploy/main
docker build -t dji-cloud-api:latest -f deploy/Dockerfile .
docker run --rm dji-cloud-api:latest
```

## 架构

三层设计，单线程运行，通过 Qt 信号/槽实现异步 I/O：

```
UI 层 (src/ui/)          — MainWindow、DeviceTreeWidget、OsdPanel、RawJsonPanel、ConfigDialog、TopicEditDialog
核心层 (src/core/)        — DeviceManager（中心调度器）、ConfigStore、TopicManager、DeviceInfo/OsdData 数据结构
通信层 (src/mqtt/)        — MqttClientManager（QMqttClient 封装，指数退避自动重连）
```

**数据流：** MQTT 消息 → `MqttClientManager::messageReceived` → `DeviceManager::parseAndRoute()` 通过 `TopicManager` 将主题匹配到设备，解析 DJI 格式 JSON（`{"tid":..., "data":...}`），缓存 OSD，发出 `deviceOsdUpdated(sn, rawJson)` → UI 面板在用户选择设备时响应式更新。

**线程模型：** 一切运行在 Qt 主线程。`QMqttClient` 是异步的——无需工作线程。

### 关键类

| 类                           | 职责                                                                                                            |
| ---------------------------- | --------------------------------------------------------------------------------------------------------------- |
| `DeviceManager`            | 中心调度器——持有 ConfigStore、TopicManager、MqttClientManager；设备增删改查；消息路由；OSD 缓存               |
| `ConfigStore`              | JSON 配置持久化（`config.json`）。处理设备列表中机场→子飞机的拆分/合并逻辑                                   |
| `TopicManager`             | 主题到设备 SN 的映射及反向索引；发出 `topicsChanged` 信号触发 MQTT 重新订阅                                   |
| `MqttClientManager`        | `QMqttClient` 封装；指数退避自动重连（基数 1s，上限 30s）；去重订阅管理                                       |
| `DeviceInfo` / `OsdData` | 纯头文件数据结构。`AircraftOsd` 和 `DockOsd` 继承 `OsdBase`；各自包含 `parse()` 方法从 QJsonObject 解析 |
| `MainWindow`               | 顶层窗口（1280×760），水平分割器：左侧设备树，右侧选项卡详情。内联 Qt 样式表实现 Material 风格外观             |
| `ConfigDialog`             | MQTT 连接配置对话框，含测试按钮，创建临时 QMqttClient 验证连通性（5 秒超时）                                    |
| `PublishPanel`             | v1.1 占位界面——发送按钮当前禁用                                                                               |

### 配置文件格式（`config.json`）

位于可执行文件同目录（或工作目录）。首次运行自动创建默认配置：

```json
{
    "mqtt": { "host": "...", "port": 8883, "username": "admin", "password": "" },
    "devices": [
        { "type": "dock", "sn": "dock_001", "aircraft_sn": "drone_001",
          "topics": ["thing/product/dock_001/osd", "thing/product/drone_001/osd"] }
    ]
}
```

主题字符串支持 `{sn}` 占位符，运行时会替换为设备 SN。

## 关键约定

- **C++17** 必须；CMake ≥ 3.10
- Qt 6 模块：Core、Widgets、Mqtt（不依赖外部 MQTT 库）
- 源码编码：UTF-8（MSVC 通过 `/utf-8`、GCC/Clang 通过 `-fexec-charset=UTF-8` 强制）
- CMake `AUTOMOC`、`AUTORCC`、`AUTOUIC` 已开启——无需手动调用 MOC/UIC
- 有业务逻辑的 UI 面板使用 `.cpp` 文件；简单的只读面板（`RawJsonPanel`、`PublishPanel`）采用纯头文件内联实现
- `PublishPanel` 的发送功能明确推迟到 v1.1——不要在未更新版本计划的情况下去实现它
- 交叉编译工具链（`cmake/toolchains/linux-x64.cmake`）使用 Zig 作为 C/C++ 编译器，目标平台 `x86_64-linux-gnu.2.39`
- Git 提交信息使用中文
- 每次推送前需编译项目，并将 `build_mingw/DjiCloudApi.exe` 复制到 `deploy/DjiCloudApi.exe` 一并推送
