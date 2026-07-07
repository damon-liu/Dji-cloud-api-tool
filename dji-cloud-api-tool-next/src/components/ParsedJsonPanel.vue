<template>
  <section class="panel parsed-json-panel">
    <header class="panel-header">
      <h2>Topic / JSON 解析字段</h2>
    </header>
    <div v-if="!selectedSn" class="panel-body empty">请选择设备</div>
    <div v-else-if="!latestMessage" class="panel-body empty">暂无消息</div>
    <div v-else-if="groups.length === 0" class="panel-body empty">暂无可解析字段</div>
    <div v-else class="panel-body parsed-body">
      <section v-for="group in groups" :key="group.id" class="field-group">
        <h3>{{ group.label }}</h3>
        <dl>
          <div v-for="row in group.rows" :key="row.key" class="field-row">
            <dt>{{ row.label }}</dt>
            <dd>
              <span class="field-value">{{ row.value }}</span>
              <span v-if="row.unit" class="field-unit">{{ row.unit }}</span>
            </dd>
            <small>{{ row.key }}</small>
          </div>
        </dl>
      </section>
      <p class="updated-at">更新时间：{{ formatTime(latestMessage.receivedAt) }}</p>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import {
  defaultTopicMapping,
  mappingForTopic,
  parseMappedFields,
  type ParsedFieldRow,
} from '../services/topicMapping'
import { useDeviceStore } from '../stores/deviceStore'
import { useMonitorStore } from '../stores/monitorStore'
import { useTopicStore } from '../stores/topicStore'

const devices = useDeviceStore()
const monitor = useMonitorStore()
const topics = useTopicStore()

const selectedSn = computed(() => devices.selectedSn)
const selectedTopic = computed(() => {
  const sn = selectedSn.value
  if (!sn) {
    return undefined
  }

  return topics.selectedTopicForDevice(sn)
})
const latestMessage = computed(() => {
  const topic = selectedTopic.value
  if (!topic?.enabled) {
    return undefined
  }

  return monitor.history(selectedSn.value, topic.topic).at(-1)
})
const parsedPayload = computed(() => parseJsonObject(latestMessage.value?.payloadText))
const matchedMapping = computed(() => {
  const topic = latestMessage.value?.topic
  return topic ? mappingForTopic(defaultTopicMapping, topic) : undefined
})
const groupLabels = computed(() => new Map(
  matchedMapping.value?.groups.map((group) => [group.id, group.label]) ?? [],
))
const rows = computed(() => {
  const payload = parsedPayload.value
  const message = latestMessage.value
  if (!payload || !message) {
    return []
  }

  const data = isRecord(payload.data) ? payload.data : payload
  return parseMappedFields(defaultTopicMapping, message.topic, data)
})
const groups = computed(() => {
  const grouped = new Map<string, ParsedFieldRow[]>()
  for (const row of rows.value) {
    grouped.set(row.groupId, [...(grouped.get(row.groupId) ?? []), row])
  }

  return Array.from(grouped, ([id, groupRows]) => ({
    id,
    label: groupLabels.value.get(id) ?? id,
    rows: groupRows,
  }))
})

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

function formatTime(value: string): string {
  const date = new Date(value)
  if (Number.isNaN(date.getTime())) {
    return value
  }

  return date.toLocaleString()
}
</script>

<style scoped>
.parsed-body {
  display: grid;
  align-content: start;
  gap: 12px;
}

.field-group h3 {
  margin: 0 0 6px;
  color: #20242a;
  font-size: 13px;
}

dl {
  display: grid;
  gap: 4px;
  margin: 0;
}

.field-row {
  display: grid;
  grid-template-columns: minmax(88px, 0.8fr) minmax(0, 1fr);
  gap: 2px 8px;
  padding: 6px 8px;
  border: 1px solid #edf0f4;
  border-radius: 6px;
}

dt {
  color: #5e6673;
}

dd {
  margin: 0;
  min-width: 0;
  text-align: right;
}

.field-value {
  font-weight: 600;
}

.field-unit {
  margin-left: 4px;
  color: #77808c;
}

small {
  grid-column: 1 / -1;
  min-width: 0;
  overflow: hidden;
  color: #9aa3af;
  font-family: "SFMono-Regular", Consolas, monospace;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.updated-at {
  margin: 0;
  color: #77808c;
  font-size: 12px;
}
</style>
