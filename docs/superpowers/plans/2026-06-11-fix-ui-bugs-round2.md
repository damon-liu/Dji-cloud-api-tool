# fix-ui-bugs-round2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 6 round-2 UI bugs: connection test message, button size consistency, JSON topic filter, pause button, OSD simplification, version centering.

**Architecture:** `DeviceManager` JSON history restructured to `SN→topic→history[]` for topic-level filtering. `RawJsonPanel` gains pause/ resume with internal buffer. `OsdPanel` strips position info and dock data group boxes. Other changes are single-line fixes.

**Tech Stack:** Qt 6 (Core, Widgets, Mqtt), C++17, CMake 3.10+

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `src/ui/ConfigDialog.cpp` | Modify | Unified error messages (lines 105, 110) |
| `src/ui/MainWindow.cpp` | Modify | Button sizes, version centering, topic-filter wiring |
| `src/ui/RawJsonPanel.h` | Modify | Pause button, pause buffer, toggle logic |
| `src/ui/OsdPanel.h` | Modify | Remove position/dock member declarations |
| `src/ui/OsdPanel.cpp` | Modify | Remove position/dock group boxes and fill logic |
| `src/core/DeviceManager.h` | Modify | `mJsonHistory` type change, `jsonHistory(sn, topic)` overload |
| `src/core/DeviceManager.cpp` | Modify | Topic-level storage in `parseAndRoute()`, new `jsonHistory()` logic |

---

### Task 1: Connection test error message

**Files:**
- Modify: `src/ui/ConfigDialog.cpp:91-111`

- [ ] **Step 1: Replace error callback message**

In `ConfigDialog::startTest()`, change line 105:

```cpp
// OLD
QMessageBox::warning(this, "连接测试", "✗ 连接失败: " + errMsg);
// NEW
QMessageBox::warning(this, "连接测试", "连接失败请检查配置参数是否有误");
```

- [ ] **Step 2: Replace timeout callback message**

Change line 110:

```cpp
// OLD
QMessageBox::warning(this, "连接测试", "✗ 连接超时（5 秒无响应）");
// NEW
QMessageBox::warning(this, "连接测试", "连接失败请检查配置参数是否有误");
```

- [ ] **Step 3: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings.

---

### Task 2: Button size consistency

**Files:**
- Modify: `src/ui/MainWindow.cpp:242-243,252-253`

- [ ] **Step 1: Change mAddDeviceBtn size to 28×28**

Find `mAddDeviceBtn->setFixedSize(32, 32);` and change to:

```cpp
mAddDeviceBtn->setFixedSize(28, 28);
```

- [ ] **Step 2: Change mDeleteDeviceBtn size to 28×28**

Find `mDeleteDeviceBtn->setFixedSize(32, 32);` and change to:

```cpp
mDeleteDeviceBtn->setFixedSize(28, 28);
```

- [ ] **Step 3: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings.

---

### Task 3: JSON history by topic (DeviceManager)

**Files:**
- Modify: `src/core/DeviceManager.h:80-84`
- Modify: `src/core/DeviceManager.cpp:246-250,168-176`

- [ ] **Step 1: Change mJsonHistory type in DeviceManager.h**

```cpp
// OLD
QMap<QString, QStringList> mJsonHistory;
// NEW
QMap<QString, QMap<QString, QStringList>> mJsonHistory;  // SN → topic → history[]
```

- [ ] **Step 2: Change jsonHistory() signature and clearJsonHistory() in DeviceManager.h**

```cpp
// OLD
QString jsonHistory(const QString& sn) const;
void clearJsonHistory(const QString& sn);
// NEW
QString jsonHistory(const QString& sn, const QString& topic = {}) const;
void clearJsonHistory(const QString& sn, const QString& topic = {});
```

- [ ] **Step 3: Update parseAndRoute() to store by topic**

In `DeviceManager.cpp`, change the history accumulation lines (currently ~246-250):

```cpp
// OLD
mJsonHistory[sn].append(formatted);
while (mJsonHistory[sn].size() > MAX_JSON_HISTORY)
    mJsonHistory[sn].removeFirst();

// NEW
mJsonHistory[sn][topic].append(formatted);
while (mJsonHistory[sn][topic].size() > MAX_JSON_HISTORY)
    mJsonHistory[sn][topic].removeFirst();
```

- [ ] **Step 4: Implement new jsonHistory()**

Replace the existing `jsonHistory()` and `clearJsonHistory()`:

```cpp
QString DeviceManager::jsonHistory(const QString& sn, const QString& topic) const {
    if (!mJsonHistory.contains(sn))
        return {};
    if (topic.isEmpty()) {
        // 空 topic → 返回所有 topic 的合并历史
        QStringList all;
        const auto& topicMap = mJsonHistory[sn];
        for (auto it = topicMap.begin(); it != topicMap.end(); ++it)
            all.append(it.value());
        return all.join("\n---\n");
    }
    return mJsonHistory[sn].value(topic).join("\n---\n");
}

void DeviceManager::clearJsonHistory(const QString& sn, const QString& topic) {
    if (topic.isEmpty())
        mJsonHistory.remove(sn);
    else if (mJsonHistory.contains(sn))
        mJsonHistory[sn].remove(topic);
}
```

- [ ] **Step 5: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings (unused param warnings OK temporarily).

---

### Task 4: RawJsonPanel — pause button

**Files:**
- Modify: `src/ui/RawJsonPanel.h`

- [ ] **Step 1: Add pause members to RawJsonPanel**

Add to private section (before `QPlainTextEdit* mEditor;`):

```cpp
private:
    QPlainTextEdit* mEditor;
    QPushButton*    mPauseBtn       = nullptr;
    bool            mPaused         = false;
    QStringList     mPendingBuffer;
    static constexpr int MAX_BUFFER = 1000;
```

- [ ] **Step 2: Add pause button to header row**

Replace the header section (lines 26-39) in the constructor:

```cpp
        // 标题栏
        auto* header = new QHBoxLayout;
        auto* title  = new QLabel("原始 JSON");
        title->setObjectName("sectionTitle");
        title->setStyleSheet("font-size: 13px; font-weight: bold; color: #5f6368;");

        mPauseBtn = new QPushButton("⏸ 暂停");
        mPauseBtn->setObjectName("copyBtn");
        mPauseBtn->setCursor(Qt::PointingHandCursor);
        mPauseBtn->setFixedWidth(80);

        auto* copyBtn = new QPushButton("📋 复制");
        copyBtn->setObjectName("copyBtn");
        copyBtn->setCursor(Qt::PointingHandCursor);
        copyBtn->setFixedWidth(80);

        header->addWidget(title);
        header->addStretch();
        header->addWidget(mPauseBtn);
        header->addWidget(copyBtn);
        layout->addLayout(header);
```

- [ ] **Step 3: Replace appendJson() with pause-aware version**

Replace the existing `appendJson()` method:

```cpp
    void appendJson(const QString& json) {
        if (json.isEmpty()) return;
        if (mPaused) {
            // 暂停中：写入缓冲
            mPendingBuffer.append(json);
            while (mPendingBuffer.size() > MAX_BUFFER)
                mPendingBuffer.removeFirst();
            return;
        }
        if (!mEditor->toPlainText().isEmpty())
            mEditor->appendPlainText("---");
        mEditor->appendPlainText(json);
        QTextCursor cursor = mEditor->textCursor();
        cursor.movePosition(QTextCursor::End);
        mEditor->setTextCursor(cursor);
    }
```

- [ ] **Step 4: Add pause button click handler**

Add after the copy button connect in the constructor:

```cpp
        connect(mPauseBtn, &QPushButton::clicked, this, [this]() {
            mPaused = !mPaused;
            mPauseBtn->setText(mPaused ? "▶ 继续" : "⏸ 暂停");
            if (!mPaused) {
                // 恢复：一次性将缓冲写入 editor
                for (const auto& j : mPendingBuffer) {
                    if (!mEditor->toPlainText().isEmpty())
                        mEditor->appendPlainText("---");
                    mEditor->appendPlainText(j);
                }
                mPendingBuffer.clear();
                QTextCursor cursor = mEditor->textCursor();
                cursor.movePosition(QTextCursor::End);
                mEditor->setTextCursor(cursor);
            }
        });
```

- [ ] **Step 5: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings.

---

### Task 5: OsdPanel — remove position info and dock data

**Files:**
- Modify: `src/ui/OsdPanel.h`
- Modify: `src/ui/OsdPanel.cpp`

- [ ] **Step 1: Remove position and dock members from OsdPanel.h**

Remove these lines from the header:

```cpp
// REMOVE these 3 lines:
    QLabel* mLongitude;
    QLabel* mLatitude;
    QLabel* mAltitude;

// REMOVE the entire "机场专属" block (lines 59-68):
    QGroupBox* mDockGroup;
    QLabel* mCoverState;
    QLabel* mDroneInDock;
    QLabel* mWorkingVoltage;
    QLabel* mWorkingCurrent;
    QLabel* mBackupBattery;
    QLabel* mWindSpeed;
    QLabel* mEnvTemp;
    QLabel* mEnvHumidity;
```

- [ ] **Step 2: Remove position group box from setupUi()**

In `OsdPanel.cpp`, delete lines 33-43 (the entire "位置信息" group box creation block):

```cpp
// REMOVE this entire block:
    // ——— 位置信息（公共） ———
    auto* posBox = new QGroupBox("位置信息", this);
    auto* posLayout = new QFormLayout(posBox);
    posLayout->setSpacing(4);
    mLongitude = new QLabel("-", this);
    mLatitude  = new QLabel("-", this);
    mAltitude  = new QLabel("-", this);
    posLayout->addRow("经度:", mLongitude);
    posLayout->addRow("纬度:", mLatitude);
    posLayout->addRow("高度:", mAltitude);
    mMainLayout->addWidget(posBox);
```

- [ ] **Step 3: Remove dock data group box from setupUi()**

Delete lines 73-93 (the entire "机场数据" group box creation block):

```cpp
// REMOVE this entire block:
    // ——— 机场数据 ———
    mDockGroup = new QGroupBox("机场数据", this);
    auto* dockLayout = new QFormLayout(mDockGroup);
    dockLayout->setSpacing(4);
    mCoverState      = new QLabel("-", this);
    mDroneInDock     = new QLabel("-", this);
    mWorkingVoltage  = new QLabel("-", this);
    mWorkingCurrent  = new QLabel("-", this);
    mBackupBattery   = new QLabel("-", this);
    mWindSpeed       = new QLabel("-", this);
    mEnvTemp         = new QLabel("-", this);
    mEnvHumidity     = new QLabel("-", this);
    dockLayout->addRow("舱盖:", mCoverState);
    dockLayout->addRow("飞机在库:", mDroneInDock);
    dockLayout->addRow("工作电压:", mWorkingVoltage);
    dockLayout->addRow("工作电流:", mWorkingCurrent);
    dockLayout->addRow("备用电池:", mBackupBattery);
    dockLayout->addRow("风速:", mWindSpeed);
    dockLayout->addRow("温度:", mEnvTemp);
    dockLayout->addRow("湿度:", mEnvHumidity);
    mMainLayout->addWidget(mDockGroup);
```

- [ ] **Step 4: Remove position fields from showAircraftOsd()**

Delete lines 158-160 from `showAircraftOsd()`:

```cpp
// REMOVE these 3 lines:
    setFieldValue(mLongitude, QString::number(osd.longitude, 'f', 6), false);
    setFieldValue(mLatitude,  QString::number(osd.latitude, 'f', 6), false);
    setFieldValue(mAltitude,  QString::number(osd.altitude, 'f', 1) + " m", false);
```

- [ ] **Step 5: Replace showDockOsd() with empty stub**

Since the dock data group box is removed, `showDockOsd()` has nothing to display. Replace:

```cpp
void OsdPanel::showDockOsd(const DockOsd& osd) {
    // 机场数据显示已移除 (v1.0 精简)
    Q_UNUSED(osd)
}
```

- [ ] **Step 6: Update showOsd() to remove dock show/hide logic**

In `showOsd()`, change lines 115-127:

```cpp
// OLD:
    if (device->type == DeviceType::Aircraft && aircraftOsd && aircraftOsd->valid) {
        mAircraftGroup->show();
        mDockGroup->hide();
        showAircraftOsd(*aircraftOsd);
    } else if (device->type == DeviceType::Dock && dockOsd && dockOsd->valid) {
        mAircraftGroup->hide();
        mDockGroup->show();
        showDockOsd(*dockOsd);
    } else {
        mAircraftGroup->hide();
        mDockGroup->hide();
    }

// NEW:
    if (device->type == DeviceType::Aircraft && aircraftOsd && aircraftOsd->valid) {
        mAircraftGroup->show();
        showAircraftOsd(*aircraftOsd);
    } else if (device->type == DeviceType::Dock && dockOsd && dockOsd->valid) {
        mAircraftGroup->hide();
        showDockOsd(*dockOsd);
    } else {
        mAircraftGroup->hide();
    }
```

- [ ] **Step 7: Update clear() to remove dock hide**

In `clear()`, remove the `mDockGroup->hide();` line (keep only `mAircraftGroup->hide();`).

- [ ] **Step 8: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings.

---

### Task 6: Version label true centering

**Files:**
- Modify: `src/ui/MainWindow.cpp:326-335`

- [ ] **Step 1: Rewrite setupStatusBar() with centered version**

Replace `setupStatusBar()`:

```cpp
void MainWindow::setupStatusBar() {
    mStatusLabel      = new QLabel("🔴 未连接");
    mDeviceCountLabel = new QLabel("设备: 0");

    mStatusLabel->setStyleSheet("font-weight: bold; padding: 0 8px;");
    mDeviceCountLabel->setStyleSheet("padding: 0 8px;");

    // 版本信息 — 真正居中
    auto* versionContainer = new QWidget(this);
    auto* versionLayout = new QHBoxLayout(versionContainer);
    versionLayout->setContentsMargins(0, 0, 0, 0);
    versionLayout->setAlignment(Qt::AlignCenter);
    mVersionLabel = new QLabel("v1.0 · github.com/damon-liu/Dji-cloud-api-tool");
    mVersionLabel->setStyleSheet(
        "color: #80868b; font-size: 11px; letter-spacing: 0.5px;");
    versionLayout->addWidget(mVersionLabel);

    statusBar()->addWidget(mStatusLabel);
    statusBar()->addWidget(versionContainer, 1);
    statusBar()->addPermanentWidget(mDeviceCountLabel);
}
```

- [ ] **Step 2: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings.

---

### Task 7: Topic filter wiring (MainWindow)

**Files:**
- Modify: `src/ui/MainWindow.cpp` — onDeviceSelected, onOsdUpdated, connect topic signals

- [ ] **Step 1a: Add topicSelectionChanged signal to TopicListWidget.h**

Add to the signals section:

```cpp
signals:
    void topicAdded(const QString& deviceSn, const QString& topic);
    void topicToggled(const QString& deviceSn, const QString& topic);
    void topicRemoved(const QString& deviceSn, const QString& topic);
    void topicSelectionChanged(const QString& topic);  // ← ADD THIS
```

- [ ] **Step 1b: Emit signal in TopicListWidget.cpp onTopicSelectionChanged()**

Replace `onTopicSelectionChanged()`:

```cpp
void TopicListWidget::onTopicSelectionChanged() {
    bool hasSelection = !selectedTopic().isEmpty();
    mToggleBtn->setEnabled(hasSelection);
    mRemoveBtn->setEnabled(hasSelection);
    emit topicSelectionChanged(hasSelection ? selectedTopic() : QString());
}
```

- [ ] **Step 1c: Wire signal in MainWindow.cpp connectSignals()**

After the existing TopicListWidget signal connections, add:

```cpp
    // Topic 选中变化 → 原始 JSON 按 topic 过滤
    connect(mTopicListWidget, &TopicListWidget::topicSelectionChanged,
            this, [this](const QString& selectedTopic) {
        QString sn = mDeviceTree->selectedDeviceSn();
        if (!sn.isEmpty())
            mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn, selectedTopic));
    });
```

- [ ] **Step 2: Update onDeviceSelected() JSON panel call**

Change the call to use `jsonHistory(sn)` (empty topic = all):

```cpp
// Already correct since jsonHistory(sn) with default empty topic returns all
mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn));
```

This line is already in place from Task 4 of fix-ui-bugs. No change needed.

- [ ] **Step 3: Update onOsdUpdated() to use topic-aware append**

In `onOsdUpdated()`, the current append logic stays the same — it appends the latest JSON. The topic filter is only applied when the user selects a topic (not on every update). No change needed here.

- [ ] **Step 4: Build**

```bash
cmake --build build_mingw
```

Expected: 0 errors, 0 warnings.

---

### Task 8: Integration verification

- [ ] **Step 1: Full clean build**

```bash
cmake --build build_mingw --clean-first
```

Expected: 0 errors, 0 warnings.

- [ ] **Step 2: Config round-trip test**

```bash
cat > build_mingw/config.json << 'EOF'
{"mqtt":{"host":"192.168.1.100","port":8883,"username":"admin","password":""},"devices":[]}
EOF
cd build_mingw && timeout 3 ./DjiCloudApi.exe 2>&1
echo "exit=$?"
```

Expected: App launches without crash, config preserved.

- [ ] **Step 3: Manual verification checklist**

| # | Bug | Verify |
|---|-----|--------|
| 1 | 连接测试提示语 | 配置错误参数点击 Test → 提示「连接失败请检查配置参数是否有误」 |
| 2 | 按钮样式 | 设备列表 ＋/✕ 与 topic 面板 ＋/✕ 尺寸一致 (28×28) |
| 3 | JSON 过滤 | 选中不同 topic → 原始 JSON 面板内容随之切换 |
| 4 | 暂停按钮 | 点击 ⏸ → 显示冻结 → 后台数据持续接收 → 点击 ▶ → 恢复显示 |
| 5 | OSD 精简 | Dock 设备选中时仅显示设备信息+飞行数据，无位置信息和机场数据 |
| 6 | 版本居中 | 状态栏中间显示 `v1.0 · github.com/damon-liu/Dji-cloud-api-tool` |

---

### Task 9: Commit

- [ ] **Step 1: Commit all changes**

```bash
git add src/ui/ConfigDialog.cpp
git add src/ui/MainWindow.cpp src/ui/MainWindow.h
git add src/ui/RawJsonPanel.h
git add src/ui/OsdPanel.h src/ui/OsdPanel.cpp
git add src/ui/TopicListWidget.h src/ui/TopicListWidget.cpp
git add src/core/DeviceManager.h src/core/DeviceManager.cpp
git commit -m "fix: round 2 UI bugs — error message, button size, topic filter, pause, OSD simplify, version centering

- ConfigDialog: unified error messages to user-friendly Chinese
- MainWindow: device buttons 28x28, version label centered with layout
- RawJsonPanel: pause/resume button with internal buffer
- OsdPanel: removed position info and dock data group boxes
- DeviceManager: JSON history restructured to SN->topic->history for topic filtering

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```
