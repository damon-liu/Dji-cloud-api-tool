<template>
  <section class="panel device-tree">
    <header class="panel-header">
      <h2>设备列表</h2>
      <button type="button" title="添加设备" @click="addDevice">+</button>
      <button
        type="button"
        title="删除设备"
        :disabled="!devices.selectedSn"
        @click="removeSelectedDevice"
      >
        x
      </button>
    </header>
    <div v-if="devices.devices.length === 0" class="panel-body empty">暂无设备，请添加设备</div>
    <div v-else class="panel-body">
      <ul class="device-list">
        <li v-for="device in topLevelDevices" :key="device.sn">
          <button
            type="button"
            class="device-item"
            :class="{ selected: devices.selectedSn === device.sn }"
            @click="selectDevice(device.sn)"
          >
            <span class="device-type">{{ device.type === 'dock' ? 'Dock' : 'Aircraft' }}</span>
            <span class="device-name">{{ device.name }}</span>
            <span class="device-sn">{{ device.sn }}</span>
          </button>

          <ul v-if="childDevices(device.sn).length > 0" class="device-list child-list">
            <li v-for="child in childDevices(device.sn)" :key="child.sn">
              <button
                type="button"
                class="device-item"
                :class="{ selected: devices.selectedSn === child.sn }"
                @click="selectDevice(child.sn)"
              >
                <span class="device-type">Aircraft</span>
                <span class="device-name">{{ child.name }}</span>
                <span class="device-sn">{{ child.sn }}</span>
              </button>
            </li>
          </ul>
        </li>
      </ul>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useDeviceStore } from '../stores/deviceStore'
import { useTopicStore } from '../stores/topicStore'
import type { DeviceType } from '../types/domain'

const devices = useDeviceStore()
const topics = useTopicStore()

const topLevelDevices = computed(() => devices.topLevelDevices)

function childDevices(parentSn: string) {
  return devices.childDevices(parentSn)
}

function selectDevice(sn: string) {
  devices.selectedSn = sn
}

function promptText(message: string, defaultValue = ''): string | undefined {
  const value = window.prompt(message, defaultValue)
  if (value === null) {
    return undefined
  }

  const trimmed = value.trim()
  return trimmed || undefined
}

function promptDeviceType(): DeviceType | undefined {
  const value = promptText('请输入设备类型：dock 或 aircraft', 'dock')
  if (!value) {
    return undefined
  }

  if (value !== 'dock' && value !== 'aircraft') {
    window.alert('设备类型只能是 dock 或 aircraft。')
    return undefined
  }

  return value
}

function addDevice() {
  const selectedDevice = devices.selectedDevice
  const parentSn = selectedDevice?.type === 'dock' ? selectedDevice.sn : undefined
  const type = parentSn ? 'aircraft' : promptDeviceType()
  if (!type) {
    return
  }

  const sn = promptText('请输入设备 SN')
  if (!sn) {
    return
  }

  const name = promptText('请输入设备名称', sn) ?? sn

  try {
    devices.addDevice(sn, name, type, parentSn)
    topics.addDefaultTopic(sn)
    devices.selectedSn = sn
  } catch (error) {
    window.alert(error instanceof Error ? error.message : '添加设备失败。')
  }
}

function collectDeviceSns(sn: string): string[] {
  const sns = new Set<string>([sn])
  let changed = true

  while (changed) {
    changed = false
    for (const device of devices.devices) {
      if (device.parentSn && sns.has(device.parentSn) && !sns.has(device.sn)) {
        sns.add(device.sn)
        changed = true
      }
    }
  }

  return Array.from(sns)
}

function removeTopicsForDevices(deviceSns: string[]) {
  for (const deviceSn of deviceSns) {
    for (const topic of topics.topicsForDevice(deviceSn)) {
      topics.removeTopic(deviceSn, topic.topic)
    }
  }
}

function removeSelectedDevice() {
  const selectedSn = devices.selectedSn
  if (!selectedSn) {
    return
  }

  const selectedDevice = devices.selectedDevice
  const label = selectedDevice ? `${selectedDevice.name} (${selectedDevice.sn})` : selectedSn
  if (!window.confirm(`确认删除设备 ${label}？`)) {
    return
  }

  const removedSns = collectDeviceSns(selectedSn)
  removeTopicsForDevices(removedSns)
  devices.removeDevice(selectedSn)
}
</script>

<style scoped>
.device-list {
  display: grid;
  gap: 4px;
  margin: 0;
  padding: 0;
  list-style: none;
}

.child-list {
  margin-top: 4px;
  padding-left: 16px;
}

.device-item {
  width: 100%;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 2px 8px;
  align-items: center;
  padding: 6px 8px;
  text-align: left;
  border-radius: 6px;
}

.device-item.selected {
  border-color: #1f6feb;
  background: #eef5ff;
}

.device-type {
  grid-row: span 2;
  color: #5e6673;
  font-size: 11px;
  text-transform: uppercase;
}

.device-name,
.device-sn {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.device-sn {
  color: #77808c;
  font-size: 12px;
}
</style>
