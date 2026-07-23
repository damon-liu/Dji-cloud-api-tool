#ifndef DOCKCONTROLDIALOG_H
#define DOCKCONTROLDIALOG_H

#include <QCloseEvent>
#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include "DockControlPanel.h"

// 机场控制独立窗口：非模态薄壳，内嵌 DockControlPanel。
// 关闭仅隐藏，再次打开恢复同一实例。
class DockControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit DockControlDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , mPanel(new DockControlPanel)
    {
        setWindowTitle(QString::fromUtf8("机场控制"));
        setWindowFlags(windowFlags()
                       | Qt::WindowMaximizeButtonHint);
        setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, false);  // 允许拖拽调整大小
        setSizeGripEnabled(true);
        setMinimumSize(680, 500);
        resize(800, 620);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidget(mPanel);

        layout->addWidget(scrollArea);
    }

    DockControlPanel* panel() const { return mPanel; }

protected:
    void closeEvent(QCloseEvent* event) override {
        hide();
        event->ignore();
    }

private:
    DockControlPanel* mPanel;
};

#endif // DOCKCONTROLDIALOG_H
