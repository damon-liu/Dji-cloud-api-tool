#ifndef DOCKCONTROLDIALOG_H
#define DOCKCONTROLDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include "DockControlPanel.h"

// 机场控制独立窗口：非模态薄壳，内嵌 DockControlPanel。
// 关闭仅隐藏（QDialog 默认行为），再次打开为同一实例。
class DockControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit DockControlDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , mPanel(new DockControlPanel(this))
    {
        setWindowTitle(QString::fromUtf8("机场控制"));
        resize(720, 520);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->addWidget(mPanel);
    }

    DockControlPanel* panel() const { return mPanel; }

private:
    DockControlPanel* mPanel;
};

#endif // DOCKCONTROLDIALOG_H
