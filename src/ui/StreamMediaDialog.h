#ifndef STREAMMEDIADIALOG_H
#define STREAMMEDIADIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include "ConfigStore.h"

class DeviceManager;

// 流媒体推流服务器配置对话框
// 修改后立即保存到配置文件
class StreamMediaDialog : public QDialog {
    Q_OBJECT
public:
    explicit StreamMediaDialog(DeviceManager* devMgr, QWidget* parent = nullptr);

    StreamMediaConfig getConfig() const;

private:
    DeviceManager*  mDevMgr;
    QLineEdit*      mIpEdit;
    QSpinBox*       mPortSpin;
    QComboBox*      mProtocolCombo;
    QLineEdit*      mStreamKeyEdit;
};

#endif // STREAMMEDIADIALOG_H
