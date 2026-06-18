#include "ConfigStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

ConfigStore::ConfigStore(QObject* parent)
    : QObject(parent) {}

ProfileData& ConfigStore::currentProfileData() {
    return mProfiles[mCurrentProfile];
}

const ProfileData& ConfigStore::currentProfileData() const {
    return const_cast<ConfigStore*>(this)->mProfiles[mCurrentProfile];
}


// —— Load / Save ——

bool ConfigStore::load(const QString& filePath) {
    mConfigPath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ConfigStore: cannot open" << filePath << ", creating default";
        mCurrentProfile = QString::fromUtf8("\xe9\xbb\x98\xe8\xae\xa4");
        mProfiles[mCurrentProfile] = ProfileData{mCurrentProfile, {}, {}, {}, {}};
        return save(filePath);
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "ConfigStore: invalid JSON, creating default";
        mCurrentProfile = QString::fromUtf8("\xe9\xbb\x98\xe8\xae\xa4");
        mProfiles[mCurrentProfile] = ProfileData{mCurrentProfile, {}, {}, {}, {}};
        return save(filePath);
    }

    QJsonObject root = doc.object();

    // 新格式：profiles 数组
    if (root.contains("profiles")) {
        mCurrentProfile = root.value("current_profile").toString(QString::fromUtf8("\xe9\xbb\x98\xe8\xae\xa4"));
        mProfiles.clear();

        QJsonArray profileArr = root["profiles"].toArray();
        for (const auto& pVal : profileArr) {
            QJsonObject pObj = pVal.toObject();
            ProfileData pd;
            pd.name = pObj.value("name").toString();

            QJsonObject mqtt = pObj["mqtt"].toObject();
            pd.mqtt.host     = mqtt.value("host").toString("192.168.1.100");
            pd.mqtt.port     = mqtt.value("port").toInt(8883);
            pd.mqtt.username = mqtt.value("username").toString();
            pd.mqtt.password = mqtt.value("password").toString();

            QJsonArray devs = pObj["devices"].toArray();
            for (const auto& dVal : devs) {
                QJsonObject devObj = dVal.toObject();
                DeviceInfo info = DeviceInfo::fromJson(devObj);

                QStringList allTopics;
                QJsonArray topicArr = devObj["topics"].toArray();
                for (const auto& t : topicArr) {
                    QString topic = t.toString();
                    if (!allTopics.contains(topic))
                        allTopics.append(topic);
                }

                QSet<QString> disabledTopics;
                QJsonArray disabledArr = devObj["disabled_topics"].toArray();
                for (const auto& t : disabledArr)
                    disabledTopics.insert(t.toString());

                if (info.type == DeviceType::Dock) {
                    QStringList dockTopics;
                    QSet<QString> dockDisabled;
                    for (const auto& t : allTopics) {
                        if (t.contains(info.sn)) dockTopics.append(t);
                    }
                    for (const auto& t : disabledTopics) {
                        if (t.contains(info.sn)) dockDisabled.insert(t);
                    }
                    pd.deviceTopics[info.sn] = dockTopics;
                    if (!dockDisabled.isEmpty())
                        pd.disabledTopics[info.sn] = dockDisabled;
                    pd.devices.append(info);

                    QString aircraftSn = devObj.value("aircraft_sn").toString();
                    if (!aircraftSn.isEmpty()) {
                        DeviceInfo child;
                        child.sn = aircraftSn;
                        child.name = info.name + QString::fromUtf8("-\xe9\xa3\x9e\xe6\x9c\xba");
                        child.type = DeviceType::Aircraft;
                        child.parentSn = info.sn;
                        pd.devices.append(child);

                        QStringList childTopics;
                        QSet<QString> childDisabled;
                        for (const auto& t : allTopics) {
                            if (t.contains(aircraftSn)) childTopics.append(t);
                        }
                        for (const auto& t : disabledTopics) {
                            if (t.contains(aircraftSn)) childDisabled.insert(t);
                        }
                        pd.deviceTopics[child.sn] = childTopics;
                        if (!childDisabled.isEmpty())
                            pd.disabledTopics[child.sn] = childDisabled;
                    }
                } else {
                    pd.deviceTopics[info.sn] = allTopics;
                    if (!disabledTopics.isEmpty())
                        pd.disabledTopics[info.sn] = disabledTopics;
                    pd.devices.append(info);
                }
            }

            mProfiles[pd.name] = pd;
        }

        if (!mProfiles.contains(mCurrentProfile)) {
            mCurrentProfile = mProfiles.isEmpty()
                ? QString::fromUtf8("\xe9\xbb\x98\xe8\xae\xa4")
                : mProfiles.firstKey();
            if (!mProfiles.contains(mCurrentProfile))
                mProfiles[mCurrentProfile] = ProfileData{mCurrentProfile, {}, {}, {}, {}};
        }

        qDebug() << "ConfigStore: loaded" << mProfiles.size() << "profiles, current:" << mCurrentProfile;
        return true;
    }

    // 旧格式兼容
    qDebug() << "ConfigStore: detected old format, migrating to profiles...";
    ProfileData pd;
    pd.name = QString::fromUtf8("\xe9\xbb\x98\xe8\xae\xa4");

    QJsonObject mqtt = root["mqtt"].toObject();
    pd.mqtt.host     = mqtt.value("host").toString("192.168.1.100");
    pd.mqtt.port     = mqtt.value("port").toInt(8883);
    pd.mqtt.username = mqtt.value("username").toString();
    pd.mqtt.password = mqtt.value("password").toString();

    QJsonArray devs = root["devices"].toArray();
    for (const auto& dVal : devs) {
        QJsonObject devObj = dVal.toObject();
        DeviceInfo info = DeviceInfo::fromJson(devObj);
        QStringList allTopics;
        QJsonArray topicArr = devObj["topics"].toArray();
        for (const auto& t : topicArr) {
            QString topic = t.toString();
            if (!allTopics.contains(topic)) allTopics.append(topic);
        }
        QSet<QString> disabledTopics;
        QJsonArray disabledArr = devObj["disabled_topics"].toArray();
        for (const auto& t : disabledArr) disabledTopics.insert(t.toString());

        if (info.type == DeviceType::Dock) {
            QStringList dockTopics; QSet<QString> dockDisabled;
            for (const auto& t : allTopics) { if (t.contains(info.sn)) dockTopics.append(t); }
            for (const auto& t : disabledTopics) { if (t.contains(info.sn)) dockDisabled.insert(t); }
            pd.deviceTopics[info.sn] = dockTopics;
            if (!dockDisabled.isEmpty()) pd.disabledTopics[info.sn] = dockDisabled;
            pd.devices.append(info);
            QString aircraftSn = devObj.value("aircraft_sn").toString();
            if (!aircraftSn.isEmpty()) {
                DeviceInfo child;
                child.sn = aircraftSn;
                child.name = info.name + QString::fromUtf8("-\xe9\xa3\x9e\xe6\x9c\xba");
                child.type = DeviceType::Aircraft; child.parentSn = info.sn;
                pd.devices.append(child);
                QStringList childTopics; QSet<QString> childDisabled;
                for (const auto& t : allTopics) { if (t.contains(aircraftSn)) childTopics.append(t); }
                for (const auto& t : disabledTopics) { if (t.contains(aircraftSn)) childDisabled.insert(t); }
                pd.deviceTopics[child.sn] = childTopics;
                if (!childDisabled.isEmpty()) pd.disabledTopics[child.sn] = childDisabled;
            }
        } else {
            pd.deviceTopics[info.sn] = allTopics;
            if (!disabledTopics.isEmpty()) pd.disabledTopics[info.sn] = disabledTopics;
            pd.devices.append(info);
        }
    }

    mCurrentProfile = pd.name;
    mProfiles[pd.name] = pd;
    save(filePath);
    qDebug() << "ConfigStore: migrated to profiles format";
    return true;
}

bool ConfigStore::save(const QString& filePath) {
    QJsonObject root;
    root["current_profile"] = mCurrentProfile;

    QJsonArray profileArr;
    for (const auto& pd : mProfiles) {
        QJsonObject pObj;
        pObj["name"] = pd.name;

        QJsonObject mqtt;
        mqtt["host"] = pd.mqtt.host;
        mqtt["port"] = pd.mqtt.port;
        mqtt["username"] = pd.mqtt.username;
        mqtt["password"] = pd.mqtt.password;
        pObj["mqtt"] = mqtt;

        QMap<QString, QJsonObject> dockMap;
        QVector<QJsonObject> pilotList;

        for (const auto& d : pd.devices) {
            if (d.type == DeviceType::Dock) {
                QJsonObject obj = d.toJson(); obj["aircraft_sn"] = "";
                QJsonArray topics;
                for (const auto& t : pd.deviceTopics.value(d.sn)) topics.append(t);
                obj["topics"] = topics;
                QJsonArray disabledArr;
                for (const auto& t : pd.disabledTopics.value(d.sn)) disabledArr.append(t);
                if (!disabledArr.isEmpty()) obj["disabled_topics"] = disabledArr;
                dockMap[d.sn] = obj;
            } else if (d.isChild()) {
                if (dockMap.contains(d.parentSn)) {
                    dockMap[d.parentSn]["aircraft_sn"] = d.sn;
                    QJsonArray topics = dockMap[d.parentSn]["topics"].toArray();
                    for (const auto& t : pd.deviceTopics.value(d.sn)) topics.append(t);
                    dockMap[d.parentSn]["topics"] = topics;
                    QSet<QString> childDisabled = pd.disabledTopics.value(d.sn);
                    if (!childDisabled.isEmpty()) {
                        QJsonArray existing = dockMap[d.parentSn]["disabled_topics"].toArray();
                        for (const auto& t : childDisabled) existing.append(t);
                        dockMap[d.parentSn]["disabled_topics"] = existing;
                    }
                }
            } else {
                QJsonObject obj = d.toJson();
                QJsonArray topics;
                for (const auto& t : pd.deviceTopics.value(d.sn)) topics.append(t);
                obj["topics"] = topics;
                QJsonArray disabledArr;
                for (const auto& t : pd.disabledTopics.value(d.sn)) disabledArr.append(t);
                if (!disabledArr.isEmpty()) obj["disabled_topics"] = disabledArr;
                pilotList.append(obj);
            }
        }

        QJsonArray devs;
        for (const auto& obj : dockMap) devs.append(obj);
        for (const auto& obj : pilotList) devs.append(obj);
        pObj["devices"] = devs;
        profileArr.append(pObj);
    }

    root["profiles"] = profileArr;

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

// —— Profile 管理 ——

QStringList ConfigStore::profileNames() const {
    return mProfiles.keys();
}

QString ConfigStore::currentProfileName() const {
    return mCurrentProfile;
}

bool ConfigStore::setCurrentProfile(const QString& name) {
    if (!mProfiles.contains(name)) return false;
    if (mCurrentProfile == name) return true;
    save(mConfigPath);
    mCurrentProfile = name;
    emit profileSwitched(name);
    return true;
}

bool ConfigStore::addProfile(const QString& name, const MqttConfig& mqtt) {
    if (mProfiles.contains(name)) return false;
    ProfileData pd; pd.name = name; pd.mqtt = mqtt;
    mProfiles[name] = pd;
    save(mConfigPath);
    return true;
}

bool ConfigStore::removeProfile(const QString& name) {
    if (!mProfiles.contains(name) || mProfiles.size() <= 1) return false;
    mProfiles.remove(name);
    if (mCurrentProfile == name) {
        mCurrentProfile = mProfiles.firstKey();
        emit profileSwitched(mCurrentProfile);
    }
    save(mConfigPath);
    return true;
}

bool ConfigStore::renameProfile(const QString& oldName, const QString& newName) {
    if (!mProfiles.contains(oldName) || mProfiles.contains(newName)) return false;
    ProfileData pd = mProfiles.take(oldName);
    pd.name = newName;
    mProfiles[newName] = pd;
    if (mCurrentProfile == oldName) mCurrentProfile = newName;
    save(mConfigPath);
    return true;
}

// —— 当前 Profile 代理 ——

MqttConfig ConfigStore::mqttConfig() const {
    return currentProfileData().mqtt;
}

void ConfigStore::setMqttConfig(const MqttConfig& config) {
    currentProfileData().mqtt = config;
}

MqttConfig ConfigStore::mqttConfigForProfile(const QString& name) const {
    if (mProfiles.contains(name))
        return mProfiles[name].mqtt;
    return {};
}

void ConfigStore::setMqttConfigForProfile(const QString& name, const MqttConfig& config) {
    if (mProfiles.contains(name))
        mProfiles[name].mqtt = config;
}

QVector<DeviceInfo> ConfigStore::devices() const {
    return currentProfileData().devices;
}

void ConfigStore::setDevices(const QVector<DeviceInfo>& devices) {
    currentProfileData().devices = devices;
}

bool ConfigStore::renameDevice(const QString& sn, const QString& newName) {
    auto& devs = currentProfileData().devices;
    for (int i = 0; i < devs.size(); ++i) {
        if (devs[i].sn == sn) {
            devs[i].name = newName;
            return true;
        }
    }
    return false;
}

QStringList ConfigStore::topicsForDevice(const QString& sn) const {
    return currentProfileData().deviceTopics.value(sn);
}

void ConfigStore::setTopicsForDevice(const QString& sn, const QStringList& topics) {
    currentProfileData().deviceTopics[sn] = topics;
}

QStringList ConfigStore::disabledTopicsForDevice(const QString& sn) const {
    return currentProfileData().disabledTopics.value(sn).values();
}

void ConfigStore::setDisabledTopicsForDevice(const QString& sn, const QStringList& topics) {
    if (topics.isEmpty())
        currentProfileData().disabledTopics.remove(sn);
    else
        currentProfileData().disabledTopics[sn] = QSet<QString>(topics.begin(), topics.end());
}
