# PSDK 喊话器面板布局优化 — 设计文档

**日期:** 2026-07-29  
**分支:** v1.0.4

---

## 目标

优化 `PsdkSpeakerPanel` 布局，减少不必要的视觉层级，提升空间利用率和表单对齐一致性。

## 改动范围

| 文件 | 改动 |
|------|------|
| `src/ui/PsdkSpeakerPanel.cpp` | 仅 `setupUi()` 方法内的布局代码 |
| `src/ui/PsdkSpeakerPanel.h` | 无需改动（成员变量不变） |

不涉及：`PsdkSpeakerDialog`、`MainWindow`、`DockCommand`、信号/槽逻辑、业务逻辑。

---

## 改动项

### 1. 删除「PSDK 设备配置」GroupBox，负载索引合并到「喊话器控制」

**现状问题：** 一个独立的 `QGroupBox("PSDK 设备配置")` 仅包含一个 `QComboBox`（负载索引 0-3）和一行说明文字，浪费垂直空间。

**改动：**
- 删除 `psdkGroup` 整个 GroupBox 代码块（约 20 行）
- 在「喊话器控制」GroupBox 内，将原先的 `QGridLayout` 包裹在一个外层 `QVBoxLayout` 中
- 顶部新增一行 `QHBoxLayout`：
  - "负载索引:" 标签 + `mPsdkIndexCombo` + 简短说明文字 + `addStretch()`
- 说明文字使用 12px 次要颜色（`#5f6368`），不与控件争夺视觉权重
- `mPsdkIndexCombo` 成员变量保留，所有业务代码中 `data["psdk_index"]` 引用完全不变

**效果：**

```
优化前:                              优化后:
┌─ PSDK 设备配置 ─────┐             ┌─ 喊话器控制 ──────────────────────┐
│ 负载索引:[2▼] 说明... │             │ 负载索引:[2▼] 说明...              │
└──────────────────────┘             │ ──────────────────────────────── │
┌─ 喊话器控制 ─────────┐             │  音量控制    │  播放模式          │
│  音量控制 │ 播放模式  │             │  播放控制    │  播放进度          │
│  播放控制 │ 播放进度  │             └──────────────────────────────────┘
└──────────────────────┘
```

### 2. 文件名行左对齐

**现状问题：** TTS 和音频喊话的「文件名」行中，`QLineEdit` 之后没有明确的对齐处理，与同区域其他行（如 URL 行使用 stretch 填充）显得不一致。

**改动：**
- TTS：`ttsNameRow->addWidget(mTtsNameEdit);` 之后添加 `ttsNameRow->addStretch();`
- 音频：`audioNameRow->addWidget(mAudioNameEdit);` 之后添加 `audioNameRow->addStretch();`

效果：标签 + 输入框组紧贴左侧，与表单整体对齐风格一致。

### 3. 文本输入区域高度缩小

**现状问题：** `mTtsTextEdit` 使用 `QSizePolicy::Expanding` 垂直策略 + `setMinimumHeight(60)`，在 QScrollArea 内会贪婪地占用可用空间，视觉上过高。

**改动：**
- `setMinimumHeight(60)` → `setMinimumHeight(40)`
- 垂直策略 `QSizePolicy::Expanding` → `QSizePolicy::Preferred`

效果：文本区域不再贪婪扩展，高度约为原来的一半，滚动区更紧凑。

---

## 不变的部分

- `mPsdkIndexCombo` 成员变量及所有引用（`data["psdk_index"] = mPsdkIndexCombo->currentText().toInt()`）
- 所有信号/槽连接逻辑
- `requestCommand()`、`setDevice()`、`updateButtonStates()` 等业务方法
- `PsdkSpeakerDialog`（薄壳对话框）
- `MainWindow` 连接代码
- `DockCommand.h` 枚举定义

---

## 实现顺序

1. 删除「PSDK 设备配置」GroupBox 代码块
2. 在「喊话器控制」GroupBox 内新增负载索引行
3. TTS 文件名行加 `addStretch()`
4. 音频文件名行加 `addStretch()`
5. 文本输入区改高度策略
6. 编译验证，手动测试布局效果
