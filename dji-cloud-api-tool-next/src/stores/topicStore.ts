import { defineStore } from 'pinia'
import type { DeviceTopic } from '../types/domain'

type TopicState = {
  topics: DeviceTopic[]
  selectedByDevice: Record<string, string | undefined>
}

type MoveDirection = 'up' | 'down'

function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? Math.random().toString(36).slice(2)
}

function ordered(topics: DeviceTopic[]): DeviceTopic[] {
  return [...topics].sort((left, right) => left.order - right.order)
}

export const useTopicStore = defineStore('topics', {
  state: (): TopicState => ({
    topics: [],
    selectedByDevice: {},
  }),
  getters: {
    topicsForDevice: (state): ((deviceSn: string) => DeviceTopic[]) => {
      return (deviceSn: string) => ordered(state.topics.filter((topic) => topic.deviceSn === deviceSn))
    },
    enabledTopics: (state): DeviceTopic[] => ordered(state.topics.filter((topic) => topic.enabled)),
    selectedTopicForDevice: (state): ((deviceSn: string) => DeviceTopic | undefined) => {
      return (deviceSn: string) => {
        const deviceTopics = ordered(state.topics.filter((topic) => topic.deviceSn === deviceSn))
        const selectedTopic = state.selectedByDevice[deviceSn]
        const selected = deviceTopics.find((topic) => topic.topic === selectedTopic)

        return selected ?? deviceTopics[0]
      }
    },
  },
  actions: {
    addDefaultTopic(deviceSn: string) {
      this.addTopic(deviceSn, `thing/product/${deviceSn}/osd`)
    },
    addTopic(deviceSn: string, topic: string) {
      if (this.findTopic(deviceSn, topic)) {
        throw new Error('该 Topic 已存在。')
      }

      this.topics.push({
        id: createId(),
        deviceSn,
        topic,
        enabled: true,
        order: this.topicsForDevice(deviceSn).length,
      })
      this.normalizeOrder(deviceSn)
      this.ensureSelectedTopic(deviceSn)
    },
    removeTopic(deviceSn: string, topic: string) {
      const found = this.findTopic(deviceSn, topic)
      if (!found) {
        return
      }

      this.topics = this.topics.filter((item) => item !== found)
      this.normalizeOrder(deviceSn)
      this.ensureSelectedTopic(deviceSn)
    },
    toggleTopic(deviceSn: string, topic: string) {
      const found = this.findTopic(deviceSn, topic)
      if (found) {
        found.enabled = !found.enabled
      }
    },
    setAllEnabled(deviceSn: string, enabled: boolean) {
      for (const topic of this.topics) {
        if (topic.deviceSn === deviceSn) {
          topic.enabled = enabled
        }
      }
    },
    moveTopic(deviceSn: string, topic: string, direction: MoveDirection) {
      const topics = this.topicsForDevice(deviceSn)
      const index = topics.findIndex((item) => item.topic === topic)
      if (index === -1) {
        return
      }

      const targetIndex = direction === 'up' ? index - 1 : index + 1
      if (targetIndex < 0 || targetIndex >= topics.length) {
        return
      }

      const [movedTopic] = topics.splice(index, 1)
      topics.splice(targetIndex, 0, movedTopic)
      topics.forEach((item, order) => {
        const source = this.topics.find((candidate) => candidate.id === item.id)
        if (source) {
          source.order = order
        }
      })
      this.normalizeOrder(deviceSn)
    },
    findTopic(deviceSn: string, topic: string): DeviceTopic | undefined {
      return this.topics.find((item) => item.deviceSn === deviceSn && item.topic === topic)
    },
    selectTopic(deviceSn: string, topic: string) {
      if (this.findTopic(deviceSn, topic)) {
        this.selectedByDevice[deviceSn] = topic
      }
    },
    ensureSelectedTopic(deviceSn: string) {
      const deviceTopics = this.topicsForDevice(deviceSn)
      const selectedTopic = this.selectedByDevice[deviceSn]
      if (selectedTopic && deviceTopics.some((topic) => topic.topic === selectedTopic)) {
        return
      }

      this.selectedByDevice[deviceSn] = deviceTopics[0]?.topic
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
