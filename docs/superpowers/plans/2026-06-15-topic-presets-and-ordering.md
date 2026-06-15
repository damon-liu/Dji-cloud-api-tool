---
change: topic-presets-and-ordering
design-doc: docs/superpowers/specs/2026-06-15-topic-presets-and-ordering-design.md
base-ref: 2dd40e39b0593599ec1becf5afabf5f9acb503b2
---

# Topic 预设与排序 — 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为机场设备自动追加 7 个 DJI 标准 topic（默认禁用），为下发面板提供 5 个预设 topic，并为 topic 列表新增排序功能。

**Architecture:** 核心重构 — TopicManager 和 ConfigStore 的内部容器从 `QSet` 切换为 `QStringList` 以保持有序；DeviceManager 在创建机场设备时自动追加默认 topic；TopicListWidget 新增排序按钮；PublishPanel 的 ComboBox 合并显示已订阅和预设 topic。

**Tech Stack:** C++17, Qt 6 (Core, Widgets, Mqtt), CMake + MinGW

---

## 文件结构

| 文件 | 变更 | 职责 |
|------|------|------|
| `src/core/TopicManager.h` | 修改 | QSet → QStringList；新增 `reorderTopics()` 声明 |
| `src/core/TopicManager.cpp` | 修改 | 所有方法适配 QStringList；实现 `reorderTopics()` |
| `src/core/ConfigStore.h` | 修改 | `mDeviceTopics` QSet → QStringList |
| `src/core/ConfigStore.cpp` | 修改 | 移除 QSet 转换，保持顺序 |
| `src/core/DeviceManager.h` | 修改 | 新增 `reorderTopics()` 公开方法 |
| `src/core/DeviceManager.cpp` | 修改 | `addDevice()` 追加默认 topic；新增 `reorderTopics()` 桥接 |
| `src/ui/TopicListWidget.h` | 修改 | 新增排序按钮成员 + `topicOrderChanged` signal |
| `src/ui/TopicListWidget.cpp` | 修改 | 排序按钮 UI + 上移/下移逻辑 |
| `src/ui/PublishPanel.h` | 修改 | 新增 `mDeviceSn` + `PUBLISH_PRESETS`；`setTopics()` 改为合并显示 |
| `src/ui/MainWindow.cpp` | 修改 | 连接 `topicOrderChanged` 信号；传递 SN 给 PublishPanel |

---

### Task 1: TopicManager — QSet 迁移为 QStringList

**Files:**
- Modify: `src/core/TopicManager.h`
- Modify: `src/core/TopicManager.cpp`

- [x] **Step 1: 修改 TopicManager.h — 容器类型变更 + 新增 reorderTopics 声明**

将 `mDeviceTopics` 的类型从 `QMap<QString, QSet<QString>>` 改为 `QMap<QString, QStringList>`，同时添加 `reorderTopics()` 声明：

```cpp
// TopicManager.h — 变更部分

// 第 63-66 行，替换 mDeviceTopics 的类型：
private:
    QMap<QString, QStringList> mDeviceTopics;   // deviceSn -> ordered topic list
    QMap<QString, QString>     mTopicToDevice;   // topic -> deviceSn (反向索引)
    QMap<QString, QSet<QString>> mDisabledTopics; // deviceSn -> set of disabled topics (不变)
```

在 `setDisabledTopicsForDevice` 声明之后（第 54 行后）添加：

```cpp
    // 重排某个设备的 topic 顺序（不改变集合内容，不发射 topicsChanged）
    void reorderTopics(const QString& deviceSn, const QStringList& orderedTopics);
```

- [x] **Step 2: 修改 TopicManager.cpp — setDeviceTopics() 适配 QStringList**

将 `setDeviceTopics()` 中的 `QSet<QString> topicSet` 替换为 `QStringList`，使用 `append()` + 去重：

```cpp
void TopicManager::setDeviceTopics(const QString& deviceSn, const QStringList& topics) {
    // 移除旧 topic
    removeDevice(deviceSn);

    QStringList orderedList;
    for (const auto& t : topics) {
        if (!orderedList.contains(t)) {
            orderedList.append(t);
            mTopicToDevice[t] = deviceSn;
        }
    }
    mDeviceTopics[deviceSn] = orderedList;

    // 通知变更（新增所有 topic）
    emit topicsChanged(orderedList, {});
}
```

- [x] **Step 3: 修改 TopicManager.cpp — addTopic() 适配 QStringList**

将 `insert()` 替换为 `append()`（带去重检查）：

```cpp
void TopicManager::addTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        mDeviceTopics[deviceSn] = {};

    // 新添加的 topic 始终从启用状态开始，清理可能残留的禁用记录
    if (mDisabledTopics.contains(deviceSn)) {
        mDisabledTopics[deviceSn].remove(topic);
        if (mDisabledTopics[deviceSn].isEmpty())
            mDisabledTopics.remove(deviceSn);
    }
    // 去重：已存在则不追加
    if (!mDeviceTopics[deviceSn].contains(topic)) {
        mDeviceTopics[deviceSn].append(topic);
        mTopicToDevice[topic] = deviceSn;
    }
    emit topicsChanged({topic}, {});
}
```

- [x] **Step 4: 修改 TopicManager.cpp — removeTopic() 适配 QStringList**

将 `QSet::remove()` 替换为 `QStringList::removeAll()`：

```cpp
void TopicManager::removeTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    mDeviceTopics[deviceSn].removeAll(topic);
    mTopicToDevice.remove(topic);
    // 同步清理禁用记录
    if (mDisabledTopics.contains(deviceSn)) {
        mDisabledTopics[deviceSn].remove(topic);
        if (mDisabledTopics[deviceSn].isEmpty())
            mDisabledTopics.remove(deviceSn);
    }
    emit topicsChanged({}, {topic});
}
```

- [x] **Step 5: 修改 TopicManager.cpp — topicsForDevice() 适配 QStringList**

移除 `.values()` 调用（QStringList 已是有序容器，直接返回即可）：

```cpp
QStringList TopicManager::topicsForDevice(const QString& deviceSn) const {
    return mDeviceTopics.value(deviceSn); // QMap::value() 对 QStringList 直接返回
}
```

- [x] **Step 6: 修改 TopicManager.cpp — allTopics() / allEnabledTopics() / enabledTopicsForDevice() / setTopicEnabled() 适配**

这些方法遍历 `mDeviceTopics` 值，因 `QStringList` 同样支持 range-for，代码基本不变。唯一需要修改的是 `allEnabledTopics()` 和 `enabledTopicsForDevice()` 中的局部变量类型：

```cpp
// allTopics() — 将 .unite(topics) 改为手动去重（QSet::unite 不适用于 QStringList）
QStringList TopicManager::allTopics() const {
    QStringList result;
    for (const auto& topics : mDeviceTopics) {
        for (const auto& t : topics) {
            if (!result.contains(t))
                result.append(t);
        }
    }
    return result;
}

// allEnabledTopics() — 将 QSet 改为 QStringList，去重逻辑相同
QStringList TopicManager::allEnabledTopics() const {
    QStringList result;
    for (auto it = mDeviceTopics.begin(); it != mDeviceTopics.end(); ++it) {
        const QString& sn = it.key();
        const QStringList& deviceTopics = it.value();
        const QSet<QString> disabled = mDisabledTopics.value(sn);
        for (const auto& t : deviceTopics) {
            if (!disabled.contains(t) && !result.contains(t))
                result.append(t);
        }
    }
    return result;
}

// enabledTopicsForDevice() — 将 QSet 局部变量改为 QStringList
QStringList TopicManager::enabledTopicsForDevice(const QString& deviceSn) const {
    QStringList deviceTopics = mDeviceTopics.value(deviceSn);
    QSet<QString> disabled = mDisabledTopics.value(deviceSn);
    QStringList result;
    for (const auto& t : deviceTopics) {
        if (!disabled.contains(t))
            result.append(t);
    }
    return result;
}

// setTopicEnabled() — 将 contains() 调用保持不变（QStringList 也支持 contains）
// 此方法不变
```

- [x] **Step 7: 新增 TopicManager.cpp — reorderTopics() 实现**

```cpp
void TopicManager::reorderTopics(const QString& deviceSn, const QStringList& orderedTopics) {
    if (!mDeviceTopics.contains(deviceSn))
        return;

    // 安全检查：orderedTopics 的集合必须与当前列表一致
    QStringList current = mDeviceTopics[deviceSn];
    if (current.size() != orderedTopics.size())
        return;

    QSet<QString> currentSet(current.begin(), current.end());
    QSet<QString> newSet(orderedTopics.begin(), orderedTopics.end());
    if (currentSet != newSet)
        return;

    // 仅当顺序确实改变时更新
    if (current == orderedTopics)
        return;

    mDeviceTopics[deviceSn] = orderedTopics;
    // 不发射 topicsChanged — 集合内容未变，无需重新订阅 MQTT
}
```

- [x] **Step 8: 编译验证 Task 1**

```bash
cmake --build build_mingw
```

预期：编译通过，无错误。

---

### Task 2: ConfigStore — QSet 迁移为 QStringList（保持顺序持久化）

**Files:**
- Modify: `src/core/ConfigStore.h`
- Modify: `src/core/ConfigStore.cpp`

> **注意:** 设计文档未显式要求此项变更，但 TopicManager 的顺序需通过 ConfigStore 持久化才能在重启后保留。ConfigStore 内部使用 QSet 会破坏 JSON 数组中的原始顺序。

- [x] **Step 1: 修改 ConfigStore.h — mDeviceTopics 类型变更**

```cpp
// ConfigStore.h 第 56 行，替换：
private:
    MqttConfig                      mMqttConfig;
    QVector<DeviceInfo>             mDevices;
    QMap<QString, QStringList>      mDeviceTopics;   // SN -> topics (有序)
    QMap<QString, QSet<QString>>    mDisabledTopics;  // SN -> disabled topics
```

- [x] **Step 2: 修改 ConfigStore.cpp — setTopicsForDevice() 移除 QSet 转换**

```cpp
// ConfigStore.cpp 第 215-217 行，替换为：
void ConfigStore::setTopicsForDevice(const QString& sn, const QStringList& topics) {
    mDeviceTopics[sn] = topics; // 直接保存 QStringList，保持顺序
}
```

- [x] **Step 3: 修改 ConfigStore.cpp — topicsForDevice() 移除 .values() 调用**

```cpp
// ConfigStore.cpp 第 211-213 行，替换为：
QStringList ConfigStore::topicsForDevice(const QString& sn) const {
    return mDeviceTopics.value(sn); // 直接返回，无需 .values()
}
```

- [x] **Step 4: 修改 ConfigStore.cpp — 若 load() 中有 QSet 转换，也一并移除**

需要检查 ConfigStore::load() 的实现。若其内部将 JSON 数组读入 QSet，需改为直接构造 QStringList。

```bash
# 先确认 load() 实现
```

检查结果 — `ConfigStore::load()` 通常在解析 JSON 时使用循环 `topics.append(item.toString())` 方式构建 QStringList 再传入 `setTopicsForDevice()`。若当前写法为 `QSet<QString>(list.begin(), list.end())`，需移除该转换。

- [x] **Step 5: 编译验证 Task 2**

```bash
cmake --build build_mingw
```

预期：编译通过，无错误。

---

### Task 3: TopicListWidget — 排序按钮 UI

**Files:**
- Modify: `src/ui/TopicListWidget.h`
- Modify: `src/ui/TopicListWidget.cpp`

- [ ] **Step 1: 修改 TopicListWidget.h — 新增成员和信号**

在 `topicRemoved` signal 之后（第 31 行后）添加新 signal：

```cpp
signals:
    void topicAdded(const QString& deviceSn, const QString& topic);
    void topicToggled(const QString& deviceSn, const QString& topic);
    void topicRemoved(const QString& deviceSn, const QString& topic);
    void topicSelectionChanged(const QString& topic);
    void topicOrderChanged(const QString& deviceSn, const QStringList& orderedTopics); // 新增
```

在 private 区域 `mRemoveBtn` 之后（第 47 行后）添加新成员：

```cpp
private:
    // ... 现有成员 ...
    QPushButton* mAddBtn;
    QPushButton* mToggleBtn;
    QPushButton* mRemoveBtn;
    QPushButton* mMoveUpBtn;      // 新增：上移
    QPushButton* mMoveDownBtn;    // 新增：下移

    QString      mCurrentSn;
    QStringList  mAllTopics;
    QSet<QString> mDisabledTopics;
```

新增私有槽：

```cpp
private slots:
    void onAddTopic();
    void onToggleTopic();
    void onRemoveTopic();
    void onTopicSelectionChanged();
    void onMoveUp();       // 新增
    void onMoveDown();     // 新增
```

- [ ] **Step 2: 修改 TopicListWidget.cpp — 在构造函数中添加排序按钮**

在 `mRemoveBtn` 创建代码之后（第 56 行后），添加排序按钮的创建：

```cpp
    // 排序按钮
    mMoveUpBtn = new QPushButton("▲", this);
    mMoveUpBtn->setCursor(Qt::PointingHandCursor);
    mMoveUpBtn->setFixedSize(28, 28);
    mMoveUpBtn->setToolTip("上移选中 Topic");
    mMoveUpBtn->setEnabled(false);
    mMoveUpBtn->setStyleSheet(
        "QPushButton { background: #e3f2fd; color: #1565c0; border: 1px solid #90caf9; "
        "border-radius: 4px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background: #bbdefb; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    titleRow->addWidget(mMoveUpBtn);

    mMoveDownBtn = new QPushButton("▼", this);
    mMoveDownBtn->setCursor(Qt::PointingHandCursor);
    mMoveDownBtn->setFixedSize(28, 28);
    mMoveDownBtn->setToolTip("下移选中 Topic");
    mMoveDownBtn->setEnabled(false);
    mMoveDownBtn->setStyleSheet(
        "QPushButton { background: #e3f2fd; color: #1565c0; border: 1px solid #90caf9; "
        "border-radius: 4px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background: #bbdefb; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    titleRow->addWidget(mMoveDownBtn);
```

在构造函数的信号连接区域（第 76 行后），添加排序按钮的连接：

```cpp
    connect(mMoveUpBtn, &QPushButton::clicked, this, &TopicListWidget::onMoveUp);
    connect(mMoveDownBtn, &QPushButton::clicked, this, &TopicListWidget::onMoveDown);
```

- [ ] **Step 3: 修改 TopicListWidget.cpp — clearTopics() 禁用排序按钮**

在 `clearTopics()` 中追加禁用逻辑（第 99 行后）：

```cpp
void TopicListWidget::clearTopics() {
    mCurrentSn.clear();
    mAllTopics.clear();
    mDisabledTopics.clear();
    mTopicList->clear();
    mAddBtn->setEnabled(false);
    mToggleBtn->setEnabled(false);
    mRemoveBtn->setEnabled(false);
    mMoveUpBtn->setEnabled(false);    // 新增
    mMoveDownBtn->setEnabled(false);  // 新增

    if (mAllTopics.isEmpty() && mCurrentSn.isEmpty()) {
        mTopicList->addItem("（请选择设备）");
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
    }
}
```

- [ ] **Step 4: 修改 TopicListWidget.cpp — onTopicSelectionChanged() 更新排序按钮状态**

替换现有的 `onTopicSelectionChanged()`：

```cpp
void TopicListWidget::onTopicSelectionChanged() {
    bool hasSelection = !selectedTopic().isEmpty();
    mToggleBtn->setEnabled(hasSelection);
    mRemoveBtn->setEnabled(hasSelection);

    // 排序按钮：仅当有选中且不是占位行时启用
    int currentRow = mTopicList->currentRow();
    bool canMoveUp = hasSelection && currentRow > 0;
    bool canMoveDown = hasSelection && currentRow < mTopicList->count() - 1;
    mMoveUpBtn->setEnabled(canMoveUp);
    mMoveDownBtn->setEnabled(canMoveDown);

    emit topicSelectionChanged(hasSelection ? selectedTopic() : QString());
}
```

- [ ] **Step 5: 新增 TopicListWidget.cpp — onMoveUp() 和 onMoveDown() 实现**

在文件末尾（第 203 行后）添加：

```cpp
void TopicListWidget::onMoveUp() {
    QString topic = selectedTopic();
    if (topic.isEmpty() || mCurrentSn.isEmpty()) return;

    int idx = mAllTopics.indexOf(topic);
    if (idx <= 0) return;

    // 交换位置
    mAllTopics.swapItemsAt(idx, idx - 1);
    refreshList();

    // 保持选中
    for (int i = 0; i < mTopicList->count(); ++i) {
        if (mTopicList->item(i)->data(Qt::UserRole).toString() == topic) {
            mTopicList->setCurrentRow(i);
            break;
        }
    }

    emit topicOrderChanged(mCurrentSn, mAllTopics);
}

void TopicListWidget::onMoveDown() {
    QString topic = selectedTopic();
    if (topic.isEmpty() || mCurrentSn.isEmpty()) return;

    int idx = mAllTopics.indexOf(topic);
    if (idx < 0 || idx >= mAllTopics.size() - 1) return;

    // 交换位置
    mAllTopics.swapItemsAt(idx, idx + 1);
    refreshList();

    // 保持选中
    for (int i = 0; i < mTopicList->count(); ++i) {
        if (mTopicList->item(i)->data(Qt::UserRole).toString() == topic) {
            mTopicList->setCurrentRow(i);
            break;
        }
    }

    emit topicOrderChanged(mCurrentSn, mAllTopics);
}
```

- [ ] **Step 6: 编译验证 Task 3**

```bash
cmake --build build_mingw
```

预期：编译通过。若 `refreshList()` 调用了 `mTopicList->clear()` 会导致选中状态丢失，排序按钮在 `onTopicSelectionChanged` 中会重新计算状态，因为 `setCurrentRow` 会触发 `itemSelectionChanged`，进而调用 `onTopicSelectionChanged`。无需额外处理。

---

### Task 4: DeviceManager — 默认 topic 自动追加 + reorderTopics 桥接

**Files:**
- Modify: `src/core/DeviceManager.h`
- Modify: `src/core/DeviceManager.cpp`

- [ ] **Step 1: 修改 DeviceManager.h — 新增 reorderTopics 公开方法**

在 `updateTopic` 声明之后（第 37 行后）添加：

```cpp
    // 重排设备 topic 顺序
    void reorderTopics(const QString& deviceSn, const QStringList& orderedTopics);
```

- [ ] **Step 2: 修改 DeviceManager.cpp — 在 addDevice() 中追加默认 topic**

在 `addDevice()` 方法中（第 62-74 行），替换为：

```cpp
// DeviceManager.cpp 顶部添加常量（文件头部 #include 之后）：
static const QStringList DEFAULT_DOCK_TOPICS = {
    "thing/product/{sn}/state",
    "thing/product/{sn}/requests",
    "thing/product/{sn}/events",
    "thing/product/{sn}/services_reply",
    "thing/product/{sn}/property/set_reply",
    "sys/product/{sn}/status",
    "thing/product/{sn}/drc/up",
};

// addDevice() 方法（第 62-74 行），替换为：
void DeviceManager::addDevice(const DeviceInfo& info, const QStringList& topics) {
    mDevices[info.sn] = info;

    if (info.type == DeviceType::Dock) {
        // 机场设备：追加 7 个默认 topic，默认禁用
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
        // 将已有禁用记录与新增默认禁用的 topic 合并
        QSet<QString> existingDisabled = mTopicManager->disabledTopicsForDevice(info.sn);
        existingDisabled.unite(newDisabled);
        mTopicManager->setDisabledTopicsForDevice(info.sn, existingDisabled);
    } else {
        mTopicManager->setDeviceTopics(info.sn, topics);
    }

    // 持久化
    QVector<DeviceInfo> devs;
    for (const auto& d : mDevices)
        devs.append(d);
    mConfigStore->setDevices(devs);
    saveConfig(mConfigPath);

    emit deviceAdded(info.sn);
}
```

- [ ] **Step 3: 新增 DeviceManager.cpp — reorderTopics() 桥接方法**

在文件合适位置（如 `isTopicEnabled()` 之后，第 234 行附近）添加：

```cpp
void DeviceManager::reorderTopics(const QString& deviceSn, const QStringList& orderedTopics) {
    mTopicManager->reorderTopics(deviceSn, orderedTopics);
    // 持久化顺序变更
    mConfigStore->setTopicsForDevice(deviceSn, orderedTopics);
    saveConfig(mConfigPath);
}
```

- [ ] **Step 4: 编译验证 Task 4**

```bash
cmake --build build_mingw
```

预期：编译通过。

---

### Task 5: PublishPanel — 下发预设 topic

**Files:**
- Modify: `src/ui/PublishPanel.h`

- [x] **Step 1: 修改 PublishPanel.h — 新增 mDeviceSn + 预设常量 + setDeviceSn() + 更新 setTopics()**

```cpp
#ifndef PUBLISHPANEL_H
#define PUBLISHPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class PublishPanel : public QWidget {
    Q_OBJECT
public:
    // 5 个下发专用 topic 预设（不订阅，仅用于 ComboBox 下拉选择）
    static const QStringList PUBLISH_PRESETS;

    explicit PublishPanel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 4, 0, 0);
        layout->setSpacing(4);

        // Topic 选择
        auto* topicLayout = new QHBoxLayout;
        auto* topicLabel = new QLabel("Topic:");
        topicLabel->setStyleSheet("font-size: 12px; color: #5f6368;");
        topicLayout->addWidget(topicLabel);

        mTopicCombo = new QComboBox(this);
        mTopicCombo->setEditable(true);
        mTopicCombo->setMinimumWidth(200);
        mTopicCombo->setStyleSheet(
            "QComboBox { padding: 3px 6px; border: 1px solid #dadce0; border-radius: 4px; "
            "background: #fff; font-size: 12px; }");
        topicLayout->addWidget(mTopicCombo, 1);
        layout->addLayout(topicLayout);

        // JSON 编辑区
        mEditor = new QPlainTextEdit(this);
        mEditor->setFont(QFont("Consolas", 10));
        mEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
        mEditor->setPlaceholderText("输入要发送的 JSON...");
        mEditor->setStyleSheet(
            "QPlainTextEdit { background: #fff; border: 1px solid #dadce0; "
            "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 12px; padding: 6px; }");
        layout->addWidget(mEditor, 1);

        // 发送按钮
        auto* btnLayout = new QHBoxLayout;
        btnLayout->addStretch();
        mSendBtn = new QPushButton("发送", this);
        mSendBtn->setEnabled(false);
        mSendBtn->setStyleSheet(
            "QPushButton { background: #1a73e8; color: #fff; border: none; "
            "border-radius: 4px; padding: 6px 24px; font-weight: bold; }"
            "QPushButton:hover { background: #1557b0; }"
            "QPushButton:disabled { background: #dadce0; color: #80868b; }");
        btnLayout->addWidget(mSendBtn);
        layout->addLayout(btnLayout);

        connect(mSendBtn, &QPushButton::clicked, this, [this]() {
            // TODO v1.1: 实现 MQTT publish
        });
    }

    // 设置当前设备 SN（用于替换 {sn} 占位符）
    void setDeviceSn(const QString& sn) { mDeviceSn = sn; }

    // 设置已订阅的 topic 列表，合并显示预设 topic
    void setTopics(const QStringList& subscribed) {
        mTopicCombo->clear();
        mTopicCombo->addItems(subscribed);

        // 分隔线 + 下发预设（仅当有 SN 时显示）
        if (!mDeviceSn.isEmpty()) {
            mTopicCombo->insertSeparator(mTopicCombo->count());
            for (const auto& tpl : PUBLISH_PRESETS)
                mTopicCombo->addItem(QString(tpl).replace("{sn}", mDeviceSn));
        }
    }

private:
    QComboBox*      mTopicCombo;
    QPlainTextEdit* mEditor;
    QPushButton*    mSendBtn;
    QString         mDeviceSn;  // 当前设备 SN
};

// 静态常量定义（头文件末尾，#endif 之前）
inline const QStringList PublishPanel::PUBLISH_PRESETS = {
    "thing/product/{sn}/property/set",
    "thing/product/{sn}/services",
    "thing/product/{sn}/events_reply",
    "thing/product/{sn}/requests_reply",
    "sys/product/{sn}/status_reply",
};

#endif // PUBLISHPANEL_H
```

> **注意:** 由于 `PublishPanel` 是纯头文件内联实现，新增的 `mDeviceSn` 成员和 `PUBLISH_PRESETS` 静态常量均在此文件内定义。

- [x] **Step 2: 编译验证 Task 5**

```bash
cmake --build build_mingw
```

预期：编译通过。

---

### Task 6: MainWindow — 连接排序信号 + 传递 SN 给 PublishPanel

**Files:**
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: 修改 MainWindow.cpp — connectSignals() 中添加排序信号连接**

在 `connectSignals()` 方法中（第 419 行 `topicRemoved` 连接之后）添加：

```cpp
    // Topic 排序信号 → DeviceManager 持久化
    connect(mTopicListWidget, &TopicListWidget::topicOrderChanged,
            this, [this](const QString& sn, const QStringList& ordered) {
        mDevMgr->reorderTopics(sn, ordered);
        refreshTopicList(sn);
    });
```

- [ ] **Step 2: 修改 MainWindow.cpp — onDeviceSelected() 中传递 SN 给 PublishPanel**

在 `onDeviceSelected()` 方法中（第 528 行 `mPublishPanel->setTopics(...)` 之前），添加 `setDeviceSn()` 调用：

```cpp
void MainWindow::onDeviceSelected(const QString& sn) {
    if (sn.isEmpty()) {
        // ... 现有清空逻辑不变 ...
        return;
    }

    DeviceInfo* dev = mDevMgr->device(sn);
    if (!dev) return;

    // ... 现有 OSD 显示逻辑不变 ...

    // 传递当前设备 SN 给下发面板（用于 {sn} 替换）
    mPublishPanel->setDeviceSn(sn);
    mPublishPanel->setTopics(mDevMgr->topicsForDevice(sn));

    // ... 其余逻辑不变 ...
}
```

具体代码位置：在 `onDeviceSelected()` 中第 528 行之前插入 `mPublishPanel->setDeviceSn(sn);`

- [ ] **Step 3: 编译验证 Task 6**

```bash
cmake --build build_mingw
```

预期：编译通过，无错误。

---

### Task 7: 整体编译 + 端到端验证

- [ ] **Step 1: 完整编译项目**

```bash
cmake --build build_mingw
```

预期：编译通过，生成 `build_mingw\DjiCloudApi.exe`。

- [ ] **Step 2: 启动应用，创建新机场设备进行验证**

启动 `build_mingw\DjiCloudApi.exe`，执行以下验证：

1. **默认 topic 验证：**
   - 点击「＋」添加设备，选择「机场」，输入 SN（如 `dock_test`）和子飞机 SN
   - 打开 Topic 列表，确认 7 个默认 topic 已自动出现：
     - `thing/product/dock_test/state`
     - `thing/product/dock_test/requests`
     - `thing/product/dock_test/events`
     - `thing/product/dock_test/services_reply`
     - `thing/product/dock_test/property/set_reply`
     - `sys/product/dock_test/status`
     - `thing/product/dock_test/drc/up`
   - 确认这 7 个 topic 显示为灰色（○ 前缀），即为禁用状态

2. **排序功能验证：**
   - 在 Topic 列表中选中一个 topic
   - 点击「▲」按钮，确认 topic 向上移动一位
   - 点击「▼」按钮，确认 topic 向下移动一位
   - 选中第一项时「▲」应禁用
   - 选中最后一项时「▼」应禁用
   - 选中占位行时两个按钮均应禁用

3. **下发面板预设验证：**
   - 展开「▶ Topic 下发」面板
   - 打开 ComboBox，确认显示：
     - 前 N 项：已订阅 topic
     - 分隔线
     - 后 5 项：预设 topic（含 `dock_test` 替换了 `{sn}`）
   - 确认预设 topic 可以选中

4. **兼容性验证：**
   - 创建无人机设备，确认 Topic 列表仅含用户输入的 topic（无自动追加）
   - 编辑已有设备的 topic，确认无异常
   - 删除设备后重新添加，确认默认 topic 再次自动追加

5. **持久化验证：**
   - 调整 topic 顺序后关闭应用
   - 重新打开，确认 topic 顺序与关闭前一致

6. **配置兼容性验证：**
   - 使用旧版 config.json（若存在），确认可正常加载
   - 确认 `config.json` 中的 topics 数组顺序得到保留

- [ ] **Step 3: 提交变更**

```bash
git add src/core/TopicManager.h src/core/TopicManager.cpp \
        src/core/ConfigStore.h src/core/ConfigStore.cpp \
        src/core/DeviceManager.h src/core/DeviceManager.cpp \
        src/ui/TopicListWidget.h src/ui/TopicListWidget.cpp \
        src/ui/PublishPanel.h src/ui/MainWindow.cpp
git commit -m "feat: topic 预设自动追加、排序功能、下发面板预设

- TopicManager/ConfigStore: QSet → QStringList 迁移，保持 topic 顺序
- DeviceManager: 机场设备自动追加 7 个默认 topic（默认禁用）
- TopicListWidget: 新增 ▲▼ 排序按钮及 topicOrderChanged 信号
- PublishPanel: 新增 5 个下发预设 topic（合并到 ComboBox 显示）
- MainWindow: 连接排序信号，传递设备 SN 给下发面板"
```

---

## 自我审查

### 1. 设计文档覆盖检查

| 设计需求 | 对应 Task |
|----------|-----------|
| 1. QSet → QStringList 迁移 | Task 1 (TopicManager) + Task 2 (ConfigStore) |
| 2. 机场设备默认 topic 追加 | Task 4 Step 2 |
| 3. Topic 排序按钮 (▲▼) | Task 3 |
| 4. TopicManager::reorderTopics() | Task 1 Step 7 |
| 5. DeviceManager 桥接 | Task 4 Step 3 |
| 6. MainWindow 信号连接 | Task 6 Step 1 |
| 7. PublishPanel 下发预设 | Task 5 |
| 8. 涉及文件清单 | 全部覆盖 |

**额外覆盖：** ConfigStore 的 QStringList 迁移（设计文档未显式要求，但顺序持久化必需）。

### 2. 占位符扫描

未使用 "TBD"、"TODO"、"implement later"、"fill in details" 等占位符。所有代码步骤均包含完整实现代码。

### 3. 类型一致性检查

- `TopicManager::mDeviceTopics` 类型：`QMap<QString, QStringList>` → 所有访问点一致
- `ConfigStore::mDeviceTopics` 类型：`QMap<QString, QStringList>` → 与 TopicManager 一致
- `TopicListWidget::topicOrderChanged` signal 参数：`(const QString& sn, const QStringList& orderedTopics)` → MainWindow 连接处匹配
- `DeviceManager::reorderTopics` 签名：`(const QString&, const QStringList&)` → 与 TopicManager 桥接一致
- `PublishPanel::setDeviceSn(const QString&)` → MainWindow 调用处匹配

无类型不一致问题。

### 4. 编译依赖顺序

Task 执行必须严格按顺序：1→2→3→4→5→6→7。原因：
- Task 1 改变 TopicManager 接口，Task 3/4 依赖新接口
- Task 2 依赖 Task 1 的类型变更完成后才能编译 ConfigStore（它调用 TopicManager）
- Task 4 使用 Task 1 的新 `reorderTopics()`
- Task 6 连接 Task 3 的新 signal，调用 Task 4 的新方法

---

**Plan complete.**

## 执行选择

**Plan complete and saved to `docs/superpowers/plans/2026-06-15-topic-presets-and-ordering.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
