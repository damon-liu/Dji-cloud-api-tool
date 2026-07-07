import { beforeEach, describe, expect, it } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useDeviceStore } from '../stores/deviceStore'

describe('deviceStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('rejects blank device SN', () => {
    const store = useDeviceStore()

    expect(() => store.addDevice('   ', 'Dock 1', 'dock')).toThrow('设备 SN 不能为空。')
  })

  it('rejects duplicate device SN', () => {
    const store = useDeviceStore()
    store.addDevice('dock_001', 'Dock 1', 'dock')

    expect(() => store.addDevice('dock_001', 'Dock 1 duplicate', 'dock')).toThrow('设备 SN 已存在。')
  })

  it('adds trimmed devices with fallback name and removes child devices', () => {
    const store = useDeviceStore()
    store.addDevice(' dock_001 ', '  ', 'dock')
    store.addDevice('aircraft_001', ' Aircraft 1 ', 'aircraft', 'dock_001')
    store.selectedSn = 'aircraft_001'

    expect(store.devices).toEqual([
      {
        sn: 'dock_001',
        name: 'dock_001',
        type: 'dock',
        parentSn: undefined,
        online: false,
      },
      {
        sn: 'aircraft_001',
        name: 'Aircraft 1',
        type: 'aircraft',
        parentSn: 'dock_001',
        online: false,
      },
    ])

    store.removeDevice('dock_001')

    expect(store.devices).toEqual([])
    expect(store.selectedSn).toBeUndefined()
  })
})
