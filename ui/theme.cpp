#include "theme.h"
#include <QWidget>

void Theme::applyTheme(QWidget *target)
{
    if (target) {
        target->setStyleSheet(styleSheet());
    }
}

QString Theme::styleSheet()
{
    return QStringLiteral(R"(
        /* ===================================================================
           CODAXIS Centralized Design System
           Warm Ivory (#faf8f5), Muted Olive-Green (#526649), Dark Charcoal (#242724)
           =================================================================== */

        /* --- Base Surface & Application Window --- */
        QMainWindow {
            background-color: #faf8f5;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        }

        QWidget#centralwidget {
            background-color: #faf8f5;
        }

        /* --- Left Navigation Sidebar --- */
        QWidget#sidebarWidget {
            background-color: #f2efe9;
            border-right: 1px solid #dedad0;
        }

        QLabel#brandLogo {
            color: #242724;
            font-size: 15px;
            font-weight: 700;
            letter-spacing: 0.5px;
        }

        QLabel#sectionHeader {
            color: #7a7d76;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 1px;
            text-transform: uppercase;
        }

        /* Navigation Buttons */
        QPushButton#navButton {
            color: #565954;
            background-color: transparent;
            border: none;
            border-left: 3px solid transparent; /* prevents layout shift on selection */
            border-radius: 4px;
            padding: 7px 10px;
            font-size: 13px;
            font-weight: 400;
            text-align: left;
        }

        QPushButton#navButton:hover {
            color: #242724;
            background-color: #e8e4db;
            border-left: 3px solid #dedad0;
        }

        QPushButton#navButton:checked {
            color: #243320;
            background-color: #e3e8df;
            font-weight: 600;
            border-left: 3px solid #526649;
            border-top-left-radius: 1px;
            border-bottom-left-radius: 1px;
        }

        /* Sidebar Status / Indicator */
        QLabel#sidebarStatusLabel {
            color: #5d6d57;
            font-size: 12px;
            padding: 6px 0px;
        }

        /* --- Main Content Area & Overview --- */
        QWidget#mainAreaWidget {
            background-color: #faf8f5;
        }

        /* Elevated Hero Surface */
        QFrame#welcomeHeroCard {
            background-color: #fdfcf9;
            border: 1px solid #dedad0;
            border-radius: 6px;
        }

        QLabel#welcomeBadge {
            color: #3f5237;
            background-color: #edf2eb;
            border: 1px solid #d4ded0;
            border-radius: 3px;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.8px;
            padding: 3px 8px;
        }

        QLabel#welcomeHeading {
            color: #202320;
            font-size: 24px;
            font-weight: 700;
            letter-spacing: -0.2px;
        }

        QLabel#welcomeDescription {
            color: #565954;
            font-size: 13px;
            line-height: 1.45;
        }

        /* Primary Action Button (Muted Olive-Green) */
        QPushButton#btnOpenRepository {
            color: #ffffff;
            background-color: #526649;
            border: 1px solid #46573e;
            border-radius: 4px;
            padding: 9px 20px;
            font-size: 13px;
            font-weight: 600;
        }

        QPushButton#btnOpenRepository:hover {
            background-color: #45563d;
            border-color: #3a4933;
        }

        QPushButton#btnOpenRepository:pressed {
            background-color: #384631;
            border-color: #2c3727;
        }

        QLabel#welcomeHint {
            color: #7a7d76;
            font-size: 12px;
        }

        QFrame#welcomeDivider {
            color: #e8e4db;
            background-color: #e8e4db;
            border: none;
            height: 1px;
            max-height: 1px;
        }

        /* Overview Pillar Cards */
        QFrame#overviewPillarCard {
            background-color: #f4f7f2;
            border: 1px solid #d8e0d3;
            border-radius: 4px;
        }

        QLabel#overviewPillarTitle {
            color: #485c41;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.8px;
        }

        QLabel#overviewPillarDetail {
            color: #565954;
            font-size: 11px;
            line-height: 1.35;
        }

        /* --- Page Badges & Headers --- */
        QLabel#pageBadge {
            color: #3f5237;
            background-color: #edf2eb;
            border: 1px solid #d4ded0;
            border-radius: 3px;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.8px;
            padding: 2px 7px;
        }

        QLabel#pageHeader {
            color: #202320;
            font-size: 19px;
            font-weight: 700;
            letter-spacing: -0.2px;
        }

        QLabel#pageDescription {
            color: #565954;
            font-size: 13px;
            line-height: 1.4;
        }

        /* --- Repository Health Metric Cards --- */
        QFrame#healthMetricCard {
            background-color: #f6f8f4;
            border: 1px solid #d5ded0;
            border-top: 2px solid #526649;
            border-radius: 5px;
        }

        QLabel#healthMetricTitle {
            color: #4a5c43;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.8px;
        }

        QLabel#healthMetricValue {
            color: #1c2719;
            font-size: 20px;
            font-weight: 700;
        }

        /* --- Data Tables (Commits, Contributors, Hotspots, Health) --- */
        QTableWidget#commitsTable {
            background-color: #faf8f5;
            alternate-background-color: #f2f6ef;
            color: #242724;
            border: 1px solid #d8ded3;
            border-radius: 5px;
            font-size: 12px;
            gridline-color: #e4ece0;
            selection-background-color: #d8e2d2;
            selection-color: #1c2619;
            outline: none;
        }

        QTableWidget#commitsTable:focus {
            border: 1px solid #9eb394;
        }

        QTableWidget#commitsTable QHeaderView::section {
            background-color: #e8eee4;
            color: #35452e;
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 0.4px;
            padding: 7px 10px;
            border: none;
            border-right: 1px solid #d5ded1;
            border-bottom: 2px solid #b4c2ae;
        }

        QTableWidget#commitsTable::item {
            padding: 6px 10px;
            border: none;
        }

        QTableWidget#commitsTable::item:selected {
            background-color: #d8e2d2;
            color: #1c2619;
        }

        /* --- Scrollbars --- */
        QScrollBar:vertical {
            background: #f2efe9;
            width: 10px;
            border: none;
            margin: 0px;
        }

        QScrollBar::handle:vertical {
            background: #c1cbbe;
            border-radius: 5px;
            min-height: 24px;
            margin: 1px;
        }

        QScrollBar::handle:vertical:hover {
            background: #a6b5a2;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            width: 0px;
        }

        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }

        QScrollBar:horizontal {
            background: #f2efe9;
            height: 10px;
            border: none;
            margin: 0px;
        }

        QScrollBar::handle:horizontal {
            background: #c1cbbe;
            border-radius: 5px;
            min-width: 24px;
            margin: 1px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #a6b5a2;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            height: 0px;
            width: 0px;
        }

        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
        }

        /* --- Progress Indicators --- */
        QProgressBar {
            background-color: #e6ede2;
            border: 1px solid #cbd7c6;
            border-radius: 3px;
            text-align: center;
            font-size: 11px;
            font-weight: 600;
            color: #242724;
            min-height: 14px;
            max-height: 16px;
        }

        QProgressBar::chunk {
            background-color: #4d6344;
            border-radius: 2px;
        }

        /* --- Status Bar --- */
        QStatusBar {
            background-color: #ebefe6;
            border-top: 1px solid #d5ded1;
            color: #495744;
            font-size: 11px;
        }

        QLabel#footerStatusLabel {
            color: #3e4e39;
            font-size: 11px;
            padding-left: 6px;
        }

        QLabel#footerReadyLabel {
            color: #63755e;
            font-size: 11px;
            padding-right: 10px;
        }

        /* --- Tooltips & Dialogs --- */
        QToolTip {
            background-color: #242724;
            color: #faf8f5;
            border: 1px solid #373a34;
            border-radius: 3px;
            padding: 4px 8px;
            font-size: 11px;
        }

        QMessageBox {
            background-color: #faf8f5;
        }

        QMessageBox QLabel {
            color: #242724;
            font-size: 12px;
        }

        QMessageBox QPushButton {
            background-color: #ede8df;
            color: #242724;
            border: 1px solid #dedad0;
            border-radius: 3px;
            padding: 5px 14px;
            font-size: 12px;
            font-weight: 500;
            min-width: 65px;
        }

        QMessageBox QPushButton:hover {
            background-color: #e2ded3;
            border-color: #cdc8bd;
        }

        QMessageBox QPushButton:pressed {
            background-color: #d8d3c7;
        }
    )");
}
