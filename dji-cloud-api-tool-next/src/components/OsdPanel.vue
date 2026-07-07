<template>
  <section class="panel osd-panel">
    <header class="panel-header">
      <h2>OSD / 设备信息</h2>
    </header>
    <div v-if="!device" class="panel-body empty">请选择设备</div>
    <div v-else class="panel-body osd-body">
      <section class="device-summary">
        <div>
          <strong>{{ device.name }}</strong>
          <span>{{ device.sn }}</span>
        </div>
        <span class="online-state" :class="{ online: device.online }">
          {{ device.online ? '在线' : '离线' }}
        </span>
      </section>

      <dl class="device-fields">
        <div>
          <dt>类型</dt>
          <dd>{{ device.type === 'dock' ? 'Dock' : 'Aircraft' }}</dd>
        </div>
        <div>
          <dt>最后在线</dt>
          <dd>{{ device.lastSeenAt ? formatTime(device.lastSeenAt) : '-' }}</dd>
        </div>
      </dl>

      <div v-if="osdItems.length === 0" class="empty">暂无 OSD 数据</div>
      <dl v-else class="osd-fields">
        <div v-for="item in osdItems" :key="item.label">
          <dt>{{ item.label }}</dt>
          <dd>{{ item.value }}<span v-if="item.unit">{{ item.unit }}</span></dd>
        </div>
      </dl>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { getByPath } from '../services/topicMapping'
import { useDeviceStore } from '../stores/deviceStore'
import { useMonitorStore } from '../stores/monitorStore'
import { useTopicStore } from '../stores/topicStore'

type OsdItem = {
  label: string
  value: string
  unit?: string
}

const devices = useDeviceStore()
const monitor = useMonitorStore()
const topics = useTopicStore()

const device = computed(() => devices.selectedDevice)
const selectedTopic = computed(() => {
  const selectedDevice = device.value
  if (!selectedDevice) {
    return undefined
  }

  return topics.selectedTopicForDevice(selectedDevice.sn)
})
const latestMessage = computed(() => {
  const topic = selectedTopic.value
  if (!topic?.enabled) {
    return undefined
  }

  return monitor.history(device.value?.sn, topic.topic).at(-1)
})
const osdSource = computed(() => {
  const root = parseJsonObject(latestMessage.value?.payloadText)
  if (!root) {
    return undefined
  }

  return isRecord(root.data) ? root.data : root
})
const osdItems = computed<OsdItem[]>(() => {
  const source = osdSource.value
  if (!source) {
    return []
  }

  return [
    readItem(source, '电量', ['battery', 'capacity_percent', 'battery.capacity_percent', 'battery.remain_percent'], '%'),
    readItem(source, '速度', ['speed', 'horizontal_speed', 'velocity', 'flight.speed'], 'm/s'),
    readPosition(source),
    readItem(source, '舱盖', ['cover_state', 'dock.cover_state', 'cover.status', 'dock_cover_state']),
    readItem(source, '温度', ['temperature', 'environment.temperature', 'dock.temperature'], '℃'),
    readItem(source, '湿度', ['humidity', 'environment.humidity', 'dock.humidity'], '%'),
  ].filter((item): item is OsdItem => item !== undefined)
})

function readItem(
  source: Record<string, unknown>,
  label: string,
  paths: string[],
  unit?: string,
): OsdItem | undefined {
  const value = firstValue(source, paths)
  if (value === undefined) {
    return undefined
  }

  return {
    label,
    value: formatValue(value),
    unit,
  }
}

function readPosition(source: Record<string, unknown>): OsdItem | undefined {
  const latitude = firstValue(source, ['latitude', 'position.latitude', 'location.latitude'])
  const longitude = firstValue(source, ['longitude', 'position.longitude', 'location.longitude'])
  const altitude = firstValue(source, ['height', 'elevation', 'altitude', 'position.height', 'position.altitude'])

  if (latitude === undefined && longitude === undefined && altitude === undefined) {
    return undefined
  }

  const coordinates = [
    latitude !== undefined ? `lat ${formatValue(latitude)}` : undefined,
    longitude !== undefined ? `lng ${formatValue(longitude)}` : undefined,
    altitude !== undefined ? `h ${formatValue(altitude)}m` : undefined,
  ].filter(Boolean)

  return {
    label: '位置',
    value: coordinates.join(', '),
  }
}

function firstValue(source: Record<string, unknown>, paths: string[]): unknown {
  for (const path of paths) {
    const value = getByPath(source, path)
    if (value !== undefined && value !== null && value !== '') {
      return value
    }
  }

  return undefined
}

function parseJsonObject(payloadText?: string): Record<string, unknown> | undefined {
  if (!payloadText) {
    return undefined
  }

  try {
    const value = JSON.parse(payloadText)
    return isRecord(value) ? value : undefined
  } catch {
    return undefined
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return value !== null && typeof value === 'object' && !Array.isArray(value)
}

function formatValue(value: unknown): string {
  if (typeof value === 'number') {
    return Number.isInteger(value) ? String(value) : value.toFixed(6).replace(/0+$/, '').replace(/\.$/, '')
  }

  return String(value)
}

function formatTime(value: string): string {
  const date = new Date(value)
  if (Number.isNaN(date.getTime())) {
    return value
  }

  return date.toLocaleString()
}
</script>

<style scoped>
.osd-body {
  display: grid;
  align-content: start;
  gap: 10px;
}

.device-summary {
  display: flex;
  align-items: center;
  gap: 8px;
  justify-content: space-between;
}

.device-summary div {
  min-width: 0;
  display: grid;
  gap: 2px;
}

.device-summary strong,
.device-summary span {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.device-summary span {
  color: #77808c;
  font-size: 12px;
}

.online-state {
  flex: 0 0 auto;
  color: #77808c;
  font-size: 12px;
}

.online-state.online {
  color: #0f8a3b;
}

.device-fields,
.osd-fields {
  display: grid;
  gap: 6px;
  margin: 0;
}

.device-fields div,
.osd-fields div {
  display: grid;
  grid-template-columns: minmax(72px, auto) minmax(0, 1fr);
  gap: 8px;
  align-items: center;
  padding: 6px 8px;
  border: 1px solid #edf0f4;
  border-radius: 6px;
}

dt {
  color: #5e6673;
}

dd {
  min-width: 0;
  margin: 0;
  overflow: hidden;
  font-weight: 600;
  text-align: right;
  text-overflow: ellipsis;
  white-space: nowrap;
}

dd span {
  margin-left: 4px;
  color: #77808c;
  font-weight: 400;
}
</style>
