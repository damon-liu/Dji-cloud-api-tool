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