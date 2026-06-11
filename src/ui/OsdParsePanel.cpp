#include "OsdParsePanel.h"
#include "DeviceManager.h"
#include <QFrame>
#include <QDateTime>

// Helper: convert QJsonValue to display string
static QString valToString(const QJsonValue& val) {
    if (val.isBool())
        return val.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (val.isDouble()) {
        double d = val.toDouble();
        if (d == static_cast<qint64>(d))
            return QString::number(static_cast<qint64>(d));
        return QString::number(d, 'f', 2);
    }
    if (val.isString())
        return val.toString();
    return val.toVariant().toString();
}

OsdParsePanel::OsdParsePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();

    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(mIntervalMs);
    connect(mRefreshTimer, &QTimer::timeout, this, &OsdParsePanel::refresh);
    mRefreshTimer->start();
}

void OsdParsePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // --- Title bar ---
    auto* header = new QHBoxLayout;
    header->setSpacing(6);

    mTitleLabel = new QLabel(QStringLiteral("\xF0\x9F\x94\x8D JSON \xE8\xA7\xA3\xE6\x9E\x90"));
    mTitleLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: bold; color: #1a73e8;"));

    mTopicLabel = new QLabel(QStringLiteral(""));
    mTopicLabel->setStyleSheet(QStringLiteral("color: #80868b; font-size: 10px;"));

    header->addWidget(mTitleLabel);
    header->addWidget(mTopicLabel, 1);

    // Refresh interval selector
    auto* intervalLabel = new QLabel(QStringLiteral("\xE5\x88\xB7\xE6\x96\xB0\xE9\x97\xB4\xE9\x9A\x94:"));
    intervalLabel->setStyleSheet(QStringLiteral("color: #80868b; font-size: 11px;"));

    mIntervalCombo = new QComboBox;
    mIntervalCombo->addItems({QStringLiteral("1s"), QStringLiteral("2s"), QStringLiteral("5s"), QStringLiteral("10s")});
    mIntervalCombo->setCurrentIndex(1); // default 2s
    mIntervalCombo->setFixedWidth(60);
    mIntervalCombo->setStyleSheet(
        QStringLiteral("QComboBox { border: 1px solid #dadce0; border-radius: 3px; padding: 2px 4px; font-size: 11px; }"));
    connect(mIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        static const int sIntervals[] = {1000, 2000, 5000, 10000};
        mIntervalMs = sIntervals[idx];
        if (!mPaused)
            mRefreshTimer->start(mIntervalMs);
    });

    mPauseBtn = new QPushButton(QStringLiteral("\xE2\x8F\xB8 \xE6\x9A\x82\xE5\x81\x9C"));
    mPauseBtn->setCursor(Qt::PointingHandCursor);
    mPauseBtn->setFixedWidth(80);
    mPauseBtn->setStyleSheet(
        QStringLiteral("QPushButton { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 12px; "
               "font-size: 12px; background: #fff; color: #5f6368; }"
               "QPushButton:hover { background: #f1f3f4; }"));
    connect(mPauseBtn, &QPushButton::clicked, this, &OsdParsePanel::togglePause);

    header->addWidget(intervalLabel);
    header->addWidget(mIntervalCombo);
    header->addWidget(mPauseBtn);
    mainLayout->addLayout(header);

    // Separator line
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("color: #e0e0e0;"));
    mainLayout->addWidget(sep);

    // --- Scrollable content area ---
    mScrollArea = new QScrollArea;
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setFrameShape(QFrame::NoFrame);
    mScrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));

    mContentWidget = new QWidget;
    mContentLayout = new QVBoxLayout(mContentWidget);
    mContentLayout->setContentsMargins(0, 0, 0, 0);
    mContentLayout->setSpacing(4);
    mContentLayout->addStretch();

    mScrollArea->setWidget(mContentWidget);
    mainLayout->addWidget(mScrollArea, 1);
}

void OsdParsePanel::setTopic(const QString& deviceSn, const QString& topic) {
    mDeviceSn = deviceSn;
    mTopic    = topic;
    mPrevValues.clear();
    mTopicLabel->setText(topic.isEmpty() ? QStringLiteral("") : topic);

    if (deviceSn.isEmpty() || topic.isEmpty()) {
        clear();
        return;
    }

    refresh();
}

void OsdParsePanel::clear() {
    // Remove all group widgets from content layout
    QLayoutItem* item;
    while ((item = mContentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    mContentLayout->addStretch();
    mPrevValues.clear();
    mTopicLabel->setText(QStringLiteral(""));
}

void OsdParsePanel::refresh() {
    if (!mDevMgr || mDeviceSn.isEmpty() || mTopic.isEmpty())
        return;

    QString rawJson = mDevMgr->latestRawJson(mDeviceSn);
    if (rawJson.isEmpty())
        return;

    QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8());
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();
    QJsonObject data = root.value("data").toObject();
    if (data.isEmpty())
        return;

    renderGroups(data);
}

void OsdParsePanel::togglePause() {
    mPaused = !mPaused;
    if (mPaused) {
        mRefreshTimer->stop();
        mPauseBtn->setText(QStringLiteral("\xE2\x96\xB6 \xE7\xBB\xA7\xE7\xBB\xAD"));
    } else {
        mRefreshTimer->start(mIntervalMs);
        mPauseBtn->setText(QStringLiteral("\xE2\x8F\xB8 \xE6\x9A\x82\xE5\x81\x9C"));
        refresh();
    }
}

QMap<QString, QString> OsdParsePanel::flattenJson(const QJsonObject& obj, const QString& prefix) const {
    QMap<QString, QString> result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString key = prefix.isEmpty() ? it.key() : prefix + QStringLiteral(".") + it.key();
        QJsonValue val = it.value();

        if (val.isObject()) {
            result.insert(flattenJson(val.toObject(), key));
        } else if (val.isArray()) {
            QJsonArray arr = val.toArray();
            for (int i = 0; i < arr.size(); ++i) {
                QString arrayKey = key + QStringLiteral("[") + QString::number(i) + QStringLiteral("]");
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

void OsdParsePanel::renderGroups(const QJsonObject& data) {
    QMap<QString, QString> flatData = flattenJson(data);

    TopicMappingConfig cfg;
    if (mMapping)
        cfg = mMapping->mappingForTopic(mTopic);

    QSet<QString> renderedKeys;
    QMap<QString, QString> newValues;

    // Clear old content
    clear();

    if (cfg.fields.isEmpty()) {
        auto* noMapLabel = new QLabel(QStringLiteral("\xE8\xAF\xA5 topic \xE6\x9A\x82\xE6\x97\xA0\xE6\x98\xA0\xE5\xB0\x84\xE9\x85\x8D\xE7\xBD\xAE"));
        noMapLabel->setStyleSheet(QStringLiteral("color: #9e9e9e; font-size: 12px; padding: 16px;"));
        noMapLabel->setAlignment(Qt::AlignCenter);
        mContentLayout->insertWidget(mContentLayout->count() - 1, noMapLabel);
        return;
    }

    // Render by groups
    for (const auto& group : cfg.groups) {
        auto* groupBox = new QGroupBox(group.label);
        groupBox->setStyleSheet(
            QStringLiteral("QGroupBox { font-weight: bold; color: #333; border: 1px solid #e0e0e0; "
                   "border-radius: 4px; margin-top: 8px; padding: 12px 8px 8px 8px; background: #ffffff; }"
                   "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"));

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
                displayValue = QStringLiteral("-");
            } else if (!fm.values.isEmpty() && fm.values.contains(rawValue)) {
                displayValue = fm.values[rawValue];
            } else {
                displayValue = rawValue;
            }

            if (!fm.unit.isEmpty() && displayValue != QStringLiteral("-"))
                displayValue += QStringLiteral(" ") + fm.unit;

            auto* nameLabel = new QLabel(zhName);
            nameLabel->setStyleSheet(QStringLiteral("color: #5f6368; font-size: 11px;"));

            auto* valueLabel = new QLabel(displayValue);
            valueLabel->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 500;"));

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

    // Unmapped fields (grey at bottom)
    QStringList unmappedKeys;
    for (auto it = flatData.begin(); it != flatData.end(); ++it) {
        if (!renderedKeys.contains(it.key()))
            unmappedKeys.append(it.key());
    }

    if (!unmappedKeys.isEmpty()) {
        auto* unmappedGroup = new QGroupBox(QStringLiteral("\xE6\x9C\xAA\xE6\x98\xA0\xE5\xB0\x84\xE5\xAD\x97\xE6\xAE\xB5"));
        unmappedGroup->setStyleSheet(
            QStringLiteral("QGroupBox { font-weight: bold; color: #9e9e9e; border: 1px solid #e0e0e0; "
                   "border-radius: 4px; margin-top: 8px; padding: 12px 8px 8px 8px; background: #fafafa; }"
                   "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"));

        auto* unmappedLayout = new QFormLayout(unmappedGroup);
        unmappedLayout->setSpacing(2);
        unmappedLayout->setContentsMargins(4, 4, 4, 4);

        for (const auto& key : unmappedKeys) {
            auto* keyLabel = new QLabel(key);
            keyLabel->setStyleSheet(QStringLiteral("color: #b0b0b0; font-size: 10px;"));

            auto* valLabel = new QLabel(flatData[key]);
            valLabel->setStyleSheet(QStringLiteral("color: #b0b0b0; font-size: 10px;"));

            unmappedLayout->addRow(keyLabel, valLabel);
            newValues[key] = flatData[key];
        }

        mContentLayout->insertWidget(mContentLayout->count() - 1, unmappedGroup);
    }

    // Value change highlighting
    for (auto it = newValues.begin(); it != newValues.end(); ++it) {
        QString oldVal = mPrevValues.value(it.key());
        bool changed = !oldVal.isEmpty() && oldVal != it.value();
        if (changed) {
            // Find the value label by iterating rendered groups
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

void OsdParsePanel::setFieldValue(QLabel* label, const QString& value, bool highlight) {
    label->setText(value);
    if (highlight) {
        label->setStyleSheet(QStringLiteral("color: #1a73e8; font-weight: bold; font-size: 11px;"));
        QTimer::singleShot(1200, this, [label]() {
            label->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 500;"));
        });
    }
}
