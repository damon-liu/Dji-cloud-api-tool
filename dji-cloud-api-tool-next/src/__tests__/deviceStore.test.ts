import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useDeviceStore } from '../stores/deviceStore'
import { tauriApi } from '../services/tauriApi'

vi.mock('../services/tauriApi', () => ({
  tauriApi: {
    loadDevices: vi.fn(),
    saveDevices: vi.fn(),
  },
}))

const loadDevices = vi.mocked(tauriApi.loadDevices)
const saveDevices = vi.mocked(tauriApi.saveDevices)

describe('deviceStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
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

  it('loads devices and selects the first device when needed', async () => {
    loadDevices.mockResolvedValue([{
      sn: 'dock_001',
      name: 'Dock 1',
      type: 'dock',
      online: true,
    }])

    const store = useDeviceStore()
    await store.load()

    expect(store.devices).toEqual([{
      sn: 'dock_001',
      name: 'Dock 1',
      type: 'dock',
      online: true,
    }])
    expect(store.selectedSn).toBe('dock_001')
  })

  it('persists device mutations', async () => {
    saveDevices.mockResolvedValue()

    const store = useDeviceStore()
    await store.addDevice('dock_001', 'Dock 1', 'dock')
    await store.removeDevice('dock_001')

    expect(saveDevices).toHaveBeenCalledTimes(2)
    expect(saveDevices).toHaveBeenLastCalledWith([])
  })

  it('serializes device saves in mutation order', async () => {
    const calls: string[][] = []
    let releaseFirstSave: (() => void) | undefined
    saveDevices.mockImplementation((devices) => {
      calls.push(devices.map((device) => device.sn))
      if (calls.length === 1) {
        return new Promise<void>((resolve) => {
          releaseFirstSave = resolve
        })
      }

      return Promise.resolve()
    })

    const store = useDeviceStore()
    const firstSave = store.addDevice('dock_001', 'Dock 1', 'dock')
    const secondSave = store.removeDevice('dock_001')

    await Promise.resolve()
    expect(calls).toEqual([['dock_001']])

    releaseFirstSave?.()
    await firstSave
    await secondSave

    expect(calls).toEqual([['dock_001'], []])
  })
})
