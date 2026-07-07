import { defineStore } from 'pinia'
import type { ConnectionProfile } from '../types/domain'

type ConnectionState = {
  profiles: ConnectionProfile[]
  currentId?: string
  connected: boolean
  error?: string
}

export const useConnectionStore = defineStore('connection', {
  state: (): ConnectionState => ({
    profiles: [],
    currentId: undefined,
    connected: false,
    error: undefined,
  }),
  getters: {
    currentProfile: (state): ConnectionProfile | undefined =>
      state.profiles.find((profile) => profile.id === state.currentId),
  },
})
