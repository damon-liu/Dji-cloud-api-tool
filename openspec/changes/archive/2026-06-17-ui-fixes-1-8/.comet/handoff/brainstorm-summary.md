# Brainstorm Summary

- Change: ui-fixes-1-8
- Date: 2026-06-17

## 确认的技术方案

### 1. OsdPanel 定时刷新
- 传递 DeviceManager 指针（方案A），与 TopicParsePanel 模式一致
- QTimer 默认 1000ms，ComboBox 提供 1s/2s/5s/10s 选项（默认 1s）
- 定时器回调从 DeviceManager 缓存读 OSD → 调用已有 showOsd()
- 事件驱动路径（ManiWindow::onOsdUpdated → showOsd）保持不变，两者并行

### 2. 断连自动暂停/恢复
- 手动暂停优先（方案A）：mAutoPaused（断连暂停）+ mPaused（手动暂停）双标记
- resume() 仅在 mAutoPaused=true 且 mPaused=false 时重启定时器
- TopicParsePanel 已有 mPaused 保持不变，新增 mAutoPaused

### 3. 全量 Topic 切换
- 智能切换（方案A）：一个按钮，根据状态自动判断执行全启用还是全禁用
- TopicManager::setAllTopicsEnabled() 遍历调用已有 setTopicEnabled()
- DeviceManager 代理并触发 saveConfig

## 关键取舍与风险

| 决策 | 取舍 | 风险 |
|------|------|------|
| OsdPanel 持有 DeviceManager 指针 | 增加耦合，但遵循现有模式 | 低 |
| 定时器+事件驱动并行 | 同数据可能短时刷新两次（无副作用） | 极低 |
| mAutoPaused 独立于 mPaused | 两层状态判断，增加复杂度 | 中—需仔细测试边界场景 |
| 批量切换逐个调用 setTopicEnabled | N 次 topicsChanged 信号 → N 次重订阅 | 中—可后续优化为批量信号 |

## 测试策略

手动验证 5 个场景：间隔切换、断连冻结/恢复、手动暂停优先、全量禁用/启用、状态持久化

## Spec Patch

无（本次 change 不涉及 delta spec）
