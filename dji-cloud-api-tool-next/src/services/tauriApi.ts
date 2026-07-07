import { invoke } from '@tauri-apps/api/core'
import type { ConnectionProfile, Device, DeviceTopic } from '../types/domain'

export function loadConnections(): Promise<ConnectionProfile[]> {
  return invoke('load_connections')
}

export function saveConnections(profiles: ConnectionProfile[]): Promise<void> {
  return invoke('save_connections', { profiles })
}

export function loadDevices(): Promise<Device[]> {
  return invoke('load_devices')
}

export function saveDevices(devices: Device[]): Promise<void> {
  return invoke('save_devices', { devices })
}

export function loadTopics(): Promise<DeviceTopic[]> {
  return invoke('load_topics')
}

export function saveTopics(topics: DeviceTopic[]): Promise<void> {
  return invoke('save_topics', { topics })
}

export function exportConfig(directory: string): Promise<void> {
  return invoke('export_config', { directory })
}

export function importConfig(directory: string): Promise<void> {
  return invoke('import_config', { directory })
}

export function connect(
  profile: ConnectionProfile,
  topics: DeviceTopic[],
  devices: Device[],
): Promise<void> {
  return invoke('connect', { profile, topics, devices })
}

export function disconnect(): Promise<void> {
  return invoke('disconnect')
}

export function publishMessage(topic: string, payloadText: string): Promise<void> {
  return invoke('publish_message', { topic, payloadText })
}

export const tauriApi = {
  loadConnections,
  saveConnections,
  loadDevices,
  saveDevices,
  loadTopics,
  saveTopics,
  exportConfig,
  importConfig,
  connect,
  disconnect,
  publishMessage,
}
