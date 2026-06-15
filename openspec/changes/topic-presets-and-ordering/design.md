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

## 向后兼容

- 已有设备不受影响
- config.json 格式不变
- 仅新增 topic 条目，不修改已有条目
