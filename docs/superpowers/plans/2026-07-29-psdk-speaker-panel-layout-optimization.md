# PSDK 喊话器面板布局优化 — 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 优化 `PsdkSpeakerPanel::setupUi()` 布局——删除冗余的「PSDK 设备配置」GroupBox，将负载索引合并到「喊话器控制」GroupBox，修复文件名行对齐，缩小文本输入区高度。

**Architecture:** 单文件改动，仅 `src/ui/PsdkSpeakerPanel.cpp` 的 `setupUi()` 方法。纯布局代码重组，不变更任何成员变量、信号/槽、业务逻辑。

**Tech Stack:** C++17, Qt 6 (Widgets)

## Global Constraints

- 仅修改 `setupUi()` 方法内的布局代码
- `mPsdkIndexCombo` 成员变量保留，其创建代码从 psdkGroup 迁移到 ctrlGroup
- 所有信号/槽连接、`requestCommand()`、`setDevice()` 等业务逻辑不变
- 编译平台：MinGW (Windows)

---

### Task 1: PSDK 喊话器面板布局三步优化

**Files:**
- Modify: `src/ui/PsdkSpeakerPanel.cpp:95-263`

**Interfaces:**
- Consumes: 现有 `PsdkSpeakerPanel` 类成员变量（`mPsdkIndexCombo`、`mTtsTextEdit` 等）
- Produces: 相同的成员变量和 public 接口，行为不变

---

- [ ] **Step 1: 删除「PSDK 设备配置」GroupBox（第 95-113 行）**

删除以下代码块：
```cpp
    // --- PSDK 设备配置 ---
    auto* psdkGroup = new QGroupBox(QString::fromUtf8("PSDK 设备配置"), scrollContent);
    auto* psdkLayout = new QHBoxLayout(psdkGroup);
    psdkLayout->addWidget(makeLabel(QString::fromUtf8("负载索引:"), psdkGroup));

    mPsdkIndexCombo = new QComboBox(psdkGroup);
    mPsdkIndexCombo->addItems({"0", "1", "2", "3"});
    mPsdkIndexCombo->setCurrentIndex(2);
    mPsdkIndexCombo->setFixedWidth(80);
    mPsdkIndexCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 8px;"
        "font-size: 13px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }");
    psdkLayout->addWidget(mPsdkIndexCombo);
    psdkLayout->addSpacing(12);
    psdkLayout->addWidget(new QLabel(
        QString::fromUtf8("喊话器在飞机 E-Port 上的物理挂载位置，通常为 2"), psdkGroup));
    psdkLayout->addStretch();
    contentLayout->addWidget(psdkGroup);
```

---

- [ ] **Step 2: 重构「喊话器控制」GroupBox，顶部新增负载索引行**

将第 115-233 行从：

```cpp
    // --- 喊话器控制卡片（2×2 网格） ---
    auto* ctrlGroup = new QGroupBox(QString::fromUtf8("喊话器控制"), scrollContent);
    auto* ctrlGrid = new QGridLayout(ctrlGroup);
    ctrlGrid->setSpacing(12);

    // (0,0) 音量控制
    auto* volCard = new QWidget(ctrlGroup);
    // ... [2x2 grid content unchanged] ...

    ctrlGrid->setColumnStretch(0, 1);
    ctrlGrid->setColumnStretch(1, 1);
    contentLayout->addWidget(ctrlGroup);
```

改为：

```cpp
    // --- 喊话器控制 ---
    auto* ctrlGroup = new QGroupBox(QString::fromUtf8("喊话器控制"), scrollContent);
    auto* ctrlOuterLayout = new QVBoxLayout(ctrlGroup);
    ctrlOuterLayout->setSpacing(8);

    // 负载索引行
    auto* psdkIndexRow = new QHBoxLayout;
    auto* psdkIndexLabel = new QLabel(QString::fromUtf8("负载索引:"), ctrlGroup);
    psdkIndexLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 13px;");
    psdkIndexLabel->setFixedWidth(72);
    psdkIndexLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    psdkIndexRow->addWidget(psdkIndexLabel);

    mPsdkIndexCombo = new QComboBox(ctrlGroup);
    mPsdkIndexCombo->addItems({"0", "1", "2", "3"});
    mPsdkIndexCombo->setCurrentIndex(2);
    mPsdkIndexCombo->setFixedWidth(80);
    mPsdkIndexCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dadce0; border-radius: 4px; padding: 4px 8px;"
        "font-size: 13px; background: #fff; }"
        "QComboBox:hover { border-color: #1a73e8; }");
    psdkIndexRow->addWidget(mPsdkIndexCombo);
    psdkIndexRow->addSpacing(12);
    auto* psdkHint = new QLabel(
        QString::fromUtf8("喊话器在飞机 E-Port 上的物理挂载位置，通常为 2"), ctrlGroup);
    psdkHint->setStyleSheet("color: #5f6368; font-size: 12px;");
    psdkIndexRow->addWidget(psdkHint);
    psdkIndexRow->addStretch();
    ctrlOuterLayout->addLayout(psdkIndexRow);

    // 分隔线
    auto* ctrlSep = new QFrame(ctrlGroup);
    ctrlSep->setFrameShape(QFrame::HLine);
    ctrlSep->setFrameShadow(QFrame::Sunken);
    ctrlOuterLayout->addWidget(ctrlSep);

    // 2×2 控制卡片网格
    auto* ctrlGrid = new QGridLayout;
    ctrlGrid->setSpacing(12);

    // (0,0) 音量控制
    auto* volCard = new QWidget(ctrlGroup);
    // ... [2x2 grid content — keep EXACTLY as-is, no changes] ...

    ctrlGrid->setColumnStretch(0, 1);
    ctrlGrid->setColumnStretch(1, 1);
    ctrlOuterLayout->addLayout(ctrlGrid);
    contentLayout->addWidget(ctrlGroup);
```

> **注意：** 2×2 网格内的所有卡片代码（volCard、modeCard、actionCard、progCard）及它们的信号连接 **完全不变**，只是外层包裹结构从 `ctrlGroup → QGridLayout` 变为 `ctrlGroup → QVBoxLayout → {psdkIndexRow, separator, QGridLayout}`。

---

- [ ] **Step 3: TTS 文件名行添加 `addStretch()` 实现左对齐**

第 248 行 `ttsNameRow->addWidget(mTtsNameEdit);` 之后新增一行：

```cpp
    ttsNameRow->addStretch();
```

完整上下文（第 240-248 行变为）：

```cpp
    auto* ttsNameRow = new QHBoxLayout;
    ttsNameRow->addWidget(makeLabel(QString::fromUtf8("文件名:"), ttsGroup));
    mTtsNameEdit = new QLineEdit(ttsGroup);
    mTtsNameEdit->setPlaceholderText(QString::fromUtf8("用于机场侧标识，如：安全提醒（留空则自动生成）"));
    mTtsNameEdit->setMaximumWidth(240);
    mTtsNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 10px;"
        "font-size: 13px; } QLineEdit:focus { border-color: #1a73e8; }");
    ttsNameRow->addWidget(mTtsNameEdit);
    ttsNameRow->addStretch();
```

---

- [ ] **Step 4: 音频文件名行添加 `addStretch()` 实现左对齐**

第 334 行 `audioNameRow->addWidget(mAudioNameEdit);` 之后新增一行：

```cpp
    audioNameRow->addStretch();
```

完整上下文（第 326-334 行变为）：

```cpp
    auto* audioNameRow = new QHBoxLayout;
    audioNameRow->addWidget(makeLabel(QString::fromUtf8("文件名:"), audioGroup));
    mAudioNameEdit = new QLineEdit(audioGroup);
    mAudioNameEdit->setPlaceholderText(QString::fromUtf8("如：alert_20230720（留空则自动生成）"));
    mAudioNameEdit->setMaximumWidth(240);
    mAudioNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dadce0; border-radius: 4px; padding: 6px 10px;"
        "font-size: 13px; } QLineEdit:focus { border-color: #1a73e8; }");
    audioNameRow->addWidget(mAudioNameEdit);
    audioNameRow->addStretch();
```

---

- [ ] **Step 5: 文本输入区域缩小高度**

两处修改：

**5a.** 第 262 行，`setMinimumHeight(60)` → `setMinimumHeight(40)`：

```cpp
    mTtsTextEdit->setMinimumHeight(40);
```

**5b.** 第 263 行，垂直策略 `QSizePolicy::Expanding` → `QSizePolicy::Preferred`：

```cpp
    mTtsTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
```

完整上下文（第 260-263 行变为）：

```cpp
    mTtsTextEdit = new QPlainTextEdit(ttsGroup);
    mTtsTextEdit->setPlaceholderText(QString::fromUtf8("输入 TTS 喊话文本内容（最大 1000 字符）..."));
    mTtsTextEdit->setMinimumHeight(40);
    mTtsTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
```

---

- [ ] **Step 6: 编译验证**

```bash
cmake --build build_mingw
```

预期：编译成功，无错误无警告。

---

- [ ] **Step 7: 审查提交**

```bash
git add src/ui/PsdkSpeakerPanel.cpp
git diff --cached -- src/ui/PsdkSpeakerPanel.cpp
```

确认 diff 内容与上述改动一致。

---

- [ ] **Step 8: 提交**

```bash
git commit -m "refactor: PSDK喊话器面板布局优化

- 删除冗余的「PSDK设备配置」GroupBox，负载索引合并到「喊话器控制」
- TTS/音频文件名行添加左对齐
- 文本输入区域高度缩小至原来一半

Co-Authored-By: Claude <noreply@anthropic.com>"
```
