# 大疆机场可集成功能调研报告

调研日期：2026-07-18
数据来源：大疆官方 [dji-sdk/Cloud-API-Doc](https://github.com/dji-sdk/Cloud-API-Doc) 仓库（dock2 全量 MQTT 接口定义）
用途：为「功能中心」菜单的后续功能集成提供依据

## 背景

v1.0.2 已实现「机场控制」窗口，包含 8 个远程调试指令（调试模式开/关、飞行器开/关机、舱盖开/关、充电开/关），走 `thing/product/{gateway_sn}/services` 下发 + `services_reply` 回复，由 `DockCommandExecutor` 统一执行（tid 匹配 + 10s 超时）。

大疆机场功能集全景（官方文档目录）：设备接入、设备管理、直播、媒体管理、航线管理、HMS、**远程调试**、固件升级、远程日志、DRC 实时控制、自定义飞行区、多机场任务。

## 可集成功能分类（按集成成本从低到高）

### ⭐⭐⭐ 第一梯队：零新协议，纯加按钮（完全复用 DockCommandExecutor）

data 为 null 或极简，与现有 8 指令完全同构：

| 功能 | method | 类型 | 备注 |
|------|--------|------|------|
| 补光灯开/关 | `supplement_light_open` / `supplement_light_close` | cmd | |
| 一键返航 / 取消返航 | `return_home` / `return_home_cancel` | cmd | 调试场景最高频，属航线管理章节但同走 services |
| 机场重启 | `device_reboot` | job | 需强确认 |
| 强制关舱盖 | `cover_force_close` | job | 应急操作 |
| 飞行器/机场数据格式化 | `drone_format` / `device_format` | job | ⚠️ 危险操作，建议双重确认或不做 |

> job 类指令的完整进度经 `thing/product/{gateway_sn}/events` 上报（该 topic 已在默认订阅列表）。当前执行器只解析 `services_reply` 的即时回复；若需展示 job 全程进度（已下发→执行中→成功/失败），需在执行器中增加 events 进度解析。

### ⭐⭐⭐ 第二梯队：同协议但带参数（执行器需支持 data 参数）

| 功能 | method | 参数 |
|------|--------|------|
| 机场声光报警开关 | `alarm_state_switch` | action 0/1 |
| 机场空调工作模式切换 | `air_conditioner_mode_switch` | 制冷/除湿/关闭等模式枚举 |
| 电池保养状态切换 | `battery_maintenance_switch` | action 0/1 |
| 电池运行模式切换 | `battery_store_mode_switch` | 计划模式/待命模式 |
| 增强图传开关 | `sdr_workmode_switch` | link_workmode |
| eSIM 激活 / SIM 切换 / 运营商切换 | `esim_activate` / `sim_slot_switch` / `esim_operator_switch` | Dock 2 及以上 |

### ⭐⭐ 第三梯队：新协议类别，中等工作量

1. **机场参数设置面板**（`thing/product/{gateway_sn}/property/set`，`property/set_reply` 已默认订阅）
   可设置属性：限高 `height_limit`、限远 `distance_limit_status`、避障 `obstacle_avoidance`、返航高度 `rth_altitude`、夜航灯 `night_lights_state`、失控行为 `out_of_control_action` 等
2. **HMS 健康告警查看器**：解析 `hms` 上报 topic，纯展示；可与规划中的"关键事件桌面通知"联动
3. **日志列表查询**：`fileupload_list` 单指令即可获取设备可上传日志清单（完整日志拉取需 OSS/STS 对接，可后置）
4. **直播控制**：`live_start_push` / `live_stop_push` / `live_set_quality` / `live_camera_change` / `live_lens_change`——工具可下发指令验证链路；观看画面需另建 RTMP/WebRTC 服务端

### ⚠️ 暂缓（依赖重 / 风险高）

| 功能 | 相关 method | 暂缓原因 |
|------|-------------|----------|
| DRC 实时控制 + 一键起飞 | `takeoff_to_point`、`fly_to_point`、`drone_control`、`drone_emergency_stop`、负载 `camera_*` | 需进入 DRC 模式、维持心跳（`heart_beat`）、抢飞行控制权；真机风险高 |
| 航线任务 | `flighttask_prepare/execute/undo/pause/recovery/stop` | 需 KMZ 航线文件 + 对象存储；`pause/recovery/stop` 可视需要单独先做 |
| 固件升级 | `ota_create` | 需固件包 URL/MD5 分发 |
| 媒体管理 | `fileupload_*`（媒体） | 需 STS/OSS 凭证 |

## 集成建议（功能中心菜单演进路线）

1. **v1.0.2 内补齐**：机场控制窗口增加第一梯队按钮——补光灯、一键返航/取消、机场重启、强制关舱（格式化谨慎，建议不做）
2. **v1.0.3**：第二梯队带参开关指令 + 新功能项「机场参数设置」（property/set）
3. **配合桌面通知规划**：HMS 告警查看器
4. **不建议**在调试工具中实现 DRC/一键起飞、完整航线管理、固件升级

## 参考链接

- [远程调试指令参考（dock2 cmd.md）](https://github.com/dji-sdk/Cloud-API-Doc/blob/master/docs/cn/60.api-reference/20.dock-to-cloud/00.mqtt/20.dock/10.dock2/70.cmd.md)
- [航线管理（50.wayline.md）](https://github.com/dji-sdk/Cloud-API-Doc/blob/master/docs/cn/60.api-reference/20.dock-to-cloud/00.mqtt/20.dock/10.dock2/50.wayline.md)
- [DRC 实时控制（110.drc.md）](https://github.com/dji-sdk/Cloud-API-Doc/blob/master/docs/cn/60.api-reference/20.dock-to-cloud/00.mqtt/20.dock/10.dock2/110.drc.md)
- [直播（30.live.md）](https://github.com/dji-sdk/Cloud-API-Doc/blob/master/docs/cn/60.api-reference/20.dock-to-cloud/00.mqtt/20.dock/10.dock2/30.live.md)
- [远程日志（90.log.md）](https://github.com/dji-sdk/Cloud-API-Doc/blob/master/docs/cn/60.api-reference/20.dock-to-cloud/00.mqtt/20.dock/10.dock2/90.log.md)
- [远程调试功能集概述（developer.dji.com）](https://developer.dji.com/doc/cloud-api-tutorial/cn/feature-set/dock-feature-set/remote-debug.html)
