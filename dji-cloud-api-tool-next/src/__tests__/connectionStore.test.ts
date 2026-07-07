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

  it('does not commit draft profiles when saving fails', async () => {
    const originalProfile: ConnectionProfile = {
      id: 'profile-1',
      name: 'Broker 1',
      host: 'localhost',
      port: 1883,
      tls: { enabled: false },
    }
    const draftProfile: ConnectionProfile = {
      id: 'profile-2',
      name: 'Broker 2',
      host: 'mqtt.example.com',
      port: 1884,
      tls: { enabled: true },
    }
    saveConnections.mockRejectedValue(new Error('disk full'))

    const store = useConnectionStore()
    store.profiles = [originalProfile]
    store.currentId = 'profile-1'

    await expect(store.saveProfiles([draftProfile], 'profile-2')).rejects.toThrow('disk full')

    expect(store.profiles).toEqual([originalProfile])
    expect(store.currentId).toBe('profile-1')
  })

  it('rejects invalid ports before saving draft profiles', async () => {
    const store = useConnectionStore()

    await expect(
      store.saveProfiles([
        {
          id: 'profile-1',
          name: 'Bad Broker',
          host: 'localhost',
          port: 70000,
          tls: { enabled: false },
        },
      ], 'profile-1'),
    ).rejects.toThrow('端口必须是 1 到 65535 之间的整数。')

    expect(saveConnections).not.toHaveBeenCalled()
    expect(store.profiles).toEqual([])
  })
})
