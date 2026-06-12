# DJI Cloud API 监控客户端

基于 Qt 6 的桌面应用，连接 MQTT Broker 订阅 DJI 无人机/机场的遥测主题，实时展示 OSD 数据并自动翻译为中文。

## 快速开始（Windows）

直接运行 `deploy/DjiCloudApi.exe` 即可，无需安装任何依赖。

## 功能

- 连接 DJI Cloud API MQTT Broker
- 设备列表 + Topic 订阅管理（新增/禁用/删除）
- OSD 遥测数据实时展示（设备信息 + 机场数据）
- JSON key → 中文 自动翻译（分组表格展示）
- 原始 JSON 展示（支持暂停/复制）
- 可配置的刷新间隔

## 界面截图

> TODO: 添加截图

<!-- ![主界面](docs/screenshots/main.png) -->
<!-- ![JSON解析](docs/screenshots/json-parse.png) -->

## 开发

### 构建（Windows MinGW + Qt 6）

```bash
cmake -B build_mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_mingw
```

### 交叉编译到 Linux（通过 Zig）

```bash
export ZIG_PATH=/path/to/zig/zig.exe
cmake -B build_linux -G "MinGW Makefiles" \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x64.cmake \
      -DZIG_PATH="$ZIG_PATH" \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build_linux
```

### 技术栈

- Qt 6 (Core, Widgets, Mqtt)
- C++17
- CMake ≥ 3.10

## License

MIT
