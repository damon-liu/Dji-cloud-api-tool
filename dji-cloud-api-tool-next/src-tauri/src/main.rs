fn main() {
    tauri::Builder::default()
        .manage(dji_cloud_api_tool_next_lib::config_store::ConfigStore::app_data_default())
        .manage(dji_cloud_api_tool_next_lib::mqtt_service::MqttService::default())
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            dji_cloud_api_tool_next_lib::commands::load_connections,
            dji_cloud_api_tool_next_lib::commands::save_connections,
            dji_cloud_api_tool_next_lib::commands::load_devices,
            dji_cloud_api_tool_next_lib::commands::save_devices,
            dji_cloud_api_tool_next_lib::commands::load_topics,
            dji_cloud_api_tool_next_lib::commands::save_topics,
            dji_cloud_api_tool_next_lib::commands::connect,
            dji_cloud_api_tool_next_lib::commands::disconnect,
            dji_cloud_api_tool_next_lib::commands::publish_message,
        ])
        .run(tauri::generate_context!())
        .expect("error while running Tauri application");
}
