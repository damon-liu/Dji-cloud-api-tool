#include "TopicParsePanel.h"
#include "DeviceManager.h"
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QGridLayout>
#include <QPointer>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QToolTip>

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

    mViewModeBtn = new QPushButton("☰ 列表");
    mViewModeBtn->setCursor(Qt::PointingHandCursor);
    mViewModeBtn->setFixedWidth(80);
    mViewModeBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 12px; "
        "font-size: 12px; background: #fff; color: #5f6368; }"
        "QPushButton:hover { background: #f1f3f4; }");
    connect(mViewModeBtn, &QPushButton::clicked, this, &TopicParsePanel::toggleViewMode);

    mPauseBtn = new QPushButton("⏸ 暂停");
    mPauseBtn->setCursor(Qt::PointingHandCursor);
    mPauseBtn->setFixedWidth(80);
    mPauseBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 12px; "
        "font-size: 12px; background: #fff; color: #5f6368; }"
        "QPushButton:hover { background: #f1f3f4; }");
    connect(mPauseBtn, &QPushButton::clicked, this, &TopicParsePanel::togglePause);

    header->addWidget(mViewModeBtn);
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
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    mContentWidget = new QWidget;
    mContentLayout = new QVBoxLayout(mContentWidget);
    mContentLayout->setContentsMargins(0, 0, 0, 0);
    mContentLayout->setSpacing(4);
    mContentLayout->addStretch();

    mScrollArea->setWidget(mContentWidget);
    mainLayout->addWidget(mScrollArea, 1);
}

void TopicParsePanel::setTopic(const QString& deviceSn, const QString& topic, const QString& deviceType) {
    mDeviceSn   = deviceSn;
    mTopic      = topic;
    mDeviceType = deviceType;
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
    QLabel* label = qobject_cast<QLabel*>(obj);
    if (!label)
        return QWidget::eventFilter(obj, event);

    QString key = label->property("copyKey").toString();

    if (event->type() == QEvent::Enter && !key.isEmpty()) {
        QToolTip::showText(QCursor::pos(), key, label, QRect(), 3600000);
        return true;
    }

    if (event->type() == QEvent::Leave) {
        QToolTip::hideText();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress && !key.isEmpty()) {
        QApplication::clipboard()->setText(key);
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

    return QWidget::eventFilter(obj, event);
}

void TopicParsePanel::clear() {
    QLayoutItem* item;
    while ((item = mContentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();  // 立即删除，避免 deleteLater 延迟导致切换时新旧控件共存
        }
        delete item;
    }
    mContentLayout->addStretch();
    mTopicLabel->setText("");
}

void TopicParsePanel::refresh() {
    if (!mDevMgr || mDeviceSn.isEmpty() || mTopic.isEmpty())
        return;

    // 使用字段级合并数据，解决 DJI 机场 OSD 分消息推送问题：
    // 同一条 topic 的字段可能分散在多个消息中，合并后确保面板显示所有已知字段
    QString mergedJson = mDevMgr->mergedOsdJson(mDeviceSn, mTopic);
    if (mergedJson.isEmpty())
        return;

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(mergedJson.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        qWarning() << "TopicParsePanel: JSON parse error:" << parseErr.errorString();
        return;
    }
    if (!doc.isObject())
        return;

    QJsonObject data = doc.object();
    if (data.isEmpty())
        return;

    // 数据无变化则跳过，避免重复清空重建闪烁
    if (mergedJson == mLastJson)
        return;
    mLastJson = mergedJson;

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

void TopicParsePanel::toggleViewMode() {
    mCardMode = !mCardMode;
    mViewModeBtn->setText(mCardMode ? "☰ 列表" : "⊞ 网格");
    mLastJson.clear();  // 强制重绘，切换卡片/列表模式
    if (!mPaused)
        refresh();
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
        cfg = mMapping->mappingForTopic(mTopic, mDeviceType);

    QSet<QString> renderedKeys;
    QMap<QString, QString> newValues;
    QMap<QString, QLabel*> valueLabels;  // key → value label (for highlighting)

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

    // ---- 辅助：解析字段的显示名称和值 ----
    auto resolveField = [&](const QString& key, QString& zhName, QString& displayValue) -> bool {
        if (!flatData.contains(key))
            return false;
        FieldMapping fm = cfg.fields.value(key);
        zhName = fm.zh.isEmpty() ? key : fm.zh;
        QString rawValue = flatData.value(key);
        if (!fm.values.isEmpty() && fm.values.contains(rawValue))
            displayValue = fm.values[rawValue];
        else
            displayValue = rawValue;
        if (!fm.unit.isEmpty())
            displayValue += " " + fm.unit;
        return true;
    };

    // ---- 辅助：创建带复制功能的标签 ----
    auto makeCopyLabel = [&](const QString& text, const QString& copyKey,
                             const QString& styleSheet) -> QLabel* {
        auto* label = new QLabel(text);
        label->setStyleSheet(styleSheet);
        label->setCursor(Qt::PointingHandCursor);
        label->installEventFilter(this);
        label->setProperty("copyKey", copyKey);
        return label;
    };

    // ---- 辅助：网格模式渲染，QGridLayout 固定 3 列，卡片等宽 ----
    auto renderCards = [&](QWidget* container, const QList<QStringList>& rows,
                           const QStringList& extraKeys = {}) {
        // 收集所有有效 key
        QStringList allKeys;
        for (const auto& rowKeys : rows)
            for (const auto& key : rowKeys)
                if (flatData.contains(key) && !renderedKeys.contains(key))
                    allKeys.append(key);
        for (const auto& key : extraKeys)
            if (flatData.contains(key) && !renderedKeys.contains(key))
                allKeys.append(key);

        if (allKeys.isEmpty()) return false;

        const int maxPerRow = 3;
        // 根据容器可用宽度计算统一卡片宽度（仅用于限制最大宽度）
        int viewportW = mScrollArea->viewport()->width();
        int cardW = (viewportW - 24) / maxPerRow;  // 含各种 margin

        auto* grid = new QGridLayout(container);
        grid->setSpacing(6);
        grid->setContentsMargins(4, 4, 4, 4);

        int total = allKeys.size();
        int rowsCount = (total + maxPerRow - 1) / maxPerRow;

        // 创建占位行确定行高（除最后一行外）
        for (int i = 0; i < allKeys.size(); ++i) {
            const QString& key = allKeys[i];
            QString zhName, displayValue;
            if (!resolveField(key, zhName, displayValue)) continue;

            int r = i / maxPerRow;
            int c = i % maxPerRow;

            auto* card = new QFrame;
            card->setStyleSheet(
                "QFrame { background: #f8f9fa; border: none; border-radius: 4px; }");
            card->setMinimumWidth(100);
            card->setMaximumWidth(qMax(cardW, 100));

            auto* cl = new QVBoxLayout(card);
            cl->setContentsMargins(8, 4, 8, 4);
            cl->setSpacing(2);
            cl->addWidget(makeCopyLabel(zhName, key,
                                        "color: #5f6368; font-size: 11px;"));
            auto* valLabel = makeCopyLabel(displayValue, key,
                                           "font-size: 11px; font-weight: 500;");
            cl->addWidget(valLabel);
            valueLabels[key] = valLabel;

            grid->addWidget(card, r, c);
            renderedKeys.insert(key);
            newValues[key] = displayValue;
        }

        // 3 列等比例拉伸，确保所有卡片等宽
        for (int c = 0; c < maxPerRow; ++c)
            grid->setColumnStretch(c, 1);

        // 如果最后一行不满，在空单元格填充占位以保持等宽
        int remainder = total % maxPerRow;
        if (remainder > 0) {
            int lastRow = rowsCount - 1;
            for (int c = remainder; c < maxPerRow; ++c) {
                auto* spacer = new QWidget;
                spacer->setFixedHeight(0);
                grid->addWidget(spacer, lastRow, c);
            }
        }

        return true;
    };

    // ---- 辅助：列表模式渲染一组字段为单列 QFormLayout ----
    auto renderList = [&](QWidget* container, const QList<QStringList>& rows,
                          const QStringList& extraKeys = {}) {
        auto* form = new QFormLayout(container);
        form->setHorizontalSpacing(15);
        form->setVerticalSpacing(6);
        form->setContentsMargins(4, 4, 4, 4);
        bool any = false;

        auto addRow = [&](const QString& key) {
            QString zhName, displayValue;
            if (!resolveField(key, zhName, displayValue)) return;
            form->addRow(makeCopyLabel(zhName, key,
                                       "color: #5f6368; font-size: 11px;"),
                         makeCopyLabel(displayValue, key,
                                       "font-size: 11px; font-weight: 500;"));
            renderedKeys.insert(key);
            newValues[key] = displayValue;
            valueLabels[key] = qobject_cast<QLabel*>(
                form->itemAt(form->rowCount() - 1, QFormLayout::FieldRole)->widget());
            any = true;
        };

        for (const auto& rowKeys : rows)
            for (const auto& key : rowKeys)
                addRow(key);
        for (const auto& key : extraKeys)
            addRow(key);

        return any;
    };

    // ---- 按分组渲染 ----
    for (const auto& group : cfg.groups) {
        auto* groupBox = new QGroupBox(group.label);
        bool groupHasContent;

        if (mCardMode)
            groupHasContent = renderCards(groupBox, group.rows);
        else
            groupHasContent = renderList(groupBox, group.rows);

        if (groupHasContent) {
            mContentLayout->insertWidget(mContentLayout->count() - 1, groupBox);
        } else {
            delete groupBox;
        }
    }

    // ---- 未映射字段 ----
    QStringList unmappedKeys;
    for (auto it = flatData.begin(); it != flatData.end(); ++it) {
        if (!renderedKeys.contains(it.key()))
            unmappedKeys.append(it.key());
    }

    if (!unmappedKeys.isEmpty()) {
        auto* unmappedGroup = new QGroupBox("其他字段");
        bool hasContent;
        // 未映射字段无 row 分组 → 每个 key 独立（单元素行列表）
        QList<QStringList> singleRows;
        for (const auto& key : unmappedKeys)
            singleRows.append(QStringList{key});

        if (mCardMode)
            hasContent = renderCards(unmappedGroup, singleRows);
        else
            hasContent = renderList(unmappedGroup, singleRows);

        if (hasContent) {
            mContentLayout->insertWidget(mContentLayout->count() - 1, unmappedGroup);
        } else {
            delete unmappedGroup;
        }
    }

    // 值变化高亮（通过 key → valueLabel 映射直接定位）
    for (auto it = newValues.begin(); it != newValues.end(); ++it) {
        QString oldVal = prevValues.value(it.key());
        bool changed = !oldVal.isEmpty() && oldVal != it.value();
        if (changed && valueLabels.contains(it.key())) {
            setFieldValue(valueLabels[it.key()], it.value(), true);
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

void TopicParsePanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // 缩放时用缓存数据重新布局，确保卡片宽度跟随面板变化
    if (!mLastJson.isEmpty() && !mPaused) {
        QJsonDocument doc = QJsonDocument::fromJson(mLastJson.toUtf8());
        if (doc.isObject())
            renderGroups(doc.object());
    }
}
