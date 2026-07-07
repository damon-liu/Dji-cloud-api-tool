# Tauri + Vue + Rust 跨平台重写 — 技术设计

## 目标

将现有 Qt 6 C++ 桌面应用重写为跨平台桌面应用，尽可能完整迁移旧版功能与使用流程，同时解决 Windows 分发时 Qt DLL、Qt MQTT 模块和运行时依赖难以打包的问题。

新版不兼容旧版 `config.json`、`topic_mappings.json` 的文件格式。迁移目标是功能和交互，不是配置文件格式。新版使用自己的配置模型，并提供默认配置模板与新版配置导入/导出能力。

## 技术选型

- 桌面壳：Tauri 2
- 前端：Vue 3 + TypeScript
- 状态管理：Pinia
- 构建工具：Vite
- 后端：Rust，运行在同一个 Tauri 应用进程内
- MQTT：Rust 后端负责 TCP/TLS MQTT 连接、订阅、发布、自动重连和消息分发
- 前后端通信：Tauri commands + events
- 打包目标：Windows、macOS、Linux

Rust 后端不作为独立服务部署。用户最终拿到的是单个桌面应用安装包或便携包。

## 旧版功能映射

| 旧版 Qt 功能 | 新版模块 |
| --- | --- |
| 顶部配置按钮、连接、断开、Broker 状态 | AppShell + ConnectionStore + Rust MQTT service |
| MQTT 多 Connections/Profile | Connections 模块 |
| 连接测试，5 秒超时，成功/失败提示 | Rust `test_connection` command |
| 设备树：机场、手飞无人机、机场下挂飞机 | Devices 模块 |
| 设备新增、删除、选择后刷新右侧面板 | Devices 模块 + UI selection state |
| 每个设备的 Topic 列表 | Topics 模块 |
| Topic 新增、删除、启用/禁用、全部启用/禁用、排序 | Topics 模块 |
| 双击 Topic 复制 | Topics UI |
| 选中 Topic 后过滤 Raw JSON 和解析面板 | MonitorStore selection filters |
| OSD 设备信息面板 | OSD 模块 |
| JSON key 中文映射、分组展示、值翻译、单位 | Topic Mapping + Parsed JSON 模块 |
| 原始 JSON 历史、追加、暂停、复制、清空 | Raw JSON 模块 |
| 折叠 Topic 下发面板 | Publish 模块 |
| 状态栏连接状态、设备数量、错误提示 | AppShell + Notifications |

## 应用布局

新版沿用旧版主工作流，而不是改成全新的产品结构。

主窗口包含：

- 顶部工具栏：配置、当前连接、连接、断开、连接状态
- 左侧栏上半区：设备列表
- 左侧栏下半区：当前设备 Topic 列表
- 右侧主区左半：OSD 摘要 + Topic 解析字段
- 右侧主区右半：原始 JSON
- 底部折叠区：Topic 下发
- 底部状态栏：连接状态、错误提示、设备数量、版本信息

设备选择、Topic 选择、连接状态变化时，各面板按照旧版逻辑联动刷新。

## 数据模型

### Connection

```ts
type ConnectionProfile = {
  id: string
  name: string
  host: string
  port: number
  clientId?: string
  username?: string
  password?: string
  tls: {
    enabled: boolean
    caCertPath?: string
    clientCertPath?: string
    clientKeyPath?: string
    insecureSkipVerify?: boolean
  }
}
```

第一版默认支持账号密码和 TLS 开关。证书路径保留在模型里，UI 可做成高级选项。

### Device

```ts
type Device = {
  sn: string
  name: string
  type: 'dock' | 'aircraft'
  parentSn?: string
  online: boolean
  lastSeenAt?: string
}
```

机场和独立手飞飞机为顶级设备。机场下可添加子飞机。选中飞机时禁用继续添加子设备的入口，保持旧版行为。

### DeviceTopic

```ts
type DeviceTopic = {
  id: string
  deviceSn: string
  topic: string
  enabled: boolean
  order: number
}
```

Topic 顺序持久化。禁用 Topic 不参与订阅；禁用 Topic 被选中时不显示 Raw JSON 和解析结果。

### TopicMapping

```ts
type TopicMapping = {
  topics: Record<string, {
    description: string
    fields: Record<string, {
      zh: string
      unit?: string
      values?: Record<string, string>
    }>
    groups: Array<{
      id: string
      label: string
      keys: string[]
    }>
  }>
}
```

保留旧版的映射能力：支持 `thing/product/{sn}/osd` 模板、点路径字段、中文名、单位、枚举值翻译、分组显示。新版内置默认映射；用户可在应用数据目录中覆盖。

### Runtime Message

```ts
type MqttRuntimeMessage = {
  connectionId: string
  topic: string
  payloadText: string
  receivedAt: string
  deviceSn?: string
}
```

Rust 收到 MQTT 消息后做基础 topic 匹配，带上 `deviceSn` 推给前端。前端保存每个设备、每个 topic 的历史，默认上限沿用旧版 500 条。

## 配置存储

新版配置放在 Tauri 应用数据目录：

```text
app-data/
  connections.json
  devices.json
  topics.json
  topic-mappings.json
  app-settings.json
  logs/
```

配置不兼容旧版。第一版需要支持：

- 首次启动生成默认 Connection、示例设备、默认 Topic 映射
- 保存当前 Connection、设备、Topic、禁用状态、排序
- 导出新版配置包
- 导入新版配置包
- 连接密码本地保存；如平台能力允许，后续可迁移到系统钥匙串

## Rust 后端

Rust 后端负责所有不能可靠放在浏览器 WebView 里的能力。

Commands:

- `list_connections`
- `save_connection`
- `delete_connection`
- `test_connection`
- `connect`
- `disconnect`
- `subscribe_topics`
- `publish_message`
- `load_config`
- `save_config`
- `export_config`
- `import_config`

Events:

- `mqtt:connected`
- `mqtt:disconnected`
- `mqtt:error`
- `mqtt:message`
- `mqtt:publish-result`
- `config:changed`

MQTT 服务要求：

- 支持标准 TCP MQTT 与 TLS MQTT
- 支持账号密码
- 支持连接测试 5 秒超时
- 支持连接状态事件
- 支持自动重连，指数退避，基数 1 秒，上限 30 秒
- Topic 变化时增量订阅/取消订阅
- 发布消息时返回成功或错误

## 前端模块

### AppShell

负责全局布局、工具栏、状态栏和通知。连接按钮在已连接时禁用，断开按钮在未连接时禁用。Broker 标签显示当前 host 和 port。

### Connections

提供配置窗口或侧边抽屉，支持新增、重命名、删除、切换 Connection，编辑 Broker、端口、用户名、密码、TLS 选项，并进行连接测试。

### Devices

提供设备树，显示 Dock 与 Aircraft。支持新增顶级 Dock、顶级 Aircraft，以及在 Dock 下新增 Aircraft。删除设备时确认，并删除该设备及其 Topic。

### Topics

显示当前设备的有序 Topic 列表。每行显示启用/禁用状态。支持新增、删除、启用/禁用、全部启用/禁用、上移、下移、双击复制。默认新增 Topic 为 `thing/product/{sn}/osd`。

### OSD

显示当前设备的关键遥测信息。机场和飞机展示不同字段。没有数据时显示空态；断开连接时暂停刷新但保留已有数据。

### Parsed JSON

根据当前设备和当前 Topic 查找映射模板，解析 payload 中的 `data` 对象。支持嵌套点路径字段，展示中文名、原 key、值、单位、更新时间。未知字段可放入“其他字段”区域。

### Raw JSON

按当前设备和当前 Topic 展示消息历史。支持暂停/恢复、复制当前消息、复制全部历史、清空。消息应格式化显示，保留原始字符串用于复制。

### Publish

底部折叠面板。支持选择当前设备 Topic、使用预设 Topic、编辑 JSON payload、发布。发布前校验 JSON；发布后显示成功或失败。

## Topic 与设备匹配

第一版沿用旧版思路：

- 设备 SN 出现在 topic 中时，认为该 topic 属于该设备
- `thing/product/{sn}/osd` 作为默认 OSD topic
- DJI 标准 Topic 预设可在设备创建时追加，默认可禁用
- 选中已启用 Topic 时，Raw JSON 与 Parsed JSON 才显示该 Topic 数据

后续如需要更精确匹配，可增加可配置 topic 模板规则。

## 错误处理

- MQTT 连接失败：状态栏和通知显示错误，连接按钮恢复可用
- 测试连接失败：显示失败提示，保留编辑中的配置
- Topic 重复：阻止新增并提示
- 设备 SN 为空或重复：阻止保存并提示
- JSON payload 非法：阻止发布并定位错误
- Topic 映射文件损坏：使用内置默认映射并提示
- 配置保存失败：通知用户并保留内存状态

## 测试策略

Rust:

- 配置读写单元测试
- topic 匹配测试
- MQTT 服务状态机测试
- 发布 payload 校验测试

TypeScript:

- Pinia store 单元测试
- OSD `data` 解析测试
- Topic 映射点路径解析测试
- Topic 排序、启用禁用、历史上限测试

端到端:

- 首次启动生成默认配置
- 新增设备、Topic、连接配置并持久化
- 模拟 MQTT 消息后 UI 更新 OSD、Parsed JSON、Raw JSON
- 发布面板校验与调用后端 command
- Windows/macOS/Linux 打包产物能启动

## 非目标

- 不兼容旧版配置文件格式
- 不做服务端 Web 多用户系统
- 不做 Docker 部署作为主要使用方式
- 不做移动端
- 不在第一版实现复杂告警、云同步、权限系统

## 迁移边界

本次重写应把旧版 Qt 代码作为行为参考，不逐行翻译 C++。优先迁移用户可见功能、数据流和交互逻辑。实现时可以复用旧版的默认 topic 映射内容和字段分组思想，但配置格式和内部模块边界重新设计。
