# 机场控制面板增强实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 增强机场控制面板：在线状态彩色显示、可搜索机场切换、一键起飞/返航飞行控制

**Architecture:** 后端扩展 DockCommand 枚举和 Builder 支持带数据的飞行指令；前端 DockControlPanel 增加 QComboBox 机场选择器和飞行控制卡片；新建 TakeoffDialog 起飞参数弹窗；MainWindow 负责联动机场列表和位置数据

**Tech Stack:** Qt 6 C++17, QMqttClient, 无外部依赖

---

### Task 1: 扩展 DockCommand — 新增飞行指令类型和 data 参数

**Files:**
- Modify: `src/core/DockCommand.h:9-62`
- Modify: `src/core/DockCommand.cpp:1-75`

- [ ] **Step 1: 扩展枚举和 build 签名**

在 `DockCommand.h` 的 `DockCommandType` 枚举新增 `Takeoff`、`Return`：

```cpp
enum class DockCommandType {
    DebugModeOpen,
    DebugModeClose,
    DroneOpen,
    DroneClose,
    CoverOpen,
    CoverClose,
    ChargeOpen,
    ChargeClose,
    Takeoff,   // 一键起飞 takeoff_to_point
    Return     // 一键返航 return_home
};
```

修改 `DockCommandBuilder::build()` 声明，增加可选的 `data` 参数：

```cpp
class DockCommandBuilder {
public:
    static DockCommandRequest build(const QString& gatewaySn, DockCommandType type,
                                    const QJsonObject& data = {});
    // ... 其余不变
};
```

- [ ] **Step 2: 实现 method()、displayName()、requiresDebugMode()**

在 `DockCommand.cpp` 的 `method()` 新增：

```cpp
case DockCommandType::Takeoff: return QStringLiteral("takeoff_to_point");
case DockCommandType::Return:  return QStringLiteral("return_home");
```

在 `displayName()` 新增：

```cpp
case DockCommandType::Takeoff: return QString::fromUtf8("一键起飞");
case DockCommandType::Return:  return QString::fromUtf8("一键返航");
```

在 `requiresDebugMode()` 新增（Takeoff 和 Return 不依赖调试模式）：

```cpp
bool DockCommandBuilder::requiresDebugMode(DockCommandType type) {
    return type != DockCommandType::DebugModeOpen
        && type != DockCommandType::DebugModeClose
        && type != DockCommandType::Takeoff
        && type != DockCommandType::Return;
}
```

- [ ] **Step 3: 修改 build() 实现支持 data 参数**

将 `DockCommand.cpp` 中 `build()` 的签名和实现更新：

```cpp
DockCommandRequest DockCommandBuilder::build(const QString& gatewaySn, DockCommandType type,
                                             const QJsonObject& data) {
    DockCommandRequest request;
    request.type = type;
    request.gatewaySn = gatewaySn.trimmed();
    request.topic = QStringLiteral("thing/product/%1/services").arg(request.gatewaySn);
    request.tid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.bid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.method = method(type);

    request.payload[QStringLiteral("tid")] = request.tid;
    request.payload[QStringLiteral("bid")] = request.bid;
    request.payload[QStringLiteral("timestamp")] = QDateTime::currentMSecsSinceEpoch();
    request.payload[QStringLiteral("gateway")] = request.gatewaySn;
    request.payload[QStringLiteral("method")] = request.method;
    request.payload[QStringLiteral("data")] = data.isEmpty() ? QJsonObject{} : data;
    return request;
}
```

- [ ] **Step 4: 编译验证**

```bash
cmake --build build_mingw
```

---

### Task 2: 新建 TakeoffDialog — 起飞参数弹窗

**Files:**
- Create: `src/ui/TakeoffDialog.h`

- [ ] **Step 1: 创建 TakeoffParams 结构体和 TakeoffDialog 类**

```cpp
#ifndef TAKEOFFDIALOG_H
#define TAKEOFFDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>

struct TakeoffParams {
    double latitude  = 0.0;
    double longitude = 0.0;
    double height    = 100.0;   // 飞行高度 (m)
    double speed     = 0.0;     // 飞行速度 (m/s)，0 表示使用默认值
};

class TakeoffDialog : public QDialog {
    Q_OBJECT
public:
    explicit TakeoffDialog(double defaultLat, double defaultLon, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QString::fromUtf8("一键起飞 - 飞行参数设置"));
        setFixedSize(420, 320);

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(10);

        // 提示标签
        auto* hint = new QLabel(
            QString::fromUtf8("请输入目标位置和飞行参数，目标位置默认为机场当前位置"));
        hint->setWordWrap(true);
        hint->setStyleSheet("color: #5f6368; font-size: 12px; margin-bottom: 4px;");
        layout->addWidget(hint);

        auto* form = new QFormLayout;
        form->setSpacing(8);
        form->setContentsMargins(8, 0, 8, 0);

        mLatSpin = new QDoubleSpinBox;
        mLatSpin->setRange(-90.0, 90.0);
        mLatSpin->setDecimals(6);
        mLatSpin->setValue(defaultLat);
        mLatSpin->setSuffix(QString::fromUtf8("°"));
        form->addRow(QString::fromUtf8("目标纬度:"), mLatSpin);

        mLonSpin = new QDoubleSpinBox;
        mLonSpin->setRange(-180.0, 180.0);
        mLonSpin->setDecimals(6);
        mLonSpin->setValue(defaultLon);
        mLonSpin->setSuffix(QString::fromUtf8("°"));
        form->addRow(QString::fromUtf8("目标经度:"), mLonSpin);

        mHeightSpin = new QDoubleSpinBox;
        mHeightSpin->setRange(20.0, 500.0);
        mHeightSpin->setDecimals(1);
        mHeightSpin->setValue(100.0);
        mHeightSpin->setSuffix(QString::fromUtf8(" m"));
        form->addRow(QString::fromUtf8("飞行高度:"), mHeightSpin);

        mSpeedSpin = new QDoubleSpinBox;
        mSpeedSpin->setRange(0.0, 15.0);
        mSpeedSpin->setDecimals(1);
        mSpeedSpin->setValue(0.0);
        mSpeedSpin->setSpecialValueText(QString::fromUtf8("默认"));
        mSpeedSpin->setSuffix(QString::fromUtf8(" m/s"));
        form->addRow(QString::fromUtf8("飞行速度:"), mSpeedSpin);

        layout->addLayout(form);

        // 分隔线
        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setFrameShadow(QFrame::Sunken);
        layout->addWidget(sep);

        // 环境确认
        mConfirmCheck = new QCheckBox(
            QString::fromUtf8("我确认当前环境适合起飞，已完成飞行前检查"));
        mConfirmCheck->setStyleSheet("font-weight: bold; color: #333;");
        layout->addWidget(mConfirmCheck);

        // 确定/取消按钮
        auto* btnRow = new QHBoxLayout;
        btnRow->addStretch();

        mOkBtn = new QPushButton(QString::fromUtf8("确定起飞"));
        mOkBtn->setStyleSheet(
            "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
            "border: none; border-radius: 4px; padding: 8px 24px; }"
            "QPushButton:hover { background: #1557b0; }"
            "QPushButton:disabled { background: #dadce0; color: #80868b; }");
        mOkBtn->setEnabled(false);
        mOkBtn->setCursor(Qt::PointingHandCursor);

        auto* cancelBtn = new QPushButton(QString::fromUtf8("取消"));
        cancelBtn->setStyleSheet(
            "QPushButton { border: 1px solid #dadce0; border-radius: 4px;"
            "padding: 8px 20px; background: #fff; color: #5f6368; }"
            "QPushButton:hover { background: #f1f3f4; }");
        cancelBtn->setCursor(Qt::PointingHandCursor);

        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(mOkBtn);
        layout->addLayout(btnRow);

        connect(mConfirmCheck, &QCheckBox::toggled, mOkBtn, &QPushButton::setEnabled);
        connect(mOkBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }

    TakeoffParams params() const {
        TakeoffParams p;
        p.latitude  = mLatSpin->value();
        p.longitude = mLonSpin->value();
        p.height    = mHeightSpin->value();
        p.speed     = mSpeedSpin->value();
        return p;
    }

private:
    QDoubleSpinBox* mLatSpin;
    QDoubleSpinBox* mLonSpin;
    QDoubleSpinBox* mHeightSpin;
    QDoubleSpinBox* mSpeedSpin;
    QCheckBox*      mConfirmCheck;
    QPushButton*    mOkBtn;
};

#endif // TAKEOFFDIALOG_H
```

- [ ] **Step 2: 编译验证**

```bash
cmake --build build_mingw
```

---

### Task 3: 扩展信号链路 — commandRequested 支持 data 参数

**Files:**
- Modify: `src/ui/DockControlPanel.h:24` — 信号签名
- Modify: `src/ui/DockControlPanel.cpp:162-178` — requestCommand 实现
- Modify: `src/core/DeviceManager.h:83` — executeDockCommand 签名
- Modify: `src/core/DeviceManager.cpp:629-631` — executeDockCommand 实现
- Modify: `src/core/DockCommandExecutor.h:19` — execute 签名
- Modify: `src/core/DockCommandExecutor.cpp:17-43` — execute 实现

- [ ] **Step 1: 修改 DockControlPanel 信号和 requestCommand**

在 `DockControlPanel.h` 中修改信号：

```cpp
signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});
```

在 `DockControlPanel.cpp` 中修改 `requestCommand()` — 增加 `data` 参数：

```cpp
void DockControlPanel::requestCommand(DockCommandType type, const QJsonObject& data) {
    if (mGatewaySn.isEmpty() || mPending)
        return;

    if (type == DockCommandType::Return) {
        // 一键返航：简单确认对话框
        if (QMessageBox::question(this, QString::fromUtf8("确认返航"),
                QString::fromUtf8("确定要让飞机返航吗？"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes)
            return;
    } else if (type == DockCommandType::Takeoff) {
        // 一键起飞不需要这里弹确认 —— TakeoffDialog 在外层处理
        // 如果走到这里说明外层没有处理，默认放行
    } else {
        // 原有确认逻辑：调试模式相关指令
        const QString action = DockCommandBuilder::displayName(type);
        const QString text = QString::fromUtf8("确定要对机场 %1（%2）执行"%3"吗？")
            .arg(mDisplayName, mGatewaySn, action);
        if (QMessageBox::question(this, QString::fromUtf8("确认机场控制"), text,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes)
            return;
    }

    mPending = true;
    setStatus(QString::fromUtf8("正在执行：%1").arg(DockCommandBuilder::displayName(type)));
    updateButtonStates();
    emit commandRequested(mGatewaySn, type, data);
}
```

更新 `DockControlPanel.h` 中 `requestCommand` 声明：

```cpp
void requestCommand(DockCommandType type, const QJsonObject& data = {});
```

- [ ] **Step 2: 修改 DockCommandExecutor::execute()**

在 `DockCommandExecutor.h` 中：

```cpp
bool execute(const QString& gatewaySn, DockCommandType type,
             const QJsonObject& data = {});
```

在 `DockCommandExecutor.cpp` 中：

```cpp
bool DockCommandExecutor::execute(const QString& gatewaySn, DockCommandType type,
                                  const QJsonObject& data) {
    if (mHasPending) {
        qWarning() << "DockCommandExecutor: command already pending, ignored";
        return false;
    }

    mPending = DockCommandBuilder::build(gatewaySn, type, data);
    // ... 其余保持不变
```

- [ ] **Step 3: 修改 DeviceManager::executeDockCommand()**

在 `DeviceManager.h` 中：

```cpp
void executeDockCommand(const QString& gatewaySn, DockCommandType type,
                        const QJsonObject& data = {});
```

在 `DeviceManager.cpp` 中：

```cpp
void DeviceManager::executeDockCommand(const QString& gatewaySn, DockCommandType type,
                                       const QJsonObject& data) {
    mDockCmdExecutor->execute(gatewaySn, type, data);
}
```

- [ ] **Step 4: 编译验证**

```bash
cmake --build build_mingw
```

---

### Task 4: 改造 DockControlPanel UI — 在线状态 + 机场选择器 + 飞行控制卡片

**Files:**
- Modify: `src/ui/DockControlPanel.h`
- Modify: `src/ui/DockControlPanel.cpp`

- [ ] **Step 1: 更新 DockControlPanel.h — 新增成员和方法**

```cpp
#ifndef DOCKCONTROLPANEL_H
#define DOCKCONTROLPANEL_H

#include <QWidget>
#include <QComboBox>
#include "DockCommand.h"
#include "DeviceInfo.h"
#include "TakeoffDialog.h"

class QLabel;
class QPushButton;
class QPlainTextEdit;

class DockControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit DockControlPanel(QWidget* parent = nullptr);

    void setDevice(const QString& displayName, const QString& gatewaySn, bool online);
    void clearDevice();
    void setConnected(bool connected);
    void setAvailableDocks(const QVector<DeviceInfo>& docks, const QString& currentSn,
                           double dockLat, double dockLon);
    QString currentGatewaySn() const { return mGatewaySn; }

public slots:
    void onCommandStateChanged(const DockCommandResult& result);

signals:
    void commandRequested(const QString& gatewaySn, DockCommandType type,
                          const QJsonObject& data = {});

private:
    enum class DebugModeState { Unknown, Disabled, Enabled };

    void setupUi();
    void requestCommand(DockCommandType type, const QJsonObject& data = {});
    void updateButtonStates();
    void setStatus(const QString& text, bool error = false);
    void appendHistory(const DockCommandResult& result);
    void onTakeoffClicked();

    // — 顶行：机场选择器 —
    QComboBox*    mDockCombo = nullptr;
    QLabel*       mOnlineLabel = nullptr;

    // — 状态标签 —
    QLabel*       mDebugModeLabel = nullptr;
    QLabel*       mStatusLabel = nullptr;

    // — 调试模式 —
    QPushButton*  mDebugOpenBtn = nullptr;
    QPushButton*  mDebugCloseBtn = nullptr;

    // — 设备控制 —
    QPushButton*  mDroneOpenBtn = nullptr;
    QPushButton*  mDroneCloseBtn = nullptr;
    QPushButton*  mCoverOpenBtn = nullptr;
    QPushButton*  mCoverCloseBtn = nullptr;
    QPushButton*  mChargeOpenBtn = nullptr;
    QPushButton*  mChargeCloseBtn = nullptr;

    // — 飞行控制 —
    QPushButton*  mTakeoffBtn = nullptr;
    QPushButton*  mReturnBtn = nullptr;

    QPlainTextEdit* mHistoryEdit = nullptr;

    // — 数据 —
    QVector<DeviceInfo> mAvailableDocks;
    QString mDisplayName;
    QString mGatewaySn;
    double  mDockLat = 0.0;
    double  mDockLon = 0.0;
    bool    mConnected = false;
    bool    mOnline = false;
    bool    mPending = false;
    DebugModeState mDebugModeState = DebugModeState::Unknown;
    bool    mUpdatingCombo = false;  // 防止 combo 变化信号递归
};

#endif // DOCKCONTROLPANEL_H
```

- [ ] **Step 2: 重写 setupUi() — 全部替换**

完整替换 `DockControlPanel.cpp` 中的 `setupUi()`：

```cpp
#include "DockControlPanel.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTime>

DockControlPanel::DockControlPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateButtonStates();
}

void DockControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ——— 顶行：机场选择器（左） + 在线状态（中） + 执行状态（右） ———
    auto* topRow = new QHBoxLayout;

    auto* dockLabel = new QLabel(QString::fromUtf8("控制机场:"), this);
    dockLabel->setStyleSheet("font-weight: bold; color: #333;");
    topRow->addWidget(dockLabel);

    mDockCombo = new QComboBox(this);
    mDockCombo->setEditable(true);
    mDockCombo->setInsertPolicy(QComboBox::NoInsert);
    mDockCombo->setMinimumWidth(200);
    mDockCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 8px;"
        "font-size: 13px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }"
        "QComboBox QAbstractItemView { border: 1px solid #dadce0;"
        "selection-background-color: #e8f0fe; selection-color: #1a73e8; }");
    connect(mDockCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (mUpdatingCombo || idx < 0 || mAvailableDocks.isEmpty()) return;
        const auto& dock = mAvailableDocks[idx];
        setDevice(dock.name, dock.sn, true);
    });
    topRow->addWidget(mDockCombo);

    mOnlineLabel = new QLabel(this);
    topRow->addWidget(mOnlineLabel);

    topRow->addStretch();

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(false);
    setStatus(QString::fromUtf8("连接机场后可使用快捷控制"));
    topRow->addWidget(mStatusLabel);

    mainLayout->addLayout(topRow);

    // ——— 卡片行：远程调试 | 飞机电源 | 机场舱盖 | 飞机充电 | 飞行控制 ———
    auto* cardRow = new QHBoxLayout;
    cardRow->setSpacing(10);

    // 远程调试卡片
    auto* debugGroup = new QGroupBox(QString::fromUtf8("远程调试"), this);
    auto* debugLayout = new QVBoxLayout(debugGroup);
    mDebugModeLabel = new QLabel(QString::fromUtf8("状态：未知"), debugGroup);
    debugLayout->addWidget(mDebugModeLabel);
    debugLayout->addStretch();
    auto* debugBtnRow = new QHBoxLayout;
    mDebugOpenBtn  = new QPushButton(QString::fromUtf8("进入"), debugGroup);
    mDebugCloseBtn = new QPushButton(QString::fromUtf8("退出"), debugGroup);
    debugBtnRow->addWidget(mDebugOpenBtn);
    debugBtnRow->addWidget(mDebugCloseBtn);
    debugLayout->addLayout(debugBtnRow);
    cardRow->addWidget(debugGroup, 3);

    // 功能卡片工厂
    auto makeCard = [this](const QString& title, const QString& openText,
                           const QString& closeText,
                           QPushButton*& openBtn, QPushButton*& closeBtn) {
        auto* group = new QGroupBox(title, this);
        auto* v = new QVBoxLayout(group);
        v->addStretch();
        auto* row = new QHBoxLayout;
        openBtn  = new QPushButton(openText, group);
        closeBtn = new QPushButton(closeText, group);
        row->addWidget(openBtn);
        row->addWidget(closeBtn);
        v->addLayout(row);
        return group;
    };

    cardRow->addWidget(makeCard(QString::fromUtf8("飞机电源"), QString::fromUtf8("开机"),
                                QString::fromUtf8("关机"), mDroneOpenBtn, mDroneCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("机场舱盖"), QString::fromUtf8("打开"),
                                QString::fromUtf8("关闭"), mCoverOpenBtn, mCoverCloseBtn), 2);
    cardRow->addWidget(makeCard(QString::fromUtf8("飞机充电"), QString::fromUtf8("开启"),
                                QString::fromUtf8("关闭"), mChargeOpenBtn, mChargeCloseBtn), 2);

    // ——— 飞行控制卡片 ———
    auto* flightGroup = new QGroupBox(QString::fromUtf8("飞行控制"), this);
    auto* flightLayout = new QVBoxLayout(flightGroup);
    flightLayout->addStretch();
    auto* flightBtnRow = new QHBoxLayout;
    mTakeoffBtn = new QPushButton(QString::fromUtf8("一键起飞"), flightGroup);
    mReturnBtn  = new QPushButton(QString::fromUtf8("一键返航"), flightGroup);
    mTakeoffBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background: #1557b0; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    mReturnBtn->setStyleSheet(
        "QPushButton { background: #ea4335; color: #fff; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background: #c5221f; }"
        "QPushButton:disabled { background: #dadce0; color: #80868b; }");
    flightBtnRow->addWidget(mTakeoffBtn);
    flightBtnRow->addWidget(mReturnBtn);
    flightLayout->addLayout(flightBtnRow);
    cardRow->addWidget(flightGroup, 2);

    mainLayout->addLayout(cardRow);

    // ——— 下发记录 ———
    auto* historyGroup = new QGroupBox(QString::fromUtf8("下发记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    mHistoryEdit = new QPlainTextEdit(historyGroup);
    mHistoryEdit->setReadOnly(true);
    mHistoryEdit->setPlaceholderText(QString::fromUtf8("暂无下发记录"));
    historyLayout->addWidget(mHistoryEdit);
    mainLayout->addWidget(historyGroup, 1);

    // 按钮样式统一
    const QList<QPushButton*> buttons = {
        mDebugOpenBtn, mDebugCloseBtn, mDroneOpenBtn, mDroneCloseBtn,
        mCoverOpenBtn, mCoverCloseBtn, mChargeOpenBtn, mChargeCloseBtn
    };
    for (auto* button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(30);
    }
    mTakeoffBtn->setCursor(Qt::PointingHandCursor);
    mTakeoffBtn->setMinimumHeight(30);
    mReturnBtn->setCursor(Qt::PointingHandCursor);
    mReturnBtn->setMinimumHeight(30);

    // 信号连接 — 调试模式 + 设备控制
    connect(mDebugOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DebugModeOpen); });
    connect(mDebugCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DebugModeClose); });
    connect(mDroneOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DroneOpen); });
    connect(mDroneCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::DroneClose); });
    connect(mCoverOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::CoverOpen); });
    connect(mCoverCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::CoverClose); });
    connect(mChargeOpenBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeOpen); });
    connect(mChargeCloseBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::ChargeClose); });

    // 飞行控制
    connect(mTakeoffBtn, &QPushButton::clicked, this, &DockControlPanel::onTakeoffClicked);
    connect(mReturnBtn, &QPushButton::clicked, this,
            [this]() { requestCommand(DockCommandType::Return); });
}
```

- [ ] **Step 3: 新增 onTakeoffClicked() 方法**

```cpp
void DockControlPanel::onTakeoffClicked() {
    if (mGatewaySn.isEmpty() || mPending) return;

    TakeoffDialog dlg(mDockLat, mDockLon, this);
    if (dlg.exec() != QDialog::Accepted) return;

    TakeoffParams p = dlg.params();
    QJsonObject data;
    data[QStringLiteral("latitude")]  = p.latitude;
    data[QStringLiteral("longitude")] = p.longitude;
    data[QStringLiteral("height")]    = p.height;
    if (p.speed > 0.0)
        data[QStringLiteral("speed")] = p.speed;

    requestCommand(DockCommandType::Takeoff, data);
}
```

- [ ] **Step 4: 更新 setDevice() — 彩色在线状态**

```cpp
void DockControlPanel::setDevice(const QString& displayName, const QString& gatewaySn, bool online) {
    if (mGatewaySn != gatewaySn || mOnline != online) {
        mDebugModeState = DebugModeState::Unknown;
        mPending = false;
    }
    mDisplayName = displayName;
    mGatewaySn = gatewaySn;
    mOnline = online;

    if (mGatewaySn.isEmpty()) {
        clearDevice();
        return;
    }

    // 在线状态：使用 emoji + 彩色文字
    if (mOnline) {
        mOnlineLabel->setText(QString::fromUtf8("\xf0\x9f\x9f\xa2 在线"));
        mOnlineLabel->setStyleSheet("color: #1e8e3e; font-weight: bold; padding: 0 8px;");
    } else {
        mOnlineLabel->setText(QString::fromUtf8("\xf0\x9f\x94\xb4 离线"));
        mOnlineLabel->setStyleSheet("color: #d93025; font-weight: bold; padding: 0 8px;");
    }

    if (!mOnline)
        setStatus(QString::fromUtf8("机场离线，无法发送控制指令"), true);
    else if (mDebugModeState == DebugModeState::Enabled)
        setStatus(QString::fromUtf8("远程调试已开启，可执行常用控制"));
    else
        setStatus(QString::fromUtf8("请先进入远程调试模式"));
    updateButtonStates();
}
```

- [ ] **Step 5: 新增 setAvailableDocks() 方法**

```cpp
void DockControlPanel::setAvailableDocks(const QVector<DeviceInfo>& docks,
                                         const QString& currentSn,
                                         double dockLat, double dockLon) {
    mAvailableDocks = docks;
    mDockLat = dockLat;
    mDockLon = dockLon;

    mUpdatingCombo = true;
    mDockCombo->clear();

    int selectIdx = -1;
    for (int i = 0; i < docks.size(); ++i) {
        const auto& d = docks[i];
        mDockCombo->addItem(
            QString::fromUtf8("\xf0\x9f\x9f\xa2 %1 - %2").arg(d.name, d.sn), d.sn);
        if (d.sn == currentSn)
            selectIdx = i;
    }

    if (selectIdx >= 0)
        mDockCombo->setCurrentIndex(selectIdx);
    else if (!docks.isEmpty())
        mDockCombo->setCurrentIndex(0);

    mUpdatingCombo = false;

    if (!docks.isEmpty() && selectIdx < 0) {
        // 当前选中项不在列表中，选中第一个
        const auto& d = docks[0];
        setDevice(d.name, d.sn, true);
    }
}
```

- [ ] **Step 6: 更新 updateButtonStates() — 飞行控制按钮独立逻辑**

```cpp
void DockControlPanel::updateButtonStates() {
    const bool available = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mDebugOpenBtn->setEnabled(available && mDebugModeState != DebugModeState::Enabled);
    mDebugCloseBtn->setEnabled(available && mDebugModeState != DebugModeState::Disabled);

    const bool debugControlsEnabled = available && mDebugModeState == DebugModeState::Enabled;
    mDroneOpenBtn->setEnabled(debugControlsEnabled);
    mDroneCloseBtn->setEnabled(debugControlsEnabled);
    mCoverOpenBtn->setEnabled(debugControlsEnabled);
    mCoverCloseBtn->setEnabled(debugControlsEnabled);
    mChargeOpenBtn->setEnabled(debugControlsEnabled);
    mChargeCloseBtn->setEnabled(debugControlsEnabled);

    // 飞行控制：不依赖调试模式，机场在线 + MQTT 连接即可
    const bool flightAvailable = mConnected && mOnline && !mGatewaySn.isEmpty() && !mPending;
    mTakeoffBtn->setEnabled(flightAvailable);
    mReturnBtn->setEnabled(flightAvailable);

    // 调试模式状态显示（与之前相同）
    switch (mDebugModeState) {
    case DebugModeState::Enabled:
        mDebugModeLabel->setText(QString::fromUtf8("状态：已开启"));
        mDebugModeLabel->setStyleSheet(QStringLiteral("color: #1e8e3e; font-weight: bold;"));
        break;
    case DebugModeState::Disabled:
        mDebugModeLabel->setText(QString::fromUtf8("状态：已退出"));
        mDebugModeLabel->setStyleSheet(QStringLiteral("color: #5f6368;"));
        break;
    case DebugModeState::Unknown:
        mDebugModeLabel->setText(QString::fromUtf8("状态：未知（以本客户端操作结果为准）"));
        mDebugModeLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
        break;
    }
}
```

- [ ] **Step 7: 更新 clearDevice()**

```cpp
void DockControlPanel::clearDevice() {
    mDisplayName.clear();
    mGatewaySn.clear();
    mOnline = false;
    mPending = false;
    mDebugModeState = DebugModeState::Unknown;
    mOnlineLabel->setText(QString());
    mOnlineLabel->setStyleSheet(QString());
    setStatus(QString::fromUtf8("未选择可控制的机场设备"));
    updateButtonStates();
}
```

- [ ] **Step 8: 编译验证**

```bash
cmake --build build_mingw
```

---

### Task 5: MainWindow 联动 — 传递机场列表和位置数据

**Files:**
- Modify: `src/ui/MainWindow.cpp:687-755`

- [ ] **Step 1: 新增辅助方法 refreshDockControlList()**

在 `MainWindow.cpp` 的 `connectSignals()` 末尾或 `onDeviceSelected()` 前添加：

```cpp
void MainWindow::refreshDockControlList(const QString& currentSn) {
    const auto& allDevs = mDevMgr->allDevices();
    QVector<DeviceInfo> onlineDocks;
    double dockLat = 0.0;
    double dockLon = 0.0;

    for (auto* d : allDevs) {
        if (d->type == DeviceType::Dock && d->online)
            onlineDocks.append(*d);
    }

    // 获取当前选中机场的位置数据
    if (!currentSn.isEmpty()) {
        DeviceInfo* dev = mDevMgr->device(currentSn);
        QString dockSn;
        if (dev && dev->type == DeviceType::Dock)
            dockSn = currentSn;
        else if (dev && !dev->parentSn.isEmpty())
            dockSn = dev->parentSn;

        if (!dockSn.isEmpty()) {
            const DockOsd* osd = mDevMgr->latestDockOsd(dockSn);
            if (osd && osd->valid) {
                dockLat = osd->latitude;
                dockLon = osd->longitude;
            }
        }
    }

    mDockControlPanel->setAvailableDocks(onlineDocks, currentSn, dockLat, dockLon);
}
```

在 `MainWindow.h` 中声明：

```cpp
void refreshDockControlList(const QString& currentSn);
```

- [ ] **Step 2: 更新 onDeviceSelected() 中的联动**

替换 `MainWindow.cpp` 中 `onDeviceSelected()` 的机场控制面板部分（约 line 744-754）：

```cpp
    // 机场控制面板联动
    QString dockSn;
    if (dev->type == DeviceType::Dock)
        dockSn = sn;
    else if (!dev->parentSn.isEmpty())
        dockSn = dev->parentSn;

    if (!dockSn.isEmpty()) {
        DeviceInfo* dockDev = mDevMgr->device(dockSn);
        if (dockDev)
            mDockControlPanel->setDevice(dockDev->name, dockDev->sn, dockDev->online);
        else
            mDockControlPanel->clearDevice();
    } else {
        mDockControlPanel->clearDevice();
    }

    // 刷新机场列表（与 setDevice 分开，确保列表数据最新）
    refreshDockControlList(dockSn);
```

- [ ] **Step 3: 设备在线状态变化时刷新机场列表**

在 `connectSignals()` 中新增信号连接（约 line 693 之后）：

```cpp
    // 设备在线状态变化 → 刷新机场控制面板列表
    connect(mDevMgr, &DeviceManager::deviceOnlineChanged,
            this, [this](const QString& sn, bool online) {
        Q_UNUSED(sn); Q_UNUSED(online);
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshDockControlList(currentSn);
    });

    // 设备新增/删除 → 刷新机场列表
    connect(mDevMgr, &DeviceManager::deviceAdded,
            this, [this](const QString&) {
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshDockControlList(currentSn);
    });
    connect(mDevMgr, &DeviceManager::deviceRemoved,
            this, [this](const QString&) {
        QString currentSn = mDeviceTree->selectedDeviceSn();
        refreshDockControlList(currentSn);
    });
```

- [ ] **Step 4: 编译验证并运行测试**

```bash
cmake --build build_mingw
```

---

### Task 6: 集成验证

- [ ] **Step 1: 完整编译**

```bash
cmake --build build_mingw
```

预期：编译成功，无警告。

- [ ] **Step 2: 提交**

```bash
git add -A
git commit -m "feat: 机场控制面板增强 — 在线状态彩色显示、可搜索机场切换、一键起飞/返航

- 在线状态改为绿色/红色显示，与设备树风格一致
- 新增机场选择器（可搜索QComboBox），支持切换不同在线机场
- 新增飞行控制卡片：一键起飞（弹窗设参数）、一键返航（确认即可）
- 新增TakeoffDialog：起飞目标位置、高度、速度、环境确认
- DockCommand扩展Takeoff/Return类型，build()支持data参数
- 飞行控制不依赖远程调试模式"
```
