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

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .manage(config_store::ConfigStore::app_data_default())
        .manage(mqtt_service::MqttService::default())
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            commands::load_connections,
            commands::save_connections,
            commands::load_devices,
            commands::save_devices,
            commands::load_topics,
            commands::save_topics,
            commands::export_config,
            commands::import_config,
            commands::connect,
            commands::disconnect,
            commands::publish_message,
        ])
        .run(tauri::generate_context!())
        .expect("error while running Tauri application");
}
