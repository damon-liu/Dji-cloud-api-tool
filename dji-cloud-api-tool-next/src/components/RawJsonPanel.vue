<template>
  <section class="panel raw-json-panel">
    <header class="panel-header">
      <h2>原始 JSON</h2>
      <button type="button" :title="monitor.paused ? '恢复' : '暂停'" @click="togglePaused">
        {{ monitor.paused ? '▶' : 'Ⅱ' }}
      </button>
      <button type="button" title="复制最新" :disabled="messages.length === 0" @click="copyLatest">⧉</button>
      <button type="button" title="复制全部历史" :disabled="messages.length === 0" @click="copyAll">≡</button>
      <button type="button" title="清空" :disabled="messages.length === 0" @click="clearMessages">×</button>
    </header>
    <div v-if="!selectedSn" class="panel-body empty">请选择设备</div>
    <div v-else-if="messages.length === 0" class="panel-body empty">暂无消息</div>
    <div v-else class="panel-body raw-json-body">
      <article v-for="message in orderedMessages" :key="messageKey(message)" class="message-item">
        <header class="message-meta">
          <span>{{ formatTime(message.receivedAt) }}</span>
          <span :title="message.topic">{{ message.topic }}</span>
        </header>
        <pre>{{ formatPayload(message.payloadText) }}</pre>
      </article>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useDeviceStore } from '../stores/deviceStore'
import { useMonitorStore } from '../stores/monitorStore'
import { useTopicStore } from '../stores/topicStore'
import type { MqttRuntimeMessage } from '../types/domain'

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
const messages = computed(() => {
  const topic = selectedTopic.value
  if (!topic?.enabled) {
    return []
  }

  return monitor.history(selectedSn.value, topic.topic)
})
const orderedMessages = computed(() => [...messages.value].reverse())

function togglePaused() {
  monitor.paused = !monitor.paused
}

async function copyLatest() {
  const latest = messages.value.at(-1)
  if (latest) {
    await copyText(formatPayload(latest.payloadText))
  }
}

async function copyAll() {
  await copyText(
    messages.value
      .map((message) => [
        `[${message.receivedAt}] ${message.topic}`,
        formatPayload(message.payloadText),
      ].join('\n'))
      .join('\n\n'),
  )
}

function clearMessages() {
  monitor.clear(selectedSn.value, selectedTopic.value?.topic)
}

async function copyText(text: string) {
  try {
    await navigator.clipboard.writeText(text)
  } catch {
    window.alert('复制失败。')
  }
}

function formatPayload(payloadText: string): string {
  try {
    return JSON.stringify(JSON.parse(payloadText), null, 2)
  } catch {
    return payloadText
  }
}

function formatTime(value: string): string {
  const date = new Date(value)
  if (Number.isNaN(date.getTime())) {
    return value
  }

  return date.toLocaleString()
}

function messageKey(message: MqttRuntimeMessage): string {
  return `${message.connectionId}:${message.topic}:${message.receivedAt}:${message.payloadText.length}`
}
</script>

<style scoped>
.raw-json-body {
  display: grid;
  align-content: start;
  gap: 8px;
}

.message-item {
  min-width: 0;
  border: 1px solid #edf0f4;
  border-radius: 6px;
  overflow: hidden;
}

.message-meta {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 8px;
  padding: 6px 8px;
  background: #f7f9fc;
  color: #5e6673;
  font-size: 12px;
}

.message-meta span:last-child {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

pre {
  margin: 0;
  padding: 8px;
  overflow: auto;
  color: #20242a;
  font-family: "SFMono-Regular", Consolas, monospace;
  font-size: 12px;
  line-height: 1.5;
  white-space: pre-wrap;
  word-break: break-word;
}
</style>
