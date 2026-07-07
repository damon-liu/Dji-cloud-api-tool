<template>
  <div class="dialog-backdrop" role="presentation" @click.self="cancel">
    <section class="connection-dialog" role="dialog" aria-modal="true" aria-labelledby="connection-dialog-title">
      <header class="dialog-header">
        <h2 id="connection-dialog-title">连接配置</h2>
        <button type="button" title="关闭" @click="cancel">x</button>
      </header>

      <div class="dialog-body">
        <aside class="profile-list" aria-label="连接配置列表">
          <button
            v-for="profile in draftProfiles"
            :key="profile.id"
            type="button"
            class="profile-item"
            :class="{ selected: profile.id === selectedId }"
            @click="selectedId = profile.id"
          >
            <span class="profile-name">{{ profile.name || profile.host || '未命名配置' }}</span>
            <span class="profile-host">{{ profile.host }}:{{ profile.port }}</span>
          </button>
          <button type="button" class="add-profile" @click="addProfile">+ 新增配置</button>
        </aside>

        <form v-if="selectedProfile" class="profile-form" @submit.prevent="save">
          <label>
            <span>名称</span>
            <input v-model.trim="selectedProfile.name" required />
          </label>
          <label>
            <span>Host</span>
            <input v-model.trim="selectedProfile.host" required />
          </label>
          <label>
            <span>Port</span>
            <input v-model.number="selectedProfile.port" type="number" min="1" max="65535" required />
          </label>
          <label>
            <span>Client ID</span>
            <input v-model.trim="selectedProfile.clientId" />
          </label>
          <label>
            <span>Username</span>
            <input v-model.trim="selectedProfile.username" autocomplete="username" />
          </label>
          <label>
            <span>Password</span>
            <input v-model="selectedProfile.password" type="password" autocomplete="current-password" />
          </label>
          <label class="checkbox-row">
            <input v-model="selectedProfile.tls.enabled" type="checkbox" />
            <span>TLS enabled</span>
          </label>
        </form>

        <div v-else class="profile-empty">暂无连接配置</div>
      </div>

      <footer class="dialog-footer">
        <span class="dialog-status">{{ statusText }}</span>
        <button type="button" :disabled="!selectedProfile" @click="testConnection">测试</button>
        <button type="button" :disabled="!selectedProfile || draftProfiles.length <= 1" @click="removeSelected">
          删除
        </button>
        <button type="button" @click="cancel">取消</button>
        <button type="button" class="primary" :disabled="connections.saving || draftProfiles.length === 0" @click="save">
          保存
        </button>
      </footer>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useConnectionStore } from '../stores/connectionStore'
import type { ConnectionProfile } from '../types/domain'

const emit = defineEmits<{
  close: []
}>()

const connections = useConnectionStore()
const draftProfiles = ref<ConnectionProfile[]>(cloneProfiles(connections.profiles))
const selectedId = ref(connections.currentId ?? draftProfiles.value[0]?.id)
const testStatus = ref('连接测试将在后续版本接入')

const selectedProfile = computed(() => draftProfiles.value.find((profile) => profile.id === selectedId.value))
const statusText = computed(() => {
  if (connections.error) {
    return connections.error
  }

  return testStatus.value
})

function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? Math.random().toString(36).slice(2)
}

function cloneProfiles(profiles: ConnectionProfile[]): ConnectionProfile[] {
  return profiles.map((profile) => ({
    ...profile,
    tls: { ...profile.tls },
  }))
}

function createProfile(): ConnectionProfile {
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

function addProfile() {
  const profile = createProfile()
  profile.name = `连接 ${draftProfiles.value.length + 1}`
  draftProfiles.value.push(profile)
  selectedId.value = profile.id
}

function removeSelected() {
  if (!selectedId.value || draftProfiles.value.length <= 1) {
    return
  }

  const index = draftProfiles.value.findIndex((profile) => profile.id === selectedId.value)
  if (index === -1) {
    return
  }

  draftProfiles.value.splice(index, 1)
  selectedId.value = draftProfiles.value[Math.max(0, index - 1)]?.id
}

function normalizeProfile(profile: ConnectionProfile): ConnectionProfile {
  return {
    ...profile,
    name: profile.name.trim() || profile.host.trim() || '未命名配置',
    host: profile.host.trim() || 'localhost',
    port: Number(profile.port) || 1883,
    clientId: profile.clientId?.trim(),
    username: profile.username?.trim(),
    password: profile.password,
    tls: { ...profile.tls, enabled: Boolean(profile.tls.enabled) },
  }
}

function testConnection() {
  testStatus.value = '连接测试将在后续版本接入'
}

async function save() {
  const normalized = draftProfiles.value.map(normalizeProfile)
  connections.profiles = normalized
  connections.currentId = normalized.some((profile) => profile.id === selectedId.value)
    ? selectedId.value
    : normalized[0]?.id
  await connections.save()
  emit('close')
}

function cancel() {
  emit('close')
}
</script>

<style scoped>
.dialog-backdrop {
  position: fixed;
  inset: 0;
  z-index: 20;
  display: grid;
  place-items: center;
  padding: 24px;
  box-sizing: border-box;
  background: rgb(32 36 42 / 42%);
}

.connection-dialog {
  width: min(760px, 100%);
  max-height: min(680px, 100%);
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
  overflow: hidden;
  border: 1px solid #cfd6df;
  border-radius: 8px;
  background: #ffffff;
  box-shadow: 0 18px 44px rgb(32 36 42 / 22%);
}

.dialog-header,
.dialog-footer {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  border-bottom: 1px solid #edf0f4;
}

.dialog-header h2 {
  flex: 1;
  margin: 0;
  font-size: 16px;
}

.dialog-header button {
  width: 28px;
  height: 28px;
  padding: 0;
}

.dialog-body {
  min-height: 0;
  display: grid;
  grid-template-columns: 220px minmax(0, 1fr);
  overflow: hidden;
}

.profile-list {
  display: grid;
  align-content: start;
  gap: 6px;
  padding: 10px;
  overflow: auto;
  border-right: 1px solid #edf0f4;
}

.profile-item {
  min-width: 0;
  display: grid;
  gap: 2px;
  padding: 8px;
  text-align: left;
}

.profile-item.selected {
  border-color: #1f6feb;
  background: #eef5ff;
}

.profile-name,
.profile-host {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.profile-host {
  color: #77808c;
  font-size: 12px;
}

.add-profile {
  justify-self: stretch;
}

.profile-form {
  min-width: 0;
  display: grid;
  align-content: start;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  padding: 14px;
  overflow: auto;
}

.profile-form label {
  min-width: 0;
  display: grid;
  gap: 5px;
  color: #5e6673;
  font-size: 12px;
}

.profile-form input {
  min-width: 0;
  box-sizing: border-box;
  border: 1px solid #cfd6df;
  border-radius: 6px;
  padding: 7px 8px;
  color: #20242a;
}

.checkbox-row {
  grid-template-columns: auto minmax(0, 1fr);
  align-items: center;
}

.profile-empty {
  padding: 14px;
  color: #77808c;
}

.dialog-footer {
  border-top: 1px solid #edf0f4;
  border-bottom: 0;
}

.dialog-status {
  min-width: 0;
  flex: 1;
  color: #5e6673;
  font-size: 12px;
}

@media (max-width: 720px) {
  .dialog-body,
  .profile-form {
    grid-template-columns: minmax(0, 1fr);
  }

  .profile-list {
    max-height: 180px;
    border-right: 0;
    border-bottom: 1px solid #edf0f4;
  }
}
</style>
