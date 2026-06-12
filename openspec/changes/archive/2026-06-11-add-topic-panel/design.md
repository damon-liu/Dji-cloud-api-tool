## Context

当前 DJI Cloud API 监控客户端采用三层架构（UI → Core → MQTT），topic 管理分散在 `TopicEditDialog` 弹窗和折叠的 `PublishPanel` 中。用户需要点击设备后打开对话框才能查看和编辑 topic，缺乏直观的 topic 可见性。

MQTT 断开按钮因 `MqttClientManager::onDisconnected()` 中无条件触发 `startReconnect()` 而失效——手动断开后立即进入自动重连循环。

本次变更在左侧面板增加独立的 topic 列表区域，将 topic 操作从弹窗迁移至内联面板，并新增单个 topic 的启用/禁用控制。

## Goals / Non-Goals

**Goals:**
- 设备树下方新增 Topic 列表面板，展示当前选中设备的所有订阅 topic
- Topic 级别启用/禁用切换，状态持久化，禁用后不参与 MQTT 订阅
- 移除旧的 ✎ 编辑 Topic 按钮，topic 增删改操作迁移到新区域
- Dock 设备可添加手飞无人机作为子设备；手飞无人机不可添加子设备
- 修复 MQTT 断开连接后自动重连的 Bug
- 连接/断开按钮移至工具栏右侧

**Non-Goals:**
- 不实现 topic 下发功能（v1.1 PublishPanel 占位保留）
- 不修改 OSD 解析、设备树渲染等已有功能
- 不改变 MQTT 连接/重连的核心逻辑（仅修复手动断开冲突）
- 不引入新的外部依赖

## Decisions

### D1: Topic 禁用状态存储方式

**选择：** 在 `ConfigStore` / `TopicManager` 中新增独立的 `mDisabledTopics` 集合（`QMap<QString, QSet<QString>>`，SN → 禁用的 topic 集合），与现有 `mDeviceTopics` 并行维护。

**替代方案：**
- A. 将 topic 存储从字符串改为结构体 `{topic, enabled}` — 改动面大，需重构所有 topic 遍历代码
- B. 使用两个独立数组分别存储启用和禁用 topic — JSON 冗余但逻辑清晰

选择方案 A+B 的折中：内存中使用两个并行集合（改动最小），JSON 中使用 `disabled_topics` 数组（向后兼容）。旧配置文件缺失 `disabled_topics` 字段时，所有 topic 默认启用。

**JSON 格式变化：**
```json
// 旧格式（兼容读取）
{ "topics": ["thing/product/dock_001/osd"] }

// 新格式
{
  "topics": ["thing/product/dock_001/osd", "thing/product/drone_001/osd"],
  "disabled_topics": ["thing/product/drone_001/osd"]
}
```

### D2: TopicListWidget 组件设计

**选择：** 新建 `TopicListWidget`（纯头文件 + `.cpp`），继承 `QWidget`，内部包含：
- `QLabel` 标题 "Topic 列表"
- `QListWidget` 展示 topic，每项前缀 `●`（启用）/ `○`（禁用，灰色）
- 右侧按钮列：`＋` 添加、`◎` 切换启用/禁用、`✕` 删除

信号接口：
- `topicAdded(QString deviceSn, QString topic)`
- `topicToggled(QString deviceSn, QString topic)`
- `topicRemoved(QString deviceSn, QString topic)`

MainWindow 连接这些信号到 DeviceManager 的对应方法。

### D3: 添加设备逻辑变更

**选择：** 修改 `MainWindow::onAddDevice()` —— 根据当前选中设备决定行为：

| 当前选中 | 行为 |
|----------|------|
| 无 | 添加顶级设备（Dock 或 Aircraft） |
| Dock | 添加手飞无人机作为该 Dock 的子设备（`parentSn` = Dock SN） |
| Aircraft | 按钮禁用，不允许添加子设备 |

当为 Dock 添加子设备时，跳过设备类型选择（固定为 Aircraft），直接询问名称和 SN。

### D4: 断开连接 Bug 修复

**选择：** `MqttClientManager` 增加 `mIntentionalDisconnect` 标志位。
- `disconnectFromBroker()` 中设置 `mIntentionalDisconnect = true`
- `onDisconnected()` 中检查标志位：若为 true 则清除并跳过 `startReconnect()`
- `connectToBroker()` 中重置 `mIntentionalDisconnect = false`（确保后续意外断线仍可重连）

### D5: 连接/断开按钮位置

**选择：** 在 `setupToolBar()` 中将连接/断开按钮移到 spacer 之后（右侧），配置按钮保持在左侧。布局变为：

```
[⚙ 配置] ———spacer——— [Broker 信息] [● 连接] [◎ 断开]
```

## Risks / Trade-offs

- **向后兼容风险**：旧 `config.json` 无 `disabled_topics` 字段 → **缓解**：读取时默认所有 topic 启用，保存时写入新格式
- **Topic 丢失风险**：禁用后再删除设备，配置文件残留 → **缓解**：设备删除时一并清理 `disabled_topics`
- **UI 空间**：左侧面板增加 topic 列表，最小窗口 960px 高度可能不足 → **缓解**：topic 列表设置最大高度 200px，使用滚动条
