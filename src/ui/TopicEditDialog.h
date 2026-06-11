#ifndef TOPICEDITDIALOG_H
#define TOPICEDITDIALOG_H

#include <QDialog>
#include <QListWidget>

class TopicEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit TopicEditDialog(const QStringList& topics,
                              const QString& deviceName,
                              const QString& deviceSn = {},
                              QWidget* parent = nullptr);

    QStringList topics() const;

private slots:
    void onAddTopic();
    void onRemoveTopic();
    void onEditTopic();

private:
    QListWidget* mTopicList;
    QString      mDeviceSn;
};

#endif // TOPICEDITDIALOG_H
