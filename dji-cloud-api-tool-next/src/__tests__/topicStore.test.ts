import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useTopicStore } from '../stores/topicStore'
import { tauriApi } from '../services/tauriApi'

vi.mock('../services/tauriApi', () => ({
  tauriApi: {
    loadTopics: vi.fn(),
    saveTopics: vi.fn(),
  },
}))

const loadTopics = vi.mocked(tauriApi.loadTopics)
const saveTopics = vi.mocked(tauriApi.saveTopics)

describe('topicStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
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
    store.addTopic('dock_001', 'a')
    store.addTopic('dock_001', 'b')

    store.moveTopic('dock_001', 'a', 'down')

    const topics = store.topicsForDevice('dock_001')
    expect(topics.map((topic) => topic.topic)).toEqual(['b', 'a'])
    expect(topics.map((topic) => topic.order)).toEqual([0, 1])
  })

  it('rejects duplicate topics for the same device', () => {
    const store = useTopicStore()
    store.addTopic('dock_001', 'a')

    expect(() => store.addTopic('dock_001', 'a')).toThrow('该 Topic 已存在。')
  })

  it('shares selected topic per device', () => {
    const store = useTopicStore()
    store.addTopic('dock_001', 'a')
    store.addTopic('dock_001', 'b')
    store.addTopic('dock_002', 'c')

    store.selectTopic('dock_001', 'b')
    store.selectTopic('dock_002', 'c')

    expect(store.selectedTopicForDevice('dock_001')?.topic).toBe('b')
    expect(store.selectedTopicForDevice('dock_002')?.topic).toBe('c')
  })

  it('falls back when the selected topic is removed', () => {
    const store = useTopicStore()
    store.addTopic('dock_001', 'a')
    store.addTopic('dock_001', 'b')
    store.selectTopic('dock_001', 'b')

    store.removeTopic('dock_001', 'b')

    expect(store.selectedTopicForDevice('dock_001')?.topic).toBe('a')

    store.removeTopic('dock_001', 'a')

    expect(store.selectedTopicForDevice('dock_001')).toBeUndefined()
  })

  it('keeps disabled selected topics identifiable', () => {
    const store = useTopicStore()
    store.addTopic('dock_001', 'a')
    store.addTopic('dock_001', 'b')
    store.selectTopic('dock_001', 'b')

    store.toggleTopic('dock_001', 'b')

    expect(store.selectedTopicForDevice('dock_001')?.topic).toBe('b')
    expect(store.selectedTopicForDevice('dock_001')?.enabled).toBe(false)
  })

  it('loads topics without replacing selected topics when they still exist', async () => {
    loadTopics.mockResolvedValue([
      { id: 'a', deviceSn: 'dock_001', topic: 'a', enabled: true, order: 0 },
      { id: 'b', deviceSn: 'dock_001', topic: 'b', enabled: true, order: 1 },
    ])
    const store = useTopicStore()
    store.selectedByDevice.dock_001 = 'b'

    await store.load()

    expect(store.topicsForDevice('dock_001').map((topic) => topic.topic)).toEqual(['a', 'b'])
    expect(store.selectedTopicForDevice('dock_001')?.topic).toBe('b')
  })

  it('persists topic mutations but not plain selection changes', async () => {
    saveTopics.mockResolvedValue()
    const store = useTopicStore()

    await store.addTopic('dock_001', 'a')
    store.selectTopic('dock_001', 'a')
    await store.toggleTopic('dock_001', 'a')

    expect(saveTopics).toHaveBeenCalledTimes(2)
    expect(saveTopics).toHaveBeenLastCalledWith([{
      id: expect.any(String),
      deviceSn: 'dock_001',
      topic: 'a',
      enabled: false,
      order: 0,
    }])
  })

  it('serializes topic saves in mutation order', async () => {
    const calls: string[][] = []
    let releaseFirstSave: (() => void) | undefined
    saveTopics.mockImplementation((topics) => {
      calls.push(topics.map((topic) => topic.topic))
      if (calls.length === 1) {
        return new Promise<void>((resolve) => {
          releaseFirstSave = resolve
        })
      }

      return Promise.resolve()
    })

    const store = useTopicStore()
    const firstSave = store.addTopic('dock_001', 'a')
    const secondSave = store.removeTopic('dock_001', 'a')

    await Promise.resolve()
    expect(calls).toEqual([['a']])

    releaseFirstSave?.()
    await firstSave
    await secondSave

    expect(calls).toEqual([['a'], []])
  })
})
