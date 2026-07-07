use crate::models::{Device, DeviceType};
use crate::topic_matcher::{default_osd_topic, match_device_sn};

fn device(sn: &str) -> Device {
    Device {
        sn: sn.to_string(),
        name: sn.to_string(),
        device_type: DeviceType::Dock,
        parent_sn: None,
        online: true,
        last_seen_at: None,
    }
}

#[test]
fn matches_sn_contained_in_topic() {
    let devices = vec![device("dock_001")];

    let matched = match_device_sn("thing/product/dock_001/osd", &devices);

    assert_eq!(matched, Some("dock_001".to_string()));
}

#[test]
fn returns_none_when_no_device_matches() {
    let devices = vec![device("dock_001")];

    let matched = match_device_sn("thing/product/dock_002/osd", &devices);

    assert_eq!(matched, None);
}

#[test]
fn ignores_empty_device_sn() {
    let devices = vec![device("")];

    let matched = match_device_sn("thing/product/dock_001/osd", &devices);

    assert_eq!(matched, None);
}

#[test]
fn prefers_longest_matching_sn() {
    let devices = vec![device("dock"), device("dock_001")];

    let matched = match_device_sn("thing/product/dock_001/osd", &devices);

    assert_eq!(matched, Some("dock_001".to_string()));
}

#[test]
fn builds_default_osd_topic() {
    assert_eq!(default_osd_topic("dock_001"), "thing/product/dock_001/osd");
}
