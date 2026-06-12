## Why

v1.0 topic 列表面板上线后发现 5 个界面体验问题：设备树与 topic 面板未对齐、设备选中后无法取消导致无法添加顶级设备、topic 列表在 OSD 数据更新时频繁跳回第一项、原始 JSON 面板只显示最新一条数据、状态栏缺少版本信息。这些问题影响日常使用流畅度，需要在 v1.1 之前修复。

## What Changes

- **设备树与 Topic 面板宽度对齐**：统一 `DeviceTree` 与 `TopicListWidget` 内部列表的宽度约束，确保左右边缘对齐
- **设备选中可取消**：点击设备树空白区域取消选中，恢复「添加顶级设备」能力；选中 Dock 时 `＋` 按钮不再强制添加子设备
- **Topic 列表保持用户选中项**：`refreshList()` 重建列表时保存并恢复用户手动选中的 topic，不再在数据更新时强制跳回首项
- **原始 JSON 面板累积显示**：新增历史数据缓冲区，每次 MQTT 消息追加到面板底部而非替换，最新数据在末尾
- **状态栏添加版本号**：状态栏中间显示 `v1.0 | github.com/damon-liu/Dji-cloud-api-tool`

## Capabilities

### New Capabilities
- `json-history`: 原始 JSON 面板累积显示所有历史上报数据，最新数据在末尾，支持清空和容量限制

### Modified Capabilities
- `topic-list-panel`: 设备树与 topic 面板宽度对齐；topic 列表刷新时保持用户选中项
- `dock-add-handheld`: 设备选中可取消，允许在选中 Dock 后仍可添加顶级设备

## Impact

- **UI 层**: `MainWindow.cpp`（布局对齐、onAddDevice 逻辑、状态栏）、`DeviceTreeWidget.cpp`（点击空白取消选中）、`TopicListWidget.cpp`（refreshList 保持选中）、`RawJsonPanel.h`（追加模式+历史缓冲）
- **数据层**: `DeviceManager.cpp`（JSON 历史数据缓存）
- **无破坏性变更**: 不影响 MQTT 连接、topic 订阅、OSD 解析
