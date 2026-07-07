import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useConnectionStore } from '../stores/connectionStore'
import { tauriApi } from '../services/tauriApi'
import type { ConnectionProfile } from '../types/domain'

vi.mock('../services/tauriApi', () => ({
  tauriApi: {
    loadConnections: vi.fn(),
    saveConnections: vi.fn(),
  },
}))

const loadConnections = vi.mocked(tauriApi.loadConnections)
const saveConnections = vi.mocked(tauriApi.saveConnections)

describe('connectionStore persistence', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
  })

  it('loads saved profiles and selects the first profile by default', async () => {
    const profile: ConnectionProfile = {
      id: 'profile-1',
      name: 'Broker 1',
      host: 'mqtt.example.com',
      port: 1884,
      clientId: 'client-1',
      username: 'demo',
      password: 'secret',
      tls: { enabled: true },
    }
    loadConnections.mockResolvedValue([profile])

    const store = useConnectionStore()
    await store.load()

    expect(store.profiles).toEqual([profile])
    expect(store.currentId).toBe('profile-1')
    expect(store.currentProfile).toEqual(profile)
    expect(store.loading).toBe(false)
  })

  it('creates a localhost profile when storage is empty', async () => {
    loadConnections.mockResolvedValue([])

    const store = useConnectionStore()
    await store.load()

    expect(store.profiles).toHaveLength(1)
    expect(store.currentProfile).toMatchObject({
      name: 'Localhost',
      host: 'localhost',
      port: 1883,
      tls: { enabled: false },
    })
  })

  it('saves profiles through tauriApi', async () => {
    saveConnections.mockResolvedValue()
    const store = useConnectionStore()
    store.profiles = [{
      id: 'profile-1',
      name: 'Broker 1',
      host: 'localhost',
      port: 1883,
      tls: { enabled: false },
    }]
    store.currentId = 'profile-1'

    await store.save()

    expect(saveConnections).toHaveBeenCalledWith(store.profiles)
    expect(store.saving).toBe(false)
  })
})
