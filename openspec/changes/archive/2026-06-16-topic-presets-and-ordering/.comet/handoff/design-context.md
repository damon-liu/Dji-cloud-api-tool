# Comet Design Handoff

- Change: topic-presets-and-ordering
- Phase: design
- Mode: compact
- Context hash: f059b3df2fee6ff0c1092070b8ede868781749a94935807d5f93fcca3e9cc33c

Generated-by: comet-handoff.sh

OpenSpec remains the canonical capability spec. This handoff is a deterministic, source-traceable context pack, not an agent-authored summary.

## openspec/changes/topic-presets-and-ordering/proposal.md

- Source: openspec/changes/topic-presets-and-ordering/proposal.md
- Lines: 1-33
- SHA256: 132db552e8d7619545fc85097b4ec3c401e6cfa7a32c063e1e4d7c3d0204e4f3

```md
# Proposal: 完善订阅和发送 Topic

## 问题背景

当前系统在创建机场设备时仅默认添加 `thing/product/{sn}/osd` 一个 topic。DJI Cloud API 协议包含多种标准 topic（状态、事件、请求/响应等），用户需要手动逐个添加，操作繁琐且容易遗漏。同时下发面板仅展示已订阅的 topic，缺少下发专用的 topic 预设。

## 目标

1. **创建机场设备时自动追加 7 个默认 topic**（默认禁用状态），覆盖 DJI 协议常用的订阅 topic
2. **下发面板额外提供 5 个下发专用 topic**，独立于订阅列表，便于用户选择后发送指令
3. **Topic 列表支持上移/下移排序**，用户可手动调整 topic 顺序并持久化

## 范围

### 涉及

- `DeviceManager::addDevice()` — 创建机场时追加默认 topic
- `TopicListWidget` — 新增上移/下移按钮
- `PublishPanel` — 新增独立的下发 topic 预设列表
- `TopicManager` / `ConfigStore` — topic 顺序持久化

### 不涉及

- 无人机设备（仅机场受影响）
- 已有设备的 topic 列表（不自动补全）
- MQTT 连接/断开逻辑
- TopicEditDialog

## 非目标

- 不实现拖拽排序
- 下发 panel 的 5 个 topic 不出现在订阅列表中
- 不修改无人机创建设备流程
```

## openspec/changes/topic-presets-and-ordering/design.md

- Source: openspec/changes/topic-presets-and-ordering/design.md
- Lines: 1-86
- SHA256: 1987b4f1a3558431e3811a645ab74604f1809b6b821b8379bef8115bfa4ac0ed

[TRUNCATED]

```md
# Design: 完善订阅和发送 Topic

## 架构决策

### 1. 默认 Topic 预设

**问题**: 创建机场设备时需要自动追加 7 个 DJI 标准 topic。

**方案**: 在 `DeviceManager::addDevice()` 中检测设备类型，当 `DeviceType::Dock` 时，在用户提供的 topic 列表后追加 7 个默认 topic（默认禁用状态）。

默认 topic 列表（硬编码常量）:

```
thing/product/{sn}/state
thing/product/{sn}/requests
thing/product/{sn}/events
thing/product/{sn}/services_reply
thing/product/{sn}/property/set_reply
sys/product/{sn}/status
thing/product/{sn}/drc/up
```

**数据流**:
```
MainWindow::onAddDevice()
  → DeviceManager::addDevice(info, topics)
    → 检测 info.type == Dock
    → 追加 7 个默认 topic
    → TopicManager::setDeviceTopics(sn, allTopics)
    → TopicManager::setDisabledTopics()  // 新 7 个设为禁用
    → ConfigStore 持久化
```

### 2. 下发面板独立 Topic 列表

**问题**: PublishPanel 当前仅显示订阅 topic，需要额外 5 个下发专用 topic。

**方案**: PublishPanel 新增 `mPublishTopics` 成员，存储下发专用 topic 预设。`setTopics()` 合并订阅 topic + 下发预设 topic 到 ComboBox。

下发预设（硬编码常量，替换 `{sn}` 为当前设备 SN）:

```
thing/product/{sn}/property/set
thing/product/{sn}/services
thing/product/{sn}/events_reply
thing/product/{sn}/requests_reply
sys/product/{sn}/status_reply
```

**注意**: 这 5 个 topic 仅出现在 ComboBox 下拉中，不通过 TopicManager 订阅。

### 3. Topic 排序

**问题**: TopicListWidget 需要上移/下移按钮调整 topic 顺序。

**方案**: 
- TopicListWidget 新增 ▲（上移）和 ▼（下移）按钮
- 移动操作直接修改 `mAllTopics` 列表顺序
- 通过 signal `topicOrderChanged(deviceSn, orderedTopics)` 通知外部
- MainWindow 连接信号 → DeviceManager 更新 TopicManager → 重新订阅（先移除再添加）
- 持久化到 ConfigStore / config.json

**按钮位置**: 放在现有 ＋、◎、✕ 按钮旁边

## 涉及文件

| 文件 | 变更 |
|------|------|
| `src/core/DeviceManager.cpp` | `addDevice()` 中追加 7 个默认 topic + 设置禁用 |
| `src/core/TopicManager.h/cpp` | 新增 `reorderTopics()` 方法 |
| `src/core/ConfigStore.h/cpp` | topic 顺序通过 topicsForDevice/setTopicsForDevice 已支持（QStringList 保序） |
| `src/ui/TopicListWidget.h/cpp` | 新增上移/下移按钮和信号 |
| `src/ui/PublishPanel.h` | 新增下发预设 topic 列表 |
| `src/ui/MainWindow.cpp` | 连接新信号，设备选中时传给 PublishPanel |

## 数据持久化

Topic 顺序通过 `ConfigStore::setTopicsForDevice()` 的 QStringList 自然保序，无需新增配置字段。

禁用状态已有 `mDisabledTopics` 支持（QSet），config.json 中存储为 `disabled_topics` 数组。
```

Full source: openspec/changes/topic-presets-and-ordering/design.md

## openspec/changes/topic-presets-and-ordering/tasks.md

- Source: openspec/changes/topic-presets-and-ordering/tasks.md
- Lines: 1-33
- SHA256: 78603f963cee375bc65a95e86486246296769a43a485d81db1f428e206b23f45

```md
# Tasks: 完善订阅和发送 Topic

## 实现任务

- [ ] 1. **DeviceManager 默认 topic 追加**
  在 `addDevice()` 中，检测设备类型为 `Dock` 时，追加 7 个预设 topic 并默认禁用。
  - 文件: `src/core/DeviceManager.cpp`
  - 新增常量 `DEFAULT_DOCK_TOPICS`（7 个 topic 模式）
  - 调用 `mTopicManager->setDisabledTopicsForDevice()` 将新 topic 设为禁用

- [ ] 2. **TopicListWidget 上移/下移按钮**
  新增 ▲ 和 ▼ 按钮，实现 topic 顺序调整功能。
  - 文件: `src/ui/TopicListWidget.h`, `src/ui/TopicListWidget.cpp`
  - 新增 signal: `topicOrderChanged(const QString& sn, const QStringList& orderedTopics)`
  - 按钮仅在选中有效 topic 时启用
  - 保持启用/禁用状态标记

- [ ] 3. **MainWindow 连接排序信号**
  连接 `topicOrderChanged` 信号到 DeviceManager，更新 topic 顺序并持久化。
  - 文件: `src/ui/MainWindow.cpp`

- [ ] 4. **PublishPanel 下发预设 topic**
  新增 5 个下发专用 topic 预设，合并到 ComboBox 中（不去订阅）。
  - 文件: `src/ui/PublishPanel.h`
  - 新增 `setDeviceSn(sn)` 方法用于替换 {sn}
  - ComboBox 显示: 订阅 topic + 分隔线 + 下发预设 topic

- [ ] 5. **编译验证与端到端测试**
  - 编译项目
  - 创建新机场设备，验证 7 个默认 topic 已添加且处于禁用状态
  - 验证上移/下移功能正常
  - 验证下发面板额外显示 5 个预设 topic
  - 验证无人机不受影响
```

