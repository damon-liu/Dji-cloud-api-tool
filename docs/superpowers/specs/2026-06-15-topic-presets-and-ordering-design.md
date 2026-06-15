---
comet_change: topic-presets-and-ordering
role: technical-design
canonical_spec: openspec
---

# Topic 预设与排序 — 技术设计

## 概述

为机场设备自动追加 7 个 DJI 标准 topic（默认禁用），为下发面板提供 5 个预设 topic，并为 topic 列表新增排序功能。

## 1. 容器切换：QSet → QStringList

**背景**: `TopicManager::mDeviceTopics` 当前为 `QMap<QString, QSet<QString>>`，不保证顺序。排序功能需要有序容器。

**方案**: 切换为 `QMap<QString, QStringList>`。

**影响范围** (TopicManager.cpp):

| 操作 | Before (QSet) | After (QStringList) |
|------|--------------|---------------------|
| 添加 | `set.insert(t)` | `list.append(t)` + 去重 |
| 删除 | `set.remove(t)` | `list.removeAll(t)` |
| 检查存在 | `set.contains(t)` | `list.contains(t)` |
| 遍历 | range-for | range-for（兼容） |

**信号兼容**: `topicsChanged(QStringList added, QStringList removed)` — 调用方已用 `QStringList`，无需改动。

## 2. 默认 Topic 追加

**位置**: `DeviceManager::addDevice()`

**逻辑**:
```cpp
static const QStringList DEFAULT_DOCK_TOPICS = {
    "thing/product/{sn}/state",
    "thing/product/{sn}/requests",
    "thing/product/{sn}/events",
    "thing/product/{sn}/services_reply",
    "thing/product/{sn}/property/set_reply",
    "sys/product/{sn}/status",
    "thing/product/{sn}/drc/up",
};

void DeviceManager::addDevice(const DeviceInfo& info, const QStringList& topics) {
    // ... 现有逻辑 ...

    if (info.type == DeviceType::Dock) {
        QStringList extendedTopics = topics;
        QSet<QString> newDisabled;
        for (const auto& tpl : DEFAULT_DOCK_TOPICS) {
            QString topic = QString(tpl).replace("{sn}", info.sn);
            if (!extendedTopics.contains(topic)) {
                extendedTopics.append(topic);
                newDisabled.insert(topic);
            }
        }
        mTopicManager->setDeviceTopics(info.sn, extendedTopics);
        mTopicManager->setDisabledTopicsForDevice(info.sn, newDisabled);
    }
    // ... 持久化 ...
}
```

## 3. Topic 排序

**TopicListWidget 新增**:
- `▲` 按钮 (moveUpBtn) — 将选中 topic 向上移动一位
- `▼` 按钮 (moveDownBtn) — 将选中 topic 向下移动一位
- 仅当选中有非分隔线的 topic 时启用
- Signal: `topicOrderChanged(const QString& sn, const QStringList& orderedTopics)`

**TopicManager 新增**:
```cpp
void TopicManager::reorderTopics(const QString& deviceSn, const QStringList& orderedTopics) {
    // 仅当顺序确实改变时更新
    // 不发射 topicsChanged（集合不变）
    mDeviceTopics[deviceSn] = orderedTopics;
}
```

**MainWindow 连接**:
```cpp
connect(mTopicListWidget, &TopicListWidget::topicOrderChanged, this,
    [this](const QString& sn, const QStringList& ordered) {
        mDevMgr->reorderTopics(sn, ordered);  // 持久化
        refreshTopicList(sn);                  // 刷新 UI
    });
```

## 4. PublishPanel 下发预设

**新增成员**:
```cpp
QString mDeviceSn;  // 当前设备 SN，用于替换 {sn}
static const QStringList PUBLISH_PRESETS;  // 5 个下发预设
```

**预设列表**:
```
thing/product/{sn}/property/set
thing/product/{sn}/services
thing/product/{sn}/events_reply
thing/product/{sn}/requests_reply
sys/product/{sn}/status_reply
```

**setTopics() 改为合并显示**:
```cpp
void PublishPanel::setTopics(const QStringList& subscribed) {
    mTopicCombo->clear();
    mTopicCombo->addItems(subscribed);
    mTopicCombo->insertSeparator(mTopicCombo->count());  // 分隔线
    for (const auto& tpl : PUBLISH_PRESETS)
        mTopicCombo->addItem(tpl.replace("{sn}", mDeviceSn));
}
```

## 5. 涉及文件清单

| 文件 | 变更类型 |
|------|---------|
| `src/core/TopicManager.h` | QSet → QStringList；新增 `reorderTopics()` |
| `src/core/TopicManager.cpp` | 实现适配 + reorderTopics |
| `src/core/DeviceManager.h` | 新增 `reorderTopics()` 公开方法 |
| `src/core/DeviceManager.cpp` | addDevice 追加默认 topic；新增 reorderTopics 桥接 |
| `src/ui/TopicListWidget.h` | 新增 moveUp/moveDown 按钮和 signal |
| `src/ui/TopicListWidget.cpp` | 实现排序 UI |
| `src/ui/PublishPanel.h` | 新增 deviceSn + publish presets |
| `src/ui/MainWindow.cpp` | 连接排序信号 + 传递 SN 给 PublishPanel |

## 6. 向后兼容

- config.json 格式不变
- 已有设备不受影响
- TopicManager 的 `topicsChanged` 信号语义不变
