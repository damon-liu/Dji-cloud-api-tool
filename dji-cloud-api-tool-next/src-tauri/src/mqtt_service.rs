use crate::models::{ConnectionProfile, Device, DeviceTopic, MqttRuntimeMessage};
use crate::topic_matcher::match_device_sn;
use chrono::Utc;
use rumqttc::{AsyncClient, Event, MqttOptions, Outgoing, Packet, QoS, Transport};
use serde::Serialize;
use std::sync::Arc;
use std::time::Duration;
use tauri::{AppHandle, Emitter, Runtime};
use thiserror::Error;
use tokio::sync::Mutex;

#[derive(Debug, Error)]
pub enum MqttServiceError {
    #[error("host is required")]
    EmptyHost,
    #[error("port must be between 1 and 65535")]
    InvalidPort,
    #[error("{0}")]
    Client(String),
}

pub type Result<T> = std::result::Result<T, MqttServiceError>;

pub fn validate_connection_profile(profile: &ConnectionProfile) -> Result<()> {
    if profile.host.trim().is_empty() {
        return Err(MqttServiceError::EmptyHost);
    }

    if profile.port == 0 {
        return Err(MqttServiceError::InvalidPort);
    }

    Ok(())
}

#[derive(Default)]
struct MqttServiceState {
    client: Option<AsyncClient>,
    connection_id: Option<String>,
}

#[derive(Clone, Default)]
pub struct MqttService {
    state: Arc<Mutex<MqttServiceState>>,
}

#[derive(Clone, Serialize)]
#[serde(rename_all = "camelCase")]
struct ConnectionEvent {
    connection_id: String,
}

#[derive(Clone, Serialize)]
#[serde(rename_all = "camelCase")]
struct ErrorEvent {
    connection_id: Option<String>,
    message: String,
}

impl MqttService {
    pub async fn connect<R: Runtime>(
        &self,
        app: AppHandle<R>,
        profile: ConnectionProfile,
        topics: Vec<DeviceTopic>,
        devices: Vec<Device>,
    ) -> Result<()> {
        validate_connection_profile(&profile)?;

        self.disconnect().await?;

        let connection_id = profile.id.clone();
        let client_id = profile
            .client_id
            .clone()
            .filter(|id| !id.trim().is_empty())
            .unwrap_or_else(|| format!("dji-cloud-api-tool-next-{}", uuid::Uuid::new_v4()));
        let mut options = MqttOptions::new(client_id, profile.host.trim(), profile.port);
        options.set_keep_alive(Duration::from_secs(30));

        if let Some(username) = profile.username.as_ref().filter(|value| !value.is_empty()) {
            options.set_credentials(username, profile.password.clone().unwrap_or_default());
        }

        if profile.tls.enabled {
            options.set_transport(Transport::tls_with_default_config());
        }

        let (client, mut eventloop) = AsyncClient::new(options, 100);
        let enabled_topics: Vec<String> = topics
            .iter()
            .filter(|topic| topic.enabled && !topic.topic.trim().is_empty())
            .map(|topic| topic.topic.trim().to_string())
            .collect();

        for topic in &enabled_topics {
            client
                .subscribe(topic, QoS::AtLeastOnce)
                .await
                .map_err(|error| MqttServiceError::Client(error.to_string()))?;
        }

        {
            let mut state = self.state.lock().await;
            state.client = Some(client);
            state.connection_id = Some(connection_id.clone());
        }

        emit(
            &app,
            "mqtt:connected",
            ConnectionEvent {
                connection_id: connection_id.clone(),
            },
        );

        let service = self.clone();
        tauri::async_runtime::spawn(async move {
            loop {
                match eventloop.poll().await {
                    Ok(Event::Incoming(Packet::Publish(publish))) => {
                        let topic = publish.topic.clone();
                        let payload_text = String::from_utf8_lossy(&publish.payload).to_string();
                        let message = MqttRuntimeMessage {
                            connection_id: connection_id.clone(),
                            device_sn: match_device_sn(&topic, &devices),
                            topic,
                            payload_text,
                            received_at: Utc::now().to_rfc3339(),
                        };

                        emit(&app, "mqtt:message", message);
                    }
                    Ok(Event::Outgoing(Outgoing::Disconnect)) => break,
                    Ok(_) => {}
                    Err(error) => {
                        if !service.is_current_connection(&connection_id).await {
                            break;
                        }

                        emit(
                            &app,
                            "mqtt:error",
                            ErrorEvent {
                                connection_id: Some(connection_id.clone()),
                                message: error.to_string(),
                            },
                        );
                        break;
                    }
                }
            }

            emit(
                &app,
                "mqtt:disconnected",
                ConnectionEvent {
                    connection_id: connection_id.clone(),
                },
            );

            let mut state = service.state.lock().await;
            if state.connection_id.as_deref() == Some(connection_id.as_str()) {
                state.client = None;
                state.connection_id = None;
            }
        });

        Ok(())
    }

    async fn is_current_connection(&self, connection_id: &str) -> bool {
        let state = self.state.lock().await;
        state.connection_id.as_deref() == Some(connection_id)
    }

    pub async fn disconnect(&self) -> Result<()> {
        let client = {
            let mut state = self.state.lock().await;
            state.connection_id = None;
            state.client.take()
        };

        if let Some(client) = client {
            client
                .disconnect()
                .await
                .map_err(|error| MqttServiceError::Client(error.to_string()))?;
        }

        Ok(())
    }

    pub async fn publish(&self, topic: String, payload_text: String) -> Result<()> {
        let client = {
            let state = self.state.lock().await;
            state.client.clone()
        }
        .ok_or_else(|| MqttServiceError::Client("not connected".to_string()))?;

        client
            .publish(topic, QoS::AtLeastOnce, false, payload_text)
            .await
            .map_err(|error| MqttServiceError::Client(error.to_string()))
    }
}

fn emit<R, S>(app: &AppHandle<R>, event: &str, payload: S)
where
    R: Runtime,
    S: Serialize + Clone,
{
    let _ = app.emit(event, payload);
}
