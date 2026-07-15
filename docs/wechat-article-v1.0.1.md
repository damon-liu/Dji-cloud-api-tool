# 大疆上云API调试不再痛苦，我写了个专用工具

做 DJI 行业设备开发的兄弟都懂——用 MQTTX 调试上云 API 有多难受：Topic 字符串密密麻麻分不清哪个设备、英文 JSON 字段看得眼花、想下发一条指令得手动拼 Topic 和参数、换个环境得重新填一遍连接信息。

于是我写了一个专为 DJI Cloud API 设计的桌面调试工具，**Dji-Cloud-Api-Tool**。开源免费，下载解压就能用。

---

## 🆚 和 MQTTX 到底差在哪

用过 MQTTX 的开发者一眼就能看出区别：

| 对比维度 | MQTTX | DjiCloudApiTool |
|----------|-------|-----------------|
| 设备识别 | 只能看到 Topic 字符串，需人肉判断设备 | 自动识别 SN，树形展示机场与子飞机层级 |
| 设备管理 | 所有 Topic 混在一起，无层级 | 每个设备独立管理 Topic，支持启用/禁用/拖拽排序 |
| 自动订阅 | 需手动逐个添加 Topic | 添加机场自动订阅 7 个常用 Topic，连接后自动发现并订阅子飞机 |
| 数据可读性 | 原始 JSON，字段全英文 | 自动翻译中文，按分组展示 |
| Topic 下发 | 手动输入 Topic + 手写 JSON | 预设 6 个下发 Topic，参数模板一键填入，发送历史双击恢复 |
| 抓包导出 | 手动复制粘贴或配外部工具 | 一键抓包，数据实时写入文件，按设备/Topic 分类保存 |
| 配置切换 | 每次切换服务器手动改参数 | 多 Connection 配置，一键切换生产/测试环境 |
| 断线处理 | 断线需手动重连 | 自动重连 + 指数退避（1s→2s→4s→…→30s），恢复后自动刷新 |
| JSON 字段定位 | 长 JSON 里用眼找字段 | 点击翻译面板中的字段名直接复制原始 key |
| 环境依赖 | 需安装 Node.js 或下桌面版 | 单文件 exe，零依赖 |
| 学习成本 | 通用 MQTT 工具，需自己理解 DJI 协议 | 专为 DJI Cloud API 设计，开箱即用 |

---

## 🆕 v1.0.1 新增了这些

**🤖 自动发现子飞机**

连接机场后自动识别下挂的无人机，添加到设备列表并订阅 Topic，不用再手动添加。

**📨 Topic 下发正式开放**

v1.0 只能看不能发，现在下发功能已完整可用。预设 6 个下发 Topic 参数模板，选 Topic → 改参数 → 点发送，三步搞定。发送历史支持双击恢复，方便重复发送相同指令。

**📐 设备树与 Topic 列表可拖拽**

左侧设备树和 Topic 列表之间的分隔线支持上下拖拽，设备多/Topic 多时灵活调整可视区域。

**❓ 帮助按钮**

工具栏新增帮助按钮，一键跳转大疆上云 API 官网和项目主页。

**🔧 Connection 切换即时生效**

修复了配置页切换 Connection 不立即刷新的问题。

---

## 📥 三步上手

**1. 配置 MQTT** — 点「⚙ 配置」→ 填 Broker 信息 → 点「Test」测试 → 「OK」保存

**2. 添加设备** — 点「＋」→ 选 Dock 或 Pilot → 输入 SN → 确定。连接后子飞机会自动发现

**3. 开始调试** — 点「● 连接」，OSD 面板和 JSON 面板实时刷新。底部「▶ Topic 下发」可发送控制指令

![image-20260715152241596](C:\Users\lhx\AppData\Roaming\Typora\typora-user-images\image-20260715152241596.png)

---

## 📥 下载

👉 [**DjiCloudApiTool-v1.0.1.zip**](https://github.com/damon-liu/Dji-cloud-api-tool/releases/download/v1.0.1/DjiCloudApiTool-v1.0.1.zip)（约 29 MB）

解压双击 `DjiCloudApi.exe` 即可运行，**无需安装**。支持 Windows 10/11 x64。

> 首次启动自动生成 `config.json` 在软件同目录下，内含 MQTT 密码，**请勿分享或上传到公开仓库**。

详细使用指南：[https://github.com/damon-liu/Dji-cloud-api-tool/blob/main/docs/user-guide.md](https://github.com/damon-liu/Dji-cloud-api-tool/blob/main/docs/user-guide.md)

---

## 🔮 下版本预告 (v1.0.2)

**🔔 关键事件桌面通知**：电量过低、飞机降落、返航时 Windows 弹窗提醒，不用一直盯着屏幕

**🎮 常用控制按钮**：开关飞机、开关舱门、开关充电、一键起飞，点按钮即发送

---

