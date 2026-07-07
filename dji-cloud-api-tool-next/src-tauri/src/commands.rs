use crate::config_store::ConfigStore;
use crate::models::{ConnectionProfile, Device, DeviceTopic};
use crate::mqtt_service::MqttService;
use std::path::Path;
use tauri::{AppHandle, Runtime, State};

fn command_error(error: impl std::fmt::Display) -> String {
    error.to_string()
}

#[tauri::command]
pub fn load_connections(store: State<'_, ConfigStore>) -> Result<Vec<ConnectionProfile>, String> {
    store.load_connections().map_err(command_error)
}

#[tauri::command]
pub fn save_connections(
    store: State<'_, ConfigStore>,
    profiles: Vec<ConnectionProfile>,
) -> Result<(), String> {
    store.save_connections(&profiles).map_err(command_error)
}

#[tauri::command]
pub fn load_devices(store: State<'_, ConfigStore>) -> Result<Vec<Device>, String> {
    store.load_devices().map_err(command_error)
}

#[tauri::command]
pub fn save_devices(store: State<'_, ConfigStore>, devices: Vec<Device>) -> Result<(), String> {
    store.save_devices(&devices).map_err(command_error)
}

#[tauri::command]
pub fn load_topics(store: State<'_, ConfigStore>) -> Result<Vec<DeviceTopic>, String> {
    store.load_topics().map_err(command_error)
}

#[tauri::command]
pub fn save_topics(store: State<'_, ConfigStore>, topics: Vec<DeviceTopic>) -> Result<(), String> {
    store.save_topics(&topics).map_err(command_error)
}

#[tauri::command]
pub fn export_config(store: State<'_, ConfigStore>, directory: String) -> Result<(), String> {
    store
        .export_config(Path::new(&directory))
        .map_err(command_error)
}

#[tauri::command]
pub fn import_config(store: State<'_, ConfigStore>, directory: String) -> Result<(), String> {
    store
        .import_config(Path::new(&directory))
        .map_err(command_error)
}

#[tauri::command]
pub async fn connect<R: Runtime>(
    app: AppHandle<R>,
    service: State<'_, MqttService>,
    profile: ConnectionProfile,
    topics: Vec<DeviceTopic>,
    devices: Vec<Device>,
) -> Result<(), String> {
    let service = service.inner().clone();
    service
        .connect(app, profile, topics, devices)
        .await
        .map_err(command_error)
}

#[tauri::command]
pub async fn disconnect(service: State<'_, MqttService>) -> Result<(), String> {
    let service = service.inner().clone();
    service.disconnect().await.map_err(command_error)
}

#[tauri::command]
pub async fn publish_message(
    service: State<'_, MqttService>,
    topic: String,
    payload_text: String,
) -> Result<(), String> {
    let service = service.inner().clone();
    service
        .publish(topic, payload_text)
        .await
        .map_err(command_error)
}
