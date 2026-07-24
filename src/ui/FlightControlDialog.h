#ifndef FLIGHTCONTROLDIALOG_H
#define FLIGHTCONTROLDIALOG_H

#include <QCloseEvent>
#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include "FlightControlPanel.h"

// 飞行控制独立窗口：非模态薄壳，内嵌 FlightControlPanel。
// 关闭仅隐藏（QDialog 默认行为），再次打开为同一实例。
class FlightControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit FlightControlDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , mPanel(new FlightControlPanel)
    {
        setWindowTitle(QString::fromUtf8("飞行控制"));
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

    FlightControlPanel* panel() const { return mPanel; }

protected:
    void closeEvent(QCloseEvent* event) override {
        hide();
        event->ignore();
    }

private:
    FlightControlPanel* mPanel;
};

#endif // FLIGHTCONTROLDIALOG_H
