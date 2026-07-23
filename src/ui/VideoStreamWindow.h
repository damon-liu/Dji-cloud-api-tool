#ifndef VIDEOSTREAMWINDOW_H
#define VIDEOSTREAMWINDOW_H

#include <QWidget>
#include <QStringList>

class QLabel;
class QPushButton;
class QMenu;

class VideoStreamWindow : public QWidget {
    Q_OBJECT
public:
    // deviceLabel: "飞机" / "机场"
    // defaultSource: 默认视频源 "红外相机" / "机场外视频"
    // switchSources: 可切换的视频源列表
    // switchLabel: 切换按钮文字 "切换镜头" / "切换视频"
    explicit VideoStreamWindow(int index,
                               const QString& deviceLabel,
                               const QString& defaultSource,
                               const QStringList& switchSources,
                               const QString& switchLabel,
                               QWidget* parent = nullptr);
    void setStreamUrl(const QString& url);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStart();
    void onStop();
    void onQualitySelected(const QString& quality);
    void onSwitchSource(const QString& source);

private:
    void setupUi();
    void updateTitle();
    void updateButtonStates();
    void updateQualityButtonText();

    int         mIndex;
    QString     mDeviceLabel;      // "飞机" / "机场"
    QString     mCurrentSource;    // 当前视频源
    QStringList mSwitchSources;    // 可切换的视频源列表
    QString     mSwitchLabel;      // 切换按钮文字
    QString     mCurrentQuality;   // 当前清晰度
    bool        mStreaming = false;

    // 标题栏
    QLabel* mTitleLabel;

    // 视频区域
    QWidget* mVideoArea;
    QLabel*  mPlaceholderLabel;

    // 控制按钮
    QPushButton* mStartBtn;
    QPushButton* mStopBtn;
    QPushButton* mQualityBtn;
    QMenu*       mQualityMenu;

    // 切换视频源按钮
    QPushButton* mSwitchBtn;
    QMenu*       mSwitchMenu;
};

#endif // VIDEOSTREAMWINDOW_H
