import { defineStore } from 'pinia'
import type { DeviceTopic } from '../types/domain'

type TopicState = {
  topics: DeviceTopic[]
}

type MoveDirection = 'up' | 'down'

function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? Math.random().toString(36).slice(2)
}

function ordered(topics: DeviceTopic[]): DeviceTopic[] {
  return [...topics].sort((left, right) => left.order - right.order)
}

export const useTopicStore = defineStore('topic', {
  state: (): TopicState => ({
    topics: [],
  }),
  getters: {
    topicsForDevice: (state): ((deviceSn: string) => DeviceTopic[]) => {
      return (deviceSn: string) => ordered(state.topics.filter((topic) => topic.deviceSn === deviceSn))
    },
    enabledTopics: (state): DeviceTopic[] => ordered(state.topics.filter((topic) => topic.enabled)),
  },
  actions: {
    addDefaultTopic(deviceSn: string) {
      this.addTopic({
        id: createId(),
        deviceSn,
        topic: `thing/product/${deviceSn}/osd`,
        enabled: true,
        order: this.topicsForDevice(deviceSn).length,
      })
    },
    addTopic(topic: DeviceTopic) {
      this.topics.push(topic)
      this.normalizeOrder(topic.deviceSn)
    },
    removeTopic(id: string) {
      const topic = this.topics.find((item) => item.id === id)
      if (!topic) {
        return
      }

      this.topics = this.topics.filter((item) => item.id !== id)
      this.normalizeOrder(topic.deviceSn)
    },
    toggleTopic(id: string, enabled?: boolean) {
      const topic = this.topics.find((item) => item.id === id)
      if (topic) {
        topic.enabled = enabled ?? !topic.enabled
      }
    },
    setAllEnabled(enabled: boolean, deviceSn?: string) {
      for (const topic of this.topics) {
        if (deviceSn === undefined || topic.deviceSn === deviceSn) {
          topic.enabled = enabled
        }
      }
    },
    moveTopic(deviceSn: string, id: string, direction: MoveDirection) {
      const topics = this.topicsForDevice(deviceSn)
      const index = topics.findIndex((topic) => topic.id === id)
      if (index === -1) {
        return
      }

      const targetIndex = direction === 'up' ? index - 1 : index + 1
      if (targetIndex < 0 || targetIndex >= topics.length) {
        return
      }

      const [topic] = topics.splice(index, 1)
      topics.splice(targetIndex, 0, topic)
      topics.forEach((item, order) => {
        const source = this.topics.find((candidate) => candidate.id === item.id)
        if (source) {
          source.order = order
        }
      })
      this.normalizeOrder(deviceSn)
    },
    normalizeOrder(deviceSn?: string) {
      const deviceSns = deviceSn
        ? [deviceSn]
        : Array.from(new Set(this.topics.map((topic) => topic.deviceSn)))

      for (const sn of deviceSns) {
        ordered(this.topics.filter((topic) => topic.deviceSn === sn)).forEach((topic, index) => {
          topic.order = index
        })
      }
    },
  },
})
