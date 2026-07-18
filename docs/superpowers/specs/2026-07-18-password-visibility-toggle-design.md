# MQTT 配置对话框密码可见切换按钮 — 设计文档

日期：2026-07-18
状态：已确认

## 需求

在 MQTT 连接配置对话框（`ConfigDialog`）的密码输入框旁添加一个按钮，点击可切换密码明文/密文显示。

- 交互方式：点击切换（click to toggle），再点一次切回
- 按钮风格：与对话框现有 `✎`（重命名）、`✕`（删除）字形按钮一致的固定宽度 Unicode 字形按钮

## 方案选择

| 方案 | 说明 | 结论 |
|------|------|------|
| 1. 行内 Unicode 字形按钮 | 密码行改为 QHBoxLayout：输入框 + 眼睛字形按钮 | ✅ 采用——零图标资源，贴合现有风格 |
| 2. QLineEdit::addAction 嵌入图标 | 图标嵌在输入框内部右侧 | ❌ 需 QIcon，项目无图标资源 |
| 3. 「显示密码」复选框 | 密码框下加一行 QCheckBox | ❌ 多占一行，不够紧凑 |

## 设计

**改动范围**：仅 `src/ui/ConfigDialog.cpp` 构造函数（密码行创建处，约第 62-68 行），不改头文件。

**实现要点**：

1. 密码行由单个 `mPasswordEdit` 改为一个 `QHBoxLayout`：`mPasswordEdit`（拉伸）+ 眼睛按钮（`setFixedWidth(30)`）
2. 按钮文本 `👁`，`setCheckable(true)`，`setToolTip("显示/隐藏密码")`
3. `connect(btn, &QPushButton::toggled, ...)`：选中时 `mPasswordEdit->setEchoMode(QLineEdit::Normal)`，取消时恢复 `QLineEdit::Password`
4. 按钮用构造函数局部变量持有（其他方法不引用），不新增类成员
5. `form->addRow("密码:", passwordRow)` — `QFormLayout` 支持以布局作为字段

**行为**：

- 按下状态 = 明文显示；再点一次恢复密文
- 切换 profile / 重新打开对话框时，按钮回到默认未选中（密文）状态——对话框每次都是新建的，无需额外处理

## 测试

项目无自动化测试基础设施，采用手动验证：

1. `cmake --build build_mingw` 构建通过
2. 打开配置对话框，点击眼睛按钮：明文 ↔ 密文切换正常
3. 切换 profile 后密码回显正常，按钮状态不残留
