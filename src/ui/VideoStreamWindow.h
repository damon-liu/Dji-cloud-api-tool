#ifndef VIDEOSTREAMWINDOW_H
#define VIDEOSTREAMWINDOW_H

#include <QWidget>

class QLabel;

class VideoStreamWindow : public QWidget {
    Q_OBJECT
public:
    explicit VideoStreamWindow(int index, QWidget* parent = nullptr);
    void setStreamUrl(const QString& url);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    int mIndex;
    QLabel* mTitleLabel;
    QWidget* mVideoArea;
    QLabel* mPlaceholderLabel;
};

#endif // VIDEOSTREAMWINDOW_H
