# DJI Cloud API Tool Next

这是新版跨平台桌面应用的独立项目目录。

目标是用 Tauri + Vue + Rust MQTT 后端重写现有 Qt 版 DJI Cloud API 监控客户端，尽可能完整迁移原软件功能，同时避免 Qt DLL 和 Qt MQTT 运行时分发问题。

原仓库中的 Qt/C++ 文件只作为功能和交互参考。新版源码、设计文档、实现计划、测试和打包配置都应放在本目录下，避免与原项目混在一起。
