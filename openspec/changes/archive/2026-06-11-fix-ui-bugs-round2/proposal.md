## Why

v1.0 第二轮界面体验问题：连接测试错误提示技术化不友好、设备与 topic 按钮尺寸不统一、原始 JSON 面板无法按 topic 过滤且缺少暂停控制、OSD 面板包含不需要的位置信息和机场数据、版本标签未真正居中。这些细节影响产品完成度，需在正式发布前修复。

## What Changes

- **连接测试提示语统一**：所有连接失败（含超时）统一显示「连接失败请检查配置参数是否有误」
- **按钮样式统一**：设备列表与 Topic 面板的 ＋/✕ 按钮统一为 28×28px，消除视觉差异
- **原始 JSON 按选中 Topic 过滤**：用户选中 topic 列表中的某个 topic 时，原始 JSON 面板仅显示该 topic 的订阅数据；未选中 topic 时显示全部
- **原始 JSON 面板暂停按钮**：在复制按钮旁添加暂停/继续按钮，暂停时冻结显示但后台持续订阅
- **删除位置信息和机场数据**：OSD 面板移除「位置信息」group（经度/纬度/高度）和「机场数据」group（舱盖/电压/风速等）
- **版本信息居中**：状态栏版本标签改为真正水平居中，采用简洁美化样式

## Capabilities

### New Capabilities
- `json-topic-filter`: 原始 JSON 面板根据用户选中的 topic 过滤显示，未选中 topic 时显示全部
- `json-pause-control`: 原始 JSON 面板暂停/继续控制，暂停时冻结显示但保持后台订阅

### Modified Capabilities
- `json-history`: 新增暂停控制按钮和 topic 过滤行为
- `topic-list-panel`: 按钮样式与设备列表按钮统一

## Impact

- **UI 层**: `ConfigDialog.cpp`（提示语）、`MainWindow.cpp`（按钮尺寸+版本居中）、`TopicListWidget.cpp`（按钮尺寸）、`RawJsonPanel.h`（暂停按钮+topic过滤接口）、`OsdPanel.h/cpp`（删除位置+机场数据）
- **数据层**: `DeviceManager.h/cpp`（JSON 历史按 topic 存储而非按设备 SN）
- **无破坏性变更**: 不影响 MQTT 连接、topic 订阅/禁用逻辑、OSD 解析
