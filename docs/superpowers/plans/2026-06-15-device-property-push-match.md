---
change: device-property-push-match
design-doc: docs/superpowers/specs/2026-06-15-device-property-push-match-design.md
base-ref: 8887a91175bdc8864c9d405bb9474c8b0a36a358
archived-with: 2026-06-15-device-property-push-match
---

# 设备属性推送匹配 实施计划

**Goal:** 扩展 JSON 解析面板支持 `thing/product/{sn}/state` 和 `sys/product/{sn}/status`，修复 `mRawJsonCache` 同设备多 topic 覆盖问题。

**Architecture:** 重命名 `OsdParsePanel`→`TopicParsePanel`；`mRawJsonCache` 升级为二层 map；`topic_mappings.json` 新增 state/status 映射。

**Tech Stack:** Qt 6 C++17, CMake, MinGW

archived-with: 2026-06-15-device-property-push-match
---

## 文件结构总览

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/ui/TopicParsePanel.h` | 新建（重命名） | JSON 解析面板类声明 |
| `src/ui/TopicParsePanel.cpp` | 新建（重命名） | JSON 解析面板类实现 |
| `src/ui/OsdParsePanel.h` | 删除 | 旧文件 |
| `src/ui/OsdParsePanel.cpp` | 删除 | 旧文件 |
| `src/ui/MainWindow.h` | 修改 | include + 成员类型 |
| `src/ui/MainWindow.cpp` | 修改 | 所有引用 |
| `CMakeLists.txt` | 修改 | 源文件列表 |
| `src/core/DeviceManager.h` | 修改 | 缓存类型 + 接口签名 |
| `src/core/DeviceManager.cpp` | 修改 | 实现 |
| `config/topic_mappings.json` | 修改 | 新增映射 |

archived-with: 2026-06-15-device-property-push-match
---

### Task 1: 重命名 OsdParsePanel → TopicParsePanel

**依赖:** 无
**验证:** `cmake --build build_mingw`

- [x] Step 1: 创建 `src/ui/TopicParsePanel.h`（从 OsdParsePanel.h 复制，修改 include guard + 类名 + 构造函数名）
- [x] Step 2: 创建 `src/ui/TopicParsePanel.cpp`（从 OsdParsePanel.cpp 复制，修改 include + 所有 `OsdParsePanel::` → `TopicParsePanel::`）
- [x] Step 3: 更新 `src/ui/MainWindow.h`（include + 成员类型 `mOsdParsePanel` → `mTopicParsePanel`）
- [x] Step 4: 更新 `src/ui/MainWindow.cpp` 所有引用（5 处：构造、setWidget、connect、setTopicMapping、setDeviceManager、clear）
- [x] Step 5: 更新 `CMakeLists.txt` 源文件列表
- [x] Step 6: 删除旧文件 `OsdParsePanel.h`、`OsdParsePanel.cpp`
- [x] Step 7: 编译验证 `cmake --build build_mingw`
- [x] Step 8: 提交

archived-with: 2026-06-15-device-property-push-match
---

### Task 2: DeviceManager mRawJsonCache 改为 per-topic 存储

**依赖:** 无
**验证:** `cmake --build build_mingw`

- [x] Step 1: 修改 `DeviceManager.h` — `latestRawJson` 增加 topic 参数 + `mRawJsonCache` 改为二层 map
- [x] Step 2: 修改 `DeviceManager.cpp` — `latestRawJson` 实现（精确匹配 → 回退兼容）
- [x] Step 3: 修改 `parseAndRoute` 缓存写入：`mRawJsonCache[sn][topic] = formatted`
- [x] Step 4: 编译验证
- [x] Step 5: 提交

archived-with: 2026-06-15-device-property-push-match
---

### Task 3: TopicParsePanel 传入 topic 获取精确数据

**依赖:** Task 1 + Task 2
**验证:** `cmake --build build_mingw`

- [x] Step 1: `TopicParsePanel::refresh()` 调用 `latestRawJson(mDeviceSn, mTopic)`
- [x] Step 2: 编译验证
- [x] Step 3: 提交

archived-with: 2026-06-15-device-property-push-match
---

### Task 4: topic_mappings.json 新增 state 和 status 映射

**依赖:** 无
**验证:** JSON 格式校验 + `cmake --build build_mingw`

- [x] Step 1: 新增 `thing/product/{sn}/state`（复用 osd 的 fields + groups）
- [x] Step 2: 新增 `sys/product/{sn}/status`（基于 dock-status.md）
- [x] Step 3: Python 校验 JSON 格式 + topics 数量
- [x] Step 4: 复制到 `deploy/topic_mappings.json`
- [x] Step 5: 编译验证
- [x] Step 6: 提交

archived-with: 2026-06-15-device-property-push-match
---

### Task 5: 全量编译 + 回归验证

**依赖:** Task 1-4
**验证:** 全量编译 + 残留引用检查

- [x] Step 1: 清理重新编译 `cmake --build build_mingw --clean-first`
- [x] Step 2: `grep -r "OsdParsePanel" src/` 确认无残留
- [x] Step 3: 部署文件同步
- [x] Step 4: 提交

archived-with: 2026-06-15-device-property-push-match
---

## 依赖关系

```
Task 1 (重命名) ──┐
                  ├──> Task 3 (传入topic) ──> Task 5 (验证)
Task 2 (缓存)  ──┘

Task 4 (JSON映射) ──────────────────────────> Task 5 (验证)
```
