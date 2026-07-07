import { defineStore } from 'pinia'
import { tauriApi } from '../services/tauriApi'
import type { ConnectionProfile } from '../types/domain'

type ConnectionState = {
  profiles: ConnectionProfile[]
  currentId?: string
  connected: boolean
  error?: string
  loading: boolean
  saving: boolean
}

function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? Math.random().toString(36).slice(2)
}

function createDefaultProfile(): ConnectionProfile {
  return {
    id: createId(),
    name: 'Localhost',
    host: 'localhost',
    port: 1883,
    clientId: '',
    username: '',
    password: '',
    tls: { enabled: false },
  }
}

export const useConnectionStore = defineStore('connections', {
  state: (): ConnectionState => ({
    profiles: [],
    currentId: undefined,
    connected: false,
    error: undefined,
    loading: false,
    saving: false,
  }),
  getters: {
    currentProfile: (state): ConnectionProfile | undefined =>
      state.profiles.find((profile) => profile.id === state.currentId),
  },
  actions: {
    async load() {
      this.loading = true
      this.error = undefined

      try {
        const loadedProfiles = await tauriApi.loadConnections()
        this.profiles = loadedProfiles.length > 0 ? loadedProfiles : [createDefaultProfile()]

        if (!this.currentId || !this.profiles.some((profile) => profile.id === this.currentId)) {
          this.currentId = this.profiles[0]?.id
        }
      } catch (error) {
        this.error = error instanceof Error ? error.message : '加载连接配置失败。'
        if (this.profiles.length === 0) {
          this.profiles = [createDefaultProfile()]
          this.currentId = this.profiles[0]?.id
        }
      } finally {
        this.loading = false
      }
    },
    async save() {
      this.saving = true
      this.error = undefined

      try {
        await tauriApi.saveConnections(this.profiles)
      } catch (error) {
        this.error = error instanceof Error ? error.message : '保存连接配置失败。'
        throw error
      } finally {
        this.saving = false
      }
    },
  },
})
