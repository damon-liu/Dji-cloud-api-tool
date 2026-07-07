use crate::config_store::ConfigStore;
use crate::models::{ConnectionProfile, TlsConfig};
use std::fs;
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

#[test]
fn exports_existing_config_files_and_skips_missing_files() {
    let app_dir = tempdir().unwrap();
    let export_dir = tempdir().unwrap();
    let store = ConfigStore::new(app_dir.path().to_path_buf());

    store.save_connections(&[profile()]).unwrap();
    fs::write(app_dir.path().join("app-settings.json"), "{\"theme\":\"light\"}").unwrap();

    store.export_config(export_dir.path()).unwrap();

    assert!(export_dir.path().join("connections.json").exists());
    assert!(export_dir.path().join("app-settings.json").exists());
    assert!(!export_dir.path().join("devices.json").exists());
}

#[test]
fn imports_existing_config_files_and_skips_missing_files() {
    let app_dir = tempdir().unwrap();
    let import_dir = tempdir().unwrap();
    let store = ConfigStore::new(app_dir.path().to_path_buf());

    fs::write(import_dir.path().join("topics.json"), "[]").unwrap();
    fs::write(import_dir.path().join("topic-mappings.json"), "{\"topics\":{}}").unwrap();

    store.import_config(import_dir.path()).unwrap();

    assert_eq!(fs::read_to_string(app_dir.path().join("topics.json")).unwrap(), "[]");
    assert_eq!(
        fs::read_to_string(app_dir.path().join("topic-mappings.json")).unwrap(),
        "{\"topics\":{}}",
    );
    assert!(!app_dir.path().join("connections.json").exists());
}
