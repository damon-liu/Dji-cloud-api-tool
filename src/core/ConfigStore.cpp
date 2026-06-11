#include "ConfigStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

ConfigStore::ConfigStore(QObject* parent)
    : QObject(parent) {}

bool ConfigStore::load(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ConfigStore: cannot open" << filePath << ", creating default";
        return save(filePath);  // 首次运行生成默认模板
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "ConfigStore: invalid JSON, creating default";
        return save(filePath);
    }

    QJsonObject root = doc.object();

    // 解析 MQTT 配置
    QJsonObject mqtt = root["mqtt"].toObject();
    mMqttConfig.host     = mqtt.value("host").toString("192.168.1.100");
    mMqttConfig.port     = mqtt.value("port").toInt(8883);
    mMqttConfig.username = mqtt.value("username").toString();
    mMqttConfig.password = mqtt.value("password").toString();

    // 解析设备列表
    mDevices.clear();
    mDeviceTopics.clear();
    QJsonArray devs = root["devices"].toArray();
    for (const auto& val : devs) {
        QJsonObject devObj = val.toObject();
        DeviceInfo info = DeviceInfo::fromJson(devObj);

        // 该设备自己的所有 topic
        QStringList allTopics;
        QJsonArray topicArr = devObj["topics"].toArray();
        for (const auto& t : topicArr)
            allTopics.append(t.toString());

        if (info.type == DeviceType::Dock) {
            // 机场 topics（只保留包含 dock_sn 的）
            QStringList dockTopics;
            for (const auto& t : allTopics) {
                if (t.contains(info.sn))
                    dockTopics.append(t);
            }
            mDeviceTopics[info.sn] = dockTopics;
            mDevices.append(info);

            // 子飞机
            QString aircraftSn = devObj.value("aircraft_sn").toString();
            if (!aircraftSn.isEmpty()) {
                DeviceInfo child;
                child.sn       = aircraftSn;
                child.name     = info.name + "-飞机";
                child.type     = DeviceType::Aircraft;
                child.parentSn = info.sn;
                mDevices.append(child);

                // 子飞机 topics
                QStringList childTopics;
                for (const auto& t : allTopics) {
                    if (t.contains(aircraftSn))
                        childTopics.append(t);
                }
                mDeviceTopics[child.sn] = childTopics;
            }
        } else {
            // 独立手飞
            mDeviceTopics[info.sn] = allTopics;
            mDevices.append(info);
        }
    }

    qDebug() << "ConfigStore: loaded" << mDevices.size() << "devices";
    return true;
}

bool ConfigStore::save(const QString& filePath) {
    QJsonObject root;

    // MQTT
    QJsonObject mqtt;
    mqtt["host"]     = mMqttConfig.host;
    mqtt["port"]     = mMqttConfig.port;
    mqtt["username"] = mMqttConfig.username;
    mqtt["password"] = mMqttConfig.password;
    root["mqtt"]     = mqtt;

    // Devices — 按父设备聚合
    QMap<QString, QJsonObject> dockMap;
    QVector<QJsonObject> pilotList;

    for (const auto& d : mDevices) {
        if (d.type == DeviceType::Dock) {
            QJsonObject obj = d.toJson();
            obj["aircraft_sn"] = "";
            QJsonArray topics;
            for (const auto& t : mDeviceTopics.value(d.sn))
                topics.append(t);
            obj["topics"] = topics;
            dockMap[d.sn] = obj;
        } else if (d.isChild()) {
            // 库内飞机合并到父机场
            if (dockMap.contains(d.parentSn)) {
                dockMap[d.parentSn]["aircraft_sn"] = d.sn;
                QJsonArray topics = dockMap[d.parentSn]["topics"].toArray();
                for (const auto& t : mDeviceTopics.value(d.sn))
                    topics.append(t);
                dockMap[d.parentSn]["topics"] = topics;
            }
        } else {
            // 独立手飞
            QJsonObject obj = d.toJson();
            QJsonArray topics;
            for (const auto& t : mDeviceTopics.value(d.sn))
                topics.append(t);
            obj["topics"] = topics;
            pilotList.append(obj);
        }
    }

    QJsonArray devs;
    for (const auto& obj : dockMap)
        devs.append(obj);
    for (const auto& obj : pilotList)
        devs.append(obj);
    root["devices"] = devs;

    // 写入文件
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ConfigStore: cannot write" << filePath;
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    emit configChanged();
    return true;
}

MqttConfig ConfigStore::mqttConfig() const { return mMqttConfig; }

void ConfigStore::setMqttConfig(const MqttConfig& config) {
    mMqttConfig = config;
}

QVector<DeviceInfo> ConfigStore::devices() const { return mDevices; }

void ConfigStore::setDevices(const QVector<DeviceInfo>& devices) {
    mDevices = devices;
}

QStringList ConfigStore::topicsForDevice(const QString& sn) const {
    return mDeviceTopics.value(sn);
}

void ConfigStore::setTopicsForDevice(const QString& sn, const QStringList& topics) {
    mDeviceTopics[sn] = topics;
}
