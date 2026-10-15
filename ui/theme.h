#ifndef THEME_H
#define THEME_H

#include <QString>

class QWidget;

/**
 * @brief Centralized visual design system for CODAXIS.
 * Encapsulates the warm ivory, dark charcoal, and muted olive-green identity,
 * establishing a restrained, tactile, and professional desktop appearance.
 */
class Theme {
public:
    // Core Color Palette Tokens
    static constexpr const char *WarmIvory = "#faf8f5";
    static constexpr const char *SidebarBackground = "#f2efe9";
    static constexpr const char *TextPrimary = "#242724";
    static constexpr const char *TextSecondary = "#565954";
    static constexpr const char *TextMuted = "#7a7d76";
    static constexpr const char *OliveAccent = "#526649";
    static constexpr const char *OliveHover = "#45563d";
    static constexpr const char *OlivePressed = "#384631";
    static constexpr const char *BorderLight = "#dedad0";
    static constexpr const char *BorderSubtle = "#e8e4db";
    static constexpr const char *CardSurface = "#f4f0e8";
    static constexpr const char *TableHeader = "#e9eee4";
    static constexpr const char *RowAlternate = "#f3f6f0";
    static constexpr const char *SelectionBg = "#d8e2d2";
    static constexpr const char *PaleSage = "#edf2eb";
    static constexpr const char *SageSurface = "#f4f6f1";
    static constexpr const char *SageHeader = "#e9eee4";
    static constexpr const char *SageSelection = "#d8e2d2";

    /**
     * @brief Applies the centralized CODAXIS stylesheet to the given widget hierarchy.
     * @param target Pointer to the target QWidget (typically MainWindow).
     */
    static void applyTheme(QWidget *target);

    /**
     * @brief Generates the complete, fine-tuned Qt Style Sheet (QSS).
     * @return Formatted QSS string with warm ivory, olive-green, and charcoal typography.
     */
    static QString styleSheet();
};

#endif // THEME_H
