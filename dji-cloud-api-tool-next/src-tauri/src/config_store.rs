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
    Read {
        path: String,
        source: std::io::Error,
    },
    #[error("failed to write config file {path}: {source}")]
    Write {
        path: String,
        source: std::io::Error,
    },
    #[error("failed to parse config file {path}: {source}")]
    Parse {
        path: String,
        source: serde_json::Error,
    },
    #[error("failed to serialize config file {path}: {source}")]
    Serialize {
        path: String,
        source: serde_json::Error,
    },
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
        let text =
            serde_json::to_string_pretty(value).map_err(|source| ConfigStoreError::Serialize {
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
