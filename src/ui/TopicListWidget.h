#ifndef TOPICLISTWIDGET_H
#define TOPICLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSet>

// TopicListWidget: 展示当前设备 topic 列表，支持启用/禁用、添加、删除
class TopicListWidget : public QWidget {
    Q_OBJECT
public:
    explicit TopicListWidget(QWidget* parent = nullptr);

    // 设置当前显示的 topic 列表
    // sn: 当前设备 SN，topics: 所有 topic，disabledTopics: 禁用的 topic 集合
    void setTopics(const QString& sn,
                   const QStringList& topics,
                   const QSet<QString>& disabledTopics);

    // 清除显示（无设备选中时）
    void clearTopics();

    // 获取当前列表中选中的 topic 字符串，无选中返回空
    QString selectedTopic() const;

signals:
    void topicAdded(const QString& deviceSn, const QString& topic);
    void topicToggled(const QString& deviceSn, const QString& topic);
    void topicRemoved(const QString& deviceSn, const QString& topic);
    void topicSelectionChanged(const QString& topic);

private slots:
    void onAddTopic();
    void onToggleTopic();
    void onRemoveTopic();
    void onTopicSelectionChanged();

private:
    void refreshList();

    QLabel*      mTitleLabel;
    QListWidget* mTopicList;
    QPushButton* mAddBtn;
    QPushButton* mToggleBtn;
    QPushButton* mRemoveBtn;

    QString      mCurrentSn;
    QStringList  mAllTopics;
    QSet<QString> mDisabledTopics;
};

#endif // TOPICLISTWIDGET_H
