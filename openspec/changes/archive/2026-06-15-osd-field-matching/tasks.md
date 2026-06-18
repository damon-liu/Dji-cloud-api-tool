# Tasks: OSD 字段匹配

## 任务列表

- [x] **Task 1: 创建 Markdown 表格解析模块**
  - 编写 `parseDockOsdMd()` 函数（Node.js）
  - 正确解析 `| Column | Name | Type | constraint | ...` 表头
  - 通过 `»` 前缀计数确定嵌套层级
  - 生成带嵌套路径的字段列表（如 `air_conditioner.air_conditioner_state`）
  - 处理 constraint 列的 JSON 提取（枚举值和单位）

- [x] **Task 2: 创建字段合并逻辑**
  - 编写 `mergeFields()` 函数
  - 加载现有 `topic_mappings.json`
  - 对比找出缺失字段
  - 保留独有字段（飞行器专属字段）
  - 生成合理的 `values`（枚举）和 `unit`（单位）
  - 自动创建 12 个新 group 收纳新字段

- [x] **Task 3: 创建主脚本入口**
  - 编写 `main()` 函数串联解析+合并+输出
  - 支持命令行参数：`--dry-run` 仅报告不写入
  - 输出变更报告：新增字段列表、保留字段说明
  - 幂等保证：重复执行不重复添加（已验证 0 新增）

- [x] **Task 4: 执行脚本并验证结果**
  - 运行 `node scripts/sync_topic_mappings.js`
  - 验证 `air_conditioner.air_conditioner_state` 已添加 ✓
  - 验证现有飞行器字段未丢失 ✓ (23/23)
  - 验证 JSON 格式有效可被 C++ 加载 ✓
  - 编译项目确认无破坏 ✓
