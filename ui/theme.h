#ifndef THEME_H
#define THEME_H

#include <QString>

class QWidget;

/**
 * @brief Centralized visual design system for CODAXIS: Neon Pulse.
 * Built around deep midnight navy, dark-indigo surface panels,
 * vivid hot pink (#FF2D95) and electric magenta (#E000FF) signature accents,
 * and multicolored data visualization hues (cyan, violet, teal, emerald, amber).
 */
class Theme {
public:
    // Core Prism Studio Palette Tokens - Smoky Charcoal, Aubergine, Iridescent Indigo & Plum
    static constexpr const char *SmokyBackground    = "#13111c";
    static constexpr const char *SidebarBackground  = "#171422";
    static constexpr const char *CardSurface        = "#1e192c";
    static constexpr const char *SubSurface          = "#241d34";
    static constexpr const char *BorderSubtle        = "#3d3257";
    static constexpr const char *BorderHighlight     = "#5a4a7e";
    static constexpr const char *PlumSurface         = "#291f3b";
    static constexpr const char *LavenderSurface     = "#261e38";

    // Legacy aliases for compatibility
    static constexpr const char *MidnightBackground = "#13111c";

    // Signature Accents
    static constexpr const char *HotPink            = "#ff2d95";
    static constexpr const char *ElectricMagenta    = "#e000ff";
    static constexpr const char *Violet             = "#8b5cf6";
    static constexpr const char *Cyan               = "#22d3ee";
    static constexpr const char *Teal               = "#14f1d9";
    static constexpr const char *Emerald            = "#34d399";
    static constexpr const char *Amber              = "#ffb547";
    static constexpr const char *Coral              = "#ff6b6b";

    // Text & Typography - High contrast & readable secondary labels
    static constexpr const char *TextPrimary        = "#f8fafc";
    static constexpr const char *TextSecondary      = "#cbd5e1";
    static constexpr const char *TextMuted          = "#948da6";

    // Tables & Controls
    static constexpr const char *TableHeader        = "#201a2e";
    static constexpr const char *RowAlternate       = "#261e38";
    static constexpr const char *SelectionBg        = "#423363";

    /**
     * @brief Applies the centralized Neon Pulse stylesheet to the given widget hierarchy.
     * @param target Pointer to the target QWidget (typically MainWindow).
     */
    static void applyTheme(QWidget *target);

    /**
     * @brief Generates the complete, fine-tuned Qt Style Sheet (QSS) for Neon Pulse.
     */
    static QString styleSheet();
};

#endif // THEME_H
