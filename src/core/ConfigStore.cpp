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
    mDisabledTopics.clear();
    QJsonArray devs = root["devices"].toArray();
    for (const auto& val : devs) {
        QJsonObject devObj = val.toObject();
        DeviceInfo info = DeviceInfo::fromJson(devObj);

        // 该设备自己的所有 topic
        QSet<QString> allTopics;
        QJsonArray topicArr = devObj["topics"].toArray();
        for (const auto& t : topicArr)
            allTopics.insert(t.toString());

        // 该设备禁用的 topic（向后兼容：旧配置无此字段）
        QSet<QString> disabledTopics;
        QJsonArray disabledArr = devObj["disabled_topics"].toArray();
        for (const auto& t : disabledArr)
            disabledTopics.insert(t.toString());

        if (info.type == DeviceType::Dock) {
            // 机场 topics（只保留包含 dock_sn 的）
            QSet<QString> dockTopics;
            QSet<QString> dockDisabled;
            for (const auto& t : allTopics) {
                if (t.contains(info.sn))
                    dockTopics.insert(t);
            }
            for (const auto& t : disabledTopics) {
                if (t.contains(info.sn))
                    dockDisabled.insert(t);
            }
            mDeviceTopics[info.sn] = dockTopics;
            if (!dockDisabled.isEmpty())
                mDisabledTopics[info.sn] = dockDisabled;
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
                QSet<QString> childTopics;
                QSet<QString> childDisabled;
                for (const auto& t : allTopics) {
                    if (t.contains(aircraftSn))
                        childTopics.insert(t);
                }
                for (const auto& t : disabledTopics) {
                    if (t.contains(aircraftSn))
                        childDisabled.insert(t);
                }
                mDeviceTopics[child.sn] = childTopics;
                if (!childDisabled.isEmpty())
                    mDisabledTopics[child.sn] = childDisabled;
            }
        } else {
            // 独立手飞
            mDeviceTopics[info.sn] = allTopics;
            if (!disabledTopics.isEmpty())
                mDisabledTopics[info.sn] = disabledTopics;
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
            // 禁用 topic
            QJsonArray disabledArr;
            QSet<QString> deviceDisabled = mDisabledTopics.value(d.sn);
            for (const auto& t : deviceDisabled)
                disabledArr.append(t);
            if (!disabledArr.isEmpty())
                obj["disabled_topics"] = disabledArr;
            dockMap[d.sn] = obj;
        } else if (d.isChild()) {
            // 库内飞机合并到父机场
            if (dockMap.contains(d.parentSn)) {
                dockMap[d.parentSn]["aircraft_sn"] = d.sn;
                QJsonArray topics = dockMap[d.parentSn]["topics"].toArray();
                for (const auto& t : mDeviceTopics.value(d.sn))
                    topics.append(t);
                dockMap[d.parentSn]["topics"] = topics;
                // 合并子飞机禁用 topic 到父条目
                QSet<QString> childDisabled = mDisabledTopics.value(d.sn);
                if (!childDisabled.isEmpty()) {
                    QJsonArray existingDisabled = dockMap[d.parentSn]["disabled_topics"].toArray();
                    for (const auto& t : childDisabled)
                        existingDisabled.append(t);
                    dockMap[d.parentSn]["disabled_topics"] = existingDisabled;
                }
            }
        } else {
            // 独立手飞
            QJsonObject obj = d.toJson();
            QJsonArray topics;
            for (const auto& t : mDeviceTopics.value(d.sn))
                topics.append(t);
            obj["topics"] = topics;
            // 禁用 topic
            QJsonArray disabledArr;
            QSet<QString> deviceDisabled = mDisabledTopics.value(d.sn);
            for (const auto& t : deviceDisabled)
                disabledArr.append(t);
            if (!disabledArr.isEmpty())
                obj["disabled_topics"] = disabledArr;
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
    return mDeviceTopics.value(sn).values();
}

void ConfigStore::setTopicsForDevice(const QString& sn, const QStringList& topics) {
    mDeviceTopics[sn] = QSet<QString>(topics.begin(), topics.end());
}

QStringList ConfigStore::disabledTopicsForDevice(const QString& sn) const {
    return mDisabledTopics.value(sn).values();
}

void ConfigStore::setDisabledTopicsForDevice(const QString& sn, const QStringList& topics) {
    if (topics.isEmpty())
        mDisabledTopics.remove(sn);
    else
        mDisabledTopics[sn] = QSet<QString>(topics.begin(), topics.end());
}
