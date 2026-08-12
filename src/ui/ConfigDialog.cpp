#include "ConfigDialog.h"
#include "DeviceManager.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>
#include <QGroupBox>

// QSpinBox 子类：值为 0 时显示空白，不显示数字
class PortSpinBox : public QSpinBox {
public:
    using QSpinBox::QSpinBox;
    QString textFromValue(int value) const override {
        return (value == 0) ? QString() : QSpinBox::textFromValue(value);
    }
};

ConfigDialog::ConfigDialog(DeviceManager* devMgr, QWidget* parent)
    : QDialog(parent)
    , mDevMgr(devMgr)
{
    setWindowTitle(QString::fromUtf8("\xe8\xbf\x9e\xe6\x8e\xa5\xe9\x85\x8d\xe7\xbd\xae"));
    setMinimumWidth(750);

    auto* layout = new QVBoxLayout(this);

    // ——— Profile 选择器 ———
    auto* profileRow = new QHBoxLayout;

    mProfileCombo = new QComboBox(this);
    mProfileCombo->setMinimumWidth(120);
    connect(mProfileCombo, &QComboBox::currentTextChanged,
            this, &ConfigDialog::onProfileSelected);

    mAddProfileBtn = new QPushButton("+", this);
    mAddProfileBtn->setFixedWidth(30);
    mAddProfileBtn->setToolTip(QString::fromUtf8("\xe6\x96\xb0\xe5\xa2\x9e Connections"));
    connect(mAddProfileBtn, &QPushButton::clicked, this, &ConfigDialog::onAddProfile);

    mRenameProfileBtn = new QPushButton(QString::fromUtf8("\xe2\x9c\x8e"), this);
    mRenameProfileBtn->setFixedWidth(30);
    mRenameProfileBtn->setToolTip(QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"));
    connect(mRenameProfileBtn, &QPushButton::clicked, this, &ConfigDialog::onRenameProfile);

    mDeleteProfileBtn = new QPushButton(QString::fromUtf8("\xe2\x9c\x95"), this);
    mDeleteProfileBtn->setFixedWidth(30);
    mDeleteProfileBtn->setToolTip(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4 Connections"));
    connect(mDeleteProfileBtn, &QPushButton::clicked, this, &ConfigDialog::onDeleteProfile);

    profileRow->addWidget(new QLabel("Connections:", this));
    profileRow->addWidget(mProfileCombo, 1);
    profileRow->addWidget(mAddProfileBtn);
    profileRow->addWidget(mRenameProfileBtn);
    profileRow->addWidget(mDeleteProfileBtn);
    layout->addLayout(profileRow);

    // 分隔线
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #e0e0e0;");
    layout->addWidget(sep);

    // ——— 左右分栏：MQTT 连接 | 流媒体推流 ———
    auto* splitLayout = new QHBoxLayout;
    splitLayout->setSpacing(12);

    // ===== 左侧：MQTT 连接配置 =====
    auto* mqttGroup = new QGroupBox(QString::fromUtf8("\xf0\x9f\x94\x8c \xe8\xbf\x9e\xe6\x8e\xa5\xe9\x85\x8d\xe7\xbd\xae"), this);
    auto* mqttGroupLayout = new QVBoxLayout(mqttGroup);
    auto* form = new QFormLayout;

    mHostEdit = new QLineEdit(this);
    mPortSpin = new PortSpinBox(this);
    mPortSpin->setRange(0, 65535);
    mUsernameEdit = new QLineEdit(this);
    mPasswordEdit = new QLineEdit(this);
    mPasswordEdit->setEchoMode(QLineEdit::Password);

    // 密码可见切换按钮（👁）：点击切换明/密文
    auto* togglePwdBtn = new QPushButton(QString::fromUtf8("\xf0\x9f\x91\x81"), this);
    togglePwdBtn->setFixedWidth(30);
    togglePwdBtn->setCheckable(true);
    togglePwdBtn->setToolTip(QString::fromUtf8("\xe6\x98\xbe\xe7\xa4\xba/\xe9\x9a\x90\xe8\x97\x8f\xe5\xaf\x86\xe7\xa0\x81"));
    connect(togglePwdBtn, &QPushButton::toggled, this, [this](bool checked) {
        mPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto* passwordRow = new QHBoxLayout;
    passwordRow->addWidget(mPasswordEdit, 1);
    passwordRow->addWidget(togglePwdBtn);

    form->addRow("Broker IP:", mHostEdit);
    form->addRow(QString::fromUtf8("\xe7\xab\xaf\xe5\x8f\xa3:"), mPortSpin);
    form->addRow(QString::fromUtf8("\xe7\x94\xa8\xe6\x88\xb7\xe5\x90\x8d:"), mUsernameEdit);
    form->addRow(QString::fromUtf8("\xe5\xaf\x86\xe7\xa0\x81:"), passwordRow);

    mClientIdEdit = new QLineEdit(this);
    mClientIdEdit->setPlaceholderText(QString::fromUtf8("\xe7\x95\x99\xe7\xa9\xba\xe4\xbd\xbf\xe7\x94\xa8\xe9\xbb\x98\xe8\xae\xa4\xe5\x80\xbc"));
    form->addRow("Client ID:", mClientIdEdit);

    mqttGroupLayout->addLayout(form);
    splitLayout->addWidget(mqttGroup);

    // ===== 右侧：流媒体服务配置 =====
    auto* streamGroup = new QGroupBox(QString::fromUtf8("\xf0\x9f\x93\xb9 \xe6\xb5\x81\xe5\xaa\x92\xe4\xbd\x93\xe6\x9c\x8d\xe5\x8a\xa1\xe9\x85\x8d\xe7\xbd\xae"), this);
    auto* streamGroupLayout = new QVBoxLayout(streamGroup);
    auto* streamForm = new QFormLayout;

    mStreamIpEdit = new QLineEdit(this);
    mStreamIpEdit->setPlaceholderText(QString::fromUtf8("\xe7\x95\x99\xe7\xa9\xba\xe5\x88\x99\xe4\xb8\x8d\xe5\x90\xaf\xe7\x94\xa8\xe6\x8e\xa8\xe6\xb5\x81"));
    streamForm->addRow(QString::fromUtf8("\xe6\x9c\x8d\xe5\x8a\xa1\xe5\x99\xa8 IP:"), mStreamIpEdit);

    mStreamPortSpin = new PortSpinBox(this);
    mStreamPortSpin->setRange(0, 65535);
    mStreamPortSpin->setValue(1935);
    streamForm->addRow(QString::fromUtf8("\xe7\xab\xaf\xe5\x8f\xa3:"), mStreamPortSpin);

    mStreamProtocolCombo = new QComboBox(this);
    mStreamProtocolCombo->addItem("RTMP",   1);
    mStreamProtocolCombo->addItem("GB28181", 3);
    mStreamProtocolCombo->addItem("WebRTC",  4);
    streamForm->addRow(QString::fromUtf8("\xe5\x8d\x8f\xe8\xae\xae\xe7\xb1\xbb\xe5\x9e\x8b:"), mStreamProtocolCombo);

    streamGroupLayout->addLayout(streamForm);
    streamGroupLayout->addStretch();
    splitLayout->addWidget(streamGroup);

    layout->addLayout(splitLayout);

    // 底部按钮行
    auto* bottomLayout = new QHBoxLayout;

    mTestBtn = new QPushButton("Test", this);
    mTestBtn->setFixedWidth(80);
    connect(mTestBtn, &QPushButton::clicked, this, &ConfigDialog::onTestClicked);
    bottomLayout->addWidget(mTestBtn);

    bottomLayout->addStretch();

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        // 保存当前编辑到当前活跃 profile
        mDevMgr->setMqttConfigForProfile(mSelectedProfile, getConfig());
        if (mSelectedProfile == mDevMgr->currentProfileName())
            mDevMgr->setMqttConfig(getConfig());
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    bottomLayout->addWidget(buttons);

    layout->addLayout(bottomLayout);

    // 初始加载
    refreshProfileList();
    mSelectedProfile = mDevMgr->currentProfileName();
    mProfileCombo->setCurrentText(mSelectedProfile);
    loadProfile(mSelectedProfile);
}

void ConfigDialog::refreshProfileList() {
    QString current = mProfileCombo->currentText();
    mProfileCombo->blockSignals(true);
    mProfileCombo->clear();
    mProfileCombo->addItems(mDevMgr->profileNames());
    mProfileCombo->blockSignals(false);

    bool single = mDevMgr->profileNames().size() <= 1;
    mDeleteProfileBtn->setEnabled(!single);
}

void ConfigDialog::loadProfile(const QString& name) {
    MqttConfig cfg = mDevMgr->mqttConfigForProfile(name);
    mHostEdit->setText(cfg.host);
    mPortSpin->setValue(cfg.port);
    mUsernameEdit->setText(cfg.username);
    mPasswordEdit->setText(cfg.password);
    mClientIdEdit->setText(cfg.clientId);

    // 流媒体推流配置
    mStreamIpEdit->setText(cfg.streamMedia.ip);
    mStreamPortSpin->setValue(cfg.streamMedia.port);
    int protoIdx = mStreamProtocolCombo->findData(cfg.streamMedia.protocol);
    if (protoIdx >= 0)
        mStreamProtocolCombo->setCurrentIndex(protoIdx);

}

MqttConfig ConfigDialog::getConfig() const {
    MqttConfig cfg;
    cfg.host     = mHostEdit->text().trimmed();
    cfg.port     = mPortSpin->value();
    cfg.username = mUsernameEdit->text().trimmed();
    cfg.password = mPasswordEdit->text();
    cfg.clientId = mClientIdEdit->text().trimmed();

    // 流媒体推流配置
    cfg.streamMedia.ip        = mStreamIpEdit->text().trimmed();
    cfg.streamMedia.port      = mStreamPortSpin->value();
    cfg.streamMedia.protocol  = mStreamProtocolCombo->currentData().toInt();
    // streamKey 由 VideoStreamWindow 级别的配置管理，不在此全局设置

    return cfg;
}

void ConfigDialog::onProfileSelected(const QString& name) {
    if (name.isEmpty() || name == mSelectedProfile) return;

    // 保存编辑内容到上一个 profile
    if (!mSelectedProfile.isEmpty())
        mDevMgr->setMqttConfigForProfile(mSelectedProfile, getConfig());

    mSelectedProfile = name;
    loadProfile(name);

    // 立即切换到选中的 profile
    if (name != mDevMgr->currentProfileName())
        mDevMgr->switchToProfile(name);
}

void ConfigDialog::onAddProfile() {
    bool ok;
    QString name = QInputDialog::getText(this,
        QString::fromUtf8("\xe6\x96\xb0\xe5\xa2\x9e Connections"),
        QString::fromUtf8("Connections \xe5\x90\x8d\xe7\xa7\xb0:"),
        QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    MqttConfig cfg;
    cfg.host.clear();
    cfg.port = 0;
    cfg.username.clear();
    cfg.password.clear();
    cfg.clientId.clear();
    if (!mDevMgr->addProfile(name.trimmed(), cfg)) {
        QMessageBox::warning(this, QString::fromUtf8("\xe9\x94\x99\xe8\xaf\xaf"),
            QString::fromUtf8("\xe8\xaf\xa5\xe5\x90\x8d\xe7\xa7\xb0\xe5\xb7\xb2\xe5\xad\x98\xe5\x9c\xa8"));
        return;
    }

    // 保存当前编辑到旧 profile 再切换
    if (!mSelectedProfile.isEmpty())
        mDevMgr->setMqttConfigForProfile(mSelectedProfile, getConfig());

    refreshProfileList();
    // 先更新 mSelectedProfile，防止 setCurrentText 触发 onProfileSelected 时重复处理
    mSelectedProfile = name.trimmed();
    mProfileCombo->setCurrentText(name.trimmed());
    // 显式加载新 profile 到 UI，不依赖信号链
    loadProfile(name.trimmed());

    // 立即切换到新 profile
    if (name.trimmed() != mDevMgr->currentProfileName())
        mDevMgr->switchToProfile(name.trimmed());
}

void ConfigDialog::onRenameProfile() {
    QString oldName = mProfileCombo->currentText();
    if (oldName.isEmpty()) return;

    bool ok;
    QString newName = QInputDialog::getText(this,
        QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d Connections"),
        QString::fromUtf8("\xe6\x96\xb0\xe5\x90\x8d\xe7\xa7\xb0:"),
        QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == oldName) return;

    if (!mDevMgr->renameProfile(oldName, newName.trimmed())) {
        QMessageBox::warning(this, QString::fromUtf8("\xe9\x94\x99\xe8\xaf\xaf"),
            QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d\xe5\xa4\xb1\xe8\xb4\xa5\xef\xbc\x8c\xe5\x90\x8d\xe7\xa7\xb0\xe5\x8f\xaf\xe8\x83\xbd\xe5\xb7\xb2\xe5\xad\x98\xe5\x9c\xa8"));
        return;
    }
    mSelectedProfile = newName.trimmed();
    refreshProfileList();
    mProfileCombo->setCurrentText(mSelectedProfile);
}

void ConfigDialog::onDeleteProfile() {
    QString name = mProfileCombo->currentText();
    if (name.isEmpty()) return;

    auto ret = QMessageBox::question(this,
        QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4 Connections"),
        QString::fromUtf8("\xe7\xa1\xae\xe5\xae\x9a\xe5\x88\xa0\xe9\x99\xa4 Connections \"%1\" \xe5\x8f\x8a\xe5\x85\xb6\xe6\x89\x80\xe6\x9c\x89\xe8\xae\xbe\xe5\xa4\x87\xef\xbc\x9f").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!mDevMgr->removeProfile(name)) {
        QMessageBox::warning(this, QString::fromUtf8("\xe9\x94\x99\xe8\xaf\xaf"),
            QString::fromUtf8("\xe4\xb8\x8d\xe8\x83\xbd\xe5\x88\xa0\xe9\x99\xa4\xe6\x9c\x80\xe5\x90\x8e\xe4\xb8\x80\xe4\xb8\xaa Connections"));
        return;
    }
    refreshProfileList();
    mSelectedProfile = mDevMgr->currentProfileName();
    mProfileCombo->setCurrentText(mSelectedProfile);
    loadProfile(mSelectedProfile);
}

// ——— 以下保持原 Test 逻辑不变 ———

void ConfigDialog::onTestClicked() {
    if (mTestRunning) return;
    startTest();
}

void ConfigDialog::startTest() {
    mTestRunning = true;
    mTestBtn->setEnabled(false);
    mTestBtn->setText("...");

    mTestClient = new QMqttClient(this);
    mTestClient->setHostname(mHostEdit->text().trimmed());
    mTestClient->setPort(static_cast<quint16>(mPortSpin->value()));
    if (!mUsernameEdit->text().trimmed().isEmpty())
        mTestClient->setUsername(mUsernameEdit->text().trimmed());
    if (!mPasswordEdit->text().isEmpty())
        mTestClient->setPassword(mPasswordEdit->text());

    mTestTimer = new QTimer(this);
    mTestTimer->setSingleShot(true);

    connect(mTestClient, &QMqttClient::connected, this, [this]() {
        mTestClient->disconnectFromHost();
        cleanupTest();
        QMessageBox::information(this, QString::fromUtf8("\xe8\xbf\x9e\xe6\x8e\xa5\xe6\xb5\x8b\xe8\xaf\x95"),
            QString::fromUtf8("\xe2\x9c\x93 MQTT \xe8\xbf\x9e\xe6\x8e\xa5\xe6\x88\x90\xe5\x8a\x9f\xef\xbc\x81"));
    });

    connect(mTestClient, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
        if (error == QMqttClient::NoError) return;
        cleanupTest();
        QMessageBox::warning(this, QString::fromUtf8("\xe8\xbf\x9e\xe6\x8e\xa5\xe6\xb5\x8b\xe8\xaf\x95"),
            QString::fromUtf8("\xe8\xbf\x9e\xe6\x8e\xa5\xe5\xa4\xb1\xe8\xb4\xa5\xe8\xaf\xb7\xe6\xa3\x80\xe6\x9f\xa5\xe9\x85\x8d\xe7\xbd\xae\xe5\x8f\x82\xe6\x95\xb0\xe6\x98\xaf\xe5\x90\xa6\xe6\x9c\x89\xe8\xaf\xaf"));
    });

    connect(mTestTimer, &QTimer::timeout, this, [this]() {
        cleanupTest();
        QMessageBox::warning(this, QString::fromUtf8("\xe8\xbf\x9e\xe6\x8e\xa5\xe6\xb5\x8b\xe8\xaf\x95"),
            QString::fromUtf8("\xe8\xbf\x9e\xe6\x8e\xa5\xe5\xa4\xb1\xe8\xb4\xa5\xe8\xaf\xb7\xe6\xa3\x80\xe6\x9f\xa5\xe9\x85\x8d\xe7\xbd\xae\xe5\x8f\x82\xe6\x95\xb0\xe6\x98\xaf\xe5\x90\xa6\xe6\x9c\x89\xe8\xaf\xaf"));
    });

    mTestClient->connectToHost();
    mTestTimer->start(5000);
}

void ConfigDialog::cleanupTest() {
    mTestRunning = false;
    mTestBtn->setEnabled(true);
    mTestBtn->setText("Test");

    if (mTestTimer) {
        mTestTimer->stop();
        delete mTestTimer;
        mTestTimer = nullptr;
    }
    if (mTestClient) {
        mTestClient->deleteLater();
        mTestClient = nullptr;
    }
}
