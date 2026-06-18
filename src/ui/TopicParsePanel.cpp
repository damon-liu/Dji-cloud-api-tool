#include "TopicParsePanel.h"
#include "DeviceManager.h"
#include <QFrame>
#include <QPointer>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>

// Helper: convert QJsonValue to display string
static QString valToString(const QJsonValue& val) {
    if (val.isBool())
        return val.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (val.isDouble()) {
        double d = val.toDouble();
        if (d == static_cast<qint64>(d))
            return QString::number(static_cast<qint64>(d));
        // 保留最多 7 位小数，去除尾部多余的零
        QString s = QString::number(d, 'f', 7);
        while (s.endsWith('0') && !s.endsWith(".0"))
            s.chop(1);
        return s;
    }
    if (val.isString())
        return val.toString();
    return val.toVariant().toString();
}

TopicParsePanel::TopicParsePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();

    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(mIntervalMs);
    connect(mRefreshTimer, &QTimer::timeout, this, &TopicParsePanel::refresh);
    mRefreshTimer->start();
}

void TopicParsePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // --- 标题栏 ---
    auto* header = new QHBoxLayout;
    header->setSpacing(6);

    mTitleLabel = new QLabel("🔍 JSON 解析");
    mTitleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #1a73e8;");

    mTopicLabel = new QLabel("");
    mTopicLabel->setStyleSheet("color: #80868b; font-size: 10px;");

    header->addWidget(mTitleLabel);
    header->addWidget(mTopicLabel, 1);

    // 刷新间隔选择
    auto* intervalLabel = new QLabel("刷新间隔:");
    intervalLabel->setStyleSheet("color: #80868b; font-size: 11px;");

    mIntervalCombo = new QComboBox;
    mIntervalCombo->addItems({"1s", "2s", "5s", "10s"});
    mIntervalCombo->setCurrentIndex(1); // 默认 2s
    mIntervalCombo->setFixedWidth(60);
    mIntervalCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 3px; padding: 2px 4px; font-size: 11px; }");
    connect(mIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        static const int sIntervals[] = {1000, 2000, 5000, 10000};
        mIntervalMs = sIntervals[idx];
        if (!mPaused)
            mRefreshTimer->start(mIntervalMs);
    });

    mPauseBtn = new QPushButton("⏸ 暂停");
    mPauseBtn->setCursor(Qt::PointingHandCursor);
    mPauseBtn->setFixedWidth(80);
    mPauseBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 12px; "
        "font-size: 12px; background: #fff; color: #5f6368; }"
        "QPushButton:hover { background: #f1f3f4; }");
    connect(mPauseBtn, &QPushButton::clicked, this, &TopicParsePanel::togglePause);

    header->addWidget(intervalLabel);
    header->addWidget(mIntervalCombo);
    header->addWidget(mPauseBtn);
    mainLayout->addLayout(header);

    // 分隔线
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #e0e0e0;");
    mainLayout->addWidget(sep);

    // --- 可滚动内容区域 ---
    mScrollArea = new QScrollArea;
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setFrameShape(QFrame::NoFrame);
    mScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    mContentWidget = new QWidget;
    mContentLayout = new QVBoxLayout(mContentWidget);
    mContentLayout->setContentsMargins(0, 0, 0, 0);
    mContentLayout->setSpacing(4);
    mContentLayout->addStretch();

    mScrollArea->setWidget(mContentWidget);
    mainLayout->addWidget(mScrollArea, 1);
}

void TopicParsePanel::setTopic(const QString& deviceSn, const QString& topic) {
    mDeviceSn = deviceSn;
    mTopic    = topic;
    mPrevValues.clear();
    mLastJson.clear();
    mTopicLabel->setText(topic.isEmpty() ? "" : topic);

    // 切换 topic 时先清除旧内容
    clear();

    if (deviceSn.isEmpty() || topic.isEmpty()) {
        return;
    }

    refresh();
}

bool TopicParsePanel::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel* label = qobject_cast<QLabel*>(obj);
        if (label) {
            QString key = label->property("copyKey").toString();
            if (!key.isEmpty()) {
                QApplication::clipboard()->setText(key);
                // 短暂显示已复制提示
                QString orig = label->text();
                label->setText("已复制: " + key);
                label->setStyleSheet("color: #1a73e8; font-size: 11px;");
                QPointer<QLabel> weakLabel(label);
                QTimer::singleShot(1500, this, [weakLabel, orig]() {
                    if (weakLabel) {
                        weakLabel->setText(orig);
                        weakLabel->setStyleSheet("color: #5f6368; font-size: 11px;");
                    }
                });
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TopicParsePanel::clear() {
    QLayoutItem* item;
    while ((item = mContentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    mContentLayout->addStretch();
    mTopicLabel->setText("");
}

void TopicParsePanel::refresh() {
    if (!mDevMgr || mDeviceSn.isEmpty() || mTopic.isEmpty())
        return;

    QString rawJson = mDevMgr->latestRawJson(mDeviceSn, mTopic);
    if (rawJson.isEmpty())
        return;

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        qWarning() << "TopicParsePanel: JSON parse error:" << parseErr.errorString();
        return;
    }
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();
    QJsonObject data = root.value("data").toObject();
    if (data.isEmpty())
        return;

    // 数据无变化则跳过，避免重复清空重建闪烁
    if (rawJson == mLastJson)
        return;
    mLastJson = rawJson;

    renderGroups(data);
}

void TopicParsePanel::togglePause() {
    mPaused = !mPaused;
    if (mPaused) {
        mRefreshTimer->stop();
        mPauseBtn->setText("▶ 继续");
    } else {
        mRefreshTimer->start(mIntervalMs);
        mPauseBtn->setText("⏸ 暂停");
        refresh();
    }
}

QMap<QString, QString> TopicParsePanel::flattenJson(const QJsonObject& obj, const QString& prefix) const {
    QMap<QString, QString> result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
        QJsonValue val = it.value();

        if (val.isObject()) {
            result.insert(flattenJson(val.toObject(), key));
        } else if (val.isArray()) {
            QJsonArray arr = val.toArray();
            for (int i = 0; i < arr.size(); ++i) {
                QString arrayKey = key + "[" + QString::number(i) + "]";
                if (arr[i].isObject()) {
                    result.insert(flattenJson(arr[i].toObject(), arrayKey));
                } else {
                    result[arrayKey] = valToString(arr[i]);
                }
            }
        } else {
            result[key] = valToString(val);
        }
    }
    return result;
}

void TopicParsePanel::renderGroups(const QJsonObject& data) {
    QMap<QString, QString> flatData = flattenJson(data);

    TopicMappingConfig cfg;
    if (mMapping)
        cfg = mMapping->mappingForTopic(mTopic);

    QSet<QString> renderedKeys;
    QMap<QString, QString> newValues;

    // 在清空前保存旧值（用于变化检测）
    QMap<QString, QString> prevValues = mPrevValues;

    // 清空旧内容
    clear();

    if (cfg.fields.isEmpty()) {
        auto* noMapLabel = new QLabel("该 topic 暂无映射配置");
        noMapLabel->setStyleSheet("color: #9e9e9e; font-size: 12px; padding: 16px;");
        noMapLabel->setAlignment(Qt::AlignCenter);
        mContentLayout->insertWidget(mContentLayout->count() - 1, noMapLabel);
        return;
    }

    // 按分组渲染
    for (const auto& group : cfg.groups) {
        auto* groupBox = new QGroupBox(group.label);

        auto* formLayout = new QFormLayout(groupBox);
        formLayout->setSpacing(2);
        formLayout->setContentsMargins(4, 4, 4, 4);

        bool groupHasContent = false;

        for (const auto& key : group.keys) {
            FieldMapping fm = cfg.fields.value(key);
            QString zhName = fm.zh.isEmpty() ? key : fm.zh;

            QString rawValue = flatData.value(key);
            QString displayValue;

            if (!flatData.contains(key)) {
                displayValue = "-";
            } else if (!fm.values.isEmpty() && fm.values.contains(rawValue)) {
                displayValue = fm.values[rawValue];
            } else {
                displayValue = rawValue;
            }

            if (!fm.unit.isEmpty() && displayValue != "-")
                displayValue += " " + fm.unit;

            auto* nameLabel = new QLabel(zhName);
            nameLabel->setStyleSheet("color: #5f6368; font-size: 11px;");
            nameLabel->setCursor(Qt::PointingHandCursor);
            nameLabel->setToolTip("点击复制: " + key);
            nameLabel->installEventFilter(this);
            nameLabel->setProperty("copyKey", key);

            auto* valueLabel = new QLabel(displayValue);
            valueLabel->setStyleSheet("font-size: 11px; font-weight: 500;");

            formLayout->addRow(nameLabel, valueLabel);
            renderedKeys.insert(key);
            newValues[key] = displayValue;
            groupHasContent = true;
        }

        if (groupHasContent) {
            mContentLayout->insertWidget(mContentLayout->count() - 1, groupBox);
        } else {
            delete groupBox;
        }
    }

    // 未映射字段
    QStringList unmappedKeys;
    for (auto it = flatData.begin(); it != flatData.end(); ++it) {
        if (!renderedKeys.contains(it.key()))
            unmappedKeys.append(it.key());
    }

    if (!unmappedKeys.isEmpty()) {
        auto* unmappedGroup = new QGroupBox("其他字段");

        auto* unmappedLayout = new QFormLayout(unmappedGroup);
        unmappedLayout->setSpacing(2);
        unmappedLayout->setContentsMargins(4, 4, 4, 4);

        for (const auto& key : unmappedKeys) {
            auto* keyLabel = new QLabel(key);
            keyLabel->setStyleSheet("color: #5f6368; font-size: 11px;");
            keyLabel->setCursor(Qt::PointingHandCursor);
            keyLabel->setToolTip("点击复制: " + key);
            keyLabel->installEventFilter(this);
            keyLabel->setProperty("copyKey", key);

            auto* valLabel = new QLabel(flatData[key]);
            valLabel->setStyleSheet("font-size: 11px; font-weight: 500;");

            unmappedLayout->addRow(keyLabel, valLabel);
            newValues[key] = flatData[key];
        }

        mContentLayout->insertWidget(mContentLayout->count() - 1, unmappedGroup);
    }

    // 值变化高亮
    for (auto it = newValues.begin(); it != newValues.end(); ++it) {
        QString oldVal = prevValues.value(it.key());
        bool changed = !oldVal.isEmpty() && oldVal != it.value();
        if (changed) {
            for (int i = 0; i < mContentLayout->count(); ++i) {
                QLayoutItem* item = mContentLayout->itemAt(i);
                if (!item || !item->widget()) continue;
                QGroupBox* gb = qobject_cast<QGroupBox*>(item->widget());
                if (!gb) continue;
                QFormLayout* fl = qobject_cast<QFormLayout*>(gb->layout());
                if (!fl) continue;
                for (int r = 0; r < fl->rowCount(); ++r) {
                    QLayoutItem* labelItem = fl->itemAt(r, QFormLayout::LabelRole);
                    QLayoutItem* fieldItem = fl->itemAt(r, QFormLayout::FieldRole);
                    if (!labelItem || !fieldItem) continue;
                    QLabel* nameLbl = qobject_cast<QLabel*>(labelItem->widget());
                    QLabel* valLbl  = qobject_cast<QLabel*>(fieldItem->widget());
                    if (!nameLbl || !valLbl) continue;
                    for (auto fit = cfg.fields.begin(); fit != cfg.fields.end(); ++fit) {
                        if (fit.value().zh == nameLbl->text() && fit.key() == it.key()) {
                            setFieldValue(valLbl, it.value(), true);
                            break;
                        }
                    }
                }
            }
        }
    }

    mPrevValues = newValues;
}

void TopicParsePanel::setFieldValue(QLabel* label, const QString& value, bool highlight) {
    label->setText(value);
    if (highlight) {
        label->setStyleSheet("color: #1a73e8; font-weight: bold; font-size: 11px;");
        QPointer<QLabel> weakLabel(label);
        QTimer::singleShot(1200, this, [weakLabel]() {
            if (weakLabel) {
                weakLabel->setStyleSheet("font-size: 11px; font-weight: 500;");
            }
        });
    }
}

void TopicParsePanel::pause() {
    if (!mAutoPaused) {
        mAutoPaused = true;
        mRefreshTimer->stop();
    }
}

void TopicParsePanel::resume() {
    if (mAutoPaused) {
        mAutoPaused = false;
        if (!mPaused) {
            mRefreshTimer->start(mIntervalMs);
            refresh();
        }
    }
}
