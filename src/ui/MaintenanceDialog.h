#ifndef MAINTENANCEDIALOG_H
#define MAINTENANCEDIALOG_H

#include <QCloseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include "MaintenancePanel.h"

// 运维模式独立窗口：非模态薄壳，内嵌 MaintenancePanel。
// 关闭仅隐藏（QDialog 默认行为），再次打开为同一实例。
class MaintenanceDialog : public QDialog {
    Q_OBJECT
public:
    explicit MaintenanceDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , mPanel(new MaintenancePanel(this))
    {
        setWindowTitle(QString::fromUtf8("运维模式"));
        setWindowFlags(windowFlags()
                       | Qt::WindowMaximizeButtonHint);
        setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, false);
        setSizeGripEnabled(true);
        setMinimumSize(560, 440);
        resize(720, 560);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->addWidget(mPanel);
    }

    MaintenancePanel* panel() const { return mPanel; }

protected:
    void closeEvent(QCloseEvent* event) override {
        hide();
        event->ignore();
    }

private:
    MaintenancePanel* mPanel;
};

#endif // MAINTENANCEDIALOG_H
