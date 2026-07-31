#ifndef PSDKSPEAKERDIALOG_H
#define PSDKSPEAKERDIALOG_H

#include <QCloseEvent>
#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include "PsdkSpeakerPanel.h"

// PSDK 喊话器控制独立窗口：非模态薄壳，内嵌 PsdkSpeakerPanel。
// 关闭仅隐藏（QDialog 默认行为），再次打开为同一实例。
class PsdkSpeakerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PsdkSpeakerDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , mPanel(new PsdkSpeakerPanel)
    {
        setWindowTitle(QString::fromUtf8("PSDK 喊话器控制"));
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

    PsdkSpeakerPanel* panel() const { return mPanel; }

protected:
    void closeEvent(QCloseEvent* event) override {
        hide();
        event->ignore();
    }

private:
    PsdkSpeakerPanel* mPanel;
};

#endif // PSDKSPEAKERDIALOG_H
