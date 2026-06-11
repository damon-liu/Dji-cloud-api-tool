#ifndef TOPICMAPPING_H
#define TOPICMAPPING_H

#include <QString>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QRegularExpression>
#include <QDebug>

// 单个字段的映射定义
struct FieldMapping {
    QString zh;                         // 中文名称
    QString unit;                       // 单位 (如 "V", "m/s", "℃", 可为空)
    QMap<QString, QString> values;      // 枚举值翻译 (如 "0" → "待机")
};

// 分组定义
struct GroupDef {
    QString id;                         // 分组 ID (如 "power")
    QString label;                      // 分组标签 (如 "🔋 电源")
    QStringList keys;                   // 该分组包含的字段 key (支持点号分隔的嵌套路径)
};

// 一个 topic 的完整映射配置
struct TopicMappingConfig {
    QString description;                         // topic 描述
    QMap<QString, FieldMapping> fields;          // key → FieldMapping
    QList<GroupDef> groups;                      // 分组定义 (有序)
};

// TopicMapping: 加载 topic_mappings.json，提供映射查询
class TopicMapping {
public:
    TopicMapping() = default;

    // 从 JSON 文件加载映射配置
    bool load(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "TopicMapping: cannot open" << path;
            return false;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();

        if (err.error != QJsonParseError::NoError) {
            qWarning() << "TopicMapping: JSON parse error:" << err.errorString();
            return false;
        }

        return loadFromDocument(doc);
    }

    // 从 JSON 字符串加载（内置默认映射降级使用）
    bool loadFromString(const QString& json) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "TopicMapping: built-in JSON parse error:" << err.errorString();
            return false;
        }
        return loadFromDocument(doc);
    }

    // 根据实际 topic 查找映射配置（支持 {sn} 通配符模式匹配）
    TopicMappingConfig mappingForTopic(const QString& topic) const {
        // 1. 精确匹配
        if (mConfigs.contains(topic))
            return mConfigs[topic];

        // 2. 模式匹配：将配置中的 {sn} 替换为 [^/]+ 做正则匹配
        for (auto it = mConfigs.begin(); it != mConfigs.end(); ++it) {
            QString pattern = it.key();
            if (!pattern.contains("{sn}"))
                continue;
            QString escaped = QRegularExpression::escape(pattern);
            escaped.replace("\\{sn\\}", "[^/]+");
            QRegularExpression re("^" + escaped + "$");
            if (re.match(topic).hasMatch())
                return it.value();
        }

        return TopicMappingConfig{}; // 空配置
    }

    // 检查是否有可用的映射
    bool isEmpty() const { return mConfigs.isEmpty(); }

    // 获取所有已加载的 topic 模式
    QStringList topicPatterns() const { return mConfigs.keys(); }

private:
    bool loadFromDocument(const QJsonDocument& doc) {
        if (!doc.isObject()) {
            qWarning() << "TopicMapping: root is not a JSON object";
            return false;
        }

        QJsonObject root = doc.object();
        QJsonObject topics = root.value("topics").toObject();

        for (auto tit = topics.begin(); tit != topics.end(); ++tit) {
            TopicMappingConfig cfg;
            QJsonObject topicObj = tit.value().toObject();
            cfg.description = topicObj.value("description").toString();

            // 解析 fields
            QJsonObject fieldsObj = topicObj.value("fields").toObject();
            for (auto fit = fieldsObj.begin(); fit != fieldsObj.end(); ++fit) {
                FieldMapping fm;
                QJsonObject fieldObj = fit.value().toObject();
                fm.zh   = fieldObj.value("zh").toString();
                fm.unit = fieldObj.value("unit").toString();

                QJsonObject valuesObj = fieldObj.value("values").toObject();
                for (auto vit = valuesObj.begin(); vit != valuesObj.end(); ++vit) {
                    fm.values[vit.key()] = vit.value().toString();
                }
                cfg.fields[fit.key()] = fm;
            }

            // 解析 groups
            QJsonArray groupsArr = topicObj.value("groups").toArray();
            for (const auto& gv : groupsArr) {
                QJsonObject gobj = gv.toObject();
                GroupDef gd;
                gd.id    = gobj.value("id").toString();
                gd.label = gobj.value("label").toString();
                QJsonArray keysArr = gobj.value("keys").toArray();
                for (const auto& kv : keysArr)
                    gd.keys.append(kv.toString());
                cfg.groups.append(gd);
            }

            mConfigs[tit.key()] = cfg;
        }

        qDebug() << "TopicMapping: loaded" << mConfigs.size() << "topic mappings";
        return true;
    }

    QMap<QString, TopicMappingConfig> mConfigs; // topic 模式 → 配置
};

#endif // TOPICMAPPING_H
