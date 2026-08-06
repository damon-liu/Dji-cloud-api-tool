#ifndef COMMANDHISTORYDIALOG_H
#define COMMANDHISTORYDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include <QString>
#include "DockCommand.h"

// 历史记录来源分类
enum class HistorySource {
    All = 0,
    Dock,         // 远程调试
    Flight,       // 飞行控制
    PSDK,         // PSDK 功能
    Maintenance,  // 运维工具
    Publish,      // Topic 下发
    Video         // 视频直播
};

// 统一的下发记录非模态窗口
// 所有功能面板的下发记录汇总到此处，可筛选、搜索、清空
class CommandHistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit CommandHistoryDialog(QWidget* parent = nullptr);

    // 追加记录（由 MainWindow 在信号中调用）
    void appendDockCommand(HistorySource source, const DockCommandResult& result);
    void appendTopicPublish(const QString& topic, const QString& json,
                            bool success, const QString& message);
    void appendMaintenanceAction(const QString& action);
    void appendLivePushCommand(const QString& gatewaySn, const QString& method,
                               const QString& json);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onFilterChanged(int index);
    void onSearchChanged(const QString& text);
    void onClearClicked();
    void onExportClicked();

private:
    struct Entry {
        HistorySource source;
        QString       formattedText;  // 已格式化的完整文本块
    };

    void refreshDisplay();
    QString formatVerdict(const DockCommandResult& result) const;

    // UI
    QComboBox*      mSourceFilter = nullptr;
    QLineEdit*      mSearchEdit = nullptr;
    QPushButton*    mClearBtn = nullptr;
    QPushButton*    mExportBtn = nullptr;
    QLabel*         mCountLabel = nullptr;
    QPlainTextEdit* mHistoryEdit = nullptr;

    // Data
    QVector<Entry>  mEntries;
    static constexpr int MAX_ENTRIES = 1000;
};

#endif // COMMANDHISTORYDIALOG_H
