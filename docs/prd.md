# 产品需求记录文档

## 功能迭代

### 2026-06-11

#### 1.1 新增topic分区

1. 设备订阅的topic列表显示在设备列表下的一个独立的区域，该区域与设备列表处于界面布局的同一列，并且topic的新增、禁用、删除按钮在该区域右侧
2. 机场设备列表可新增订阅手飞无人机设备，新增时包含设备名称、sn，并且手飞无人机订阅的topic同样处于下面独立区域
3. 修复broker断开按钮未生效Bug

#### 1.2 界面BUG修复

1. 设备列表和topic列表面板要对齐，以topic列表为准
2. 设备列表设备选中后无法取消，只能对当前选中的设备添加子设备，无法重新新增父设备
3. topic列表有多个topic时，频繁自动切换，改成用户选中哪个topic就固定该topic
4. 原始JSON中不能只显示最新上报的，topic订阅以后历史上报的数据也需要展示，最下面展示最新的数据
5. 在界面最下面的【已连接】那行中间添加版本及个人github地址https://github.com/damon-liu/Dji-cloud-api-tool

#### 1.3 界面BUG修复

1. 连接测试报错提示语改成：连接失败请检查配置参数是否有误
2. 设备列表的新增删除按钮与topic的不一样，需保持一致
3. 原始JSON界面频繁自动切换，改成用户选中哪个topic就固定显示topic订阅的JSON数据，
4. 原始JSON界面上复制按钮旁边加一个暂停按钮，点击暂定以后原始JSON数据窗口数据暂停刷新，但数据还在持续订阅
5. 删除设备信息里面的位置信息、机场数据
6. v1.0/https://github.com/damon-liu/Dji-cloud-api-tool软件版本和项目地址要居中，结合一下大厂设计要有美感

#### 1.4 OSD JSON数据解析

设备信息下面新增一个JSON解析独立的区域，该区域定时获取（间隔时间可以设置）右侧原始JSON中的最新上报的一条数据，并将原始JSON数据按照key值翻译成中文显示在该区域中，其中每个topic的JSON数据都不一样，所以需要维护每个topic原始JSON中key对应的中文，如：

Topic:thing/product/*{device_sn}*/osd原始JSON数据对应的key可以从大疆上云API官网这个地址去匹配对应的中文：https://developer.dji.com/doc/cloud-api-tutorial/cn/api-reference/dock-to-cloud/mqtt/dock/dock3/properties.html

先实现osd这个topic的JSON解析功能

#### 1.5  OSD字段匹配

读取本项目config目录下dock-osd.md中表格里面**Column** **Name** **constraint** 字段，对比topic_mappings.json文件中缺失的字段，将缺失的字段加入进去，未匹配到的字段继续保留在其他字段中，比如：air_conditioner_state字段在该链接中是存在的，为什么没有匹配到

#### 1.6 设备属性推送匹配

读取本项目config目录下dock-osd.md中表格里面**Column** **Name** **constraint** 字段，当用户手动订阅topic：thing/product/*{device_sn}*/state时，自动在JSON解析栏解析原始JSON上报的数据，方案可以遵循osd解析

读取本项目config目录下dock-status.md，当用户手动订阅topic：sys/product/{gateway_sn}/status时，自动在JSON解析栏解析原始JSON上报的数据

#### 1.7 完善订阅和发送topic

topic列表处默认添加以下topic，{gateway_sn}替换成当前所选中的机场或无人机的SN，topic顺序可以手动调准

```
thing/product/{gateway_sn}/state
thing/product/{gateway_sn}/requests
thing/product/{gateway_sn}/events
thing/product/{gateway_sn}/services_reply
thing/product/{gateway_sn}/property/set_reply
sys/product/{gateway_sn}/status
thing/product/{gateway_sn}/drc/up
```

在topic下发输入栏新增以下topic供用户选择，{gateway_sn}替换成当前所选中的机场或无人机的SN，topic顺序可以手动调准

```
thing/product/{gateway_sn}/property/set
thing/product/{gateway_sn}/services
thing/product/{gateway_sn}/events_reply
thing/product/{gateway_sn}/requests_reply
sys/product/{gateway_sn}/status_reply
```

#### 1.8 界面问题修复

1. 机场数据默认1s刷新一次，刷新时间间隔支持手动配置
2. 原始JSON断开时，机场数据和JSON解析数据都应该暂停，继续以后恢复正常
3. 在设备列表处支持一键禁用或启用设备下所有的topic

#### 1.9 OSD面板改造

1. 删除设备信息，将机场信息移至设备信息位置
2. 机场信息显示机场osd上报的字段：经纬度、舱盖状态、飞行器开机状态、舱内温度、环境温度、风速、降雨量，尽量每行显示两个字段
3. 机场信息原来位置新增飞机信息，显示飞机osd上报的状态字段：飞行器状态、当前位置经纬度、电池剩余电量、电池温度、相对起飞点高度、距离 Home 点的距离、风速、GPS 搜星数量，尽量每行显示两个字段
