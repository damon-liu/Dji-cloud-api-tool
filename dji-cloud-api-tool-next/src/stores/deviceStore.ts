import { defineStore } from 'pinia'
import type { Device } from '../types/domain'

type DeviceState = {
  devices: Device[]
  selectedSn?: string
}

export const useDeviceStore = defineStore('device', {
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
    addDevice(device: Device) {
      const index = this.devices.findIndex((item) => item.sn === device.sn)
      if (index === -1) {
        this.devices.push(device)
        return
      }

      this.devices[index] = device
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
