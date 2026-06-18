---
comet_change: ui-fixes-1-8
role: technical-design
canonical_spec: openspec
archived-with: 2026-06-17-ui-fixes-1-8
status: final
---

# 界面问题修复 (PRD 1.8) — 技术设计

## 概述

实现三项界面行为优化：OsdPanel 定时刷新、断连自动暂停/恢复、Topic 一键全量切换。

## 架构修改

### 1. OsdPanel 定时刷新

**方案：事件驱动 + 定时器并行**

OsdPanel 新增以下成员：
- `DeviceManager* mDevMgr` — 定时刷新数据源
- `QTimer* mRefreshTimer` — 默认 1000ms
- `QComboBox* mIntervalCombo` — 1s/2s/5s/10s，默认 1s（index=0）
- `bool mPaused` — 暂停标记
- `QString mCurrentSn` — 当前选中设备 SN

新增方法：
- `setDeviceManager(DeviceManager*)` — MainWindow 在 connectSignals 中调用
- `setCurrentSn(const QString&)` — MainWindow 在 onDeviceSelected 中调用
- `refresh()` — 定时器槽函数，从 DeviceManager 缓存读最新 OSD 并调用已有 `showOsd()`
- `pause()` / `resume()` — 公开接口

数据流：
```
MQTT 消息 → DeviceManager 缓存 OSD → ┬→ deviceOsdUpdated 信号 → MainWindow::onOsdUpdated → showOsd()（实时）
                                     └→ OsdPanel::refresh() 定时读缓存 → showOsd()（兜底）
```

注意：`setupUi()` 需调整——返回 QComboBox 新增在标题栏，与"设备信息"/"机场数据" GroupBox 同行。

### 2. 断连自动暂停/恢复

**方案：双暂停标记 + MainWindow 信号连接**

TopicParsePanel 新增：
- `bool mAutoPaused` — 断连自动暂停标记
- `pause()` — 公开槽函数：停止 mRefreshTimer，设 mAutoPaused=true
- `resume()` — 公开槽函数：仅在 mAutoPaused=true 且 mPaused=false 时重启定时器，清 mAutoPaused

OsdPanel 新增：
- `bool mAutoPaused` — 同上
- `pause()` / `resume()` — 同上逻辑

MainWindow 信号连接：
```cpp
// brokerDisconnected lambda 中追加：
mOsdPanel->pause();
mTopicParsePanel->pause();

// brokerConnected lambda 中追加：
mOsdPanel->resume();
mTopicParsePanel->resume();
```

手动暂停优先逻辑（在 resume() 中实现）：
```
resume():
  if (mAutoPaused):
    mAutoPaused = false
    if (!mPaused):   // 手动未暂停
      mRefreshTimer->start()
```

### 3. Topic 一键全量切换

**方案：TopicListWidget 新增智能切换按钮 + TopicManager 批量接口**

TopicManager 新增：
```cpp
void setAllTopicsEnabled(const QString& deviceSn, bool enabled) {
    for (const auto& topic : mDeviceTopics.value(deviceSn))
        setTopicEnabled(deviceSn, topic, enabled);
}
```

DeviceManager 新增代理方法，调用后触发 `saveConfig()`。

TopicListWidget 新增：
- `QPushButton* mToggleAllBtn` — 标题栏位于 ▲ ▼ 之后
- 样式：与 mToggleBtn 区分（不同底色/图标 `⊘`）
- 点击逻辑：
  ```cpp
  void onToggleAll() {
      int disabledCount = mDisabledTopics.size();
      int totalCount = mAllTopics.size();
      bool enable = (disabledCount > 0);  // 有禁用则全部启用
      emit topicAllToggled(mCurrentSn, enable);
  }
  ```
- 新增信号 `topicAllToggled(QString deviceSn, bool enabled)`

MainWindow 连接：
```cpp
connect(mTopicListWidget, &TopicListWidget::topicAllToggled,
        this, [this](const QString& sn, bool enabled) {
    mDevMgr->setAllTopicsEnabled(sn, enabled);
    refreshTopicList(sn);
});
```

## 涉及文件

| 文件 | 改动类型 |
|------|----------|
| `src/ui/OsdPanel.h` | 新增成员：mDevMgr, mRefreshTimer, mIntervalCombo, mPaused, mAutoPaused, mCurrentSn；新增方法声明 |
| `src/ui/OsdPanel.cpp` | 新增 setupUi 中的定时器+ComboBox 初始化；实现 refresh/pause/resume |
| `src/ui/TopicParsePanel.h` | 新增 mAutoPaused；pause/resume 改为公开 |
| `src/ui/TopicParsePanel.cpp` | pause/resume 实现调整 |
| `src/ui/MainWindow.cpp` | 断连/重连信号中调用面板 pause/resume；连接 topicAllToggled；传递 DeviceManager 给 OsdPanel |
| `src/ui/TopicListWidget.h` | 新增 mToggleAllBtn、topicAllToggled 信号 |
| `src/ui/TopicListWidget.cpp` | 按钮创建、样式、onToggleAll 逻辑 |
| `src/core/TopicManager.h` | 新增 setAllTopicsEnabled 声明 |
| `src/core/TopicManager.cpp` | 新增 setAllTopicsEnabled 实现 |
| `src/core/DeviceManager.h` | 新增 setAllTopicsEnabled 声明 |
| `src/core/DeviceManager.cpp` | 新增 setAllTopicsEnabled 实现 |

## 边界条件

1. 无设备选中时 OsdPanel 定时器继续运行但 refresh() 直接返回（mCurrentSn 为空）
2. 设备切换时 OsdPanel::setCurrentSn() 更新 mCurrentSn
3. TopicParsePanel 的 topic 为空时 refresh() 直接返回（已有检查）
4. 批量切换时若设备无 topic → 按钮不执行任何操作
5. 重连后若 mPaused=true — resume() 不启动定时器，由用户手动继续
