import { defineStore } from 'pinia'
import type { Device, DeviceType } from '../types/domain'

type DeviceState = {
  devices: Device[]
  selectedSn?: string
}

export const useDeviceStore = defineStore('devices', {
  state: (): DeviceState => ({
    devices: [],
    selectedSn: undefined,
  }),
  getters: {
    selectedDevice: (state): Device | undefined =>
      state.devices.find((device) => device.sn === state.selectedSn),
    topLevelDevices: (state): Device[] =>
      state.devices.filter((device) => device.parentSn === undefined),
    childDevices: (state): ((parentSn: string) => Device[]) => {
      return (parentSn: string) =>
        state.devices.filter((device) => device.parentSn === parentSn)
    },
  },
  actions: {
    addDevice(sn: string, name: string, type: DeviceType, parentSn?: string) {
      const trimmedSn = sn.trim()
      if (!trimmedSn) {
        throw new Error('设备 SN 不能为空。')
      }

      if (this.devices.some((device) => device.sn === trimmedSn)) {
        throw new Error('设备 SN 已存在。')
      }

      this.devices.push({
        sn: trimmedSn,
        name: name.trim() || trimmedSn,
        type,
        parentSn,
        online: false,
      })
    },
    removeDevice(sn: string) {
      const descendants = new Set<string>([sn])
      let changed = true

      while (changed) {
        changed = false
        for (const device of this.devices) {
          if (device.parentSn && descendants.has(device.parentSn) && !descendants.has(device.sn)) {
            descendants.add(device.sn)
            changed = true
          }
        }
      }

      this.devices = this.devices.filter((device) => !descendants.has(device.sn))
      if (this.selectedSn && descendants.has(this.selectedSn)) {
        this.selectedSn = undefined
      }
    },
  },
})
