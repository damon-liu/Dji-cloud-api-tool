<template>
  <section class="panel topic-list">
    <header class="panel-header">
      <h2>Topic 列表</h2>
      <button type="button" title="添加 Topic" :disabled="!selectedSn" @click="addTopic">+</button>
      <button type="button" title="启用/禁用全部" :disabled="!selectedSn || deviceTopics.length === 0" @click="toggleAll">
        all
      </button>
    </header>
    <div v-if="!selectedSn" class="panel-body empty">请选择设备</div>
    <div v-else-if="deviceTopics.length === 0" class="panel-body empty">暂无 Topic，请添加 Topic</div>
    <div v-else class="panel-body">
      <ul class="topic-items">
        <li v-for="(topic, index) in deviceTopics" :key="topic.id" class="topic-row">
          <button
            type="button"
            class="topic-main"
            :class="{ selected: selectedTopic === topic.topic }"
            :title="topic.topic"
            @click="selectTopic(topic.topic)"
            @dblclick="copyTopic(topic.topic)"
          >
            <span class="topic-status">{{ topic.enabled ? '●' : '○' }}</span>
            <span class="topic-text">{{ topic.topic }}</span>
          </button>
          <button type="button" title="启用/禁用" @click="toggleTopic(topic.topic)">切</button>
          <button type="button" title="上移" :disabled="index === 0" @click="moveTopic(topic.topic, 'up')">↑</button>
          <button
            type="button"
            title="下移"
            :disabled="index === deviceTopics.length - 1"
            @click="moveTopic(topic.topic, 'down')"
          >
            ↓
          </button>
          <button type="button" title="删除 Topic" @click="removeTopic(topic.topic)">x</button>
        </li>
      </ul>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useDeviceStore } from '../stores/deviceStore'
import { useTopicStore } from '../stores/topicStore'

type MoveDirection = 'up' | 'down'

const devices = useDeviceStore()
const topics = useTopicStore()
const selectedTopic = ref<string>()

const selectedSn = computed(() => devices.selectedSn)
const deviceTopics = computed(() => (selectedSn.value ? topics.topicsForDevice(selectedSn.value) : []))

watch([selectedSn, deviceTopics], () => {
  if (!deviceTopics.value.some((topic) => topic.topic === selectedTopic.value)) {
    selectedTopic.value = deviceTopics.value[0]?.topic
  }
})

function promptText(message: string, defaultValue = ''): string | undefined {
  const value = window.prompt(message, defaultValue)
  if (value === null) {
    return undefined
  }

  const trimmed = value.trim()
  return trimmed || undefined
}

function addTopic() {
  const deviceSn = selectedSn.value
  if (!deviceSn) {
    return
  }

  const topic = promptText('请输入 Topic', `thing/product/${deviceSn}/osd`)
  if (!topic) {
    return
  }

  try {
    topics.addTopic(deviceSn, topic)
    selectedTopic.value = topic
  } catch (error) {
    window.alert(error instanceof Error ? error.message : '添加 Topic 失败。')
  }
}

function selectTopic(topic: string) {
  selectedTopic.value = topic
}

function toggleTopic(topic: string) {
  const deviceSn = selectedSn.value
  if (deviceSn) {
    topics.toggleTopic(deviceSn, topic)
    selectedTopic.value = topic
  }
}

function removeTopic(topic: string) {
  const deviceSn = selectedSn.value
  if (!deviceSn || !window.confirm(`确认删除 Topic ${topic}？`)) {
    return
  }

  topics.removeTopic(deviceSn, topic)
}

function moveTopic(topic: string, direction: MoveDirection) {
  const deviceSn = selectedSn.value
  if (deviceSn) {
    topics.moveTopic(deviceSn, topic, direction)
    selectedTopic.value = topic
  }
}

function toggleAll() {
  const deviceSn = selectedSn.value
  if (!deviceSn) {
    return
  }

  const shouldEnable = deviceTopics.value.some((topic) => !topic.enabled)
  topics.setAllEnabled(deviceSn, shouldEnable)
}

async function copyTopic(topic: string) {
  selectedTopic.value = topic

  try {
    await navigator.clipboard.writeText(topic)
  } catch {
    window.alert('复制 Topic 失败。')
  }
}
</script>

<style scoped>
.topic-items {
  display: grid;
  gap: 4px;
  margin: 0;
  padding: 0;
  list-style: none;
}

.topic-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) repeat(4, 28px);
  gap: 4px;
  align-items: center;
}

.topic-row > button:not(.topic-main) {
  width: 28px;
  height: 28px;
  padding: 0;
  line-height: 1;
}

.topic-main {
  min-width: 0;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 6px;
  align-items: center;
  padding: 5px 8px;
  text-align: left;
}

.topic-main.selected {
  border-color: #1f6feb;
  background: #eef5ff;
}

.topic-status {
  color: #1f6feb;
  font-size: 12px;
  line-height: 1;
}

.topic-text {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>
