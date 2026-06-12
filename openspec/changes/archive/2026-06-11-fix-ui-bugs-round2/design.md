## Context

第二轮界面 BUG 修复，涉及连接测试提示、按钮一致性、JSON 面板交互增强、OSD 面板精简、状态栏美化。这些改动聚焦于细节体验和产品完成度。

## Goals / Non-Goals

**Goals:**
- 统一连接测试失败提示语为用户友好中文
- 统一设备与 topic 按钮尺寸为 28×28
- JSON 面板支持按选中 topic 过滤和历史暂停
- 精简 OSD 面板（去除位置信息和机场数据）
- 版本标签真正居中

**Non-Goals:**
- 不修改 MQTT 连接/订阅逻辑
- 不修改 OSD 解析逻辑
- 不引入新外部依赖

## Decisions

### D1: JSON 历史按 Topic 存储

**选择：** `DeviceManager` 中 `mJsonHistory` 类型从 `QMap<QString, QStringList>`（SN→list）改为 `QMap<QString, QMap<QString, QStringList>>`（SN→(topic→list)）。`parseAndRoute()` 中按 topic 存储 JSON。`jsonHistory(sn, topic)` 查询指定 topic 的历史。

**替代方案：**
- A. 保持按 SN 存储，在 UI 层按 topic 字符串过滤 → JSON 中需要解析 topic 字段，不可靠
- B. 完全替换为 topic 维度的存储 → 切换设备时无法快速获取全部 JSON

**最终选择：** 两级索引 `SN → topic → [json]`，支持按 topic 过滤（传入 topic）和全量查询（传入空 topic 或遍历所有 topic）。

### D2: 暂停按钮实现

**选择：** `RawJsonPanel` 内部增加 `mPaused` 标志。暂停时 `appendJson()` 将数据写入 `mPendingBuffer` 而非 editor。恢复时一次性将缓冲数据写入 editor。

**状态机：**
```
运行中 → (点击⏸) → 暂停中 (数据进缓冲)
暂停中 → (点击▶) → 运行中 (缓冲 → editor → 订阅恢复)
```

### D3: OSD 面板精简

**选择：** 从 `setupUi()` 中删除位置信息 group box（经度/纬度/高度）和机场数据 group box 的创建代码。从 `showAircraftOsd()` 和 `showDockOsd()` 中删除对应的 setFieldValue 调用。保留 `OsdData.h` 中的字段定义（避免破坏解析逻辑）。

### D4: 版本标签居中

**选择：** 用 `QWidget` 容器 + `QHBoxLayout` + `Qt::AlignCenter` 实现真正居中，放入 `statusBar()->addWidget(container, 1)`。样式采用 `font-size: 11px; color: #80868b` 低调处理。

### D5: 连接测试提示语

**选择：** 将 `ConfigDialog` 中所有 `QMessageBox::warning` 的消息统一为 `"连接失败请检查配置参数是否有误"`，不再区分具体错误类型。

## Risks / Trade-offs

- **JSON 按 topic 存储增加内存** → 两级 map 比一级略多内存，但总量由条目数控制（500 条/device/topic），实际影响可忽略
- **删除位置信息后飞机追踪受限** → 如后续需要位置地图展示，可从原始 JSON 面板重新获取
- **暂停期间数据积压** → 暂停时间过长时缓冲可能堆积，恢复时一次性写入大量数据到 editor 可能卡顿。限制缓冲上限为 1000 条。
