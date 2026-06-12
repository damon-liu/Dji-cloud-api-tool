# Topic 列表面板 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add inline topic list panel with enable/disable toggles below device tree, fix MQTT disconnect reconnect bug, and allow Dock devices to add child Aircraft.

**Architecture:** Add `mDisabledTopics` collection to TopicManager and ConfigStore for per-topic enable/disable state. New `TopicListWidget` component sits in left panel below device tree, emitting signals wired through MainWindow to DeviceManager. `MqttClientManager` gains `mIntentionalDisconnect` flag to suppress auto-reconnect on manual disconnect.

**Tech Stack:** Qt 6 (Core, Widgets, Mqtt), C++17, CMake 3.10+

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `src/core/TopicManager.h` | Modify | Add `mDisabledTopics`, enable/disable methods, `allEnabledTopics()` |
| `src/core/TopicManager.cpp` | Modify | Implement enable/disable logic, emit correct topicsChanged signals |
| `src/core/ConfigStore.h` | Modify | Add `mDisabledTopics` storage, getters/setters, change `mDeviceTopics` to `QSet` |
| `src/core/ConfigStore.cpp` | Modify | Parse/write `disabled_topics` JSON field, backward-compatible load |
| `src/core/DeviceManager.h` | Modify | Add `setTopicEnabled()`/`isTopicEnabled()`, update `onTopicsChanged` signature |
| `src/core/DeviceManager.cpp` | Modify | Wire enable/disable between TopicManager and ConfigStore, filter MQTT subs |
| `src/mqtt/MqttClientManager.h` | Modify | Add `mIntentionalDisconnect` flag |
| `src/mqtt/MqttClientManager.cpp` | Modify | Fix auto-reconnect on manual disconnect |
| `src/ui/TopicListWidget.h` | **Create** | New widget: QListWidget + title + action buttons |
| `src/ui/TopicListWidget.cpp` | **Create** | Widget implementation |
| `src/ui/MainWindow.h` | Modify | Replace `mEditTopicBtn` with `mTopicListWidget`, new helpers |
| `src/ui/MainWindow.cpp` | Modify | Layout refactor, signal wiring, add-device logic, toolbar reorder |
| `CMakeLists.txt` | Modify | Add `TopicListWidget.cpp` to build |

---

### Task 1: TopicManager — Add topic enabled/disabled state

**Files:**
- Modify: `src/core/TopicManager.h`
- Modify: `src/core/TopicManager.cpp`

- [ ] **Step 1: Add declarations to TopicManager.h**

Add the disabled-topic collection and new methods to the class. Insert after the existing `removeDevice` declaration (line 38) and before `clear()`:

```cpp
// In src/core/TopicManager.h, inside the class declaration:

    // 获取所有启用的 topic（过滤掉禁用的）——供 MQTT 订阅用
    QStringList allEnabledTopics() const;

    // 设置/查询某个 topic 的启用/禁用状态
    void setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled);
    bool isTopicEnabled(const QString& deviceSn, const QString& topic) const;

    // 获取某个设备所有启用的 topic
    QStringList enabledTopicsForDevice(const QString& deviceSn) const;

    // 获取某个设备所有禁用的 topic（供 ConfigStore 持久化用）
    QSet<QString> disabledTopicsForDevice(const QString& deviceSn) const;

    // 批量设置禁用 topic（供 ConfigStore 加载用）
    void setDisabledTopicsForDevice(const QString& deviceSn, const QSet<QString>& topics);
```

Also add the private member after `mTopicToDevice`:

```cpp
private:
    QMap<QString, QSet<QString>> mDeviceTopics;   // deviceSn -> set of topics
    QMap<QString, QString>       mTopicToDevice;   // topic -> deviceSn (反向索引)
    QMap<QString, QSet<QString>> mDisabledTopics;  // deviceSn -> set of disabled topics
```

- [ ] **Step 2: Implement new methods in TopicManager.cpp**

Add the following implementations at the end of the file (before the closing of the namespace, after `clear()`):

```cpp
QStringList TopicManager::allEnabledTopics() const {
    QSet<QString> result;
    for (auto it = mDeviceTopics.begin(); it != mDeviceTopics.end(); ++it) {
        const QString& sn = it.key();
        const QSet<QString>& deviceTopics = it.value();
        const QSet<QString> disabled = mDisabledTopics.value(sn);
        for (const auto& t : deviceTopics) {
            if (!disabled.contains(t))
                result.insert(t);
        }
    }
    return result.values();
}

void TopicManager::setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled) {
    if (!mDeviceTopics.contains(deviceSn))
        return;

    bool currentlyEnabled = !mDisabledTopics.value(deviceSn).contains(topic);

    if (enabled && !currentlyEnabled) {
        // 启用：从禁用集合中移除
        mDisabledTopics[deviceSn].remove(topic);
        if (mDisabledTopics[deviceSn].isEmpty())
            mDisabledTopics.remove(deviceSn);
        emit topicsChanged({topic}, {});
    } else if (!enabled && currentlyEnabled) {
        // 禁用：加入禁用集合
        mDisabledTopics[deviceSn].insert(topic);
        emit topicsChanged({}, {topic});
    }
    // 状态未变则不操作
}

bool TopicManager::isTopicEnabled(const QString& deviceSn, const QString& topic) const {
    return !mDisabledTopics.value(deviceSn).contains(topic);
}

QStringList TopicManager::enabledTopicsForDevice(const QString& deviceSn) const {
    QSet<QString> deviceTopics = mDeviceTopics.value(deviceSn);
    QSet<QString> disabled = mDisabledTopics.value(deviceSn);
    QStringList result;
    for (const auto& t : deviceTopics) {
        if (!disabled.contains(t))
            result.append(t);
    }
    return result;
}

QSet<QString> TopicManager::disabledTopicsForDevice(const QString& deviceSn) const {
    return mDisabledTopics.value(deviceSn);
}

void TopicManager::setDisabledTopicsForDevice(const QString& deviceSn, const QSet<QString>& topics) {
    if (topics.isEmpty())
        mDisabledTopics.remove(deviceSn);
    else
        mDisabledTopics[deviceSn] = topics;
}
```

- [ ] **Step 3: Update removeDevice() to clean up disabled topics**

In `src/core/TopicManager.cpp`, modify the `removeDevice()` method to also clear disabled topics for the device. Replace the existing method:

```cpp
void TopicManager::removeDevice(const QString& deviceSn) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    QStringList removed = mDeviceTopics[deviceSn].values();
    for (const auto& t : removed)
        mTopicToDevice.remove(t);
    mDeviceTopics.remove(deviceSn);
    mDisabledTopics.remove(deviceSn);  // ← added line
    emit topicsChanged({}, removed);
}
```

- [ ] **Step 4: Update removeTopic() to clean up disabled entry**

In `src/core/TopicManager.cpp`, modify `removeTopic()` to also remove from disabled set:

```cpp
void TopicManager::removeTopic(const QString& deviceSn, const QString& topic) {
    if (!mDeviceTopics.contains(deviceSn))
        return;
    mDeviceTopics[deviceSn].remove(topic);
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

- [ ] **Step 5: Update clear() to clean up disabled topics**

In `src/core/TopicManager.cpp`, modify `clear()`:

```cpp
void TopicManager::clear() {
    QStringList removed = allTopics();
    mDeviceTopics.clear();
    mTopicToDevice.clear();
    mDisabledTopics.clear();  // ← added line
    emit topicsChanged({}, removed);
}
```

- [ ] **Step 6: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds with no errors (unused method warnings are OK at this stage since no caller yet).

---

### Task 2: ConfigStore — Parse and persist disabled_topics

**Files:**
- Modify: `src/core/ConfigStore.h`
- Modify: `src/core/ConfigStore.cpp`

- [ ] **Step 1: Update ConfigStore.h**

Change `mDeviceTopics` type from `QMap<QString, QStringList>` to `QMap<QString, QSet<QString>>`, and add disabled topics storage + accessors.

Replace the relevant section in `src/core/ConfigStore.h`:

```cpp
    // 获取设备的所有 topic（通过 SN）
    QStringList topicsForDevice(const QString& sn) const;
    void setTopicsForDevice(const QString& sn, const QStringList& topics);

    // 禁用 topic 管理
    QStringList disabledTopicsForDevice(const QString& sn) const;
    void setDisabledTopicsForDevice(const QString& sn, const QStringList& topics);
```

Add `#include <QSet>` at the top if not already present (it's already there from TopicManager.h inclusion pattern — verify it's included). Change the private member:

```cpp
private:
    QString defaultConfigPath() const;

    MqttConfig                  mMqttConfig;
    QVector<DeviceInfo>         mDevices;
    QMap<QString, QSet<QString>> mDeviceTopics;        // SN -> topics (changed from QStringList)
    QMap<QString, QSet<QString>> mDisabledTopics;      // SN -> disabled topics
```

- [ ] **Step 2: Update load() in ConfigStore.cpp to parse disabled_topics**

The `load()` method needs to parse `disabled_topics` from JSON. In the device parsing loop (inside `for (const auto& val : devs)`), after reading the topics array, add parsing of `disabled_topics`. Replace the device parsing section in `load()` (the entire for-loop body):

```cpp
    // 解析设备列表
    mDevices.clear();
    mDeviceTopics.clear();
    mDisabledTopics.clear();
    QJsonArray devs = root["devices"].toArray();
    for (const auto& val : devs) {
        QJsonObject devObj = val.toObject();
        DeviceInfo info = DeviceInfo::fromJson(devObj);

        // 该设备自己的所有 topic
        QSet<QString> allTopics;
        QJsonArray topicArr = devObj["topics"].toArray();
        for (const auto& t : topicArr)
            allTopics.insert(t.toString());

        // 该设备禁用的 topic（向后兼容：旧配置无此字段）
        QSet<QString> disabledTopics;
        QJsonArray disabledArr = devObj["disabled_topics"].toArray();
        for (const auto& t : disabledArr)
            disabledTopics.insert(t.toString());

        if (info.type == DeviceType::Dock) {
            // 机场 topics（只保留包含 dock_sn 的）
            QSet<QString> dockTopics;
            QSet<QString> dockDisabled;
            for (const auto& t : allTopics) {
                if (t.contains(info.sn))
                    dockTopics.insert(t);
            }
            for (const auto& t : disabledTopics) {
                if (t.contains(info.sn))
                    dockDisabled.insert(t);
            }
            mDeviceTopics[info.sn] = dockTopics;
            if (!dockDisabled.isEmpty())
                mDisabledTopics[info.sn] = dockDisabled;
            mDevices.append(info);

            // 子飞机
            QString aircraftSn = devObj.value("aircraft_sn").toString();
            if (!aircraftSn.isEmpty()) {
                DeviceInfo child;
                child.sn       = aircraftSn;
                child.name     = info.name + "-飞机";
                child.type     = DeviceType::Aircraft;
                child.parentSn = info.sn;
                mDevices.append(child);

                // 子飞机 topics
                QSet<QString> childTopics;
                QSet<QString> childDisabled;
                for (const auto& t : allTopics) {
                    if (t.contains(aircraftSn))
                        childTopics.insert(t);
                }
                for (const auto& t : disabledTopics) {
                    if (t.contains(aircraftSn))
                        childDisabled.insert(t);
                }
                mDeviceTopics[child.sn] = childTopics;
                if (!childDisabled.isEmpty())
                    mDisabledTopics[child.sn] = childDisabled;
            }
        } else {
            // 独立手飞
            mDeviceTopics[info.sn] = allTopics;
            if (!disabledTopics.isEmpty())
                mDisabledTopics[info.sn] = disabledTopics;
            mDevices.append(info);
        }
    }
```

- [ ] **Step 3: Update save() in ConfigStore.cpp to write disabled_topics**

The `save()` method needs to write `disabled_topics` per device. Replace the dock-device JSON-building section (the block inside `if (d.type == DeviceType::Dock)`) to include disabled_topics:

```cpp
        if (d.type == DeviceType::Dock) {
            QJsonObject obj = d.toJson();
            obj["aircraft_sn"] = "";
            QJsonArray topics;
            for (const auto& t : mDeviceTopics.value(d.sn))
                topics.append(t);
            obj["topics"] = topics;

            // 禁用 topic
            QJsonArray disabledArr;
            QSet<QString> deviceDisabled = mDisabledTopics.value(d.sn);
            for (const auto& t : deviceDisabled)
                disabledArr.append(t);
            if (!disabledArr.isEmpty())
                obj["disabled_topics"] = disabledArr;

            dockMap[d.sn] = obj;
```

And the independent aircraft section:

```cpp
        } else {
            // 独立手飞
            QJsonObject obj = d.toJson();
            QJsonArray topics;
            for (const auto& t : mDeviceTopics.value(d.sn))
                topics.append(t);
            obj["topics"] = topics;

            QJsonArray disabledArr;
            QSet<QString> deviceDisabled = mDisabledTopics.value(d.sn);
            for (const auto& t : deviceDisabled)
                disabledArr.append(t);
            if (!disabledArr.isEmpty())
                obj["disabled_topics"] = disabledArr;

            pilotList.append(obj);
        }
```

Also handle child aircraft disabled topics when merging into dock's JSON — in the `else if (d.isChild())` block after the existing topics merge:

```cpp
        } else if (d.isChild()) {
            // 库内飞机合并到父机场
            if (dockMap.contains(d.parentSn)) {
                dockMap[d.parentSn]["aircraft_sn"] = d.sn;
                QJsonArray topics = dockMap[d.parentSn]["topics"].toArray();
                for (const auto& t : mDeviceTopics.value(d.sn))
                    topics.append(t);
                dockMap[d.parentSn]["topics"] = topics;

                // 合并子飞机禁用 topic 到父条目
                QSet<QString> childDisabled = mDisabledTopics.value(d.sn);
                if (!childDisabled.isEmpty()) {
                    QJsonArray existingDisabled = dockMap[d.parentSn]["disabled_topics"].toArray();
                    for (const auto& t : childDisabled)
                        existingDisabled.append(t);
                    dockMap[d.parentSn]["disabled_topics"] = existingDisabled;
                }
            }
```

- [ ] **Step 4: Update ConfigStore getters/setters**

Update `topicsForDevice()` and `setTopicsForDevice()` to handle the QSet → QStringList conversion:

```cpp
QStringList ConfigStore::topicsForDevice(const QString& sn) const {
    return mDeviceTopics.value(sn).values();
}

void ConfigStore::setTopicsForDevice(const QString& sn, const QStringList& topics) {
    mDeviceTopics[sn] = QSet<QString>(topics.begin(), topics.end());
}
```

Add the new disabled-topics accessors:

```cpp
QStringList ConfigStore::disabledTopicsForDevice(const QString& sn) const {
    return mDisabledTopics.value(sn).values();
}

void ConfigStore::setDisabledTopicsForDevice(const QString& sn, const QStringList& topics) {
    if (topics.isEmpty())
        mDisabledTopics.remove(sn);
    else
        mDisabledTopics[sn] = QSet<QString>(topics.begin(), topics.end());
}
```

- [ ] **Step 5: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds.

---

### Task 3: DeviceManager — Wire enable/disable and filter MQTT subscriptions

**Files:**
- Modify: `src/core/DeviceManager.h`
- Modify: `src/core/DeviceManager.cpp`

- [ ] **Step 1: Add declarations to DeviceManager.h**

Add `setTopicEnabled()` and `isTopicEnabled()` to the public interface. Insert after the existing `updateTopic()` declaration (line 37):

```cpp
    // Topic 启用/禁用控制
    void setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled);
    bool isTopicEnabled(const QString& deviceSn, const QString& topic) const;
```

- [ ] **Step 2: Update initialize() to load disabled topics**

In `src/core/DeviceManager.cpp`, modify `initialize()` to load disabled topics from ConfigStore into TopicManager after setting device topics:

```cpp
bool DeviceManager::initialize(const QString& configPath) {
    mConfigPath = configPath;
    if (!mConfigStore->load(configPath))
        return false;

    // 加载设备到内存
    for (const auto& info : mConfigStore->devices()) {
        mDevices[info.sn] = info;
        QStringList topics = mConfigStore->topicsForDevice(info.sn);
        mTopicManager->setDeviceTopics(info.sn, topics);

        // 加载禁用 topic 状态
        QStringList disabled = mConfigStore->disabledTopicsForDevice(info.sn);
        if (!disabled.isEmpty()) {
            mTopicManager->setDisabledTopicsForDevice(
                info.sn,
                QSet<QString>(disabled.begin(), disabled.end()));
        }
    }

    qDebug() << "DeviceManager: initialized with" << mDevices.size() << "devices";
    return true;
}
```

- [ ] **Step 3: Update onMqttConnected() to use enabled topics only**

In `DeviceManager::onMqttConnected()`, change `allTopics()` to `allEnabledTopics()`:

```cpp
void DeviceManager::onMqttConnected() {
    // 连接成功后订阅所有启用的 topic
    QStringList all = mTopicManager->allEnabledTopics();
    mMqttManager->subscribeTopics(all);
    emit brokerConnected();
}
```

- [ ] **Step 4: Implement setTopicEnabled() and isTopicEnabled()**

Add the new methods at the end of `DeviceManager.cpp` (before the private slots section):

```cpp
void DeviceManager::setTopicEnabled(const QString& deviceSn, const QString& topic, bool enabled) {
    mTopicManager->setTopicEnabled(deviceSn, topic, enabled);
    // 持久化到 ConfigStore
    QStringList disabled = mTopicManager->disabledTopicsForDevice(deviceSn).values();
    mConfigStore->setDisabledTopicsForDevice(deviceSn, disabled);
    saveConfig(mConfigPath);
}

bool DeviceManager::isTopicEnabled(const QString& deviceSn, const QString& topic) const {
    return mTopicManager->isTopicEnabled(deviceSn, topic);
}
```

- [ ] **Step 5: Update addDevice() to persist disabled topics on new device**

In `DeviceManager::addDevice()`, the `setDeviceTopics()` call clears any previous disabled topics for that SN, which is correct for new devices. No change needed — new devices start with all topics enabled.

- [ ] **Step 6: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds.

---

### Task 4: MqttClientManager — Fix disconnect reconnect bug

**Files:**
- Modify: `src/mqtt/MqttClientManager.h`
- Modify: `src/mqtt/MqttClientManager.cpp`

- [ ] **Step 1: Add mIntentionalDisconnect flag to header**

In `src/mqtt/MqttClientManager.h`, add the flag to private members:

```cpp
private:
    void startReconnect();
    void stopReconnect();

    QMqttClient*  mClient;
    QTimer*       mReconnectTimer;
    MqttConfig    mConfig;
    QStringList   mSubscribedTopics;
    int           mReconnectDelayMs;
    bool          mIntentionalDisconnect = false;  // ← added
    static constexpr int MAX_RECONNECT_MS = 30000;
    static constexpr int BASE_RECONNECT_MS = 1000;
```

- [ ] **Step 2: Set flag in disconnectFromBroker()**

In `src/mqtt/MqttClientManager.cpp`, modify `disconnectFromBroker()`:

```cpp
void MqttClientManager::disconnectFromBroker() {
    mIntentionalDisconnect = true;  // ← added
    stopReconnect();
    mSubscribedTopics.clear();
    mClient->disconnectFromHost();
}
```

- [ ] **Step 3: Check flag in onDisconnected()**

In `src/mqtt/MqttClientManager.cpp`, modify `onDisconnected()`:

```cpp
void MqttClientManager::onDisconnected() {
    qDebug() << "MQTT: disconnected";
    emit disconnected();
    if (mIntentionalDisconnect) {
        mIntentionalDisconnect = false;
        return;  // ← skip auto-reconnect
    }
    startReconnect();
}
```

- [ ] **Step 4: Reset flag in connectToBroker()**

In `src/mqtt/MqttClientManager.cpp`, modify `connectToBroker()`:

```cpp
void MqttClientManager::connectToBroker(const MqttConfig& config) {
    mConfig = config;
    mReconnectDelayMs = BASE_RECONNECT_MS;
    mIntentionalDisconnect = false;  // ← added

    mClient->setHostname(config.host);
    mClient->setPort(static_cast<quint16>(config.port));
    mClient->setUsername(config.username);
    mClient->setPassword(config.password);
    mClient->setClientId("DjiCloudApi_" + QString::number(QCoreApplication::applicationPid()));

    qDebug() << "MQTT: connecting to" << config.host << ":" << config.port;
    mClient->connectToHost();
}
```

- [ ] **Step 5: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds.

---

### Task 5: TopicListWidget — New UI component

**Files:**
- Create: `src/ui/TopicListWidget.h`
- Create: `src/ui/TopicListWidget.cpp`

- [ ] **Step 1: Create TopicListWidget.h**

```cpp
#ifndef TOPICLISTWIDGET_H
#define TOPICLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSet>

// TopicListWidget: 展示当前设备 topic 列表，支持启用/禁用、添加、删除
class TopicListWidget : public QWidget {
    Q_OBJECT
public:
    explicit TopicListWidget(QWidget* parent = nullptr);

    // 设置当前显示的 topic 列表
    // sn: 当前设备 SN，topics: 所有 topic，disabledTopics: 禁用的 topic 集合
    void setTopics(const QString& sn,
                   const QStringList& topics,
                   const QSet<QString>& disabledTopics);

    // 清除显示（无设备选中时）
    void clearTopics();

    // 获取当前列表中选中的 topic 字符串，无选中返回空
    QString selectedTopic() const;

signals:
    void topicAdded(const QString& deviceSn, const QString& topic);
    void topicToggled(const QString& deviceSn, const QString& topic);
    void topicRemoved(const QString& deviceSn, const QString& topic);

private slots:
    void onAddTopic();
    void onToggleTopic();
    void onRemoveTopic();
    void onTopicSelectionChanged();

private:
    void refreshList();

    QLabel*      mTitleLabel;
    QListWidget* mTopicList;
    QPushButton* mAddBtn;
    QPushButton* mToggleBtn;
    QPushButton* mRemoveBtn;

    QString      mCurrentSn;
    QStringList  mAllTopics;
    QSet<QString> mDisabledTopics;
};

#endif // TOPICLISTWIDGET_H
```

- [ ] **Step 2: Create TopicListWidget.cpp**

```cpp
#include "TopicListWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

TopicListWidget::TopicListWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // 标题
    mTitleLabel = new QLabel("Topic 列表", this);
    mTitleLabel->setObjectName("sectionTitle");
    layout->addWidget(mTitleLabel);

    // Topic 列表 + 按钮的水平布局
    auto* row = new QHBoxLayout;
    row->setSpacing(4);

    // QListWidget
    mTopicList = new QListWidget(this);
    mTopicList->setMaximumHeight(160);
    mTopicList->setStyleSheet(
        "QListWidget { background: #ffffff; border: 1px solid #e0e0e0; border-radius: 4px; "
        "font-size: 12px; }"
        "QListWidget::item { padding: 3px 6px; }"
        "QListWidget::item:selected { background: #e8f0fe; color: #1a73e8; }");
    row->addWidget(mTopicList, 1);

    // 按钮列
    auto* btnCol = new QVBoxLayout;
    btnCol->setSpacing(4);

    mAddBtn = new QPushButton("＋", this);
    mAddBtn->setCursor(Qt::PointingHandCursor);
    mAddBtn->setFixedSize(28, 28);
    mAddBtn->setToolTip("添加 Topic");
    mAddBtn->setStyleSheet(
        "QPushButton { background: #e8f5e9; color: #2e7d32; border: 1px solid #a5d6a7; "
        "border-radius: 4px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #c8e6c9; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    btnCol->addWidget(mAddBtn);

    mToggleBtn = new QPushButton("◎", this);
    mToggleBtn->setCursor(Qt::PointingHandCursor);
    mToggleBtn->setFixedSize(28, 28);
    mToggleBtn->setToolTip("启用/禁用 Topic");
    mToggleBtn->setEnabled(false);
    mToggleBtn->setStyleSheet(
        "QPushButton { background: #fff3e0; color: #e65100; border: 1px solid #ffcc80; "
        "border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background: #ffe0b2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    btnCol->addWidget(mToggleBtn);

    mRemoveBtn = new QPushButton("✕", this);
    mRemoveBtn->setCursor(Qt::PointingHandCursor);
    mRemoveBtn->setFixedSize(28, 28);
    mRemoveBtn->setToolTip("删除 Topic");
    mRemoveBtn->setEnabled(false);
    mRemoveBtn->setStyleSheet(
        "QPushButton { background: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
        "border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background: #ffcdd2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    btnCol->addWidget(mRemoveBtn);

    btnCol->addStretch();
    row->addLayout(btnCol);

    layout->addLayout(row);

    // 信号连接
    connect(mAddBtn, &QPushButton::clicked, this, &TopicListWidget::onAddTopic);
    connect(mToggleBtn, &QPushButton::clicked, this, &TopicListWidget::onToggleTopic);
    connect(mRemoveBtn, &QPushButton::clicked, this, &TopicListWidget::onRemoveTopic);
    connect(mTopicList, &QListWidget::itemSelectionChanged,
            this, &TopicListWidget::onTopicSelectionChanged);

    // 初始状态：无设备选中
    clearTopics();
}

void TopicListWidget::setTopics(const QString& sn,
                                const QStringList& topics,
                                const QSet<QString>& disabledTopics) {
    mCurrentSn = sn;
    mAllTopics = topics;
    mDisabledTopics = disabledTopics;
    refreshList();
}

void TopicListWidget::clearTopics() {
    mCurrentSn.clear();
    mAllTopics.clear();
    mDisabledTopics.clear();
    mTopicList->clear();
    mAddBtn->setEnabled(false);
    mToggleBtn->setEnabled(false);
    mRemoveBtn->setEnabled(false);

    if (mAllTopics.isEmpty()) {
        mTopicList->addItem("（请选择设备）");
        // 禁用 placeholder 的交互
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
    }
}

QString TopicListWidget::selectedTopic() const {
    auto* item = mTopicList->currentItem();
    if (!item) return {};
    // 跳过 placeholder 行
    QString text = item->text();
    if (text.startsWith("（"))
        return {};
    // 移除 ●/○ 前缀（2 个字符：emoji + space）
    return text.mid(2);
}

void TopicListWidget::refreshList() {
    mTopicList->clear();
    mAddBtn->setEnabled(true);

    if (mCurrentSn.isEmpty()) {
        mTopicList->addItem("（请选择设备）");
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
        mAddBtn->setEnabled(false);
        return;
    }

    if (mAllTopics.isEmpty()) {
        mTopicList->addItem("（无 Topic）");
        mTopicList->item(0)->setFlags(Qt::NoItemFlags);
        mTopicList->item(0)->setForeground(QColor(180, 180, 180));
        return;
    }

    for (const auto& t : mAllTopics) {
        bool enabled = !mDisabledTopics.contains(t);
        QString prefix = enabled ? QString::fromUtf8("● ") : QString::fromUtf8("○ ");
        auto* item = new QListWidgetItem(prefix + t);
        item->setData(Qt::UserRole, t);  // 存储原始 topic 字符串
        if (!enabled)
            item->setForeground(QColor(180, 180, 180));
        mTopicList->addItem(item);
    }

    // 尝试恢复选中
    if (mTopicList->count() > 0)
        mTopicList->setCurrentRow(0);
}

void TopicListWidget::onAddTopic() {
    if (mCurrentSn.isEmpty()) return;

    QString defaultTopic = QString("thing/product/%1/osd").arg(mCurrentSn);
    QString topic = QInputDialog::getText(this, "添加 Topic",
        "输入 MQTT Topic 字符串:", QLineEdit::Normal, defaultTopic);
    if (topic.trimmed().isEmpty()) return;

    // 检查重复
    QString finalTopic = topic.trimmed();
    if (mAllTopics.contains(finalTopic)) {
        QMessageBox::information(this, "提示", "该 Topic 已存在。");
        return;
    }

    emit topicAdded(mCurrentSn, finalTopic);
}

void TopicListWidget::onToggleTopic() {
    QString topic = selectedTopic();
    if (topic.isEmpty() || mCurrentSn.isEmpty()) return;
    emit topicToggled(mCurrentSn, topic);
}

void TopicListWidget::onRemoveTopic() {
    QString topic = selectedTopic();
    if (topic.isEmpty() || mCurrentSn.isEmpty()) return;

    auto ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除 Topic「%1」吗？").arg(topic),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes)
        emit topicRemoved(mCurrentSn, topic);
}

void TopicListWidget::onTopicSelectionChanged() {
    bool hasSelection = !selectedTopic().isEmpty();
    mToggleBtn->setEnabled(hasSelection);
    mRemoveBtn->setEnabled(hasSelection);
}
```

- [ ] **Step 3: Build to verify compilation (will fail until CMakeLists.txt is updated — do Task 8 first or verify syntax manually)**

Note: The build will fail until `TopicListWidget.cpp` is added to CMakeLists.txt. Proceed to Task 8 to add it, then build.

---

### Task 6: MainWindow — Layout refactoring and signal wiring

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Update MainWindow.h declarations**

Replace `mEditTopicBtn` with `mTopicListWidget`, add `refreshTopicList()` helper, and forward-declare `TopicListWidget`:

```cpp
// Add include at top:
#include "TopicListWidget.h"

// Replace in private section:
    // Sidebar buttons
    QPushButton*       mAddDeviceBtn;
    QPushButton*       mDeleteDeviceBtn;

    // Topic list panel (replaces mEditTopicBtn)
    TopicListWidget*   mTopicListWidget;

    // Helper: refresh topic list for currently selected device
    void refreshTopicList(const QString& sn);

    // Stylesheet helper
    void applyStyle();
```

Remove these lines:
```cpp
    QPushButton*       mEditTopicBtn;       // ← REMOVE
```

- [ ] **Step 2: Add TopicListWidget to setupLayout() in left panel**

In `src/ui/MainWindow.cpp`, modify `setupLayout()`. After the `treeAndBtns` layout ends and before `leftLayout->addLayout(treeAndBtns, 1)`, add the TopicListWidget. Also remove `mEditTopicBtn` from the button column.

Replace the entire left-panel section of `setupLayout()` starting from `// 设备和按钮横向排列`:

```cpp
    // 设备和按钮横向排列：设备树 | 操作按钮
    auto* treeAndBtns = new QHBoxLayout;
    treeAndBtns->setSpacing(6);

    // 设备树
    mDeviceTree = new DeviceTreeWidget(this);
    mDeviceTree->setMinimumWidth(170);
    mDeviceTree->setMaximumWidth(220);
    treeAndBtns->addWidget(mDeviceTree, 1);

    // 按钮竖排
    auto* btnCol = new QVBoxLayout;
    btnCol->setSpacing(4);

    mAddDeviceBtn = new QPushButton("＋", this);
    mAddDeviceBtn->setCursor(Qt::PointingHandCursor);
    mAddDeviceBtn->setFixedSize(32, 32);
    mAddDeviceBtn->setToolTip("添加设备");
    mAddDeviceBtn->setStyleSheet(
        "QPushButton { background: #e8f5e9; color: #2e7d32; border: 1px solid #a5d6a7; "
        "border-radius: 4px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #c8e6c9; }");
    connect(mAddDeviceBtn, &QPushButton::clicked, this, &MainWindow::onAddDevice);
    btnCol->addWidget(mAddDeviceBtn);

    mDeleteDeviceBtn = new QPushButton("✕", this);
    mDeleteDeviceBtn->setCursor(Qt::PointingHandCursor);
    mDeleteDeviceBtn->setEnabled(false);
    mDeleteDeviceBtn->setFixedSize(32, 32);
    mDeleteDeviceBtn->setToolTip("删除设备");
    mDeleteDeviceBtn->setStyleSheet(
        "QPushButton { background: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
        "border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #ffcdd2; }"
        "QPushButton:disabled { background: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }");
    connect(mDeleteDeviceBtn, &QPushButton::clicked, this, &MainWindow::onDeleteDevice);
    btnCol->addWidget(mDeleteDeviceBtn);

    btnCol->addStretch();
    treeAndBtns->addLayout(btnCol);

    leftLayout->addLayout(treeAndBtns, 1);

    // === Topic 列表面板 （设备树下方） ===
    mTopicListWidget = new TopicListWidget(this);
    mTopicListWidget->setMaximumHeight(200);
    leftLayout->addWidget(mTopicListWidget);
```

- [ ] **Step 3: Update connectSignals() for new TopicListWidget signals**

In `MainWindow::connectSignals()`, replace the old `mEditTopicBtn` and `onEditTopic` connections with TopicListWidget signal wiring. Add after the `mDeviceTree` connection:

```cpp
    // TopicListWidget signals → DeviceManager
    connect(mTopicListWidget, &TopicListWidget::topicAdded,
            this, [this](const QString& sn, const QString& topic) {
        mDevMgr->addTopic(sn, topic);
        refreshTopicList(sn);
    });
    connect(mTopicListWidget, &TopicListWidget::topicToggled,
            this, [this](const QString& sn, const QString& topic) {
        bool currentlyEnabled = mDevMgr->isTopicEnabled(sn, topic);
        mDevMgr->setTopicEnabled(sn, topic, !currentlyEnabled);
        refreshTopicList(sn);
    });
    connect(mTopicListWidget, &TopicListWidget::topicRemoved,
            this, [this](const QString& sn, const QString& topic) {
        mDevMgr->removeTopic(sn, topic);
        refreshTopicList(sn);
    });
```

Also update the `deviceAdded` and `deviceRemoved` lambdas to refresh the topic list:

```cpp
    connect(mDevMgr, &DeviceManager::deviceAdded, this, [this]() {
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        // Refresh topic list for currently selected device
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshTopicList(currentSn);
        updateStatusBar();
    });
    connect(mDevMgr, &DeviceManager::deviceRemoved, this, [this]() {
        mDeviceTree->rebuild(mDevMgr->topLevelDevices(), mDevMgr->allDevices());
        mOsdPanel->clear();
        mRawJsonPanel->clear();
        mTopicListWidget->clearTopics();
        updateStatusBar();
    });
```

- [ ] **Step 4: Update onDeviceSelected()**

Replace the `onDeviceSelected()` method:

```cpp
void MainWindow::onDeviceSelected(const QString& sn) {
    if (sn.isEmpty()) return;

    DeviceInfo* dev = mDevMgr->device(sn);
    if (!dev) return;

    const AircraftOsd* airOsd = mDevMgr->latestAircraftOsd(sn);
    const DockOsd* dockOsd   = mDevMgr->latestDockOsd(sn);
    mOsdPanel->showOsd(dev, airOsd, dockOsd, mDevMgr->latestRawJson(sn));

    mRawJsonPanel->setJson(mDevMgr->latestRawJson(sn));
    mPublishPanel->setTopics(mDevMgr->topicsForDevice(sn));

    // 刷新 topic 列表
    refreshTopicList(sn);

    // 启用操作按钮
    mDeleteDeviceBtn->setEnabled(true);

    // 根据设备类型控制添加按钮
    if (dev->type == DeviceType::Aircraft) {
        mAddDeviceBtn->setEnabled(false);
    } else {
        mAddDeviceBtn->setEnabled(true);
    }
}
```

- [ ] **Step 5: Add refreshTopicList() helper**

Add the helper method at the end of `MainWindow.cpp` (before `// ——— 设备操作 ———` section or near `updateStatusBar()`):

```cpp
void MainWindow::refreshTopicList(const QString& sn) {
    if (sn.isEmpty()) {
        mTopicListWidget->clearTopics();
        return;
    }
    QStringList topics = mDevMgr->topicsForDevice(sn);
    // Collect disabled topics from DeviceManager
    QSet<QString> disabled;
    for (const auto& t : topics) {
        if (!mDevMgr->isTopicEnabled(sn, t))
            disabled.insert(t);
    }
    mTopicListWidget->setTopics(sn, topics, disabled);
}
```

- [ ] **Step 6: Remove old mEditTopicBtn references**

In `MainWindow::onDeleteDevice()`, remove the `mEditTopicBtn->setEnabled(false)` line (keep the `mDeleteDeviceBtn->setEnabled(false)` line).

Remove the entire `onEditTopic()` method from `MainWindow.cpp` (lines 460-486).

Remove the `#include "TopicEditDialog.h"` from MainWindow.cpp if it's no longer used. (It's still used elsewhere and may be needed — keep it for now since TopicEditDialog may still be useful.)

- [ ] **Step 7: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds.

---

### Task 7: MainWindow — Toolbar button repositioning

**Files:**
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Reorder toolbar in setupToolBar()**

In `src/ui/MainWindow.cpp`, replace `setupToolBar()` to move connect/disconnect buttons to the right of the spacer:

```cpp
void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar("main");
    toolbar->setMovable(false);
    toolbar->setFloatable(false);

    // 左侧：配置按钮
    auto* configAct = toolbar->addAction("⚙ 配置");
    auto* configBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(configAct));
    if (configBtn) configBtn->setObjectName("configBtn");
    connect(configAct, &QAction::triggered, this, [this]() {
        ConfigDialog dlg(mDevMgr->mqttConfig(), this);
        if (dlg.exec() == QDialog::Accepted) {
            mDevMgr->setMqttConfig(dlg.getConfig());
            if (!mDevMgr->isConnected()) {
                mDevMgr->connectBroker();
            }
        }
    });

    // Spacer: pushes everything after it to the right
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Broker 信息标签（spacer 右侧）
    mBrokerLabel = new QLabel(" 未连接", this);
    mBrokerLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 0 12px;");
    toolbar->addWidget(mBrokerLabel);

    // 连接/断开按钮（右侧）
    mConnectAct = toolbar->addAction("● 连接");
    auto* connectBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(mConnectAct));
    if (connectBtn) connectBtn->setObjectName("connectBtn");
    connect(mConnectAct, &QAction::triggered, this, &MainWindow::onConnectAction);

    mDisconnectAct = toolbar->addAction("◎ 断开");
    auto* disconnectBtn = qobject_cast<QToolButton*>(toolbar->widgetForAction(mDisconnectAct));
    if (disconnectBtn) disconnectBtn->setObjectName("disconnectBtn");
    connect(mDisconnectAct, &QAction::triggered, this, &MainWindow::onDisconnectAction);
}
```

- [ ] **Step 2: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds.

---

### Task 8: MainWindow — Add device logic changes (Dock child device)

**Files:**
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Rewrite onAddDevice()**

Replace the existing `onAddDevice()` method in `src/ui/MainWindow.cpp`:

```cpp
void MainWindow::onAddDevice() {
    QString selectedSn = mDeviceTree->selectedDeviceSn();
    DeviceInfo* selectedDev = nullptr;
    if (!selectedSn.isEmpty())
        selectedDev = mDevMgr->device(selectedSn);

    // Determine if we're adding a child device to a Dock
    bool addingChild = (selectedDev && selectedDev->type == DeviceType::Dock);

    QString sn;
    QString name;
    DeviceType type;

    if (addingChild) {
        // Adding a child Aircraft to the selected Dock
        sn = QInputDialog::getText(this, "添加手飞无人机",
            QString("为机场「%1」添加手飞无人机\n设备序列号 (SN):").arg(selectedDev->name));
        if (sn.trimmed().isEmpty()) return;

        name = QInputDialog::getText(this, "添加手飞无人机", "设备名称:",
            QLineEdit::Normal, sn.trimmed());
        if (name.trimmed().isEmpty())
            name = sn.trimmed();
        type = DeviceType::Aircraft;
    } else {
        // Adding a top-level device
        QString typeStr = QInputDialog::getItem(this, "添加设备", "选择设备类型:",
            {"Dock (机场)", "Pilot (手飞飞机)"}, 0, false);
        if (typeStr.isEmpty()) return;

        type = typeStr.contains("Dock") ? DeviceType::Dock : DeviceType::Aircraft;

        sn = QInputDialog::getText(this, "添加设备", "设备序列号 (SN):");
        if (sn.trimmed().isEmpty()) return;

        name = QInputDialog::getText(this, "添加设备", "设备名称:",
            QLineEdit::Normal, sn.trimmed());
        if (name.trimmed().isEmpty())
            name = sn.trimmed();
    }

    // 默认订阅 OSD topic
    QString osdTopic = QString("thing/product/%1/osd").arg(sn.trimmed());
    QStringList defaultTopics;
    defaultTopics << osdTopic;

    DeviceInfo info;
    info.sn   = sn.trimmed();
    info.name = name.trimmed();
    info.type = type;
    if (addingChild)
        info.parentSn = selectedDev->sn;

    mDevMgr->addDevice(info, defaultTopics);
}
```

- [ ] **Step 2: Build to verify compilation**

```bash
cmake --build build_mingw
```

Expected: Build succeeds.

---

### Task 9: CMakeLists.txt — Add TopicListWidget to build

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add TopicListWidget.cpp to SOURCES**

In `CMakeLists.txt`, add `src/ui/TopicListWidget.cpp` to the `SOURCES` list (alphabetical order):

```cmake
set(SOURCES
    src/main.cpp
    src/core/ConfigStore.cpp
    src/core/DeviceManager.cpp
    src/core/TopicManager.cpp
    src/mqtt/MqttClientManager.cpp
    src/ui/MainWindow.cpp
    src/ui/DeviceTreeWidget.cpp
    src/ui/OsdPanel.cpp
    src/ui/RawJsonPanel.cpp
    src/ui/PublishPanel.cpp
    src/ui/ConfigDialog.cpp
    src/ui/TopicEditDialog.cpp
    src/ui/TopicListWidget.cpp
)
```

Also add `src/ui/TopicListWidget.h` to `HEADERS`:

```cmake
set(HEADERS
    src/core/DeviceInfo.h
    src/core/OsdData.h
    src/core/ConfigStore.h
    src/core/DeviceManager.h
    src/core/TopicManager.h
    src/mqtt/MqttClientManager.h
    src/ui/MainWindow.h
    src/ui/DeviceTreeWidget.h
    src/ui/OsdPanel.h
    src/ui/RawJsonPanel.h
    src/ui/PublishPanel.h
    src/ui/ConfigDialog.h
    src/ui/TopicEditDialog.h
    src/ui/TopicListWidget.h
)
```

- [ ] **Step 2: Full build**

```bash
cmake --build build_mingw
```

Expected: Build succeeds with zero errors and zero warnings.

---

### Task 10: Integration verification

- [ ] **Step 1: Verify disconnect button fix**

Run the application:
```bash
./build_mingw/DjiCloudApi.exe
```

1. Click "● 连接" to connect to a broker (or configure first)
2. Wait for connection to establish (status bar shows "🟢 已连接")
3. Click "◎ 断开"
4. **Expected:** Status shows "🔴 未连接", no auto-reconnect messages in console ("MQTT: retry in ..." should NOT appear)
5. Click "● 连接" again
6. **Expected:** Reconnects normally
7. Disconnect network cable or kill broker
8. **Expected:** Auto-reconnect messages appear ("MQTT: retry in ...")

- [ ] **Step 2: Verify topic enable/disable and persistence**

1. Select a device in the tree
2. **Expected:** Topic list shows with `●` prefix (enabled, black text)
3. Select a topic and click "◎" toggle
4. **Expected:** Topic prefix changes to `○` and text turns grey. Console shows "MQTT: unsubscribed <topic>"
5. Click "◎" again to re-enable
6. **Expected:** Topic prefix changes back to `●`. Console shows "MQTT: subscribed <topic>"
7. Close and restart the application
8. **Expected:** Topic enable/disable states are restored from config.json
9. Check `config.json` in the build directory — verify `disabled_topics` arrays are present for devices with disabled topics

- [ ] **Step 3: Verify old config compatibility**

1. Backup current `config.json`
2. Edit `config.json` to remove all `disabled_topics` fields (simulating old format)
3. Restart application
4. **Expected:** All topics show as enabled (`●`), no errors on startup
5. Restore backup `config.json`

- [ ] **Step 4: Verify topic add/remove**

1. Select a device
2. Click "＋" to add a new topic (e.g., `thing/product/test_sn/state`)
3. **Expected:** Topic appears in list with `●` prefix. Console shows MQTT subscription
4. Select the topic and click "✕" delete
5. **Expected:** Confirmation dialog appears. After confirming, topic is removed from list. Console shows MQTT unsubscription.

- [ ] **Step 5: Verify Dock add child device**

1. Select a Dock device in the tree
2. Click "＋" (add device)
3. **Expected:** Dialog asks for handheld drone SN (not device type), then name
4. Enter SN and name → confirm
5. **Expected:** New Aircraft appears as child node under the Dock in device tree. Default OSD topic auto-created.
6. Select the child Aircraft
7. **Expected:** "＋" (add device) button is disabled (greyed out)
8. Select no device (click empty area or placeholder)
9. **Expected:** "＋" (add device) button is enabled — can add top-level devices

- [ ] **Step 6: Verify toolbar layout**

1. **Expected:** Toolbar shows: `[⚙ 配置]` on the left, then spacer, then `Broker info` label, then `[● 连接] [◎ 断开]` on the right.

- [ ] **Step 7: Verify topic list empty states**

1. Select no device → topic list shows "（请选择设备）", all buttons disabled
2. Select a device with no topics → topic list shows "（无 Topic）", add button enabled, toggle/delete disabled

- [ ] **Step 8: Final build verification**

```bash
cmake --build build_mingw
```

Expected: Clean build, zero errors, zero warnings.

---

### Task 11: Commit

- [ ] **Step 1: Commit all changes**

```bash
git add src/core/TopicManager.h src/core/TopicManager.cpp
git add src/core/ConfigStore.h src/core/ConfigStore.cpp
git add src/core/DeviceManager.h src/core/DeviceManager.cpp
git add src/mqtt/MqttClientManager.h src/mqtt/MqttClientManager.cpp
git add src/ui/TopicListWidget.h src/ui/TopicListWidget.cpp
git add src/ui/MainWindow.h src/ui/MainWindow.cpp
git add CMakeLists.txt
git commit -m "feat: add topic list panel with enable/disable, fix disconnect reconnect, dock child device support

- Add TopicListWidget component with inline add/toggle/remove topic buttons
- TopicManager: mDisabledTopics collection, allEnabledTopics() for MQTT subs
- ConfigStore: persist disabled_topics in JSON, backward-compatible with old format
- DeviceManager: setTopicEnabled/isTopicEnabled, use allEnabledTopics for MQTT
- MqttClientManager: mIntentionalDisconnect flag prevents auto-reconnect on manual disconnect
- MainWindow: layout refactor (topic panel below device tree, toolbar reorder, remove edit button)
- Dock devices can add child Aircraft; Aircraft cannot add child devices

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```
