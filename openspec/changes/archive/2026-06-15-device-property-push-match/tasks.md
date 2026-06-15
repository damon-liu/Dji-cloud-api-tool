# 任务清单

## 任务

- [x] **Task 1: 重命名 OsdParsePanel → TopicParsePanel**
  - 重命名 `src/ui/OsdParsePanel.h` → `src/ui/TopicParsePanel.h`（更新 include guard + 类名）
  - 重命名 `src/ui/OsdParsePanel.cpp` → `src/ui/TopicParsePanel.cpp`（更新 include + 类名引用）
  - 更新 `src/ui/MainWindow.h` 中成员类型 + include
  - 更新 `src/ui/MainWindow.cpp` 中所有引用
  - 更新 `CMakeLists.txt` 源文件列表
  - 编译验证通过

- [x] **Task 2: DeviceManager mRawJsonCache 改为 per-topic 存储**
  - 修改 `mRawJsonCache` 类型为 `QMap<QString, QMap<QString, QString>>`
  - 修改 `latestRawJson()` 签名，增加 topic 参数（默认空，空时返回任意可用 topic 数据兼容旧调用）
  - 修改 `parseAndRoute()` 中写入缓存的 key 为 sn+topic
  - 编译验证通过

- [x] **Task 3: TopicParsePanel 传入 topic 获取精确数据**
  - 修改 `TopicParsePanel::refresh()` 调用 `latestRawJson` 时传入 `mTopic`
  - 编译验证通过

- [x] **Task 4: topic_mappings.json 新增 state 和 status 映射**
  - 新增 `thing/product/{sn}/state` 条目（复用 osd 的 fields 和 groups）
  - 新增 `sys/product/{sn}/status` 条目（基于 dock-status.md 字段定义）
  - 将更新后的 `topic_mappings.json` 复制到 `deploy/` 目录
  - 编译验证通过
