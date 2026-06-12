## 1. 连接测试提示语统��

- [ ] 1.1 `ConfigDialog.cpp` 错误回调中 `QMessageBox::warning` 消息改为 `"连接失败请检查配置参数是否有误"`
- [ ] 1.2 `ConfigDialog.cpp` 超时回调中 `QMessageBox::warning` 消息改为 `"连接失败请检查配置参数是否有误"`

## 2. 按钮样式统一

- [ ] 2.1 `MainWindow.cpp` 中 `mAddDeviceBtn` 和 `mDeleteDeviceBtn` 尺寸从 32×32 改为 28×28

## 3. 原始 JSON 按 Topic 过滤

- [ ] 3.1 `DeviceManager.h` 中 `mJsonHistory` 类型从 `QMap<QString, QStringList>` 改为 `QMap<QString, QMap<QString, QStringList>>`（SN → topic → list）
- [ ] 3.2 `DeviceManager::parseAndRoute()` 中按 topic 存储 JSON：`mJsonHistory[sn][topic].append(formatted)`，容量限制 500 条/设备/topic
- [ ] 3.3 `DeviceManager::jsonHistory()` 改为 `jsonHistory(sn, topic)` 重载，支持传入 topic 过滤
- [ ] 3.4 `MainWindow::onTopicSelectionChanged()` 新增：当 topic 列表选中变化时，刷新原始 JSON 面板为选中 topic 的数据
- [ ] 3.5 `MainWindow::onDeviceSelected()` 中原始 JSON 面板传入空 topic（显示全部），等待用户选择 topic

## 4. 原始 JSON 暂停按钮

- [ ] 4.1 `RawJsonPanel.h` 新增 `QPushButton* mPauseBtn`、`bool mPaused`、`QStringList mPendingBuffer`、`static constexpr int MAX_BUFFER = 1000`
- [ ] 4.2 `RawJsonPanel` 构造函数中添加暂停按钮到 header 行（📋 复制按钮左侧）
- [ ] 4.3 `appendJson()` 中检查 `mPaused`：若暂停则写入 `mPendingBuffer`，否则写入 editor
- [ ] 4.4 暂停/继续按钮 clicked 回调：切换 `mPaused` 状态 + 按钮文字（⏸ 暂停 / ▶ 继续）；从暂停恢复时一次性将缓冲写入 editor

## 5. 删除位置信息和机场数据

- [ ] 5.1 `OsdPanel.cpp` `setupUi()` 中删除「位置信息」group box（mLongitude/mLatitude/mAltitude）的创建代码
- [ ] 5.2 `OsdPanel.cpp` `setupUi()` 中删除「机场数据」group box（mDockGroup 及所有子 label）的创建代码
- [ ] 5.3 `OsdPanel.cpp` `showAircraftOsd()` 中删除 setFieldValue for longitude/latitude/altitude 的 3 行
- [ ] 5.4 `OsdPanel.cpp` `showDockOsd()` 方法整体删除（机场数据 group box 已删除）
- [ ] 5.5 `OsdPanel.h` 中删除位置信息（mLongitude/mLatitude/mAltitude）和机场数据（mDockGroup 及所有 dock label）成员声明

## 6. 版本信息居中美化

- [ ] 6.1 `MainWindow::setupStatusBar()` 中版本标签改为 QWidget 容器 + QHBoxLayout + Qt::AlignCenter 实现真正居中
- [ ] 6.2 版本标签样式美化：`font-size: 11px; color: #80868b; letter-spacing: 0.5px;`

## 7. 集成验证

- [ ] 7.1 全量构建验证：`cmake --build build_mingw` 零错误零警告
- [ ] 7.2 交互验证：连接测试失败时提示语为「连接失败请检查配置参数是否有误」
- [ ] 7.3 交互验证：设备列表与 topic 面板按钮尺寸一致
- [ ] 7.4 交互验证：选中 topic 时原始 JSON 面板仅显示该 topic 数据，切换 topic 时过滤生效
- [ ] 7.5 交互验证：暂停/继续按钮正常工作，暂停期间显示冻结
- [ ] 7.6 交互验证：OSD 面板不显示位置信息和机场数据，仅显示设备信息和飞行数据
- [ ] 7.7 视觉验证：版本信息在状态栏中居中显示
