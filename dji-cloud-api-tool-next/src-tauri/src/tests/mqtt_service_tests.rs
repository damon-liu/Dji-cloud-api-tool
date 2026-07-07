use crate::models::{ConnectionProfile, TlsConfig};
use crate::mqtt_service::validate_connection_profile;

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
fn accepts_valid_profile() {
    let result = validate_connection_profile(&profile());

    assert!(result.is_ok());
}

#[test]
fn rejects_empty_host() {
    let mut profile = profile();
    profile.host = "  ".to_string();

    let error = validate_connection_profile(&profile).unwrap_err();

    assert!(error.to_string().contains("host is required"));
}

#[test]
fn rejects_zero_port() {
    let mut profile = profile();
    profile.port = 0;

    let error = validate_connection_profile(&profile).unwrap_err();

    assert!(error
        .to_string()
        .contains("port must be between 1 and 65535"));
}
