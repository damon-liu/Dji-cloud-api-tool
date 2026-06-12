## 1. 设备列表与 Topic 面板宽度对齐

- [ ] 1.1 `TopicListWidget` 构造函数中给 `mTopicList` (QListWidget) 添加与 `DeviceTree` 相同的宽度约束：`setMinimumWidth(170)` + `setMaximumWidth(220)`

## 2. 设备选中取消 + 顶级设备添加

- [ ] 2.1 `DeviceTreeWidget` 新增 `mousePressEvent` 重写：计算点击位置对应的 item，若为 `nullptr` 则 `clearSelection()` 并 `emit deviceSelected("")`
- [ ] 2.2 `DeviceTreeWidget.h` 声明 `mousePressEvent` 重写
- [ ] 2.3 `MainWindow::onDeviceSelected()` 处理空 SN：清空 OSD 面板、原始 JSON 面板、Topic 列表，禁用 `mDeleteDeviceBtn`，`mAddDeviceBtn` 恢复为可用状态（允许添加顶级设备）

## 3. Topic 列表刷新时保持用户选中项

- [ ] 3.1 `TopicListWidget::refreshList()`：在 `clear()` 前保存 `selectedTopic()`，重建列表后按 `Qt::UserRole` 匹配恢复选中；仅在未保存选中项时才 `setCurrentRow(0)`

## 4. 原始 JSON 面板累积显示历史数据

- [ ] 4.1 `DeviceManager.h` 新增 `mJsonHistory`（`QMap<QString, QStringList>`，每设备最多 500 条）和 `MAX_JSON_HISTORY = 500`
- [ ] 4.2 `DeviceManager::parseAndRoute()` 中追加 JSON 到 `mJsonHistory[sn]` 而非覆盖，超出 500 条时 `removeFirst()`
- [ ] 4.3 `DeviceManager` 新增 `jsonHistory(sn)` 方法，返回累积的 JSON 字符串（用分隔线连接）
- [ ] 4.4 `RawJsonPanel` 新增 `appendJson(json)` 方法：`mEditor->appendPlainText(json)` 后自动滚到底部；新增 `clearHistory()` 方法
- [ ] 4.5 `MainWindow::onDeviceSelected()` 中调用 `mRawJsonPanel->setJson(mDevMgr->jsonHistory(sn))` 加载完整历史
- [ ] 4.6 `MainWindow::onOsdUpdated()` 中改为调用 `mRawJsonPanel->appendJson(newJson)` 增量追加

## 5. 状态栏版本信息

- [ ] 5.1 `MainWindow.h` 新增 `QLabel* mVersionLabel` 成员
- [ ] 5.2 `MainWindow::setupStatusBar()` 添加 `mVersionLabel`，显示 `v1.0 | github.com/damon-liu/Dji-cloud-api-tool`，使用 `addWidget(versionLabel, 1)` 使其居中

## 6. 集成验证

- [ ] 6.1 全量构建验证：`cmake --build build_mingw` 零错误零警告
- [ ] 6.2 视觉验证：设备树与 Topic 列表左右边缘对齐
- [ ] 6.3 交互验证：点击设备树空白区域可取消选中，之后可添加顶级设备
- [ ] 6.4 交互验证：OSD 数据更新时 topic 列表保持用户选中项不变
- [ ] 6.5 交互验证：原始 JSON 面板累积显示历史消息，最新在底部，切换设备显示对应历史
- [ ] 6.6 视觉验证：状态栏中间显示版本号和 GitHub 地址
