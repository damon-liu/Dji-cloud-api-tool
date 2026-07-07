<template>
  <div class="shell">
    <header class="toolbar">
      <button type="button">配置</button>
      <span class="toolbar-spacer" />
      <span class="broker-label">{{ brokerLabel }}</span>
      <button type="button" class="primary">连接</button>
      <button type="button">断开</button>
    </header>

    <main class="workspace">
      <aside class="left-column">
        <DeviceTree />
        <TopicList />
      </aside>

      <section class="right-column">
        <div class="monitor-grid">
          <div class="analysis-column">
            <OsdPanel />
            <ParsedJsonPanel />
          </div>
          <RawJsonPanel />
        </div>
        <PublishPanel />
      </section>
    </main>

    <footer class="statusbar">
      <span>{{ connectionStatus }}</span>
      <span>DJI Cloud API Tool Next</span>
      <span>设备: {{ devices.devices.length }}</span>
    </footer>
  </div>
</template>

<script setup lang="ts">
import { listen, type UnlistenFn } from '@tauri-apps/api/event'
import { computed, onMounted, onUnmounted } from 'vue'
import DeviceTree from './DeviceTree.vue'
import OsdPanel from './OsdPanel.vue'
import ParsedJsonPanel from './ParsedJsonPanel.vue'
import PublishPanel from './PublishPanel.vue'
import RawJsonPanel from './RawJsonPanel.vue'
import TopicList from './TopicList.vue'
import { useConnectionStore } from '../stores/connectionStore'
import { useDeviceStore } from '../stores/deviceStore'
import { useMonitorStore } from '../stores/monitorStore'
import type { MqttRuntimeMessage } from '../types/domain'

const connections = useConnectionStore()
const devices = useDeviceStore()
const monitor = useMonitorStore()
const unlisteners: UnlistenFn[] = []

const brokerLabel = computed(() => connections.connected ? '已连接' : '未连接')
const connectionStatus = computed(() => {
  if (connections.error) {
    return `错误: ${connections.error}`
  }

  return connections.connected ? '已连接' : '未连接'
})

onMounted(async () => {
  unlisteners.push(
    await listen<MqttRuntimeMessage>('mqtt:message', (event) => {
      const message = normalizeMqttMessage(event.payload)
      if (message) {
        monitor.append(message)
      }
    }),
    await listen('mqtt:connected', () => {
      connections.connected = true
      connections.error = undefined
    }),
    await listen('mqtt:disconnected', () => {
      connections.connected = false
    }),
    await listen<string | { message?: string; error?: string }>('mqtt:error', (event) => {
      connections.connected = false
      connections.error = normalizeError(event.payload)
    }),
  )
})

onUnmounted(() => {
  for (const unlisten of unlisteners.splice(0)) {
    unlisten()
  }
})

function normalizeMqttMessage(payload: unknown): MqttRuntimeMessage | undefined {
  if (payload === null || typeof payload !== 'object') {
    return undefined
  }

  const source = payload as Partial<MqttRuntimeMessage>
  if (
    typeof source.connectionId !== 'string' ||
    typeof source.topic !== 'string' ||
    typeof source.payloadText !== 'string' ||
    typeof source.receivedAt !== 'string'
  ) {
    return undefined
  }

  return {
    connectionId: source.connectionId,
    topic: source.topic,
    payloadText: source.payloadText,
    receivedAt: source.receivedAt,
    deviceSn: typeof source.deviceSn === 'string' ? source.deviceSn : undefined,
  }
}

function normalizeError(payload: unknown): string {
  if (typeof payload === 'string') {
    return payload
  }

  if (payload && typeof payload === 'object') {
    const source = payload as { message?: unknown; error?: unknown }
    if (typeof source.message === 'string') {
      return source.message
    }
    if (typeof source.error === 'string') {
      return source.error
    }
  }

  return 'MQTT 连接异常'
}
</script>
