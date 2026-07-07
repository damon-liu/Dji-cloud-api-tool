<template>
  <section class="panel publish-panel" :class="{ collapsed }">
    <header class="panel-header">
      <h2>Topic 下发</h2>
      <button type="button" :title="collapsed ? '展开' : '折叠'" @click="collapsed = !collapsed">
        {{ collapsed ? '▴' : '▾' }}
      </button>
    </header>
    <div v-if="!collapsed" class="panel-body publish-body">
      <div v-if="!selectedSn" class="empty publish-empty">请选择设备</div>
      <form v-else class="publish-form" @submit.prevent="publish">
        <label class="field sn-field">
          <span>设备 SN</span>
          <input :value="selectedSn ?? ''" readonly placeholder="未选择设备" />
        </label>

        <label class="field topic-field">
          <span>Topic</span>
          <select v-model="selectedTopic" :disabled="deviceTopics.length === 0">
            <option value="" disabled>{{ deviceTopics.length === 0 ? '暂无设备 Topic' : '请选择 Topic' }}</option>
            <option v-for="topic in deviceTopics" :key="topic.id" :value="topic.topic">
              {{ topic.topic }}
            </option>
          </select>
        </label>

        <label class="field custom-topic-field">
          <span>自定义 Topic</span>
          <input
            v-model="customTopic"
            data-test="publish-custom-topic"
            placeholder="填写后优先使用"
            autocomplete="off"
          />
        </label>

        <label class="field payload-field">
          <span>JSON Payload</span>
          <textarea
            v-model="payloadText"
            data-test="publish-payload"
            spellcheck="false"
            rows="4"
          />
        </label>

        <div class="publish-actions">
          <p v-if="statusMessage" class="publish-status" :class="statusKind">{{ statusMessage }}</p>
          <span v-else />
          <button
            type="submit"
            class="primary"
            data-test="publish-send"
            :disabled="!effectiveTopic || publishing"
          >
            {{ publishing ? '发送中' : '发送' }}
          </button>
        </div>
      </form>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { tauriApi } from '../services/tauriApi'
import { useDeviceStore } from '../stores/deviceStore'
import { useTopicStore } from '../stores/topicStore'

const devices = useDeviceStore()
const topics = useTopicStore()

const collapsed = ref(false)
const customTopic = ref('')
const payloadText = ref('{}')
const publishing = ref(false)
const selectedTopic = ref('')
const statusMessage = ref('')
const statusKind = ref<'success' | 'error'>('success')

const selectedSn = computed(() => devices.selectedSn)
const deviceTopics = computed(() => {
  const sn = selectedSn.value
  return sn ? topics.topicsForDevice(sn) : []
})
const preferredTopic = computed(() => {
  const sn = selectedSn.value
  return sn ? topics.selectedTopicForDevice(sn)?.topic : ''
})
const effectiveTopic = computed(() => customTopic.value.trim() || selectedTopic.value.trim())

watch(
  [selectedSn, preferredTopic],
  () => {
    selectedTopic.value = preferredTopic.value || ''
    clearStatus()
  },
  { immediate: true },
)

watch([customTopic, selectedTopic, payloadText], clearStatus)

async function publish() {
  const topic = effectiveTopic.value
  if (!selectedSn.value || !topic || publishing.value) {
    return
  }

  try {
    JSON.parse(payloadText.value)
  } catch (error) {
    showStatus('error', `JSON 格式错误: ${error instanceof Error ? error.message : String(error)}`)
    return
  }

  publishing.value = true
  clearStatus()

  try {
    await tauriApi.publishMessage(topic, payloadText.value)
    showStatus('success', '下发成功')
  } catch (error) {
    showStatus('error', error instanceof Error ? error.message : String(error))
  } finally {
    publishing.value = false
  }
}

function showStatus(kind: 'success' | 'error', message: string) {
  statusKind.value = kind
  statusMessage.value = message
}

function clearStatus() {
  statusMessage.value = ''
}
</script>

<style scoped>
.publish-panel {
  min-height: 220px;
}

.publish-panel.collapsed {
  min-height: 0;
  grid-template-rows: auto;
}

.publish-body {
  display: grid;
  gap: 8px;
  overflow: auto;
}

.publish-empty {
  padding: 2px 0;
}

.publish-form {
  display: grid;
  grid-template-columns: minmax(160px, 220px) minmax(220px, 1fr) minmax(220px, 1fr) auto;
  gap: 8px;
  align-items: end;
}

.field {
  min-width: 0;
  display: grid;
  gap: 4px;
}

.field span {
  color: #5e6673;
  font-size: 12px;
}

.field input,
.field select,
.field textarea {
  width: 100%;
  min-width: 0;
  box-sizing: border-box;
  border: 1px solid #cfd6df;
  border-radius: 6px;
  background: #ffffff;
  color: #20242a;
  padding: 6px 8px;
}

.field input[readonly],
.field select:disabled {
  background: #f7f9fc;
  color: #77808c;
}

.payload-field {
  grid-column: 1 / -1;
}

.payload-field textarea {
  min-height: 92px;
  resize: vertical;
  font-family: "SFMono-Regular", Consolas, monospace;
  font-size: 12px;
  line-height: 1.5;
}

.publish-actions {
  grid-column: 1 / -1;
  min-width: 0;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 8px;
  align-items: center;
}

.publish-status {
  min-width: 0;
  margin: 0;
  overflow-wrap: anywhere;
}

.publish-status.success {
  color: #1f7a4d;
}

.publish-status.error {
  color: #b42318;
}

@media (max-width: 900px) {
  .publish-form {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
