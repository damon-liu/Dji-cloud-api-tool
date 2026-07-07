export type DeviceType = 'dock' | 'aircraft'

export type TlsConfig = {
  enabled: boolean
  caCertPath?: string
  clientCertPath?: string
  clientKeyPath?: string
  insecureSkipVerify?: boolean
}

export type ConnectionProfile = {
  id: string
  name: string
  host: string
  port: number
  clientId?: string
  username?: string
  password?: string
  tls: TlsConfig
}

export type Device = {
  sn: string
  name: string
  type: DeviceType
  parentSn?: string
  online: boolean
  lastSeenAt?: string
}

export type DeviceTopic = {
  id: string
  deviceSn: string
  topic: string
  enabled: boolean
  order: number
}

export type TopicFieldMapping = {
  zh: string
  unit?: string
  values?: Record<string, string>
}

export type TopicMapping = {
  topics: Record<string, {
    description: string
    fields: Record<string, TopicFieldMapping>
    groups: Array<{
      id: string
      label: string
      keys: string[]
    }>
  }>
}

export type MqttRuntimeMessage = {
  connectionId: string
  topic: string
  payloadText: string
  receivedAt: string
  deviceSn?: string
}

export type PublishRequest = {
  topic: string
  payloadText: string
}
