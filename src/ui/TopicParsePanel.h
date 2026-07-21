#ifndef TOPICPARSEPANEL_H
#define TOPICPARSEPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QTimer>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QEvent>
#include <QPointer>
#include "FlowLayout.h"
#include "TopicMapping.h"

class DeviceManager;

class TopicParsePanel : public QWidget {
    Q_OBJECT
public:
    explicit TopicParsePanel(QWidget* parent = nullptr);

    void setDeviceManager(DeviceManager* mgr) { mDevMgr = mgr; }
    void setTopic(const QString& deviceSn, const QString& topic);
    void setTopicMapping(TopicMapping* mapping) { mMapping = mapping; }
    void clear();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
    void refresh();
    void togglePause();
    void pause();
    void resume();

private:
    void setupUi();
    void renderGroups(const QJsonObject& data);
    QMap<QString, QString> flattenJson(const QJsonObject& obj, const QString& prefix = {}) const;

    QWidget* createCard(const QString& key, const QString& zhName,
                        const QString& displayValue, const QString& rawValue,
                        const FieldMapping& fm);
    QWidget* createListRow(const QString& key, const QString& zhName,
                           const QString& displayValue);
    QWidget* createGroupHeader(const QString& label);
    void highlightCard(QWidget* card);
    void toggleViewMode();
    void setFieldValue(QLabel* label, const QString& value, bool highlight);

    DeviceManager*      mDevMgr    = nullptr;
    TopicMapping*       mMapping   = nullptr;
    QString             mDeviceSn;
    QString             mTopic;
    bool                mPaused    = false;
    bool                mAutoPaused = false;
    int                 mIntervalMs = 2000;

    QLabel*             mTitleLabel;
    QLabel*             mTopicLabel;
    QComboBox*          mIntervalCombo;
    QPushButton*        mPauseBtn;
    QPushButton*        mViewModeBtn;
    bool                mCardMode    = true;   // true=卡片网格, false=列表
    QVBoxLayout*        mContentLayout;
    QScrollArea*        mScrollArea;
    QWidget*            mContentWidget;
    QTimer*             mRefreshTimer;

    QMap<QString, QString> mPrevValues;
    QString                mLastJson;

    // 跟踪卡片 widget，用于值变化高亮
    QMap<QString, QWidget*> mCards;
};

#endif // TOPICPARSEPANEL_H
