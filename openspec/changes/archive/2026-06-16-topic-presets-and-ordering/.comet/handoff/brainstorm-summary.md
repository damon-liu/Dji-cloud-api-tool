# Brainstorm Summary

- Change: topic-presets-and-ordering
- Date: 2026-06-15

## 确认的技术方案

1. **TopicManager 容器改用 `QMap<QString, QStringList>`**（方案 A）：从 `QSet` 切换为 `QStringList`，List 天然保序，最小改动
2. **排序不触发 MQTT 重订阅**：`reorderTopics()` 仅更新内存 + 持久化，不发射 `topicsChanged`
3. **PublishPanel ComboBox 合并显示**：订阅 topic 列表 + `---` 分隔线 + 5 个下发预设

## 关键取舍与风险

- `QSet` → `QStringList` 涉及 TopicManager 所有 `insert`/`contains`/`remove` 调用需适配
- PublishPanel 5 个 topic 不加入订阅，需手动订阅后才能发布

## 测试策略

- 创建机场验证 7 个默认 topic（禁用状态）
- 创建无人机验证仅 osd 不变
- 排序验证持久化重启保持
- PublishPanel ComboBox 合并验证
- 编译验证

## Spec Patch

无
