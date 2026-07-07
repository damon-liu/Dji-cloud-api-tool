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
