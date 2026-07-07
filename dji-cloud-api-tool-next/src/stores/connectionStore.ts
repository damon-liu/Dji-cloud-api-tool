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

function normalizeProfile(profile: ConnectionProfile): ConnectionProfile {
  const port = Number(profile.port)
  if (!Number.isInteger(port) || port < 1 || port > 65535) {
    throw new Error('端口必须是 1 到 65535 之间的整数。')
  }

  return {
    ...profile,
    name: profile.name.trim() || profile.host.trim() || '未命名配置',
    host: profile.host.trim() || 'localhost',
    port,
    clientId: profile.clientId?.trim(),
    username: profile.username?.trim(),
    password: profile.password,
    tls: { ...profile.tls, enabled: Boolean(profile.tls.enabled) },
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
      await this.saveProfiles(this.profiles, this.currentId)
    },
    async saveProfiles(profiles: ConnectionProfile[], currentId?: string) {
      this.saving = true
      this.error = undefined

      try {
        const normalizedProfiles = profiles.map(normalizeProfile)
        const nextCurrentId = normalizedProfiles.some((profile) => profile.id === currentId)
          ? currentId
          : normalizedProfiles[0]?.id

        await tauriApi.saveConnections(normalizedProfiles)

        this.profiles = normalizedProfiles
        this.currentId = nextCurrentId
      } catch (error) {
        this.error = error instanceof Error ? error.message : '保存连接配置失败。'
        throw error
      } finally {
        this.saving = false
      }
    },
  },
})
