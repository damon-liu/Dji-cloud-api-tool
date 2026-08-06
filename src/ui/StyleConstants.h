#pragma once

#include <QPushButton>
#include <QString>

// ============================================================================
// 统一按钮样式系统
//
// 所有功能面板（远程调试/飞行控制/PSDK功能）共用以下语义样式，
// 确保按钮在视觉上保持一致，且压缩到最小高度时仍可点击。
//
// 使用方式：
//   #include "src/ui/StyleConstants.h"
//   stylePrimaryButton(myBtn);   // 蓝色实心主操作
//   styleDangerButton(myBtn);    // 红色实心危险操作
//   ...
// ============================================================================

// --- 公共基础属性（所有语义按钮共用） ---
static const int    BUTTON_MIN_HEIGHT   = 34;
static const int    BUTTON_MIN_WIDTH    = 100;
static const int    BUTTON_BORDER_RADIUS = 4;
static const int    BUTTON_FONT_SIZE    = 13;
static const QString BUTTON_PADDING     = QStringLiteral("8px 16px");
static const QString BUTTON_FONT_WEIGHT = QStringLiteral("bold");

// --- 颜色常量 ---
static const QString COLOR_PRIMARY       = QStringLiteral("#1a73e8");  // 蓝
static const QString COLOR_PRIMARY_HOVER = QStringLiteral("#1557b0");
static const QString COLOR_DANGER        = QStringLiteral("#d93025");  // 红
static const QString COLOR_DANGER_HOVER  = QStringLiteral("#b3261e");
static const QString COLOR_WARNING       = QStringLiteral("#f29900");  // 橙
static const QString COLOR_WARNING_HOVER = QStringLiteral("#e37400");
static const QString COLOR_DISABLED_BG   = QStringLiteral("#dadce0");
static const QString COLOR_DISABLED_FG   = QStringLiteral("#80868b");
static const QString COLOR_WHITE         = QStringLiteral("#fff");
static const QString COLOR_TEXT          = QStringLiteral("#333");
static const QString COLOR_BORDER        = QStringLiteral("#dadce0");
static const QString COLOR_BORDER_FOCUS  = QStringLiteral("#1a73e8");
static const QString COLOR_HOVER_BG      = QStringLiteral("#e8f0fe");
static const QString COLOR_DANGER_BG     = QStringLiteral("#fce8e6");

// --- 公共 base stylesheet 片段 ---
#define BASE_DISABLED \
    "QPushButton:disabled { background: " COLOR_DISABLED_BG "; color: " COLOR_DISABLED_FG "; }"

// ============================================================================
// 样式应用函数 — 每个对应一种语义
// ============================================================================

/// 默认按钮：白底灰边，hover 蓝底蓝字
inline void styleDefaultButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setMinimumHeight(BUTTON_MIN_HEIGHT);
    btn->setMinimumWidth(BUTTON_MIN_WIDTH);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  border: 1px solid %1; border-radius: %2px;"
        "  background: %3; color: %4; font-weight: %5;"
        "  padding: %6; font-size: %7px;"
        "}"
        "QPushButton:hover {"
        "  border-color: %8; color: %8; background: %9;"
        "}"
        "QPushButton:disabled {"
        "  border-color: %1; color: %10; background: #f8f9fa;"
        "}")
        .arg(COLOR_BORDER)
        .arg(BUTTON_BORDER_RADIUS)
        .arg(COLOR_WHITE)
        .arg(COLOR_TEXT)
        .arg(BUTTON_FONT_WEIGHT)
        .arg(BUTTON_PADDING)
        .arg(BUTTON_FONT_SIZE)
        .arg(COLOR_BORDER_FOCUS)
        .arg(COLOR_HOVER_BG)
        .arg(COLOR_DISABLED_FG));
}

/// 主操作按钮：蓝色实心白字
inline void stylePrimaryButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setMinimumHeight(BUTTON_MIN_HEIGHT);
    btn->setMinimumWidth(BUTTON_MIN_WIDTH);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: %1; color: %2; font-weight: %3;"
        "  border: none; border-radius: %4px;"
        "  padding: %5; font-size: %6px;"
        "}"
        "QPushButton:hover { background: %7; }"
        "QPushButton:disabled { background: %8; color: %9; }")
        .arg(COLOR_PRIMARY)
        .arg(COLOR_WHITE)
        .arg(BUTTON_FONT_WEIGHT)
        .arg(BUTTON_BORDER_RADIUS)
        .arg(BUTTON_PADDING)
        .arg(BUTTON_FONT_SIZE)
        .arg(COLOR_PRIMARY_HOVER)
        .arg(COLOR_DISABLED_BG)
        .arg(COLOR_DISABLED_FG));
}

/// 危险操作按钮：红色实心白字
inline void styleDangerButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setMinimumHeight(BUTTON_MIN_HEIGHT);
    btn->setMinimumWidth(BUTTON_MIN_WIDTH);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: %1; color: %2; font-weight: %3;"
        "  border: none; border-radius: %4px;"
        "  padding: %5; font-size: %6px;"
        "}"
        "QPushButton:hover { background: %7; }"
        "QPushButton:disabled { background: %8; color: %9; }")
        .arg(COLOR_DANGER)
        .arg(COLOR_WHITE)
        .arg(BUTTON_FONT_WEIGHT)
        .arg(BUTTON_BORDER_RADIUS)
        .arg(BUTTON_PADDING)
        .arg(BUTTON_FONT_SIZE)
        .arg(COLOR_DANGER_HOVER)
        .arg(COLOR_DISABLED_BG)
        .arg(COLOR_DISABLED_FG));
}

/// 危险次要按钮：红色描边白底红字，hover 浅红底
inline void styleDangerOutlineButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setMinimumHeight(BUTTON_MIN_HEIGHT);
    btn->setMinimumWidth(BUTTON_MIN_WIDTH);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  border: 1px solid %1; border-radius: %2px;"
        "  color: %1; font-weight: %3; background: %4;"
        "  padding: %5; font-size: %6px;"
        "}"
        "QPushButton:hover { background: %7; }"
        "QPushButton:disabled {"
        "  border-color: %8; color: %9; background: #f8f9fa;"
        "}")
        .arg(COLOR_DANGER)
        .arg(BUTTON_BORDER_RADIUS)
        .arg(BUTTON_FONT_WEIGHT)
        .arg(COLOR_WHITE)
        .arg(BUTTON_PADDING)
        .arg(BUTTON_FONT_SIZE)
        .arg(COLOR_DANGER_BG)
        .arg(COLOR_DISABLED_BG)
        .arg(COLOR_DISABLED_FG));
}

/// 警告按钮：橙色实心白字
inline void styleWarningButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setMinimumHeight(BUTTON_MIN_HEIGHT);
    btn->setMinimumWidth(BUTTON_MIN_WIDTH);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: %1; color: %2; font-weight: %3;"
        "  border: none; border-radius: %4px;"
        "  padding: %5; font-size: %6px;"
        "}"
        "QPushButton:hover { background: %7; }"
        "QPushButton:disabled { background: %8; color: %9; }")
        .arg(COLOR_WARNING)
        .arg(COLOR_WHITE)
        .arg(BUTTON_FONT_WEIGHT)
        .arg(BUTTON_BORDER_RADIUS)
        .arg(BUTTON_PADDING)
        .arg(BUTTON_FONT_SIZE)
        .arg(COLOR_WARNING_HOVER)
        .arg(COLOR_DISABLED_BG)
        .arg(COLOR_DISABLED_FG));
}

/// 链接按钮：透明底蓝字，带下划线 hover
inline void styleLinkButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFlat(true);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  border: none; background: transparent; color: %1;"
        "  font-size: 12px; padding: 2px 6px; text-decoration: underline;"
        "}"
        "QPushButton:hover { color: %2; background: %3; border-radius: 3px; }")
        .arg(COLOR_PRIMARY)
        .arg(COLOR_PRIMARY_HOVER)
        .arg(COLOR_HOVER_BG));
}

/// 组内按钮等宽：以最长 sizeHint 为基准，统一设为 fixedWidth
inline void makeGroupEqualWidth(const QList<QPushButton*>& buttons) {
    int maxW = 0;
    for (auto* btn : buttons) {
        btn->ensurePolished();
        maxW = std::max(maxW, btn->sizeHint().width());
    }
    for (auto* btn : buttons) {
        btn->setMinimumWidth(maxW + 24);
        btn->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    }
}

// 清理宏，避免污染
#undef BASE_DISABLED
