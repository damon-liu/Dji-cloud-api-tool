## 负载控制—开始拍照

**Topic:** thing/product/*{gateway_sn}*/services

**Direction:** down

**Method:** camera_photo_take

**Data:**

| Column        | Name     | Type | constraint | Description                                                  |
| ------------- | -------- | ---- | ---------- | ------------------------------------------------------------ |
| payload_index | 相机枚举 | text |            | 相机枚举值。非标准的 device_mode_key，格式为 {type-subtype-gimbalindex}，可以参考[产品支持](https://developer.dji.com/doc/cloud-api-tutorial/cn/overview/product-support.html) |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"payload_index": "39-0-7"
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_photo_take"
}
```

**Topic:** thing/product/*{gateway_sn}*/services_reply

**Direction:** up

**Method:** camera_photo_take

**Data:**

| Column  | Name     | Type        | constraint               | Description                                                  |
| ------- | -------- | ----------- | ------------------------ | ------------------------------------------------------------ |
| result  | 返回码   | int         |                          | 非 0 代表错误                                                |
| output  | 输出     | struct      |                          |                                                              |
| »status | 任务状态 | enum_string | {"in_progress":"执行中"} | 当全景拍照或其他持续性拍照行为时会上报状态信息，表达后续会有持续的进度事件上报，详细内容请查看 camera_photo_take_progress 事件 |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"result": 0
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_photo_take"
}
```

## 负载控制—停止拍照

停止拍照指令，目前仅支持全景拍照模式

**Topic:** thing/product/*{gateway_sn}*/services

**Direction:** down

**Method:** camera_photo_stop

**Data:**

| Column        | Name     | Type | constraint | Description                                                  |
| ------------- | -------- | ---- | ---------- | ------------------------------------------------------------ |
| payload_index | 相机枚举 | text |            | 相机枚举值。非标准的 device_mode_key，格式为 {type-subtype-gimbalindex}，可以参考[产品支持](https://developer.dji.com/doc/cloud-api-tutorial/cn/overview/product-support.html) |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"payload_index": "39-0-7"
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_photo_stop"
}
```

**Topic:** thing/product/*{gateway_sn}*/services_reply

**Direction:** up

**Method:** camera_photo_stop

**Data:**

| Column | Name   | Type | constraint | Description   |
| ------ | ------ | ---- | ---------- | ------------- |
| result | 返回码 | int  |            | 非 0 代表错误 |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"result": 0
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_photo_stop"
}
```



## 负载控制—开始录像

**Topic:** thing/product/*{gateway_sn}*/services

**Direction:** down

**Method:** camera_recording_start

**Data:**

| Column        | Name     | Type | constraint | Description                                                  |
| ------------- | -------- | ---- | ---------- | ------------------------------------------------------------ |
| payload_index | 相机枚举 | text |            | 相机枚举值。非标准的 device_mode_key，格式为 {type-subtype-gimbalindex}，可以参考[产品支持](https://developer.dji.com/doc/cloud-api-tutorial/cn/overview/product-support.html) |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"payload_index": "39-0-7"
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_recording_start"
}
```

**Topic:** thing/product/*{gateway_sn}*/services_reply

**Direction:** up

**Method:** camera_recording_start

**Data:**

| Column | Name   | Type | constraint | Description   |
| ------ | ------ | ---- | ---------- | ------------- |
| result | 返回码 | int  |            | 非 0 代表错误 |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"result": 0
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_recording_start"
}
```



## 负载控制—停止录像

**Topic:** thing/product/*{gateway_sn}*/services

**Direction:** down

**Method:** camera_recording_stop

**Data:**

| Column        | Name     | Type | constraint | Description                                                  |
| ------------- | -------- | ---- | ---------- | ------------------------------------------------------------ |
| payload_index | 相机枚举 | text |            | 相机枚举值。非标准的 device_mode_key，格式为 {type-subtype-gimbalindex}，可以参考[产品支持](https://developer.dji.com/doc/cloud-api-tutorial/cn/overview/product-support.html) |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"payload_index": "39-0-7"
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_recording_stop"
}
```

**Topic:** thing/product/*{gateway_sn}*/services_reply

**Direction:** up

**Method:** camera_recording_stop

**Data:**

| Column | Name   | Type | constraint | Description   |
| ------ | ------ | ---- | ---------- | ------------- |
| result | 返回码 | int  |            | 非 0 代表错误 |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"result": 0
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "camera_recording_stop"
}
```

## 负载控制—重置云台

**Topic:** thing/product/*{gateway_sn}*/services

**Direction:** down

**Method:** gimbal_reset

**Data:**

| Column        | Name         | Type     | constraint                                            | Description                                                  |
| ------------- | ------------ | -------- | ----------------------------------------------------- | ------------------------------------------------------------ |
| payload_index | 负载编号     | text     |                                                       | 负载编号，相机枚举值。非标准的 device_mode_key，格式为 {type-subtype-gimbalindex}，可以参考[产品支持](https://developer.dji.com/doc/cloud-api-tutorial/cn/overview/product-support.html) |
| reset_mode    | 重置模式类型 | enum_int | {"0":"回中","1":"向下","2":"偏航回中","3":"俯仰向下"} |                                                              |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"payload_index": "39-0-7",
		"reset_mode": 0
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "gimbal_reset"
}
```

**Topic:** thing/product/*{gateway_sn}*/services_reply

**Direction:** up

**Method:** gimbal_reset

**Data:**

| Column | Name   | Type | constraint | Description   |
| ------ | ------ | ---- | ---------- | ------------- |
| result | 返回码 | int  |            | 非 0 代表错误 |

**Example:**

```json
{
	"bid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"data": {
		"result": 0
	},
	"tid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxx",
	"timestamp": 1654070968655,
	"method": "gimbal_reset"
}
```