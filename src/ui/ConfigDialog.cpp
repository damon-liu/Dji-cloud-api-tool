#include "ConfigDialog.h"
#include "DeviceManager.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>

ConfigDialog::ConfigDialog(DeviceManager* devMgr, QWidget* parent)
    : QDialog(parent)
    , mDevMgr(devMgr)
{
    setWindowTitle("MQTT 连接配置");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);

    // ——— Profile 选择器 ———
    auto* profileRow = new QHBoxLayout;

    mProfileCombo = new QComboBox(this);
    mProfileCombo->setMinimumWidth(120);
    connect(mProfileCombo, &QComboBox::currentTextChanged,
            this, &ConfigDialog::onProfileSelected);

    mAddProfileBtn = new QPushButton("+", this);
    mAddProfileBtn->setFixedWidth(30);
    mAddProfileBtn->setToolTip(QString::fromUtf8("\xe6\x96\xb0\xe5\xa2\x9e Profile"));
    connect(mAddProfileBtn, &QPushButton::clicked, this, &ConfigDialog::onAddProfile);

    mRenameProfileBtn = new QPushButton(QString::fromUtf8("\xe2\x9c\x8e"), this);
    mRenameProfileBtn->setFixedWidth(30);
    mRenameProfileBtn->setToolTip(QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"));
    connect(mRenameProfileBtn, &QPushButton::clicked, this, &ConfigDialog::onRenameProfile);

    mDeleteProfileBtn = new QPushButton(QString::fromUtf8("\xe2\x9c\x95"), this);
    mDeleteProfileBtn->setFixedWidth(30);
    mDeleteProfileBtn->setToolTip(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4 Profile"));
    connect(mDeleteProfileBtn, &QPushButton::clicked, this, &ConfigDialog::onDeleteProfile);

    profileRow->addWidget(new QLabel("Profile:", this));
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

    // ——— MQTT 参数 ———
    auto* form = new QFormLayout;
    mHostEdit = new QLineEdit(this);
    mPortSpin = new QSpinBox(this);
    mPortSpin->setRange(1, 65535);
    mUsernameEdit = new QLineEdit(this);
    mPasswordEdit = new QLineEdit(this);
    mPasswordEdit->setEchoMode(QLineEdit::Password);

    form->addRow("Broker IP:", mHostEdit);
    form->addRow(QString::fromUtf8("\xe7\xab\xaf\xe5\x8f\xa3:"), mPortSpin);
    form->addRow(QString::fromUtf8("\xe7\x94\xa8\xe6\x88\xb7\xe5\x90\x8d:"), mUsernameEdit);
    form->addRow(QString::fromUtf8("\xe5\xaf\x86\xe7\xa0\x81:"), mPasswordEdit);
    layout->addLayout(form);

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
        // 保存当前编辑到选中 profile
        mDevMgr->setMqttConfigForProfile(mSelectedProfile, getConfig());
        // 如果选中的不是当前活跃 profile，切换过去
        if (mSelectedProfile != mDevMgr->currentProfileName())
            mDevMgr->switchToProfile(mSelectedProfile);
        else
            mDevMgr->setMqttConfig(getConfig());  // 更新当前 profile 的 MQTT
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
}

MqttConfig ConfigDialog::getConfig() const {
    MqttConfig cfg;
    cfg.host     = mHostEdit->text().trimmed();
    cfg.port     = mPortSpin->value();
    cfg.username = mUsernameEdit->text().trimmed();
    cfg.password = mPasswordEdit->text();
    return cfg;
}

void ConfigDialog::onProfileSelected(const QString& name) {
    if (name.isEmpty() || name == mSelectedProfile) return;

    // 保存编辑内容到上一个 profile
    if (!mSelectedProfile.isEmpty())
        mDevMgr->setMqttConfigForProfile(mSelectedProfile, getConfig());

    mSelectedProfile = name;
    loadProfile(name);
}

void ConfigDialog::onAddProfile() {
    bool ok;
    QString name = QInputDialog::getText(this,
        QString::fromUtf8("\xe6\x96\xb0\xe5\xa2\x9e Profile"),
        QString::fromUtf8("Profile \xe5\x90\x8d\xe7\xa7\xb0:"),
        QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    MqttConfig cfg;  // 使用默认或复制当前
    if (!mDevMgr->addProfile(name.trimmed(), cfg)) {
        QMessageBox::warning(this, QString::fromUtf8("\xe9\x94\x99\xe8\xaf\xaf"),
            QString::fromUtf8("\xe8\xaf\xa5\xe5\x90\x8d\xe7\xa7\xb0\xe5\xb7\xb2\xe5\xad\x98\xe5\x9c\xa8"));
        return;
    }
    refreshProfileList();
    mProfileCombo->setCurrentText(name.trimmed());
}

void ConfigDialog::onRenameProfile() {
    QString oldName = mProfileCombo->currentText();
    if (oldName.isEmpty()) return;

    bool ok;
    QString newName = QInputDialog::getText(this,
        QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d Profile"),
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
        QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4 Profile"),
        QString::fromUtf8("\xe7\xa1\xae\xe5\xae\x9a\xe5\x88\xa0\xe9\x99\xa4 Profile \"%1\" \xe5\x8f\x8a\xe5\x85\xb6\xe6\x89\x80\xe6\x9c\x89\xe8\xae\xbe\xe5\xa4\x87\xef\xbc\x9f").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!mDevMgr->removeProfile(name)) {
        QMessageBox::warning(this, QString::fromUtf8("\xe9\x94\x99\xe8\xaf\xaf"),
            QString::fromUtf8("\xe4\xb8\x8d\xe8\x83\xbd\xe5\x88\xa0\xe9\x99\xa4\xe6\x9c\x80\xe5\x90\x8e\xe4\xb8\x80\xe4\xb8\xaa Profile"));
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
