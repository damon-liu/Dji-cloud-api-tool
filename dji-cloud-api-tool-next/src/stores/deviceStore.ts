import { defineStore } from 'pinia'
import { tauriApi } from '../services/tauriApi'
import type { Device, DeviceType } from '../types/domain'

type DeviceState = {
  devices: Device[]
  selectedSn?: string
  loading: boolean
  saving: boolean
  error?: string
  saveQueue: Promise<void>
}

export const useDeviceStore = defineStore('devices', {
  state: (): DeviceState => ({
    devices: [],
    selectedSn: undefined,
    loading: false,
    saving: false,
    error: undefined,
    saveQueue: Promise.resolve(),
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
    async load() {
      this.loading = true
      this.error = undefined

      try {
        this.devices = await tauriApi.loadDevices()
        if (this.selectedSn && !this.devices.some((device) => device.sn === this.selectedSn)) {
          this.selectedSn = undefined
        }
        if (!this.selectedSn) {
          this.selectedSn = this.devices[0]?.sn
        }
      } catch (error) {
        this.error = error instanceof Error ? error.message : '加载设备失败。'
      } finally {
        this.loading = false
      }
    },
    async save() {
      this.saving = true
      this.error = undefined

      try {
        await tauriApi.saveDevices(this.devices)
      } catch (error) {
        this.error = error instanceof Error ? error.message : '保存设备失败。'
        throw error
      } finally {
        this.saving = false
      }
    },
    queueSave() {
      const snapshot = this.devices.map((device) => ({ ...device }))
      const runSave = async () => {
        this.saving = true
        this.error = undefined

        try {
          await tauriApi.saveDevices(snapshot)
        } catch (error) {
          this.error = error instanceof Error ? error.message : '保存设备失败。'
          throw error
        } finally {
          this.saving = false
        }
      }

      const queued = this.saveQueue.then(runSave, runSave)
      this.saveQueue = queued.catch(() => undefined)
      return queued
    },
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
      return this.queueSave()
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
      return this.queueSave()
    },
  },
})
