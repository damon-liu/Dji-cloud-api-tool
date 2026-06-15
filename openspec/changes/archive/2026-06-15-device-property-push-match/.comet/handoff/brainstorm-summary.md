# Brainstorm Summary

- Change: device-property-push-match
- Date: 2026-06-15

## 确认的技术方案

方案 B：重命名 + per-topic 缓存 + topic_mappings.json 扩展。

1. **重命名 OsdParsePanel → TopicParsePanel**：类名 + 文件名 + 所有引用（MainWindow.h/cpp、CMakeLists.txt）
2. **DeviceManager::mRawJsonCache 改为 per-topic**：`QMap<QString, QMap<QString, QString>>` 结构（sn → topic → json），`latestRawJson()` 增加 topic 参数（默认空保持兼容）
3. **topic_mappings.json 扩展**：新增 `thing/product/{sn}/state`（复用 osd 字段）和 `sys/product/{sn}/status`（基于 dock-status.md）
4. **无需改 JSON 解析逻辑**：status topic 同样使用 DJI 标准信封 `{"data": {...}}`

## 关键取舍与风险

| 决策 | 选择 | 风险 |
|------|------|------|
| state 映射在 JSON 中复制而非引用 | 独立复制，允许未来分化 | 两份定义需同步维护（低风险，DJI 协议稳定） |
| latestRawJson 默认空 topic 兼容旧调用 | 空时返回最新一条任意 topic 数据 | 仅 OsdPanel 仍用旧接口，影响范围可控 |
| status 字段仅一个分组 | "网关信息"单组 | dock-status.md 字段少，无需细分 |

## 测试策略

- 每个 task 编译验证：`cmake --build build_mingw`
- 手动功能测试：订阅 state/status topic → 解析面板渲染
- 回归验证：osd topic 解析不受影响

## Spec Patch

无（无新增 capability，纯扩展已有功能）
