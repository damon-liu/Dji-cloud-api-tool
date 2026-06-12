## Context

v1.0 topic 列表面板（`add-topic-panel`）上线后，日常使用中暴露出 5 个界面体验问题。这些问题不是功能缺失，而是已实现功能的交互细节缺陷。修复范围仅限于 UI 层和少量数据层调整。

当前状态：
- 左侧面板：设备树（`DeviceTreeWidget`）+ 按钮列 → Topic 面板（`TopicListWidget`）
- 右侧面板：OSD 详情 + 原始 JSON（`RawJsonPanel`）+ 折叠的 Publish 面板
- 状态栏：`mStatusLabel`（左）+ `mDeviceCountLabel`（右）

## Goals / Non-Goals

**Goals:**
- 设备树 QTreeWidget 与 Topic 面板 QListWidget 左右边缘对齐
- 点击设备树空白区域取消选中，恢复「添加顶级设备」入口
- Topic 列表在数据刷新时保持用户手动选中的项
- 原始 JSON 面板累积显示所有历史消息
- 状态栏中间显示版本号和 GitHub 地址

**Non-Goals:**
- 不修改 MQTT 连接/订阅逻辑
- 不修改 OSD 解析
- 不修改 Topic 启用/禁用状态机
- 不引入新的外部依赖

## Decisions

### D1: 设备列表宽度对齐方案

**选择：** 给 `TopicListWidget` 的 `QListWidget` 设置与 `DeviceTree` 相同的宽度约束：`setMinimumWidth(170)` + `setMaximumWidth(220)`。

**替代方案：**
- A. 移除 DeviceTree 的宽度限制让两者自然对齐 → DeviceTree 会占用太多空间
- B. 用 QSplitter 包裹 → 过度设计，两个静态列表不需要拖拽分割

### D2: 设备取消选中方案

**选择：** 在 `DeviceTreeWidget::mousePressEvent` 中检测点击位置，若点击在空白区域（无 item）则调用 `clearSelection()` 并 emit `deviceSelected("")`。MainWindow 收到空 SN 时清空详情面板和 topic 列表，`mAddDeviceBtn` 恢复启用（走顶级设备添加流程）。

同时修改 `onAddDevice()`：选中 Dock 时 `＋` 按钮仍然可直接添加顶级设备（因为现在用户可以随时取消选中恢复该能力）。或者更好的方案：选中 Dock 时 `＋` 弹出选择菜单——"添加子设备 / 添加顶级设备"。

**最终选择：** 取消选中方案（点击空白清除选择），保持 `onAddDevice()` 现有逻辑不变（选中 Dock = 添加子设备，未选中 = 添加顶级设备）。用户想加顶级设备时只需点击空白区域取消选中即可。

### D3: Topic 列表保持选中方案

**选择：** 在 `refreshList()` 重建列表前用 `selectedTopic()` 保存当前选中 topic 字符串，重建后遍历列表按 `Qt::UserRole` 匹配恢复选中。只有在设备 SN 改变（切换设备）时才默认选第一个。

```cpp
QString saved = selectedTopic();  // save before clear
// ... rebuild list ...
// restore
if (!saved.isEmpty()) {
    for (int i = 0; i < mTopicList->count(); ++i) {
        if (mTopicList->item(i)->data(Qt::UserRole).toString() == saved) {
            mTopicList->setCurrentRow(i);
            return;
        }
    }
}
if (mTopicList->count() > 0)
    mTopicList->setCurrentRow(0);  // fallback
```

### D4: 原始 JSON 累积显示方案

**选择：** `DeviceManager` 新增 `mJsonHistory` 缓存（`QMap<QString, QStringList>`，每设备最多 500 条），`parseAndRoute()` 中追加而非覆盖。`RawJsonPanel` 新增 `appendJson(json)` 方法，用 `appendPlainText()` 追加并自动滚动到底部。

**替代方案：**
- A. 只在 UI 层累积（RawJsonPanel 内部维护列表）→ 简单但切换设备时历史丢失
- B. 不设上限 → 长时间运行内存泄漏

**容量限制：** 每设备 500 条，超出时从头部删除旧记录。

### D5: 状态栏版本信息方案

**选择：** 在 `setupStatusBar()` 中添加 `mVersionLabel`，使用 `addWidget(versionLabel, 1)` 的拉伸因子实现居中效果。内容硬编码为 `v1.0 | github.com/damon-liu/Dji-cloud-api-tool`。

## Risks / Trade-offs

- **JSON 历史内存膨胀** → 限制每设备 500 条，约 2-5MB 内存。提供清空按钮（可选）。
- **设备取消选中改变交互习惯** → 用户需要学习点击空白区域取消选中。如果反馈不好，后续可改为右键菜单或 Escape 键取消。
- **Topic 选中恢复依赖字符串匹配** → 如果两个 topic 字符串相同（不应发生，add 时有去重检查），恢复可能不准确。
