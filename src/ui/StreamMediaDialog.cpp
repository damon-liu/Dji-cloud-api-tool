#include "StreamMediaDialog.h"
#include "DeviceManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

StreamMediaDialog::StreamMediaDialog(DeviceManager* devMgr, QWidget* parent)
    : QDialog(parent)
    , mDevMgr(devMgr)
{
    setWindowTitle(QString::fromUtf8("\xe6\xb5\x81\xe5\xaa\x92\xe4\xbd\x93\xe9\x85\x8d\xe7\xbd\xae"));  // 流媒体配置
    setMinimumWidth(380);

    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout;

    // IP 地址
    mIpEdit = new QLineEdit(this);
    mIpEdit->setPlaceholderText("192.168.1.200");
    form->addRow(QString::fromUtf8("\xe6\x9c\x8d\xe5\x8a\xa1\xe5\x99\xa8\xe5\x9c\xb0\xe5\x9d\x80:"), mIpEdit);  // 服务器地址:

    // 端口
    mPortSpin = new QSpinBox(this);
    mPortSpin->setRange(1, 65535);
    mPortSpin->setValue(1935);
    form->addRow(QString::fromUtf8("\xe7\xab\xaf\xe5\x8f\xa3:"), mPortSpin);  // 端口:

    // 协议类型
    mProtocolCombo = new QComboBox(this);
    mProtocolCombo->addItem("RTMP",     1);
    mProtocolCombo->addItem("GB28181",  3);
    mProtocolCombo->addItem("WebRTC",   4);
    form->addRow(QString::fromUtf8("\xe5\x8d\x8f\xe8\xae\xae\xe7\xb1\xbb\xe5\x9e\x8b:"), mProtocolCombo);  // 协议类型:

    // 流名称（stream key）
    mStreamKeyEdit = new QLineEdit(this);
    mStreamKeyEdit->setPlaceholderText(QString::fromUtf8("\xe7\x95\x99\xe7\xa9\xba\xe5\x88\x99\xe8\x87\xaa\xe5\x8a\xa8\xe4\xbd\xbf\xe7\x94\xa8 video_id"));  // 留空则自动使用 video_id
    form->addRow(QString::fromUtf8("\xe6\xb5\x81\xe5\x90\x8d\xe7\xa7\xb0:"), mStreamKeyEdit);  // 流名称:

    layout->addLayout(form);

    // 按钮
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        mDevMgr->setStreamMediaConfig(getConfig());
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addStretch();
    layout->addWidget(buttons);

    // 加载当前配置
    StreamMediaConfig cfg = mDevMgr->streamMediaConfig();
    mIpEdit->setText(cfg.ip);
    mPortSpin->setValue(cfg.port);
    int protoIdx = mProtocolCombo->findData(cfg.protocol);
    if (protoIdx >= 0)
        mProtocolCombo->setCurrentIndex(protoIdx);
    mStreamKeyEdit->setText(cfg.streamKey);
}

StreamMediaConfig StreamMediaDialog::getConfig() const {
    StreamMediaConfig cfg;
    cfg.ip        = mIpEdit->text().trimmed();
    cfg.port      = mPortSpin->value();
    cfg.protocol  = mProtocolCombo->currentData().toInt();
    cfg.streamKey = mStreamKeyEdit->text().trimmed();
    return cfg;
}
