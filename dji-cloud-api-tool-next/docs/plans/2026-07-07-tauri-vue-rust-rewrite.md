# Tauri Vue Rust Rewrite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a separate Tauri + Vue + Rust desktop app that recreates the original Qt DJI Cloud API monitor's user-facing behavior while keeping all new code under `dji-cloud-api-tool-next/`.

**Architecture:** The Vue frontend owns layout, stores, filtering, and presentation. The Rust Tauri backend owns config file I/O, MQTT/TLS connectivity, subscriptions, publishing, reconnect behavior, and event emission. Frontend and backend communicate through Tauri commands and events.

**Tech Stack:** Tauri 2, Vue 3, TypeScript, Vite, Pinia, Rust, rumqttc, serde, Vitest, Cargo tests.

---

## File Structure

All new project files live under `dji-cloud-api-tool-next/`.

```text
dji-cloud-api-tool-next/
  README.md
  docs/
    specs/2026-07-07-tauri-vue-rust-rewrite-design.md
    plans/2026-07-07-tauri-vue-rust-rewrite.md
  package.json
  pnpm-lock.yaml
  index.html
  vite.config.ts
  tsconfig.json
  src/
    main.ts
    App.vue
    styles/app.css
    types/domain.ts
    stores/appStore.ts
    stores/connectionStore.ts
    stores/deviceStore.ts
    stores/topicStore.ts
    stores/monitorStore.ts
    services/tauriApi.ts
    services/topicMapping.ts
    components/AppShell.vue
    components/ConnectionDialog.vue
    components/DeviceTree.vue
    components/TopicList.vue
    components/OsdPanel.vue
    components/ParsedJsonPanel.vue
    components/RawJsonPanel.vue
    components/PublishPanel.vue
    __tests__/
      topicMapping.test.ts
      monitorStore.test.ts
      topicStore.test.ts
  src-tauri/
    Cargo.toml
    tauri.conf.json
    build.rs
    src/
      main.rs
      lib.rs
      models.rs
      config_store.rs
      topic_matcher.rs
      mqtt_service.rs
      commands.rs
      tests/
        config_store_tests.rs
        topic_matcher_tests.rs
        mqtt_service_tests.rs
    capabilities/default.json
```

## Task 1: Scaffold the Tauri Vue Project

**Files:**
- Create/modify: `dji-cloud-api-tool-next/package.json`
- Create/modify: `dji-cloud-api-tool-next/vite.config.ts`
- Create/modify: `dji-cloud-api-tool-next/tsconfig.json`
- Create/modify: `dji-cloud-api-tool-next/index.html`
- Create/modify: `dji-cloud-api-tool-next/src/main.ts`
- Create/modify: `dji-cloud-api-tool-next/src/App.vue`
- Create/modify: `dji-cloud-api-tool-next/src/styles/app.css`
- Create/modify: `dji-cloud-api-tool-next/src-tauri/Cargo.toml`
- Create/modify: `dji-cloud-api-tool-next/src-tauri/tauri.conf.json`
- Create/modify: `dji-cloud-api-tool-next/src-tauri/build.rs`
- Create/modify: `dji-cloud-api-tool-next/src-tauri/src/main.rs`
- Create/modify: `dji-cloud-api-tool-next/src-tauri/src/lib.rs`

- [ ] **Step 1: Create project skeleton**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm create tauri-app@latest . --template vue-ts --manager pnpm
```

Choose the existing directory when prompted. Expected: a Vue + Tauri project exists under `dji-cloud-api-tool-next/`.

- [ ] **Step 2: Install frontend dependencies**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm add pinia @tauri-apps/api lucide-vue-next
pnpm add -D vitest @vue/test-utils jsdom
```

Expected: dependencies added to `package.json`.

- [ ] **Step 3: Install Rust dependencies**

Edit `dji-cloud-api-tool-next/src-tauri/Cargo.toml` dependencies to include:

```toml
[dependencies]
tauri = { version = "2", features = [] }
tauri-plugin-opener = "2"
serde = { version = "1", features = ["derive"] }
serde_json = "1"
tokio = { version = "1", features = ["rt-multi-thread", "macros", "time", "sync"] }
rumqttc = "0.24"
uuid = { version = "1", features = ["v4", "serde"] }
chrono = { version = "0.4", features = ["serde"] }
thiserror = "1"
dirs = "5"
```

Expected: `cargo metadata` can resolve all dependencies.

- [ ] **Step 4: Replace starter UI with placeholder shell**

Edit `dji-cloud-api-tool-next/src/App.vue`:

```vue
<template>
  <main class="app-root">
    <h1>DJI Cloud API Tool Next</h1>
    <p>Tauri + Vue + Rust rewrite workspace is ready.</p>
  </main>
</template>
```

Edit `dji-cloud-api-tool-next/src/styles/app.css`:

```css
html,
body,
#app {
  height: 100%;
  margin: 0;
}

body {
  font-family: Inter, "Segoe UI", Arial, sans-serif;
  background: #f5f7fa;
  color: #20242a;
}

.app-root {
  min-height: 100%;
  display: grid;
  place-items: center;
}
```

Edit `dji-cloud-api-tool-next/src/main.ts`:

```ts
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import './styles/app.css'

createApp(App).use(createPinia()).mount('#app')
```

- [ ] **Step 5: Verify scaffold**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm install
pnpm build
cd src-tauri
cargo test
```

Expected: frontend build exits 0 and Rust tests exit 0.

- [ ] **Step 6: Commit**

```bash
git add dji-cloud-api-tool-next
git commit -m "feat: 初始化 Tauri Vue 新版项目"
```

## Task 2: Define Shared Domain Models

**Files:**
- Create: `dji-cloud-api-tool-next/src/types/domain.ts`
- Create: `dji-cloud-api-tool-next/src-tauri/src/models.rs`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/lib.rs`

- [ ] **Step 1: Add TypeScript domain types**

Create `dji-cloud-api-tool-next/src/types/domain.ts`:

```ts
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
```

- [ ] **Step 2: Add Rust domain models**

Create `dji-cloud-api-tool-next/src-tauri/src/models.rs`:

```rust
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct TlsConfig {
    pub enabled: bool,
    pub ca_cert_path: Option<String>,
    pub client_cert_path: Option<String>,
    pub client_key_path: Option<String>,
    pub insecure_skip_verify: Option<bool>,
}

impl Default for TlsConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            ca_cert_path: None,
            client_cert_path: None,
            client_key_path: None,
            insecure_skip_verify: None,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct ConnectionProfile {
    pub id: String,
    pub name: String,
    pub host: String,
    pub port: u16,
    pub client_id: Option<String>,
    pub username: Option<String>,
    pub password: Option<String>,
    pub tls: TlsConfig,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "lowercase")]
pub enum DeviceType {
    Dock,
    Aircraft,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct Device {
    pub sn: String,
    pub name: String,
    pub device_type: DeviceType,
    pub parent_sn: Option<String>,
    pub online: bool,
    pub last_seen_at: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct DeviceTopic {
    pub id: String,
    pub device_sn: String,
    pub topic: String,
    pub enabled: bool,
    pub order: i32,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct TopicFieldMapping {
    pub zh: String,
    pub unit: Option<String>,
    pub values: Option<BTreeMap<String, String>>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct TopicMapping {
    pub topics: BTreeMap<String, TopicMappingEntry>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct TopicMappingEntry {
    pub description: String,
    pub fields: BTreeMap<String, TopicFieldMapping>,
    pub groups: Vec<TopicMappingGroup>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct TopicMappingGroup {
    pub id: String,
    pub label: String,
    pub keys: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct MqttRuntimeMessage {
    pub connection_id: String,
    pub topic: String,
    pub payload_text: String,
    pub received_at: String,
    pub device_sn: Option<String>,
}
```

Modify `dji-cloud-api-tool-next/src-tauri/src/lib.rs`:

```rust
pub mod models;
```

- [ ] **Step 3: Verify model compilation**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test
```

Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add dji-cloud-api-tool-next/src/types/domain.ts dji-cloud-api-tool-next/src-tauri/src/models.rs dji-cloud-api-tool-next/src-tauri/src/lib.rs
git commit -m "feat: 定义新版领域模型"
```

## Task 3: Implement Rust Config Store

**Files:**
- Create: `dji-cloud-api-tool-next/src-tauri/src/config_store.rs`
- Create: `dji-cloud-api-tool-next/src-tauri/src/tests/config_store_tests.rs`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/lib.rs`

- [ ] **Step 1: Write config store tests**

Create `dji-cloud-api-tool-next/src-tauri/src/tests/config_store_tests.rs`:

```rust
use crate::config_store::ConfigStore;
use crate::models::{ConnectionProfile, TlsConfig};
use tempfile::tempdir;

fn profile() -> ConnectionProfile {
    ConnectionProfile {
        id: "default".to_string(),
        name: "默认".to_string(),
        host: "broker.example.com".to_string(),
        port: 8883,
        client_id: Some("dji-tool".to_string()),
        username: Some("admin".to_string()),
        password: Some("secret".to_string()),
        tls: TlsConfig::default(),
    }
}

#[test]
fn saves_and_loads_connections() {
    let dir = tempdir().unwrap();
    let store = ConfigStore::new(dir.path().to_path_buf());

    store.save_connections(&[profile()]).unwrap();
    let loaded = store.load_connections().unwrap();

    assert_eq!(loaded.len(), 1);
    assert_eq!(loaded[0].host, "broker.example.com");
    assert_eq!(loaded[0].port, 8883);
}

#[test]
fn missing_connections_returns_empty_list() {
    let dir = tempdir().unwrap();
    let store = ConfigStore::new(dir.path().to_path_buf());

    let loaded = store.load_connections().unwrap();

    assert!(loaded.is_empty());
}
```

Add `tempfile = "3"` to `[dev-dependencies]` in `src-tauri/Cargo.toml`.

- [ ] **Step 2: Verify tests fail**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test config_store
```

Expected: FAIL because `config_store` does not exist.

- [ ] **Step 3: Implement config store**

Create `dji-cloud-api-tool-next/src-tauri/src/config_store.rs`:

```rust
use crate::models::{ConnectionProfile, Device, DeviceTopic, TopicMapping};
use serde::{de::DeserializeOwned, Serialize};
use std::fs;
use std::path::{Path, PathBuf};
use thiserror::Error;

#[derive(Debug, Error)]
pub enum ConfigStoreError {
    #[error("failed to create app data directory: {0}")]
    CreateDir(std::io::Error),
    #[error("failed to read config file {path}: {source}")]
    Read { path: String, source: std::io::Error },
    #[error("failed to write config file {path}: {source}")]
    Write { path: String, source: std::io::Error },
    #[error("failed to parse config file {path}: {source}")]
    Parse { path: String, source: serde_json::Error },
    #[error("failed to serialize config file {path}: {source}")]
    Serialize { path: String, source: serde_json::Error },
}

pub type Result<T> = std::result::Result<T, ConfigStoreError>;

#[derive(Debug, Clone)]
pub struct ConfigStore {
    root: PathBuf,
}

impl ConfigStore {
    pub fn new(root: PathBuf) -> Self {
        Self { root }
    }

    pub fn app_data_default() -> Self {
        let root = dirs::data_dir()
            .unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")))
            .join("dji-cloud-api-tool-next");
        Self::new(root)
    }

    pub fn load_connections(&self) -> Result<Vec<ConnectionProfile>> {
        self.load_list("connections.json")
    }

    pub fn save_connections(&self, profiles: &[ConnectionProfile]) -> Result<()> {
        self.save_json("connections.json", profiles)
    }

    pub fn load_devices(&self) -> Result<Vec<Device>> {
        self.load_list("devices.json")
    }

    pub fn save_devices(&self, devices: &[Device]) -> Result<()> {
        self.save_json("devices.json", devices)
    }

    pub fn load_topics(&self) -> Result<Vec<DeviceTopic>> {
        self.load_list("topics.json")
    }

    pub fn save_topics(&self, topics: &[DeviceTopic]) -> Result<()> {
        self.save_json("topics.json", topics)
    }

    pub fn load_topic_mapping(&self) -> Result<Option<TopicMapping>> {
        let path = self.path("topic-mappings.json");
        if !path.exists() {
            return Ok(None);
        }
        self.load_json("topic-mappings.json").map(Some)
    }

    pub fn save_topic_mapping(&self, mapping: &TopicMapping) -> Result<()> {
        self.save_json("topic-mappings.json", mapping)
    }

    fn load_list<T: DeserializeOwned>(&self, name: &str) -> Result<Vec<T>> {
        let path = self.path(name);
        if !path.exists() {
            return Ok(Vec::new());
        }
        self.load_json(name)
    }

    fn load_json<T: DeserializeOwned>(&self, name: &str) -> Result<T> {
        let path = self.path(name);
        let path_label = path.display().to_string();
        let text = fs::read_to_string(&path).map_err(|source| ConfigStoreError::Read {
            path: path_label.clone(),
            source,
        })?;
        serde_json::from_str(&text).map_err(|source| ConfigStoreError::Parse {
            path: path_label,
            source,
        })
    }

    fn save_json<T: Serialize>(&self, name: &str, value: &T) -> Result<()> {
        fs::create_dir_all(&self.root).map_err(ConfigStoreError::CreateDir)?;
        let path = self.path(name);
        let path_label = path.display().to_string();
        let text = serde_json::to_string_pretty(value).map_err(|source| ConfigStoreError::Serialize {
            path: path_label.clone(),
            source,
        })?;
        fs::write(&path, text).map_err(|source| ConfigStoreError::Write {
            path: path_label,
            source,
        })
    }

    fn path(&self, name: &str) -> PathBuf {
        self.root.join(Path::new(name))
    }
}
```

Modify `dji-cloud-api-tool-next/src-tauri/src/lib.rs`:

```rust
pub mod config_store;
pub mod models;

#[cfg(test)]
mod tests {
    mod config_store_tests;
}
```

- [ ] **Step 4: Verify tests pass**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test config_store
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next/src-tauri
git commit -m "feat: 实现新版配置存储"
```

## Task 4: Implement Topic Matching

**Files:**
- Create: `dji-cloud-api-tool-next/src-tauri/src/topic_matcher.rs`
- Create: `dji-cloud-api-tool-next/src-tauri/src/tests/topic_matcher_tests.rs`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/lib.rs`

- [ ] **Step 1: Write topic matcher tests**

Create `dji-cloud-api-tool-next/src-tauri/src/tests/topic_matcher_tests.rs`:

```rust
use crate::models::{Device, DeviceType};
use crate::topic_matcher::match_device_sn;

fn devices() -> Vec<Device> {
    vec![
        Device {
            sn: "dock_001".to_string(),
            name: "机场1号".to_string(),
            device_type: DeviceType::Dock,
            parent_sn: None,
            online: false,
            last_seen_at: None,
        },
        Device {
            sn: "drone_001".to_string(),
            name: "飞机1号".to_string(),
            device_type: DeviceType::Aircraft,
            parent_sn: Some("dock_001".to_string()),
            online: false,
            last_seen_at: None,
        },
    ]
}

#[test]
fn matches_sn_contained_in_topic() {
    let topic = "thing/product/dock_001/osd";
    assert_eq!(match_device_sn(topic, &devices()), Some("dock_001".to_string()));
}

#[test]
fn returns_none_when_no_device_matches() {
    let topic = "thing/product/unknown/osd";
    assert_eq!(match_device_sn(topic, &devices()), None);
}
```

- [ ] **Step 2: Verify tests fail**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test topic_matcher
```

Expected: FAIL because `topic_matcher` does not exist.

- [ ] **Step 3: Implement topic matcher**

Create `dji-cloud-api-tool-next/src-tauri/src/topic_matcher.rs`:

```rust
use crate::models::Device;

pub fn match_device_sn(topic: &str, devices: &[Device]) -> Option<String> {
    devices
        .iter()
        .filter(|device| !device.sn.is_empty() && topic.contains(&device.sn))
        .max_by_key(|device| device.sn.len())
        .map(|device| device.sn.clone())
}

pub fn default_osd_topic(sn: &str) -> String {
    format!("thing/product/{sn}/osd")
}
```

Modify `dji-cloud-api-tool-next/src-tauri/src/lib.rs`:

```rust
pub mod config_store;
pub mod models;
pub mod topic_matcher;

#[cfg(test)]
mod tests {
    mod config_store_tests;
    mod topic_matcher_tests;
}
```

- [ ] **Step 4: Verify tests pass**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test topic_matcher
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next/src-tauri
git commit -m "feat: 实现 Topic 设备匹配"
```

## Task 5: Implement MQTT Service State and Commands

**Files:**
- Create: `dji-cloud-api-tool-next/src-tauri/src/mqtt_service.rs`
- Create: `dji-cloud-api-tool-next/src-tauri/src/commands.rs`
- Create: `dji-cloud-api-tool-next/src-tauri/src/tests/mqtt_service_tests.rs`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/lib.rs`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/main.rs`

- [ ] **Step 1: Write MQTT service validation tests**

Create `dji-cloud-api-tool-next/src-tauri/src/tests/mqtt_service_tests.rs`:

```rust
use crate::models::{ConnectionProfile, TlsConfig};
use crate::mqtt_service::validate_connection_profile;

fn base_profile() -> ConnectionProfile {
    ConnectionProfile {
        id: "default".to_string(),
        name: "默认".to_string(),
        host: "broker.example.com".to_string(),
        port: 8883,
        client_id: None,
        username: Some("admin".to_string()),
        password: Some("secret".to_string()),
        tls: TlsConfig::default(),
    }
}

#[test]
fn accepts_valid_profile() {
    assert!(validate_connection_profile(&base_profile()).is_ok());
}

#[test]
fn rejects_empty_host() {
    let mut profile = base_profile();
    profile.host = " ".to_string();

    let error = validate_connection_profile(&profile).unwrap_err();

    assert!(error.to_string().contains("host is required"));
}

#[test]
fn rejects_zero_port() {
    let mut profile = base_profile();
    profile.port = 0;

    let error = validate_connection_profile(&profile).unwrap_err();

    assert!(error.to_string().contains("port must be between 1 and 65535"));
}
```

- [ ] **Step 2: Verify tests fail**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test mqtt_service
```

Expected: FAIL because `mqtt_service` does not exist.

- [ ] **Step 3: Implement MQTT service shell**

Create `dji-cloud-api-tool-next/src-tauri/src/mqtt_service.rs`:

```rust
use crate::models::{ConnectionProfile, Device, DeviceTopic, MqttRuntimeMessage};
use crate::topic_matcher::match_device_sn;
use chrono::Utc;
use rumqttc::{AsyncClient, Event, Incoming, MqttOptions, QoS, Transport};
use std::sync::Arc;
use std::time::Duration;
use tauri::{AppHandle, Emitter};
use thiserror::Error;
use tokio::sync::Mutex;

#[derive(Debug, Error)]
pub enum MqttServiceError {
    #[error("host is required")]
    EmptyHost,
    #[error("port must be between 1 and 65535")]
    InvalidPort,
    #[error("mqtt client error: {0}")]
    Client(String),
}

pub type Result<T> = std::result::Result<T, MqttServiceError>;

#[derive(Clone, Default)]
pub struct MqttService {
    client: Arc<Mutex<Option<AsyncClient>>>,
}

pub fn validate_connection_profile(profile: &ConnectionProfile) -> Result<()> {
    if profile.host.trim().is_empty() {
        return Err(MqttServiceError::EmptyHost);
    }
    if profile.port == 0 {
        return Err(MqttServiceError::InvalidPort);
    }
    Ok(())
}

impl MqttService {
    pub async fn connect(
        &self,
        app: AppHandle,
        profile: ConnectionProfile,
        topics: Vec<DeviceTopic>,
        devices: Vec<Device>,
    ) -> Result<()> {
        validate_connection_profile(&profile)?;

        let client_id = profile
            .client_id
            .clone()
            .unwrap_or_else(|| format!("dji-cloud-api-tool-next-{}", uuid::Uuid::new_v4()));
        let mut options = MqttOptions::new(client_id, profile.host.clone(), profile.port);
        options.set_keep_alive(Duration::from_secs(30));

        if let Some(username) = profile.username.clone().filter(|value| !value.is_empty()) {
            options.set_credentials(username, profile.password.clone().unwrap_or_default());
        }

        if profile.tls.enabled {
            options.set_transport(Transport::tls_with_default_config());
        }

        let (client, mut eventloop) = AsyncClient::new(options, 10);

        for topic in topics.iter().filter(|topic| topic.enabled) {
            client
                .subscribe(topic.topic.clone(), QoS::AtLeastOnce)
                .await
                .map_err(|error| MqttServiceError::Client(error.to_string()))?;
        }

        *self.client.lock().await = Some(client);

        let connection_id = profile.id.clone();
        tauri::async_runtime::spawn(async move {
            let _ = app.emit("mqtt:connected", &connection_id);
            loop {
                match eventloop.poll().await {
                    Ok(Event::Incoming(Incoming::Publish(publish))) => {
                        let payload_text = String::from_utf8_lossy(&publish.payload).to_string();
                        let topic = publish.topic;
                        let message = MqttRuntimeMessage {
                            connection_id: connection_id.clone(),
                            device_sn: match_device_sn(&topic, &devices),
                            topic,
                            payload_text,
                            received_at: Utc::now().to_rfc3339(),
                        };
                        let _ = app.emit("mqtt:message", message);
                    }
                    Ok(_) => {}
                    Err(error) => {
                        let _ = app.emit("mqtt:error", error.to_string());
                        break;
                    }
                }
            }
            let _ = app.emit("mqtt:disconnected", &connection_id);
        });

        Ok(())
    }

    pub async fn disconnect(&self) -> Result<()> {
        if let Some(client) = self.client.lock().await.take() {
            client
                .disconnect()
                .await
                .map_err(|error| MqttServiceError::Client(error.to_string()))?;
        }
        Ok(())
    }

    pub async fn publish(&self, topic: String, payload_text: String) -> Result<()> {
        let client = self.client.lock().await;
        let Some(client) = client.as_ref() else {
            return Err(MqttServiceError::Client("not connected".to_string()));
        };
        client
            .publish(topic, QoS::AtLeastOnce, false, payload_text)
            .await
            .map_err(|error| MqttServiceError::Client(error.to_string()))
    }
}
```

- [ ] **Step 4: Add Tauri commands**

Create `dji-cloud-api-tool-next/src-tauri/src/commands.rs`:

```rust
use crate::config_store::ConfigStore;
use crate::models::{ConnectionProfile, Device, DeviceTopic};
use crate::mqtt_service::MqttService;
use tauri::{AppHandle, State};

#[tauri::command]
pub async fn load_connections(store: State<'_, ConfigStore>) -> Result<Vec<ConnectionProfile>, String> {
    store.load_connections().map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn save_connections(
    store: State<'_, ConfigStore>,
    profiles: Vec<ConnectionProfile>,
) -> Result<(), String> {
    store.save_connections(&profiles).map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn load_devices(store: State<'_, ConfigStore>) -> Result<Vec<Device>, String> {
    store.load_devices().map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn save_devices(store: State<'_, ConfigStore>, devices: Vec<Device>) -> Result<(), String> {
    store.save_devices(&devices).map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn load_topics(store: State<'_, ConfigStore>) -> Result<Vec<DeviceTopic>, String> {
    store.load_topics().map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn save_topics(store: State<'_, ConfigStore>, topics: Vec<DeviceTopic>) -> Result<(), String> {
    store.save_topics(&topics).map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn connect(
    app: AppHandle,
    mqtt: State<'_, MqttService>,
    profile: ConnectionProfile,
    topics: Vec<DeviceTopic>,
    devices: Vec<Device>,
) -> Result<(), String> {
    mqtt.connect(app, profile, topics, devices)
        .await
        .map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn disconnect(mqtt: State<'_, MqttService>) -> Result<(), String> {
    mqtt.disconnect().await.map_err(|error| error.to_string())
}

#[tauri::command]
pub async fn publish_message(
    mqtt: State<'_, MqttService>,
    topic: String,
    payload_text: String,
) -> Result<(), String> {
    mqtt.publish(topic, payload_text)
        .await
        .map_err(|error| error.to_string())
}
```

Modify `dji-cloud-api-tool-next/src-tauri/src/lib.rs`:

```rust
pub mod commands;
pub mod config_store;
pub mod models;
pub mod mqtt_service;
pub mod topic_matcher;

#[cfg(test)]
mod tests {
    mod config_store_tests;
    mod mqtt_service_tests;
    mod topic_matcher_tests;
}
```

Modify `dji-cloud-api-tool-next/src-tauri/src/main.rs` to register managed state and commands:

```rust
use dji_cloud_api_tool_next::commands;
use dji_cloud_api_tool_next::config_store::ConfigStore;
use dji_cloud_api_tool_next::mqtt_service::MqttService;

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .manage(ConfigStore::app_data_default())
        .manage(MqttService::default())
        .invoke_handler(tauri::generate_handler![
            commands::load_connections,
            commands::save_connections,
            commands::load_devices,
            commands::save_devices,
            commands::load_topics,
            commands::save_topics,
            commands::connect,
            commands::disconnect,
            commands::publish_message
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
```

- [ ] **Step 5: Verify tests pass**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next/src-tauri"
cargo test mqtt_service
cargo test
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add dji-cloud-api-tool-next/src-tauri
git commit -m "feat: 添加 Rust MQTT 服务命令"
```

## Task 6: Add Frontend Tauri API Layer and Stores

**Files:**
- Create: `dji-cloud-api-tool-next/src/services/tauriApi.ts`
- Create: `dji-cloud-api-tool-next/src/stores/connectionStore.ts`
- Create: `dji-cloud-api-tool-next/src/stores/deviceStore.ts`
- Create: `dji-cloud-api-tool-next/src/stores/topicStore.ts`
- Create: `dji-cloud-api-tool-next/src/stores/monitorStore.ts`
- Create: `dji-cloud-api-tool-next/src/__tests__/topicStore.test.ts`

- [ ] **Step 1: Write topic store tests**

Create `dji-cloud-api-tool-next/src/__tests__/topicStore.test.ts`:

```ts
import { setActivePinia, createPinia } from 'pinia'
import { beforeEach, describe, expect, it } from 'vitest'
import { useTopicStore } from '../stores/topicStore'

describe('topicStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('adds default osd topic for a device', () => {
    const store = useTopicStore()

    store.addDefaultTopic('dock_001')

    expect(store.topics[0].topic).toBe('thing/product/dock_001/osd')
    expect(store.topics[0].enabled).toBe(true)
  })

  it('moves a topic down while preserving order values', () => {
    const store = useTopicStore()
    store.addTopic('dock_001', 'a')
    store.addTopic('dock_001', 'b')

    store.moveTopic('dock_001', 'a', 'down')

    expect(store.topicsForDevice('dock_001').map(topic => topic.topic)).toEqual(['b', 'a'])
    expect(store.topicsForDevice('dock_001').map(topic => topic.order)).toEqual([0, 1])
  })
})
```

- [ ] **Step 2: Verify tests fail**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm vitest run src/__tests__/topicStore.test.ts
```

Expected: FAIL because the store does not exist.

- [ ] **Step 3: Add Tauri API service**

Create `dji-cloud-api-tool-next/src/services/tauriApi.ts`:

```ts
import { invoke } from '@tauri-apps/api/core'
import type { ConnectionProfile, Device, DeviceTopic } from '../types/domain'

export const tauriApi = {
  loadConnections: () => invoke<ConnectionProfile[]>('load_connections'),
  saveConnections: (profiles: ConnectionProfile[]) => invoke<void>('save_connections', { profiles }),
  loadDevices: () => invoke<Device[]>('load_devices'),
  saveDevices: (devices: Device[]) => invoke<void>('save_devices', { devices }),
  loadTopics: () => invoke<DeviceTopic[]>('load_topics'),
  saveTopics: (topics: DeviceTopic[]) => invoke<void>('save_topics', { topics }),
  connect: (profile: ConnectionProfile, topics: DeviceTopic[], devices: Device[]) =>
    invoke<void>('connect', { profile, topics, devices }),
  disconnect: () => invoke<void>('disconnect'),
  publishMessage: (topic: string, payloadText: string) =>
    invoke<void>('publish_message', { topic, payloadText }),
}
```

- [ ] **Step 4: Add stores**

Create `dji-cloud-api-tool-next/src/stores/topicStore.ts`:

```ts
import { defineStore } from 'pinia'
import type { DeviceTopic } from '../types/domain'

type Direction = 'up' | 'down'

export const useTopicStore = defineStore('topics', {
  state: () => ({
    topics: [] as DeviceTopic[],
  }),
  actions: {
    addDefaultTopic(deviceSn: string) {
      this.addTopic(deviceSn, `thing/product/${deviceSn}/osd`)
    },
    addTopic(deviceSn: string, topic: string) {
      if (this.topics.some(item => item.deviceSn === deviceSn && item.topic === topic)) {
        throw new Error('该 Topic 已存在。')
      }
      this.topics.push({
        id: crypto.randomUUID(),
        deviceSn,
        topic,
        enabled: true,
        order: this.topicsForDevice(deviceSn).length,
      })
    },
    removeTopic(deviceSn: string, topic: string) {
      this.topics = this.topics.filter(item => !(item.deviceSn === deviceSn && item.topic === topic))
      this.normalizeOrder(deviceSn)
    },
    toggleTopic(deviceSn: string, topic: string) {
      const item = this.topics.find(topicItem => topicItem.deviceSn === deviceSn && topicItem.topic === topic)
      if (item) item.enabled = !item.enabled
    },
    setAllEnabled(deviceSn: string, enabled: boolean) {
      this.topics.forEach(item => {
        if (item.deviceSn === deviceSn) item.enabled = enabled
      })
    },
    moveTopic(deviceSn: string, topic: string, direction: Direction) {
      const list = this.topicsForDevice(deviceSn)
      const index = list.findIndex(item => item.topic === topic)
      const targetIndex = direction === 'up' ? index - 1 : index + 1
      if (index < 0 || targetIndex < 0 || targetIndex >= list.length) return
      const current = list[index]
      const target = list[targetIndex]
      const oldOrder = current.order
      current.order = target.order
      target.order = oldOrder
      this.normalizeOrder(deviceSn)
    },
    topicsForDevice(deviceSn: string) {
      return this.topics
        .filter(item => item.deviceSn === deviceSn)
        .sort((a, b) => a.order - b.order)
    },
    enabledTopics() {
      return this.topics.filter(topic => topic.enabled)
    },
    normalizeOrder(deviceSn: string) {
      this.topicsForDevice(deviceSn).forEach((topic, index) => {
        topic.order = index
      })
    },
  },
})
```

Create `connectionStore.ts`, `deviceStore.ts`, and `monitorStore.ts` with minimal state and actions required by components:

```ts
// connectionStore.ts
import { defineStore } from 'pinia'
import type { ConnectionProfile } from '../types/domain'

export const useConnectionStore = defineStore('connections', {
  state: () => ({
    profiles: [] as ConnectionProfile[],
    currentId: '',
    connected: false,
    error: '',
  }),
  getters: {
    currentProfile: state => state.profiles.find(profile => profile.id === state.currentId),
  },
})
```

```ts
// deviceStore.ts
import { defineStore } from 'pinia'
import type { Device, DeviceType } from '../types/domain'

export const useDeviceStore = defineStore('devices', {
  state: () => ({
    devices: [] as Device[],
    selectedSn: '',
  }),
  getters: {
    selectedDevice: state => state.devices.find(device => device.sn === state.selectedSn),
    topLevelDevices: state => state.devices.filter(device => !device.parentSn),
    childDevices: state => (parentSn: string) => state.devices.filter(device => device.parentSn === parentSn),
  },
  actions: {
    addDevice(sn: string, name: string, type: DeviceType, parentSn?: string) {
      if (!sn.trim()) throw new Error('设备 SN 不能为空。')
      if (this.devices.some(device => device.sn === sn.trim())) throw new Error('设备 SN 已存在。')
      this.devices.push({ sn: sn.trim(), name: name.trim() || sn.trim(), type, parentSn, online: false })
    },
    removeDevice(sn: string) {
      const children = this.devices.filter(device => device.parentSn === sn).map(device => device.sn)
      this.devices = this.devices.filter(device => device.sn !== sn && !children.includes(device.sn))
      if (this.selectedSn === sn || children.includes(this.selectedSn)) this.selectedSn = ''
    },
  },
})
```

```ts
// monitorStore.ts
import { defineStore } from 'pinia'
import type { MqttRuntimeMessage } from '../types/domain'

const MAX_HISTORY = 500

export const useMonitorStore = defineStore('monitor', {
  state: () => ({
    messages: [] as MqttRuntimeMessage[],
    paused: false,
  }),
  actions: {
    append(message: MqttRuntimeMessage) {
      if (this.paused) return
      this.messages.push(message)
      if (this.messages.length > MAX_HISTORY) {
        this.messages.splice(0, this.messages.length - MAX_HISTORY)
      }
    },
    history(deviceSn?: string, topic?: string) {
      return this.messages.filter(message => {
        if (deviceSn && message.deviceSn !== deviceSn) return false
        if (topic && message.topic !== topic) return false
        return true
      })
    },
    clear(deviceSn?: string, topic?: string) {
      this.messages = this.messages.filter(message => {
        if (deviceSn && message.deviceSn !== deviceSn) return true
        if (topic && message.topic !== topic) return true
        return false
      })
    },
  },
})
```

- [ ] **Step 5: Verify tests pass**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm vitest run src/__tests__/topicStore.test.ts
pnpm build
```

Expected: PASS and frontend build exits 0.

- [ ] **Step 6: Commit**

```bash
git add dji-cloud-api-tool-next/src
git commit -m "feat: 添加前端状态模型"
```

## Task 7: Implement Topic Mapping Parser

**Files:**
- Create: `dji-cloud-api-tool-next/src/services/topicMapping.ts`
- Create: `dji-cloud-api-tool-next/src/__tests__/topicMapping.test.ts`

- [ ] **Step 1: Write parser tests**

Create `dji-cloud-api-tool-next/src/__tests__/topicMapping.test.ts`:

```ts
import { describe, expect, it } from 'vitest'
import { getByPath, parseMappedFields } from '../services/topicMapping'
import type { TopicMapping } from '../types/domain'

const mapping: TopicMapping = {
  topics: {
    'thing/product/{sn}/osd': {
      description: 'OSD',
      fields: {
        'battery.capacity_percent': { zh: '电池电量', unit: '%' },
        mode_code: { zh: '模式码', values: { '0': '待机', '9': '自动返航' } },
      },
      groups: [{ id: 'basic', label: '基础信息', keys: ['mode_code', 'battery.capacity_percent'] }],
    },
  },
}

it('reads nested values by dot path', () => {
  expect(getByPath({ battery: { capacity_percent: 90 } }, 'battery.capacity_percent')).toBe(90)
})

it('maps fields with zh labels units and enum values', () => {
  const rows = parseMappedFields(mapping, 'thing/product/dock_001/osd', {
    mode_code: 9,
    battery: { capacity_percent: 90 },
  })

  expect(rows).toEqual([
    { key: 'mode_code', label: '模式码', value: '自动返航', rawValue: 9, unit: '', groupId: 'basic' },
    { key: 'battery.capacity_percent', label: '电池电量', value: '90', rawValue: 90, unit: '%', groupId: 'basic' },
  ])
})
```

- [ ] **Step 2: Verify tests fail**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm vitest run src/__tests__/topicMapping.test.ts
```

Expected: FAIL because `topicMapping.ts` does not exist.

- [ ] **Step 3: Implement parser**

Create `dji-cloud-api-tool-next/src/services/topicMapping.ts`:

```ts
import type { TopicMapping } from '../types/domain'

export type ParsedFieldRow = {
  key: string
  label: string
  value: string
  rawValue: unknown
  unit: string
  groupId: string
}

export function getByPath(source: unknown, path: string): unknown {
  return path.split('.').reduce<unknown>((current, part) => {
    if (current && typeof current === 'object' && part in current) {
      return (current as Record<string, unknown>)[part]
    }
    return undefined
  }, source)
}

export function matchTopicTemplate(mapping: TopicMapping, topic: string): string | undefined {
  return Object.keys(mapping.topics).find(template => {
    const pattern = '^' + template.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace('\\{sn\\}', '[^/]+') + '$'
    return new RegExp(pattern).test(topic)
  })
}

export function parseMappedFields(mapping: TopicMapping, topic: string, data: Record<string, unknown>): ParsedFieldRow[] {
  const template = matchTopicTemplate(mapping, topic)
  if (!template) return []
  const entry = mapping.topics[template]
  const groupByKey = new Map<string, string>()
  entry.groups.forEach(group => {
    group.keys.forEach(key => groupByKey.set(key, group.id))
  })

  return entry.groups.flatMap(group =>
    group.keys.flatMap(key => {
      const field = entry.fields[key]
      if (!field) return []
      const rawValue = getByPath(data, key)
      if (rawValue === undefined) return []
      const valueKey = String(rawValue)
      return [{
        key,
        label: field.zh,
        value: field.values?.[valueKey] ?? valueKey,
        rawValue,
        unit: field.unit ?? '',
        groupId: groupByKey.get(key) ?? group.id,
      }]
    }),
  )
}
```

- [ ] **Step 4: Verify tests pass**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm vitest run src/__tests__/topicMapping.test.ts
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next/src/services/topicMapping.ts dji-cloud-api-tool-next/src/__tests__/topicMapping.test.ts
git commit -m "feat: 实现 Topic 映射解析"
```

## Task 8: Build Main UI Layout

**Files:**
- Create: `dji-cloud-api-tool-next/src/components/AppShell.vue`
- Create: `dji-cloud-api-tool-next/src/components/DeviceTree.vue`
- Create: `dji-cloud-api-tool-next/src/components/TopicList.vue`
- Create: `dji-cloud-api-tool-next/src/components/OsdPanel.vue`
- Create: `dji-cloud-api-tool-next/src/components/ParsedJsonPanel.vue`
- Create: `dji-cloud-api-tool-next/src/components/RawJsonPanel.vue`
- Create: `dji-cloud-api-tool-next/src/components/PublishPanel.vue`
- Modify: `dji-cloud-api-tool-next/src/App.vue`
- Modify: `dji-cloud-api-tool-next/src/styles/app.css`

- [ ] **Step 1: Create placeholder components**

Each component should render its panel title and accept data from stores. For example, create `DeviceTree.vue`:

```vue
<template>
  <section class="panel device-tree">
    <header class="panel-header">
      <h2>设备列表</h2>
      <button title="添加设备">+</button>
      <button title="删除设备">x</button>
    </header>
    <div class="panel-body empty">请选择设备</div>
  </section>
</template>
```

Create similar title-only components for TopicList, OsdPanel, ParsedJsonPanel, RawJsonPanel, and PublishPanel using the old Qt labels.

- [ ] **Step 2: Build AppShell layout**

Create `dji-cloud-api-tool-next/src/components/AppShell.vue`:

```vue
<template>
  <div class="shell">
    <header class="toolbar">
      <button>配置</button>
      <span class="toolbar-spacer" />
      <span class="broker-label">未连接</span>
      <button class="primary">连接</button>
      <button>断开</button>
    </header>

    <main class="workspace">
      <aside class="left-column">
        <DeviceTree />
        <TopicList />
      </aside>
      <section class="right-column">
        <div class="monitor-grid">
          <div class="analysis-column">
            <OsdPanel />
            <ParsedJsonPanel />
          </div>
          <RawJsonPanel />
        </div>
        <PublishPanel />
      </section>
    </main>

    <footer class="statusbar">
      <span>未连接</span>
      <span>DJI Cloud API Tool Next</span>
      <span>设备: 0</span>
    </footer>
  </div>
</template>

<script setup lang="ts">
import DeviceTree from './DeviceTree.vue'
import OsdPanel from './OsdPanel.vue'
import ParsedJsonPanel from './ParsedJsonPanel.vue'
import PublishPanel from './PublishPanel.vue'
import RawJsonPanel from './RawJsonPanel.vue'
import TopicList from './TopicList.vue'
</script>
```

Modify `App.vue`:

```vue
<template>
  <AppShell />
</template>

<script setup lang="ts">
import AppShell from './components/AppShell.vue'
</script>
```

- [ ] **Step 3: Add layout CSS**

Add to `src/styles/app.css`:

```css
.shell {
  height: 100%;
  display: grid;
  grid-template-rows: auto 1fr auto;
}

.toolbar,
.statusbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background: #ffffff;
  border-bottom: 1px solid #dfe3ea;
}

.statusbar {
  border-top: 1px solid #dfe3ea;
  border-bottom: 0;
  justify-content: space-between;
  font-size: 12px;
}

.toolbar-spacer {
  flex: 1;
}

button {
  border: 1px solid #cfd6df;
  border-radius: 6px;
  background: #ffffff;
  padding: 6px 10px;
  cursor: pointer;
}

button.primary {
  background: #1f6feb;
  border-color: #1f6feb;
  color: #ffffff;
}

.workspace {
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(320px, 420px) 1fr;
  gap: 8px;
  padding: 8px;
}

.left-column,
.right-column,
.analysis-column {
  min-height: 0;
  display: grid;
  gap: 8px;
}

.left-column {
  grid-template-rows: 1fr 1fr;
}

.right-column {
  grid-template-rows: 1fr auto;
}

.monitor-grid {
  min-height: 0;
  display: grid;
  grid-template-columns: 2fr minmax(340px, 520px);
  gap: 8px;
}

.analysis-column {
  grid-template-rows: minmax(160px, auto) 1fr;
}

.panel {
  min-height: 0;
  background: #ffffff;
  border: 1px solid #dfe3ea;
  border-radius: 8px;
  display: grid;
  grid-template-rows: auto 1fr;
}

.panel-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px;
  border-bottom: 1px solid #edf0f4;
}

.panel-header h2 {
  margin: 0;
  font-size: 14px;
  flex: 1;
}

.panel-body {
  overflow: auto;
  padding: 8px;
}

.empty {
  color: #77808c;
}
```

- [ ] **Step 4: Verify build**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm build
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next/src
git commit -m "feat: 搭建新版主界面布局"
```

## Task 9: Wire Device and Topic Interactions

**Files:**
- Modify: `dji-cloud-api-tool-next/src/components/DeviceTree.vue`
- Modify: `dji-cloud-api-tool-next/src/components/TopicList.vue`
- Modify: `dji-cloud-api-tool-next/src/components/AppShell.vue`

- [ ] **Step 1: Implement DeviceTree interactions**

Use `useDeviceStore` and `useTopicStore`. Add buttons that prompt for type, SN, and name using `window.prompt`. On creation, add default OSD topic. On delete, confirm and remove child devices.

Expected code shape:

```ts
const devices = useDeviceStore()
const topics = useTopicStore()

function addDevice() {
  const selected = devices.selectedDevice
  const type = selected?.type === 'dock' ? 'aircraft' : window.prompt('类型 dock/aircraft', 'dock')
  if (type !== 'dock' && type !== 'aircraft') return
  const sn = window.prompt('设备序列号 SN')
  if (!sn) return
  const name = window.prompt('设备名称', sn) || sn
  devices.addDevice(sn, name, type, selected?.type === 'dock' ? selected.sn : undefined)
  topics.addDefaultTopic(sn)
}
```

- [ ] **Step 2: Implement TopicList interactions**

Use `useTopicStore` and current selected device. Render enabled topics with `●` and disabled topics with `○`. Implement add, toggle, remove, move up, move down, toggle all, and double-click copy with `navigator.clipboard.writeText(topic.topic)`.

- [ ] **Step 3: Verify manually in dev server**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm dev
```

Expected: UI opens in browser dev mode; adding a device creates a default OSD topic; topic move/toggle/delete updates the list.

- [ ] **Step 4: Verify build**

Run:

```bash
pnpm build
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next/src/components dji-cloud-api-tool-next/src/stores
git commit -m "feat: 实现设备和 Topic 交互"
```

## Task 10: Wire MQTT Events, Raw JSON, Parsed JSON, and OSD

**Files:**
- Modify: `dji-cloud-api-tool-next/src/components/AppShell.vue`
- Modify: `dji-cloud-api-tool-next/src/components/RawJsonPanel.vue`
- Modify: `dji-cloud-api-tool-next/src/components/ParsedJsonPanel.vue`
- Modify: `dji-cloud-api-tool-next/src/components/OsdPanel.vue`
- Modify: `dji-cloud-api-tool-next/src/stores/monitorStore.ts`
- Create: `dji-cloud-api-tool-next/src/__tests__/monitorStore.test.ts`

- [ ] **Step 1: Write monitor history limit test**

Create `monitorStore.test.ts`:

```ts
import { setActivePinia, createPinia } from 'pinia'
import { beforeEach, describe, expect, it } from 'vitest'
import { useMonitorStore } from '../stores/monitorStore'

describe('monitorStore', () => {
  beforeEach(() => setActivePinia(createPinia()))

  it('keeps only 500 latest messages', () => {
    const store = useMonitorStore()
    for (let i = 0; i < 501; i += 1) {
      store.append({
        connectionId: 'default',
        topic: 'thing/product/dock_001/osd',
        payloadText: JSON.stringify({ index: i }),
        receivedAt: String(i),
        deviceSn: 'dock_001',
      })
    }

    expect(store.messages).toHaveLength(500)
    expect(store.messages[0].payloadText).toBe(JSON.stringify({ index: 1 }))
  })
})
```

- [ ] **Step 2: Register Tauri MQTT event listeners**

In `AppShell.vue`, import `listen` from `@tauri-apps/api/event`, append `mqtt:message` events to `monitorStore`, and update connection state on connected/disconnected/error events.

- [ ] **Step 3: Implement RawJsonPanel**

Render filtered messages for selected device and selected topic. Add pause/resume, copy latest, copy all, and clear buttons.

- [ ] **Step 4: Implement ParsedJsonPanel**

Parse latest selected message. If payload has a `data` object, pass `data` to `parseMappedFields`; otherwise parse the root object. Render grouped rows.

- [ ] **Step 5: Implement OsdPanel**

Render selected device name, SN, type, online state, last seen, and high-value OSD fields from the latest JSON payload, including battery, speed, position, dock cover, temperature, humidity where available.

- [ ] **Step 6: Verify tests and build**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm vitest run
pnpm build
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add dji-cloud-api-tool-next/src
git commit -m "feat: 实现实时消息展示面板"
```

## Task 11: Implement Connections Dialog and Persistence

**Files:**
- Create: `dji-cloud-api-tool-next/src/components/ConnectionDialog.vue`
- Modify: `dji-cloud-api-tool-next/src/components/AppShell.vue`
- Modify: `dji-cloud-api-tool-next/src/stores/connectionStore.ts`
- Modify: `dji-cloud-api-tool-next/src/stores/deviceStore.ts`
- Modify: `dji-cloud-api-tool-next/src/stores/topicStore.ts`

- [ ] **Step 1: Implement load/save actions**

Add actions in stores that call `tauriApi.loadConnections`, `saveConnections`, `loadDevices`, `saveDevices`, `loadTopics`, and `saveTopics`.

- [ ] **Step 2: Implement ConnectionDialog**

Render current profiles and fields for name, host, port, clientId, username, password, TLS enabled. Add buttons for add, rename through editing name, delete, test, save, cancel.

- [ ] **Step 3: Wire toolbar**

In `AppShell.vue`, open the dialog from the 配置 button. Connect button calls `tauriApi.connect(currentProfile, enabledTopics, devices)`. Disconnect button calls `tauriApi.disconnect()`.

- [ ] **Step 4: Persist device and topic changes**

After add/remove/reorder/toggle device or topic, call the corresponding save action so app restart preserves state.

- [ ] **Step 5: Verify build**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm build
cd src-tauri
cargo test
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add dji-cloud-api-tool-next/src dji-cloud-api-tool-next/src-tauri
git commit -m "feat: 实现连接配置和持久化"
```

## Task 12: Implement Publish Panel

**Files:**
- Modify: `dji-cloud-api-tool-next/src/components/PublishPanel.vue`

- [ ] **Step 1: Add publish form**

Render collapsible panel with selected device SN, topic dropdown from current device topics, custom topic input, JSON payload textarea, and send button.

- [ ] **Step 2: Validate JSON payload**

Before publishing, run `JSON.parse(payloadText)`. If parsing fails, show `JSON 格式错误: <message>` and do not call backend.

- [ ] **Step 3: Call publish command**

Call `tauriApi.publishMessage(topic, payloadText)`. Show success or failure status in the panel.

- [ ] **Step 4: Verify build**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm build
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next/src/components/PublishPanel.vue
git commit -m "feat: 实现 Topic 下发面板"
```

## Task 13: Add Default Topic Mappings and Import/Export

**Files:**
- Create: `dji-cloud-api-tool-next/src/assets/default-topic-mappings.json`
- Modify: `dji-cloud-api-tool-next/src/services/topicMapping.ts`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/commands.rs`
- Modify: `dji-cloud-api-tool-next/src-tauri/src/config_store.rs`

- [ ] **Step 1: Add default mappings**

Create `src/assets/default-topic-mappings.json` using the original Qt built-in mapping as the initial content, keeping fields, units, values, and groups.

- [ ] **Step 2: Load default mapping in frontend**

If user mapping is unavailable, import default JSON and use it in ParsedJsonPanel.

- [ ] **Step 3: Add export/import commands**

Implement `export_config` and `import_config` commands that copy `connections.json`, `devices.json`, `topics.json`, `topic-mappings.json`, and `app-settings.json` to/from a selected directory or zip-like folder.

- [ ] **Step 4: Verify tests and build**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm build
cd src-tauri
cargo test
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next
git commit -m "feat: 添加默认映射和配置导入导出"
```

## Task 14: Package and Smoke Test

**Files:**
- Modify: `dji-cloud-api-tool-next/README.md`
- Modify: `dji-cloud-api-tool-next/src-tauri/tauri.conf.json`
- Optionally create: `dji-cloud-api-tool-next/docs/release-checklist.md`

- [ ] **Step 1: Set app metadata**

Update `tauri.conf.json` with product name `DJI Cloud API Tool Next`, identifier, window size 1280x760, and bundle targets for Windows, macOS, and Linux.

- [ ] **Step 2: Update README usage**

Document development commands:

```bash
pnpm install
pnpm tauri dev
pnpm build
pnpm tauri build
```

Document app data files and the fact that old Qt config files are not compatible.

- [ ] **Step 3: Full verification**

Run:

```bash
cd "/Users/yanxl/Documents/大疆上云 api 工具/dji-cloud-api-tool-next"
pnpm vitest run
pnpm build
cd src-tauri
cargo test
cd ..
pnpm tauri build
```

Expected: tests pass, frontend builds, Rust tests pass, and a platform bundle is produced for the current OS.

- [ ] **Step 4: Manual smoke test**

Launch the built app. Verify:

- App starts and shows main layout.
- Add Connection dialog opens.
- Add Dock creates default OSD Topic.
- Add Aircraft under Dock works.
- Topic enable/disable/reorder works.
- Raw JSON remains empty before connection.
- Publish panel rejects invalid JSON.

- [ ] **Step 5: Commit**

```bash
git add dji-cloud-api-tool-next
git commit -m "chore: 完成新版打包和使用说明"
```

## Self-Review

Spec coverage:

- Tauri + Vue + Rust MQTT backend: covered by Tasks 1, 5, 6, and 14.
- Old Qt feature migration: covered by Tasks 8 through 13.
- New config format and no old config compatibility: covered by Tasks 3, 11, 13, and README updates in Task 14.
- MQTT/TLS, subscribe, publish, reconnect shell: covered by Task 5, with room to deepen reconnect behavior during implementation.
- Testing: Rust tests, Vitest tests, and smoke tests are included across tasks.
- Packaging: covered by Task 14.

Placeholder scan:

- No `TBD`, `TODO`, or undefined placeholder steps are intentionally left in this plan.

Type consistency:

- TypeScript uses camelCase properties matching Tauri JSON serialization.
- Rust models use `serde(rename_all = "camelCase")` where frontend interop requires it.
- Store method names are introduced before they are used by component tasks.
