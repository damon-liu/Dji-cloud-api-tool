#ifndef OSDPARSEPANEL_H
#define OSDPARSEPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
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
#include "TopicMapping.h"

class DeviceManager;

class OsdParsePanel : public QWidget {
    Q_OBJECT
public:
    explicit OsdParsePanel(QWidget* parent = nullptr);

    void setDeviceManager(DeviceManager* mgr) { mDevMgr = mgr; }
    void setTopic(const QString& deviceSn, const QString& topic);
    void setTopicMapping(TopicMapping* mapping) { mMapping = mapping; }
    void clear();

public slots:
    void refresh();
    void togglePause();

private:
    void setupUi();
    void renderGroups(const QJsonObject& data);
    QMap<QString, QString> flattenJson(const QJsonObject& obj, const QString& prefix = {}) const;
    void setFieldValue(QLabel* label, const QString& value, bool highlight);

    DeviceManager*      mDevMgr    = nullptr;
    TopicMapping*       mMapping   = nullptr;
    QString             mDeviceSn;
    QString             mTopic;
    bool                mPaused    = false;
    int                 mIntervalMs = 2000;

    QLabel*             mTitleLabel;
    QLabel*             mTopicLabel;
    QComboBox*          mIntervalCombo;
    QPushButton*        mPauseBtn;
    QVBoxLayout*        mContentLayout;
    QScrollArea*        mScrollArea;
    QWidget*            mContentWidget;
    QTimer*             mRefreshTimer;

    QMap<QString, QString> mPrevValues;
};

#endif // OSDPARSEPANEL_H
