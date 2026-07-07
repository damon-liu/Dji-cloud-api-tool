use crate::models::Device;

pub fn match_device_sn(topic: &str, devices: &[Device]) -> Option<String> {
    devices
        .iter()
        .filter(|device| topic.contains(&device.sn))
        .max_by_key(|device| device.sn.len())
        .map(|device| device.sn.clone())
}

pub fn default_osd_topic(sn: &str) -> String {
    format!("thing/product/{sn}/osd")
}
