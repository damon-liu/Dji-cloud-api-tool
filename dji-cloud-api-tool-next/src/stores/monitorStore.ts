import { defineStore } from 'pinia'
import type { MqttRuntimeMessage } from '../types/domain'

export const MAX_HISTORY = 500

type MonitorState = {
  messages: MqttRuntimeMessage[]
  paused: boolean
}

export const useMonitorStore = defineStore('monitor', {
  state: (): MonitorState => ({
    messages: [],
    paused: false,
  }),
  actions: {
    append(message: MqttRuntimeMessage) {
      if (this.paused) {
        return
      }

      this.messages.push(message)
      if (this.messages.length > MAX_HISTORY) {
        this.messages.splice(0, this.messages.length - MAX_HISTORY)
      }
    },
    history(deviceSn?: string, topic?: string): MqttRuntimeMessage[] {
      return this.messages.filter((message) => matchesMessage(message, deviceSn, topic))
    },
    clear(deviceSn?: string, topic?: string) {
      if (deviceSn === undefined && topic === undefined) {
        this.messages = []
        return
      }

      this.messages = this.messages.filter((message) => !matchesMessage(message, deviceSn, topic))
    },
  },
})

function matchesMessage(message: MqttRuntimeMessage, deviceSn?: string, topic?: string): boolean {
  if (deviceSn !== undefined && message.deviceSn !== deviceSn) {
    return false
  }

  if (topic !== undefined && message.topic !== topic) {
    return false
  }

  return true
}
