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
           CODAXIS Prism Studio: Premium Glass-Inspired Analytics Platform
           Smoky Aubergine (#13111C), Iridescent Indigo Panels,
           Coordinated Accents: Hot Pink (#FF2D95), Cyan (#22D3EE),
           Violet (#8B5CF6), Electric Magenta (#E000FF), Emerald (#34D399), Amber (#FFB547)
           =================================================================== */

        /* --- Base Surface & Application Window --- */
        QMainWindow {
            background-color: #13111c;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            color: #f8fafc;
        }

        QWidget#centralwidget {
            background-color: #13111c;
        }

        /* --- Left Navigation Sidebar (Structural Smoky Aubergine) --- */
        QWidget#sidebarWidget {
            background-color: #171422;
            border-right: 1px solid #2d2442;
        }

        QLabel#brandLogo {
            color: #f8fafc;
            font-size: 16px;
            font-weight: 800;
            letter-spacing: 0.8px;
        }

        QLabel#sectionHeader {
            color: #8c82a5;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 1.2px;
            text-transform: uppercase;
        }

        /* Navigation Buttons */
        QPushButton#navButton {
            color: #c4bcd6;
            background-color: transparent;
            border: none;
            border-left: 3px solid transparent;
            border-radius: 6px;
            padding: 9px 14px;
            font-size: 13px;
            font-weight: 500;
            text-align: left;
        }

        QPushButton#navButton:hover {
            color: #ffffff;
            background-color: rgba(139, 92, 246, 0.16);
            border-left: 3.5px solid #8b5cf6;
        }

        QPushButton#navButton:checked {
            color: #ffffff;
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(139, 92, 246, 0.35), stop:0.85 rgba(224, 0, 255, 0.12), stop:1 rgba(224, 0, 255, 0.02));
            font-weight: 700;
            border-left: 3.5px solid #ff2d95;
            border-top-left-radius: 2px;
            border-bottom-left-radius: 2px;
        }

        /* Sidebar Status / Indicator */
        QLabel#sidebarStatusLabel {
            color: #22d3ee;
            font-size: 12px;
            font-weight: 600;
            padding: 6px 0px;
        }

        /* --- Main Content Area & Overview --- */
        QWidget#mainAreaWidget {
            background-color: #13111c;
        }

        /* Elevated Hero Surface (Rich Glass Panel) */
        QFrame#welcomeHeroCard {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(42, 33, 62, 0.85), stop:0.5 rgba(50, 38, 76, 0.75), stop:1 rgba(28, 22, 42, 0.9));
            border: 1px solid rgba(160, 135, 215, 0.25);
            border-top: 1px solid rgba(225, 200, 255, 0.38);
            border-radius: 16px;
        }

        QLabel#welcomeBadge {
            color: #ff2d95;
            background-color: rgba(255, 45, 149, 0.15);
            border: 1px solid rgba(255, 45, 149, 0.45);
            border-radius: 6px;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.8px;
            padding: 4px 10px;
        }

        QLabel#welcomeHeading {
            color: #f8fafc;
            font-size: 26px;
            font-weight: 800;
            letter-spacing: -0.3px;
        }

        QLabel#welcomeDescription {
            color: #c4bcd6;
            font-size: 13px;
            line-height: 1.6;
        }

        /* Primary Action Button (Hot Pink / Magenta Gradient) */
        QPushButton#btnOpenRepository {
            color: #ffffff;
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff2d95, stop:0.6 #e000ff, stop:1 #8b5cf6);
            border: 1px solid rgba(255, 255, 255, 0.25);
            border-radius: 7px;
            padding: 10px 24px;
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 0.3px;
        }

        QPushButton#btnOpenRepository:hover {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff4fa5, stop:0.6 #ec2bff, stop:1 #9d71f7);
            border-color: rgba(255, 255, 255, 0.4);
        }

        QPushButton#btnOpenRepository:pressed {
            background-color: #d61b78;
            border-color: #b81465;
        }

        QLabel#welcomeHint {
            color: #8c82a5;
            font-size: 12px;
        }

        QFrame#welcomeDivider {
            color: #2d2442;
            background-color: #2d2442;
            border: none;
            height: 1px;
            max-height: 1px;
        }

        /* Overview Pillar Cards (Plum-Slate panels) */
        QFrame#overviewPillarCard {
            background-color: rgba(36, 28, 52, 0.65);
            border: 1px solid rgba(150, 125, 200, 0.18);
            border-radius: 8px;
        }

        QFrame#overviewPillarCard:hover {
            border-color: rgba(224, 0, 255, 0.4);
            background-color: rgba(52, 40, 75, 0.85);
        }

        QLabel#overviewPillarTitle {
            color: #22d3ee;
            font-size: 10px;
            font-weight: 800;
            letter-spacing: 0.8px;
        }

        QLabel#overviewPillarDetail {
            color: #c4bcd6;
            font-size: 11px;
            line-height: 1.4;
        }

        /* --- Page Badges & Headers --- */
        QLabel#pageBadge {
            color: #22d3ee;
            background-color: rgba(34, 211, 238, 0.12);
            border: 1px solid rgba(34, 211, 238, 0.4);
            border-radius: 6px;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.8px;
            padding: 4px 10px;
        }

        QLabel#pageHeader {
            color: #f8fafc;
            font-size: 20px;
            font-weight: 800;
            letter-spacing: -0.2px;
        }

        QLabel#pageDescription {
            color: #c4bcd6;
            font-size: 13px;
            line-height: 1.4;
        }

        /* --- Repository Health Metric Cards --- */
        QFrame#healthMetricCard {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(42, 32, 60, 0.8), stop:1 rgba(26, 20, 38, 0.85));
            border: 1px solid rgba(160, 135, 215, 0.22);
            border-top: 3px solid #ff2d95;
            border-radius: 12px;
        }

        QFrame#healthMetricCard:hover {
            border-color: #8b5cf6;
            border-top: 3px solid #e000ff;
            background-color: rgba(52, 40, 75, 0.9);
        }

        QLabel#healthMetricTitle {
            color: #a59abf;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.9px;
            text-transform: uppercase;
        }

        QLabel#healthMetricValue {
            color: #f8fafc;
            font-size: 24px;
            font-weight: 800;
        }

        /* --- Data Tables (Commits, Contributors, Hotspots, Health) --- */
        QTableWidget#commitsTable {
            background-color: transparent;
            alternate-background-color: rgba(36, 28, 52, 0.45);
            color: #f1f5f9;
            border: 1px solid rgba(150, 125, 200, 0.18);
            border-radius: 10px;
            font-size: 12px;
            gridline-color: transparent;
            selection-background-color: rgba(139, 92, 246, 0.32);
            selection-color: #ffffff;
            outline: none;
        }

        QTableWidget#commitsTable:focus {
            border: 1px solid #8b5cf6;
        }

        QTableWidget#commitsTable QHeaderView::section {
            background-color: rgba(32, 25, 48, 0.9);
            color: #a89ec0;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 0.5px;
            padding: 9px 12px;
            border: none;
            border-right: 1px solid rgba(150, 125, 200, 0.15);
            border-bottom: 2px solid #8b5cf6;
        }

        QTableWidget#commitsTable::item {
            padding: 8px 12px;
            border: none;
        }

        QTableWidget#commitsTable::item:selected {
            background-color: rgba(139, 92, 246, 0.32);
            color: #ffffff;
        }

        /* --- Scrollbars --- */
        QScrollBar:vertical {
            background: #171422;
            width: 8px;
            border: none;
            margin: 0px;
        }

        QScrollBar::handle:vertical {
            background: #3d3257;
            border-radius: 4px;
            min-height: 24px;
            margin: 1px;
        }

        QScrollBar::handle:vertical:hover {
            background: #8b5cf6;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            width: 0px;
        }

        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }

        QScrollBar:horizontal {
            background: #171422;
            height: 8px;
            border: none;
            margin: 0px;
        }

        QScrollBar::handle:horizontal {
            background: #3d3257;
            border-radius: 4px;
            min-width: 24px;
            margin: 1px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #8b5cf6;
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
            background-color: rgba(20, 16, 30, 0.7);
            border: 1px solid rgba(120, 100, 160, 0.2);
            border-radius: 4px;
            text-align: center;
            font-size: 11px;
            font-weight: 600;
            color: #f8fafc;
            min-height: 14px;
            max-height: 16px;
        }

        QProgressBar::chunk {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff2d95, stop:0.5 #e000ff, stop:1 #22d3ee);
            border-radius: 3px;
        }

        /* --- Status Bar --- */
        QStatusBar {
            background-color: #171422;
            border-top: 1px solid #2d2442;
            color: #8c82a5;
            font-size: 11px;
        }

        QLabel#footerStatusLabel {
            color: #22d3ee;
            font-size: 11px;
            padding-left: 6px;
        }

        QLabel#footerReadyLabel {
            color: #8c82a5;
            font-size: 11px;
            padding-right: 10px;
        }

        /* --- Tooltips & Dialogs --- */
        QToolTip {
            background-color: #1e182e;
            color: #f8fafc;
            border: 1px solid rgba(170, 145, 225, 0.3);
            border-radius: 6px;
            padding: 6px 10px;
            font-size: 12px;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        }

        QMessageBox {
            background-color: #1c152a;
        }

        QMessageBox QLabel {
            color: #f8fafc;
            font-size: 12px;
        }

        QMessageBox QPushButton {
            background-color: rgba(42, 32, 60, 0.8);
            color: #f8fafc;
            border: 1px solid rgba(160, 135, 215, 0.3);
            border-radius: 6px;
            padding: 6px 18px;
            font-size: 12px;
            font-weight: 600;
            min-width: 65px;
        }

        QMessageBox QPushButton:hover {
            background-color: #ff2d95;
            border-color: #ff2d95;
            color: #ffffff;
        }

        QMessageBox QPushButton:pressed {
            background-color: #d61b78;
        }
    )");
}
