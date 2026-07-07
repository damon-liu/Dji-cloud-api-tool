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
  getters: {
    history: (state): MqttRuntimeMessage[] => state.messages,
  },
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
    clear() {
      this.messages = []
    },
  },
})
