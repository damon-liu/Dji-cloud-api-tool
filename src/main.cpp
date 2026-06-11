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

    // 配置文件路径（与可执行文件同目录）
    QString configPath = QApplication::applicationDirPath() + "/config.json";
    // 如果运行目录有 config.json 优先使用
    if (QFile::exists("config.json"))
        configPath = "config.json";

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
