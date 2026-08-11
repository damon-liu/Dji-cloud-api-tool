#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "MainWindow.h"
#include "DeviceManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DjiCloudApi");
    app.setApplicationVersion("1.0.0");

    // 确保 config 目录存在
    QString configDir = QApplication::applicationDirPath() + "/config";
    QDir().mkpath(configDir);

    // 配置文件路径：优先使用 config/config.json
    QString configPath = configDir + "/config.json";
    if (!QFile::exists(configPath) && QFile::exists("config.json"))
        configPath = "config.json";  // 降级兼容旧版 CWD 根目录

    qDebug() << "Using config:" << configPath;

    // 初始化 DeviceManager
    DeviceManager devMgr;
    if (!devMgr.initialize(configPath)) {
        qWarning() << "配置加载失败，使用默认配置";
    }

    // 启动主窗口
    MainWindow window(&devMgr);
    window.show();

    // 如需要启动时自动连接，取消下面注释：
    // devMgr.connectBroker();

    return app.exec();
}
