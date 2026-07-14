# Topic 下发功能 — 设计文档

日期：2026-07-14
状态：已确认

## 概述

实现 MQTT Topic 下发功能，打通从 UI 到 MQTT Broker 的 publish 链路。用户在 PublishPanel 中选择 topic、编辑 JSON，点击发送按钮将消息发布到 MQTT Broker。

## 数据流

```
PublishPanel                        MainWindow                      DeviceManager                   MqttClientManager
    │                                   │                                │                                │
    │── publishRequested(topic,json)──▶ │                                │                                │
    │                                   │── publishMessage(topic,json)──▶│                                │
    │                                   │                                │── publish(topic,payload)──────▶│
    │                                   │                                │                                │── QMqttClient::publish()
    │                                   │                                │                                │── publishCompleted(topic)
    │                                   │                                │ ◀──────────────────────────────│
    │                                   │                                │── publishResult(success,msg)   │
    │                                   │ ◀──────────────────────────────│                                │
    │ ◀── onPublishResult(success,msg)──│                                │                                │
```

单向 publish 链路，结果通过信号异步返回。遵循现有三层架构：UI → DeviceManager → MqttClientManager。

## 组件改动

### MqttClientManager

- **新增 public method**: `void publish(const QString& topic, const QByteArray& payload)`
  - 封装 `QMqttClient::publish(topic, payload, 1)`（QoS 1）
  - 连接 `QMqttPublishProperties::finished` 信号获取结果
- **新增 signal**: `void publishCompleted(const QString& topic, bool success, const QString& errorMsg)`

### DeviceManager

- **新增 public slot**: `void publishMessage(const QString& topic, const QString& json)`
  - 将 json 字符串转为 QByteArray，调用 MqttClientManager::publish()
- **新增 signal**: `void publishResult(const QString& topic, bool success, const QString& message)`
  - 转发 MqttClientManager::publishCompleted
- 内部 connect: mqttManager.publishCompleted → DeviceManager.publishResult

### PublishPanel（纯头文件 → .h + .cpp）

重构为有 `.cpp` 实现文件的类，改动项：

1. **新信号**: `publishRequested(const QString& topic, const QString& json)` — 用户点击发送时发射
2. **新 slot**: `onPublishResult(const QString& topic, bool success, const QString& message)` — 接收发送结果
3. **新方法**: `setConnected(bool connected)` — 更新 MQTT 连接状态，控制按钮启用条件
4. **新方法**: `loadTemplates(const QString& path)` — 从外部 JSON 加载 topic 模板
5. **发送按钮启用条件**: MQTT 已连接 + topic 非空 + JSON 非空（三者同时满足，任一变化时实时检查）
6. **发送历史区域**: 发送按钮下方新增只读 QPlainTextEdit（高度 ~80px），记录最近 20 条发送记录
7. **模板自动填入**: topic 切换时，若编辑区为空则根据模板填入示例 JSON

### 初始化流程

1. `MainWindow::setupLayout()` 中创建 PublishPanel（已有）
2. `MainWindow::connectSignals()` 中添加本功能的信号连接（发送链路 + 连接状态同步）
3. 连接状态初始化：MainWindow 调用 `mPublishPanel->setConnected(mDevMgr->isConnected())`
4. 模板加载：MainWindow 在 setupLayout 结束后调用 `mPublishPanel->loadTemplates(appDir + "/publish_templates.json")`

### publish_templates.json（新增）

可执行文件同目录，结构：

```json
{
  "templates": [
    { "topic": "thing/product/{sn}/property/set",     "template": "{\n  \n}" },
    { "topic": "thing/product/{sn}/services",          "template": "{\n  \"services\": []\n}" },
    { "topic": "thing/product/{sn}/events_reply",      "template": "{\n  \"events_reply\": []\n}" },
    { "topic": "thing/product/{sn}/requests_reply",    "template": "{\n  \"requests_reply\": []\n}" },
    { "topic": "sys/product/{sn}/status_reply",        "template": "{\n  \"status_reply\": {}\n}" }
  ]
}
```

- 文件不存在时程序自动创建（使用内置默认值）
- 用户可手动编辑，下次启动生效
- PublishPanel::loadTemplates() 支持运行时重新加载

### MainWindow

在 `connectSignals()` 中新增信号连接：

```cpp
// PublishPanel → DeviceManager
connect(mPublishPanel, &PublishPanel::publishRequested,
        mDevMgr, &DeviceManager::publishMessage);
// DeviceManager → PublishPanel
connect(mDevMgr, &DeviceManager::publishResult,
        mPublishPanel, &PublishPanel::onPublishResult);
// MQTT 连接状态 → PublishPanel
connect(mDevMgr, &DeviceManager::brokerConnected,
        mPublishPanel, [this]() { mPublishPanel->setConnected(true); });
connect(mDevMgr, &DeviceManager::brokerDisconnected,
        mPublishPanel, [this]() { mPublishPanel->setConnected(false); });
```

## 发送历史

- 位置：发送按钮下方
- 组件：只读 `QPlainTextEdit`，高度 ~80px，等宽字体
- 格式：`[HH:mm:ss] ✅/❌ topic  消息`
- 配色：成功 `#1e8e3e` / 失败 `#d93025`
- 容量：最近 20 条，超出后移除最旧记录
- 交互：双击某条记录，将该条的 topic 和 JSON 还原到编辑区

## 错误处理

- MQTT 未连接：按钮禁用，不会触发发送
- JSON 为空：按钮禁用
- Topic 为空：按钮禁用
- publish API 调用失败：通过 publishCompleted 信号返回 errorMsg，显示在历史区域
- JSON 格式校验：不强制校验（由用户自行保证），但发送前尝试 QJsonDocument::fromJson 解析，若失败则警告但仍允许发送

## 约束

- 无自动化测试要求
- 线上设备无需自动化测试
- 遵循 C++17 + Qt 6 现有技术栈
- 遵循现有代码风格和分层架构
- commit 信息使用中文
