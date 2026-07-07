import { beforeEach, describe, expect, it } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useTopicStore } from '../stores/topicStore'

describe('topicStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it("addDefaultTopic('dock_001') creates an enabled osd topic", () => {
    const store = useTopicStore()

    store.addDefaultTopic('dock_001')

    expect(store.topics).toHaveLength(1)
    expect(store.topics[0]).toMatchObject({
      deviceSn: 'dock_001',
      topic: 'thing/product/dock_001/osd',
      enabled: true,
      order: 0,
    })
  })

  it("moving topic 'a' down under dock_001 reorders sibling topics", () => {
    const store = useTopicStore()
    store.addTopic({
      id: 'a',
      deviceSn: 'dock_001',
      topic: 'thing/product/dock_001/osd',
      enabled: true,
      order: 0,
    })
    store.addTopic({
      id: 'b',
      deviceSn: 'dock_001',
      topic: 'thing/product/dock_001/state',
      enabled: true,
      order: 1,
    })

    store.moveTopic('dock_001', 'a', 'down')

    const topics = store.topicsForDevice('dock_001')
    expect(topics.map((topic) => topic.id)).toEqual(['b', 'a'])
    expect(topics.map((topic) => topic.order)).toEqual([0, 1])
  })
})
