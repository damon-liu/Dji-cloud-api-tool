# DJI Cloud API Tool Next

这是新版跨平台桌面应用的独立项目目录。

目标是用 Tauri + Vue + Rust MQTT 后端重写现有 Qt 版 DJI Cloud API 监控客户端，尽可能完整迁移原软件功能，同时避免 Qt DLL 和 Qt MQTT 运行时分发问题。

原仓库中的 Qt/C++ 文件只作为功能和交互参考。新版源码、设计文档、实现计划、测试和打包配置都应放在本目录下，避免与原项目混在一起。

## 技术栈

- Tauri 2 桌面壳
- Vue 3 + TypeScript + Pinia 前端
- Rust 后端负责 MQTT 连接、订阅、发布和配置文件 I/O
- Vite/Vitest 前端构建与测试

## 开发命令

```bash
pnpm install
pnpm tauri dev
pnpm vitest run
pnpm build
pnpm tauri build
```

`pnpm build` 只构建前端资源。`pnpm tauri dev` 和 `pnpm tauri build` 需要本机已安装 Rust 工具链。

## 配置文件

新版使用自己的配置格式，默认放在 Tauri 应用数据目录下：

```text
dji-cloud-api-tool-next/
  connections.json
  devices.json
  topics.json
  topic-mappings.json
  app-settings.json
```

旧 Qt 版的 `config.json` 和 `topic_mappings.json` 不作为兼容格式读取。默认 Topic 映射已经内置到新版前端，后续可通过新版导入/导出命令迁移同结构配置包。

## 功能状态

- 连接配置、设备树、Topic 列表和持久化
- MQTT 连接、订阅、消息事件和发布命令
- Raw JSON、解析字段、OSD 摘要和 Topic 下发面板
- 默认 Topic 映射、配置导入/导出命令
