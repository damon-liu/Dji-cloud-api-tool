import { beforeEach, describe, expect, it } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useMonitorStore } from '../stores/monitorStore'
import type { MqttRuntimeMessage } from '../types/domain'

function message(overrides: Partial<MqttRuntimeMessage>): MqttRuntimeMessage {
  return {
    connectionId: 'connection_001',
    topic: 'thing/product/dock_001/osd',
    payloadText: '{}',
    receivedAt: '2026-07-07T00:00:00.000Z',
    ...overrides,
  }
}

describe('monitorStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('filters message history by deviceSn and topic', () => {
    const store = useMonitorStore()
    store.append(message({ deviceSn: 'dock_001', topic: 'thing/product/dock_001/osd' }))
    store.append(message({ deviceSn: 'dock_001', topic: 'thing/product/dock_001/state' }))
    store.append(message({ deviceSn: 'dock_002', topic: 'thing/product/dock_002/osd' }))

    expect(store.history().map((item) => item.topic)).toEqual([
      'thing/product/dock_001/osd',
      'thing/product/dock_001/state',
      'thing/product/dock_002/osd',
    ])
    expect(store.history('dock_001').map((item) => item.topic)).toEqual([
      'thing/product/dock_001/osd',
      'thing/product/dock_001/state',
    ])
    expect(store.history(undefined, 'thing/product/dock_002/osd').map((item) => item.deviceSn)).toEqual([
      'dock_002',
    ])
    expect(store.history('dock_001', 'thing/product/dock_001/osd').map((item) => item.topic)).toEqual([
      'thing/product/dock_001/osd',
    ])
  })

  it('selectively clears messages by deviceSn and topic', () => {
    const store = useMonitorStore()
    store.append(message({ deviceSn: 'dock_001', topic: 'thing/product/dock_001/osd' }))
    store.append(message({ deviceSn: 'dock_001', topic: 'thing/product/dock_001/state' }))
    store.append(message({ deviceSn: 'dock_002', topic: 'thing/product/dock_001/osd' }))
    store.append(message({ deviceSn: 'dock_002', topic: 'thing/product/dock_002/osd' }))

    store.clear('dock_001', 'thing/product/dock_001/osd')
    expect(store.history().map((item) => item.deviceSn)).toEqual(['dock_001', 'dock_002', 'dock_002'])
    expect(store.history('dock_001').map((item) => item.topic)).toEqual(['thing/product/dock_001/state'])

    store.clear(undefined, 'thing/product/dock_001/osd')
    expect(store.history().map((item) => item.topic)).toEqual([
      'thing/product/dock_001/state',
      'thing/product/dock_002/osd',
    ])

    store.clear('dock_001')
    expect(store.history().map((item) => item.deviceSn)).toEqual(['dock_002'])

    store.clear()
    expect(store.history()).toEqual([])
  })
})
