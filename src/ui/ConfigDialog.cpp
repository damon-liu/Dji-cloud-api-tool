#include "ConfigDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QLabel>

ConfigDialog::ConfigDialog(const MqttConfig& config, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("MQTT 连接配置");
    setMinimumWidth(380);

    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout;
    mHostEdit = new QLineEdit(config.host, this);
    mPortSpin = new QSpinBox(this);
    mPortSpin->setRange(1, 65535);
    mPortSpin->setValue(config.port);
    mUsernameEdit = new QLineEdit(config.username, this);
    mPasswordEdit = new QLineEdit(config.password, this);
    mPasswordEdit->setEchoMode(QLineEdit::Password);

    form->addRow("Broker IP:", mHostEdit);
    form->addRow("端口:", mPortSpin);
    form->addRow("用户名:", mUsernameEdit);
    form->addRow("密码:", mPasswordEdit);
    layout->addLayout(form);

    // 底部按钮行
    auto* bottomLayout = new QHBoxLayout;

    // Test 按钮（靠左）
    mTestBtn = new QPushButton("Test", this);
    mTestBtn->setFixedWidth(80);
    connect(mTestBtn, &QPushButton::clicked, this, &ConfigDialog::onTestClicked);
    bottomLayout->addWidget(mTestBtn);

    bottomLayout->addStretch();

    // OK / Cancel（靠右）
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    bottomLayout->addWidget(buttons);

    layout->addLayout(bottomLayout);
}

MqttConfig ConfigDialog::getConfig() const {
    MqttConfig cfg;
    cfg.host     = mHostEdit->text().trimmed();
    cfg.port     = mPortSpin->value();
    cfg.username = mUsernameEdit->text().trimmed();
    cfg.password = mPasswordEdit->text();
    return cfg;
}

void ConfigDialog::onTestClicked() {
    if (mTestRunning) return;
    startTest();
}

void ConfigDialog::startTest() {
    mTestRunning = true;
    mTestBtn->setEnabled(false);
    mTestBtn->setText("...");

    // 创建临时 MQTT 客户端
    mTestClient = new QMqttClient(this);
    mTestClient->setHostname(mHostEdit->text().trimmed());
    mTestClient->setPort(static_cast<quint16>(mPortSpin->value()));
    if (!mUsernameEdit->text().trimmed().isEmpty())
        mTestClient->setUsername(mUsernameEdit->text().trimmed());
    if (!mPasswordEdit->text().isEmpty())
        mTestClient->setPassword(mPasswordEdit->text());

    // 超时定时器（5 秒）
    mTestTimer = new QTimer(this);
    mTestTimer->setSingleShot(true);

    connect(mTestClient, &QMqttClient::connected, this, [this]() {
        mTestClient->disconnectFromHost();
        cleanupTest();
        QMessageBox::information(this, "连接测试", "✓ MQTT 连接成功！");
    });

    connect(mTestClient, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
        if (error == QMqttClient::NoError) return;
        QString errMsg;
        switch (error) {
        case QMqttClient::InvalidProtocolVersion: errMsg = "无效的协议版本"; break;
        case QMqttClient::IdRejected:             errMsg = "Client ID 被拒绝"; break;
        case QMqttClient::ServerUnavailable:     errMsg = "服务器不可用"; break;
        case QMqttClient::BadUsernameOrPassword: errMsg = "用户名或密码错误"; break;
        case QMqttClient::NotAuthorized:         errMsg = "未授权"; break;
        case QMqttClient::TransportInvalid:      errMsg = "传输层错误（无法连接）"; break;
        case QMqttClient::ProtocolViolation:     errMsg = "协议违规"; break;
        default:                                 errMsg = "未知错误"; break;
        }
        cleanupTest();
        QMessageBox::warning(this, "连接测试", "连接失败请检查配置参数是否有误");
    });

    connect(mTestTimer, &QTimer::timeout, this, [this]() {
        cleanupTest();
        QMessageBox::warning(this, "连接测试", "连接失败请检查配置参数是否有误");
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
