#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ui/theme.h"
#include "ui/repository_canvas.h"
#include "ui/dashboard_widget.h"
#include "ui/gear_widget.h"
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QProcess>
#include <QStandardPaths>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QFont>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFontMetricsF>

namespace {

class CommitItemDelegate : public QStyledItemDelegate
{
public:
    explicit CommitItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        const QRect rowRect = option.rect;
        const bool isSelected = (option.state & QStyle::State_Selected);
        const bool isHovered = (option.state & QStyle::State_MouseOver);

        // Row background
        if (isSelected) {
            painter->fillRect(rowRect, QColor(139, 92, 246, 50));
            if (index.column() == 0) {
                // Accent indicator bar on the left edge of selected row
                painter->fillRect(QRect(rowRect.left(), rowRect.top() + 6, 3, rowRect.height() - 12), QColor(255, 45, 149));
            }
        } else if (isHovered) {
            painter->fillRect(rowRect, QColor(139, 92, 246, 20));
        } else if (index.row() % 2 == 1) {
            painter->fillRect(rowRect, QColor(22, 18, 32, 130));
        }

        // Clean subtle bottom divider
        painter->setPen(QPen(QColor(255, 255, 255, 8), 1));
        painter->drawLine(rowRect.bottomLeft(), rowRect.bottomRight());

        const QString text = index.data(Qt::DisplayRole).toString();

        // Handle full-span status/empty messages gracefully
        if (text.startsWith(QStringLiteral("No ")) || text.startsWith(QStringLiteral("Failed"))) {
            painter->setPen(QColor(168, 158, 192));
            QFont f = option.font;
            f.setPointSize(11);
            f.setWeight(QFont::Medium);
            painter->setFont(f);
            painter->drawText(rowRect, Qt::AlignCenter, text);
            painter->restore();
            return;
        }

        switch (index.column()) {
        case 0: { // Commit Hash Pill
            const QString shortHash = text.left(7);
            const qreal pillW = 70.0;
            const qreal pillH = 22.0;
            const qreal pillX = rowRect.left() + (rowRect.width() - pillW) * 0.5;
            const qreal pillY = rowRect.top() + (rowRect.height() - pillH) * 0.5;
            const QRectF pillRect(pillX, pillY, pillW, pillH);

            painter->setPen(QPen(QColor(255, 45, 149, 70), 1));
            painter->setBrush(QColor(255, 45, 149, 22));
            painter->drawRoundedRect(pillRect, 5.0, 5.0);

            QFont monoFont(QStringLiteral("Consolas"));
            monoFont.setPointSize(9);
            monoFont.setWeight(QFont::Bold);
            painter->setFont(monoFont);
            painter->setPen(QColor(255, 105, 175));
            painter->drawText(pillRect, Qt::AlignCenter, shortHash);
            break;
        }
        case 1: { // Message / Subject
            QRectF textRect = rowRect.adjusted(16, 0, -16, 0);

            int colonIdx = text.indexOf(QLatin1Char(':'));
            bool isMerge = text.startsWith(QStringLiteral("Merge"), Qt::CaseInsensitive);

            QFont msgFont = option.font;
            msgFont.setPointSize(10);
            painter->setFont(msgFont);

            if (isMerge) {
                const QString tag = QStringLiteral("MERGE");
                QFont tagFont = msgFont;
                tagFont.setPointSizeF(7.5);
                tagFont.setWeight(QFont::Bold);
                QFontMetricsF tagFm(tagFont);
                qreal tagW = tagFm.horizontalAdvance(tag) + 12.0;
                qreal tagH = 19.0;
                qreal tagY = rowRect.top() + (rowRect.height() - tagH) * 0.5;
                QRectF tagRect(textRect.left(), tagY, tagW, tagH);

                painter->setPen(QPen(QColor(244, 63, 94, 90), 1));
                painter->setBrush(QColor(244, 63, 94, 30));
                painter->drawRoundedRect(tagRect, 4.0, 4.0);
                painter->setFont(tagFont);
                painter->setPen(QColor(251, 113, 133));
                painter->drawText(tagRect, Qt::AlignCenter, tag);

                QRectF restRect = textRect.adjusted(tagW + 8.0, 0, 0, 0);
                painter->setFont(msgFont);
                painter->setPen(QColor(241, 245, 249));
                QFontMetricsF msgFm(msgFont);
                QString elided = msgFm.elidedText(text, Qt::ElideRight, restRect.width());
                painter->drawText(restRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
            } else if (colonIdx > 0 && colonIdx <= 12) {
                QString prefix = text.left(colonIdx).toLower().trimmed();
                QString rest = text.mid(colonIdx + 1).trimmed();

                QColor badgeBg, badgeBorder, badgeFg;
                if (prefix == QStringLiteral("feat")) {
                    badgeBg = QColor(168, 85, 247, 30);
                    badgeBorder = QColor(168, 85, 247, 85);
                    badgeFg = QColor(216, 180, 254);
                } else if (prefix == QStringLiteral("fix")) {
                    badgeBg = QColor(34, 211, 238, 30);
                    badgeBorder = QColor(34, 211, 238, 85);
                    badgeFg = QColor(103, 232, 249);
                } else if (prefix == QStringLiteral("refactor")) {
                    badgeBg = QColor(129, 140, 248, 30);
                    badgeBorder = QColor(129, 140, 248, 85);
                    badgeFg = QColor(199, 210, 254);
                } else if (prefix == QStringLiteral("style")) {
                    badgeBg = QColor(244, 114, 182, 30);
                    badgeBorder = QColor(244, 114, 182, 85);
                    badgeFg = QColor(249, 168, 212);
                } else if (prefix == QStringLiteral("docs")) {
                    badgeBg = QColor(52, 211, 153, 30);
                    badgeBorder = QColor(52, 211, 153, 85);
                    badgeFg = QColor(110, 231, 183);
                } else {
                    badgeBg = QColor(148, 163, 184, 25);
                    badgeBorder = QColor(148, 163, 184, 60);
                    badgeFg = QColor(203, 213, 225);
                }

                QFont tagFont = msgFont;
                tagFont.setPointSizeF(7.5);
                tagFont.setWeight(QFont::Bold);
                QFontMetricsF tagFm(tagFont);
                QString tagText = prefix.toUpper();
                qreal tagW = tagFm.horizontalAdvance(tagText) + 12.0;
                qreal tagH = 19.0;
                qreal tagY = rowRect.top() + (rowRect.height() - tagH) * 0.5;
                QRectF tagRect(textRect.left(), tagY, tagW, tagH);

                painter->setPen(QPen(badgeBorder, 1));
                painter->setBrush(badgeBg);
                painter->drawRoundedRect(tagRect, 4.0, 4.0);
                painter->setFont(tagFont);
                painter->setPen(badgeFg);
                painter->drawText(tagRect, Qt::AlignCenter, tagText);

                QRectF restRect = textRect.adjusted(tagW + 8.0, 0, 0, 0);
                painter->setFont(msgFont);
                painter->setPen(QColor(241, 245, 249));
                QFontMetricsF msgFm(msgFont);
                QString elided = msgFm.elidedText(rest, Qt::ElideRight, restRect.width());
                painter->drawText(restRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
            } else {
                painter->setPen(QColor(241, 245, 249));
                QFontMetricsF msgFm(msgFont);
                QString elided = msgFm.elidedText(text, Qt::ElideRight, textRect.width());
                painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
            }
            break;
        }
        case 2: { // Author with round avatar chip
            QRectF cellRect = rowRect.adjusted(12, 0, -12, 0);
            const qreal avatarSize = 22.0;
            const qreal avatarY = rowRect.top() + (rowRect.height() - avatarSize) * 0.5;
            QRectF avatarRect(cellRect.left(), avatarY, avatarSize, avatarSize);

            QColor avatarBg = (text.contains(QStringLiteral("Tanuja"), Qt::CaseInsensitive))
                                ? QColor(168, 85, 247)
                                : QColor(6, 182, 212);

            painter->setPen(Qt::NoPen);
            painter->setBrush(avatarBg);
            painter->drawEllipse(avatarRect);

            const QString initial = text.isEmpty() ? QStringLiteral("?") : text.left(1).toUpper();
            QFont initFont = option.font;
            initFont.setPointSize(8);
            initFont.setWeight(QFont::Bold);
            painter->setFont(initFont);
            painter->setPen(Qt::white);
            painter->drawText(avatarRect, Qt::AlignCenter, initial);

            QRectF nameRect = cellRect.adjusted(avatarSize + 9.0, 0, 0, 0);
            QFont nameFont = option.font;
            nameFont.setPointSizeF(9.5);
            nameFont.setWeight(QFont::DemiBold);
            painter->setFont(nameFont);
            painter->setPen(QColor(226, 232, 240));
            QFontMetricsF nameFm(nameFont);
            QString elidedName = nameFm.elidedText(text, Qt::ElideRight, nameRect.width());
            painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, elidedName);
            break;
        }
        case 3: { // Timestamp pill
            QRectF cellRect = rowRect.adjusted(8, 0, -12, 0);
            const qreal pillW = cellRect.width() - 4.0;
            const qreal pillH = 22.0;
            const qreal pillY = rowRect.top() + (rowRect.height() - pillH) * 0.5;
            QRectF pillRect(cellRect.left() + 2.0, pillY, pillW, pillH);

            painter->setPen(QPen(QColor(255, 255, 255, 14), 1));
            painter->setBrush(QColor(255, 255, 255, 8));
            painter->drawRoundedRect(pillRect, 5.0, 5.0);

            QFont dateFont(QStringLiteral("Consolas"));
            dateFont.setPointSizeF(8.5);
            painter->setFont(dateFont);
            painter->setPen(QColor(148, 163, 184));
            painter->drawText(pillRect, Qt::AlignCenter, text);
            break;
        }
        default:
            QStyledItemDelegate::paint(painter, option, index);
            break;
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(46);
        return s;
    }
};

} // anonymous namespace

// ── HotspotDelegate ───────────────────────────────────────────────────────────
// Page-local delegate for the Hotspots table. Renders:
//   Col 0: dimmed directory prefix + bold filename (existing filePath data)
//   Col 1: proportional heat bar + commit count number (existing commits int)
//   Col 2: date pill (existing latestDate data)
// No new data, no new computation — purely visual enhancement.

namespace {

// UserRole+1 stores the max commit count across all rows so the heat bar is proportional.
static const int HotspotMaxRole = Qt::UserRole + 1;

class HotspotDelegate : public QStyledItemDelegate
{
public:
    explicit HotspotDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        const QRect rowRect = option.rect;
        const bool isSelected = (option.state & QStyle::State_Selected);

        // Row background
        if (isSelected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(251, 100, 30, 28));
            painter->drawRect(rowRect);
        }

        // Bottom separator line
        painter->setPen(QColor(255, 255, 255, 14));
        painter->drawLine(rowRect.left(), rowRect.bottom(),
                          rowRect.right(), rowRect.bottom());

        const int col = index.column();

        switch (col) {
        case 0: { // File path: dim dir + bold filename
            const QString fullPath = index.data(Qt::DisplayRole).toString();
            const QString sep = QStringLiteral("/");
            const int lastSlash = fullPath.lastIndexOf(sep);
            const QString dir  = (lastSlash >= 0) ? fullPath.left(lastSlash + 1) : QString();
            const QString name = (lastSlash >= 0) ? fullPath.mid(lastSlash + 1) : fullPath;

            const QRectF cell = QRectF(rowRect).adjusted(16, 0, -8, 0);

            QFont dirFont = option.font;
            dirFont.setPointSizeF(9.5);
            QFontMetricsF dirFm(dirFont);

            QFont nameFont = option.font;
            nameFont.setPointSizeF(10.5);
            nameFont.setWeight(QFont::DemiBold);
            QFontMetricsF nameFm(nameFont);

            // Measure directory prefix first
            const qreal dirW = dir.isEmpty() ? 0.0 : dirFm.horizontalAdvance(dir);
            const qreal availW = cell.width() - dirW;

            painter->setFont(dirFont);
            painter->setPen(QColor(100, 110, 130));
            if (!dir.isEmpty()) {
                const QRectF dirRect(cell.left(), cell.top(), dirW, cell.height());
                painter->drawText(dirRect, Qt::AlignVCenter | Qt::AlignLeft, dir);
            }

            painter->setFont(nameFont);
            painter->setPen(QColor(236, 240, 248));
            const QRectF nameRect(cell.left() + dirW, cell.top(), availW, cell.height());
            const QString elided = nameFm.elidedText(name, Qt::ElideRight, availW);
            painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
            break;
        }
        case 1: { // Heat bar + count
            const int count    = index.data(Qt::DisplayRole).toInt();
            const int maxCount = index.data(HotspotMaxRole).toInt();

            const QRectF cell = QRectF(rowRect).adjusted(8, 0, -10, 0);

            // --- Count text on the right ---
            QFont countFont = option.font;
            countFont.setPointSizeF(10.5);
            countFont.setWeight(QFont::Bold);
            QFontMetricsF countFm(countFont);
            const QString countStr = QString::number(count);
            const qreal countW = countFm.horizontalAdvance(countStr) + 6.0;
            const QRectF countRect(cell.right() - countW, cell.top(), countW, cell.height());

            // --- Heat bar on the left of count ---
            const qreal barRight = cell.right() - countW - 8.0;
            const qreal barLeft  = cell.left();
            const qreal barH     = 6.0;
            const qreal barY     = cell.top() + (cell.height() - barH) / 2.0;
            const qreal barTotalW = barRight - barLeft;
            const qreal fillFrac  = (maxCount > 0) ? (qreal(count) / qreal(maxCount)) : 0.0;
            const qreal fillW     = barTotalW * fillFrac;

            // Track (background)
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 255, 255, 16));
            painter->drawRoundedRect(QRectF(barLeft, barY, barTotalW, barH), 3, 3);

            // Fill (orange → fuchsia gradient based on heat)
            if (fillW > 0.0) {
                QLinearGradient grad(barLeft, barY, barLeft + fillW, barY);
                grad.setColorAt(0.0, QColor(251, 100, 30));   // orange-red
                grad.setColorAt(1.0, QColor(232, 42, 200));   // fuchsia
                painter->setBrush(grad);
                painter->drawRoundedRect(QRectF(barLeft, barY, fillW, barH), 3, 3);
            }

            // Count text
            painter->setFont(countFont);
            painter->setPen(isSelected ? QColor(255, 200, 120) : QColor(251, 140, 60));
            painter->drawText(countRect, Qt::AlignVCenter | Qt::AlignRight, countStr);
            break;
        }
        case 2: { // Date pill
            const QString text = index.data(Qt::DisplayRole).toString();
            const QRectF cell  = QRectF(rowRect).adjusted(8, 0, -12, 0);
            const qreal pillH  = 22.0;
            const qreal pillW  = cell.width() - 4.0;
            const qreal pillY  = cell.top() + (cell.height() - pillH) / 2.0;
            const QRectF pill(cell.left() + 2.0, pillY, pillW, pillH);

            painter->setPen(QPen(QColor(255, 255, 255, 14), 1));
            painter->setBrush(QColor(255, 255, 255, 8));
            painter->drawRoundedRect(pill, 5.0, 5.0);

            QFont dateFont(QStringLiteral("Consolas"));
            dateFont.setPointSizeF(8.5);
            painter->setFont(dateFont);
            painter->setPen(QColor(148, 163, 184));
            painter->drawText(pill, Qt::AlignCenter, text);
            break;
        }
        default:
            QStyledItemDelegate::paint(painter, option, index);
            break;
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(40);
        return s;
    }
};

} // anonymous namespace (HotspotDelegate)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_healthService(&m_gitClient)
    , m_analyticsService(&m_gitClient)
    , m_services{&m_healthService, &m_analyticsService}
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("CODAXIS | Engineering Intelligence Platform"));
    resize(1060, 660);
    setMinimumSize(780, 480);

    if (menuBar()) {
        menuBar()->hide();
    }

    setupShellUi();
    applyTheme();
    loadRepositoryHealth();
    loadDashboard();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupShellUi()
{
    // Root horizontal layout dividing the window into sidebar and main content
    auto *rootLayout = new QHBoxLayout(ui->centralwidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ==========================================
    // 1. LEFT SIDEBAR
    // ==========================================
    auto *sidebarWidget = new QWidget(this);
    sidebarWidget->setObjectName(QStringLiteral("sidebarWidget"));
    sidebarWidget->setFixedWidth(210);

    auto *sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(16, 20, 16, 16);
    sidebarLayout->setSpacing(0);

    // Brand header
    auto *brandLogo = new QLabel(QStringLiteral("<span style='color:#f1f5f9; font-weight:800; font-size:16px;'>COD</span><span style='color:#ff2d95; font-weight:800; font-size:16px;'>AXIS</span> <span style='color:#22d3ee; font-size:10px;'>●</span>"), sidebarWidget);
    brandLogo->setObjectName(QStringLiteral("brandLogo"));
    sidebarLayout->addWidget(brandLogo);
    sidebarLayout->addSpacing(24);

    // Section header
    auto *navSectionHeader = new QLabel(QStringLiteral("NAVIGATION"), sidebarWidget);
    navSectionHeader->setObjectName(QStringLiteral("sectionHeader"));
    sidebarLayout->addWidget(navSectionHeader);
    sidebarLayout->addSpacing(8);

    // Navigation entries
    const QStringList navItems = {
        QStringLiteral("◈  Overview"),
        QStringLiteral("❖  Dashboard"),
        QStringLiteral("⎇  Commits"),
        QStringLiteral("👥  Contributors"),
        QStringLiteral("🔥  Hotspots"),
        QStringLiteral("🛡️  Repository Health")
    };

    for (int i = 0; i < navItems.size(); ++i) {
        auto *btn = new QPushButton(navItems[i], sidebarWidget);
        btn->setObjectName(QStringLiteral("navButton"));
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        if (i == 0) {
            btn->setChecked(true);
        }

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            onNavigationClicked(i);
        });

        m_navButtons.append(btn);
        sidebarLayout->addWidget(btn);
        sidebarLayout->addSpacing(2);
    }

    // Push status label to the bottom of the sidebar
    sidebarLayout->addStretch(1);

    // ---- Gear widget: decorative brand element in the empty sidebar space ----
    m_gearWidget = new GearWidget(sidebarWidget);
    // Fixed height keeps the gear compact; width expands to fill the sidebar.
    // The widget's own paintEvent scales the gear to the available area.
    m_gearWidget->setFixedHeight(110);
    sidebarLayout->addWidget(m_gearWidget);
    sidebarLayout->addSpacing(8);
    // -------------------------------------------------------------------------

    m_sidebarStatusLabel = new QLabel(QStringLiteral("● No repository loaded"), sidebarWidget);
    m_sidebarStatusLabel->setObjectName(QStringLiteral("sidebarStatusLabel"));
    m_sidebarStatusLabel->setWordWrap(true);
    sidebarLayout->addWidget(m_sidebarStatusLabel);

    rootLayout->addWidget(sidebarWidget);

    // ==========================================
    // 2. STACKED CONTENT AREA
    // ==========================================
    m_contentStack = new QStackedWidget(this);
    m_contentStack->setObjectName(QStringLiteral("contentStack"));

    // --- Page 0: Overview / Welcome ---
    auto *overviewPage = new RepositoryCanvas(this);
    overviewPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *mainAreaLayout = new QVBoxLayout(overviewPage);
    mainAreaLayout->setContentsMargins(40, 36, 40, 36);
    mainAreaLayout->setSpacing(0);

    // Elevated Hero Card
    auto *welcomeCard = new QFrame(overviewPage);
    welcomeCard->setObjectName(QStringLiteral("welcomeHeroCard"));
    welcomeCard->setMaximumWidth(640);
    welcomeCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto *cardShadow = new QGraphicsDropShadowEffect(welcomeCard);
    cardShadow->setBlurRadius(28);
    cardShadow->setColor(QColor(8, 4, 16, 160));
    cardShadow->setOffset(0, 6);
    welcomeCard->setGraphicsEffect(cardShadow);

    auto *welcomeLayout = new QVBoxLayout(welcomeCard);
    welcomeLayout->setContentsMargins(34, 30, 34, 30);
    welcomeLayout->setSpacing(0);

    // 1. Eyebrow badge
    auto *badgeRow = new QHBoxLayout();
    badgeRow->setContentsMargins(0, 0, 0, 0);
    auto *badge = new QLabel(QStringLiteral("◈ REPOSITORY ARCHITECTURE & HEALTH"), welcomeCard);
    badge->setObjectName(QStringLiteral("welcomeBadge"));
    badgeRow->addWidget(badge);
    badgeRow->addStretch(1);
    welcomeLayout->addLayout(badgeRow);
    welcomeLayout->addSpacing(14);

    // 2. Main Heading
    auto *headingLabel = new QLabel(QStringLiteral("Understand your codebase."), welcomeCard);
    headingLabel->setObjectName(QStringLiteral("welcomeHeading"));
    headingLabel->setWordWrap(true);
    welcomeLayout->addWidget(headingLabel);
    welcomeLayout->addSpacing(10);

    // 3. Description
    auto *descLabel = new QLabel(
        QStringLiteral("Local git analytics and architectural intelligence platform. "
                       "Inspect commit patterns, identify hotspots, track contributor dynamics, "
                       "and evaluate repository health—completely offline."),
        welcomeCard
    );
    descLabel->setObjectName(QStringLiteral("welcomeDescription"));
    descLabel->setWordWrap(true);
    descLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    welcomeLayout->addWidget(descLabel);
    welcomeLayout->addSpacing(22);

    // 4. Action Row (Button + Hint)
    auto *openRepoBtn = new QPushButton(QStringLiteral("📁 Open Repository"), welcomeCard);
    openRepoBtn->setObjectName(QStringLiteral("btnOpenRepository"));
    openRepoBtn->setCursor(Qt::PointingHandCursor);
    connect(openRepoBtn, &QPushButton::clicked, this, &MainWindow::onOpenRepositoryClicked);

    auto *hintLabel = new QLabel(QStringLiteral("Inspect any local Git repository directory offline"), welcomeCard);
    hintLabel->setObjectName(QStringLiteral("welcomeHint"));

    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    btnRow->setSpacing(14);
    btnRow->addWidget(openRepoBtn);
    btnRow->addWidget(hintLabel);
    btnRow->addStretch(1);
    welcomeLayout->addLayout(btnRow);
    welcomeLayout->addSpacing(24);

    // 5. Divider
    auto *divider = new QFrame(welcomeCard);
    divider->setObjectName(QStringLiteral("welcomeDivider"));
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    welcomeLayout->addWidget(divider);
    welcomeLayout->addSpacing(18);

    // 6. Capability Pillar Cards Row
    auto *pillarsRow = new QHBoxLayout();
    pillarsRow->setContentsMargins(0, 0, 0, 0);
    pillarsRow->setSpacing(10);

    auto createPillar = [welcomeCard](const QString &tag, const QString &text) {
        auto *pillar = new QFrame(welcomeCard);
        pillar->setObjectName(QStringLiteral("overviewPillarCard"));
        pillar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto *pLayout = new QVBoxLayout(pillar);
        pLayout->setContentsMargins(12, 10, 12, 10);
        pLayout->setSpacing(4);

        auto *title = new QLabel(tag, pillar);
        title->setObjectName(QStringLiteral("overviewPillarTitle"));

        auto *desc = new QLabel(text, pillar);
        desc->setObjectName(QStringLiteral("overviewPillarDetail"));
        desc->setWordWrap(true);

        pLayout->addWidget(title);
        pLayout->addWidget(desc);
        return pillar;
    };

    pillarsRow->addWidget(createPillar(QStringLiteral("🔥 CHURN & HOTSPOTS"),
                                       QStringLiteral("Track high-frequency file changes and active development areas.")));
    pillarsRow->addWidget(createPillar(QStringLiteral("👥 CONTRIBUTORS"),
                                       QStringLiteral("Aggregate authorship patterns and collaborative dynamics.")));
    pillarsRow->addWidget(createPillar(QStringLiteral("🛡️ INTEGRITY"),
                                       QStringLiteral("Inspect working tree health, branch state, and commit footprint.")));

    welcomeLayout->addLayout(pillarsRow);

    auto *centerRow = new QHBoxLayout();
    centerRow->setContentsMargins(0, 0, 0, 0);
    centerRow->addStretch(1);
    centerRow->addWidget(welcomeCard);
    centerRow->addStretch(1);

    mainAreaLayout->addStretch(1);
    mainAreaLayout->addLayout(centerRow);
    mainAreaLayout->addStretch(1);

    m_contentStack->addWidget(overviewPage); // index 0

    // --- Page 1: Dashboard ---
    auto *dashboardScroll = new QScrollArea(this);
    dashboardScroll->setObjectName(QStringLiteral("dashboardScrollArea"));
    dashboardScroll->setWidgetResizable(true);
    m_dashboardWidget = new DashboardWidget(dashboardScroll);
    dashboardScroll->setWidget(m_dashboardWidget);
    dashboardScroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));
    m_contentStack->addWidget(dashboardScroll); // index 1

    // --- Page 2: Commits ---
    auto *commitsPage = new QWidget();
    commitsPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *commitsPageLayout = new QVBoxLayout(commitsPage);
    commitsPageLayout->setContentsMargins(28, 24, 28, 24);
    commitsPageLayout->setSpacing(14);

    auto *commitsHeaderLayout = new QVBoxLayout();
    commitsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    commitsHeaderLayout->setSpacing(6);

    auto *commitsBadgeRow = new QHBoxLayout();
    commitsBadgeRow->setContentsMargins(0, 0, 0, 0);
    commitsBadgeRow->setSpacing(10);

    auto *commitsBadge = new QLabel(QStringLiteral("⎇ VERSION CONTROL TIMELINE"), commitsPage);
    commitsBadge->setObjectName(QStringLiteral("pageBadge"));
    commitsBadgeRow->addWidget(commitsBadge);

    m_commitsCountBadge = new QLabel(QStringLiteral("0 COMMITS"), commitsPage);
    m_commitsCountBadge->setObjectName(QStringLiteral("commitsCountBadge"));
    commitsBadgeRow->addWidget(m_commitsCountBadge);

    m_commitsBranchBadge = new QLabel(QStringLiteral("● NO REPO"), commitsPage);
    m_commitsBranchBadge->setObjectName(QStringLiteral("commitsBranchBadge"));
    commitsBadgeRow->addWidget(m_commitsBranchBadge);

    commitsBadgeRow->addStretch(1);

    auto *commitsHeader = new QLabel(QStringLiteral("⎇ Commit History"), commitsPage);
    commitsHeader->setObjectName(QStringLiteral("pageHeader"));

    auto *commitsDesc = new QLabel(
        QStringLiteral("Chronological log of changes, authors, and commit timestamps across the repository."),
        commitsPage
    );
    commitsDesc->setObjectName(QStringLiteral("pageDescription"));
    commitsDesc->setWordWrap(true);

    commitsHeaderLayout->addLayout(commitsBadgeRow);
    commitsHeaderLayout->addWidget(commitsHeader);
    commitsHeaderLayout->addWidget(commitsDesc);

    // --- Search & Stats Toolbar ---
    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(0, 4, 0, 4);
    toolbarLayout->setSpacing(12);

    m_commitsFilterInput = new QLineEdit(commitsPage);
    m_commitsFilterInput->setObjectName(QStringLiteral("commitsSearchInput"));
    m_commitsFilterInput->setPlaceholderText(QStringLiteral("🔍  Filter by message, author, or commit hash..."));
    m_commitsFilterInput->setClearButtonEnabled(true);
    connect(m_commitsFilterInput, &QLineEdit::textChanged, this, &MainWindow::onCommitFilterChanged);
    toolbarLayout->addWidget(m_commitsFilterInput, 1);

    m_commitsStatTotal = new QLabel(QStringLiteral("📦 0 Commits"), commitsPage);
    m_commitsStatTotal->setObjectName(QStringLiteral("commitStatChip"));
    toolbarLayout->addWidget(m_commitsStatTotal);

    m_commitsStatAuthors = new QLabel(QStringLiteral("👥 0 Authors"), commitsPage);
    m_commitsStatAuthors->setObjectName(QStringLiteral("commitStatChip"));
    toolbarLayout->addWidget(m_commitsStatAuthors);

    m_commitsStatLatest = new QLabel(QStringLiteral("📅 --"), commitsPage);
    m_commitsStatLatest->setObjectName(QStringLiteral("commitStatChip"));
    toolbarLayout->addWidget(m_commitsStatLatest);

    // --- Premium Commits Table Card Container ---
    auto *tableFrame = new QFrame(commitsPage);
    tableFrame->setObjectName(QStringLiteral("commitsTableFrame"));
    auto *frameLayout = new QVBoxLayout(tableFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);

    m_commitsTable = new QTableWidget(0, 4, tableFrame);
    m_commitsTable->setObjectName(QStringLiteral("commitsTable"));
    m_commitsTable->setHorizontalHeaderLabels({
        QStringLiteral("HASH"),
        QStringLiteral("COMMIT MESSAGE"),
        QStringLiteral("AUTHOR"),
        QStringLiteral("TIMESTAMP")
    });
    m_commitsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_commitsTable->setColumnWidth(0, 95);
    m_commitsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_commitsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_commitsTable->setColumnWidth(2, 160);
    m_commitsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_commitsTable->setColumnWidth(3, 130);
    m_commitsTable->verticalHeader()->setVisible(false);
    m_commitsTable->verticalHeader()->setDefaultSectionSize(46);
    m_commitsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commitsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_commitsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_commitsTable->setShowGrid(false);
    m_commitsTable->setSortingEnabled(false);
    m_commitsTable->setMouseTracking(true);
    m_commitsTable->setItemDelegate(new CommitItemDelegate(m_commitsTable));

    frameLayout->addWidget(m_commitsTable);

    commitsPageLayout->addLayout(commitsHeaderLayout);
    commitsPageLayout->addLayout(toolbarLayout);
    commitsPageLayout->addWidget(tableFrame, 1);

    m_contentStack->addWidget(commitsPage); // index 2

    // --- Page 2: Contributors ---
    auto *contributorsPage = new QWidget();
    contributorsPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *contributorsLayout = new QVBoxLayout(contributorsPage);
    contributorsLayout->setContentsMargins(28, 24, 28, 24);
    contributorsLayout->setSpacing(14);

    auto *contribHeaderLayout = new QVBoxLayout();
    contribHeaderLayout->setContentsMargins(0, 0, 0, 0);
    contribHeaderLayout->setSpacing(4);

    auto *contributorsBadge = new QLabel(QStringLiteral("👥 AUTHORSHIP & COLLABORATION"), contributorsPage);
    contributorsBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *contribBadgeRow = new QHBoxLayout();
    contribBadgeRow->setContentsMargins(0, 0, 0, 0);
    contribBadgeRow->addWidget(contributorsBadge);
    contribBadgeRow->addStretch(1);

    auto *contributorsHeader = new QLabel(QStringLiteral("👥 Contributors"), contributorsPage);
    contributorsHeader->setObjectName(QStringLiteral("pageHeader"));

    auto *contributorsDesc = new QLabel(
        QStringLiteral("Commit distribution and recent activity per author in the repository history."),
        contributorsPage
    );
    contributorsDesc->setObjectName(QStringLiteral("pageDescription"));
    contributorsDesc->setWordWrap(true);

    contribHeaderLayout->addLayout(contribBadgeRow);
    contribHeaderLayout->addWidget(contributorsHeader);
    contribHeaderLayout->addWidget(contributorsDesc);

    m_contributorsTable = new QTableWidget(0, 3, contributorsPage);
    m_contributorsTable->setObjectName(QStringLiteral("contributorsTable"));
    m_contributorsTable->setHorizontalHeaderLabels({
        QStringLiteral("Author"),
        QStringLiteral("Commits"),
        QStringLiteral("Latest Commit")
    });
    m_contributorsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_contributorsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_contributorsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_contributorsTable->verticalHeader()->setVisible(false);
    m_contributorsTable->verticalHeader()->setDefaultSectionSize(38);
    m_contributorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_contributorsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_contributorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_contributorsTable->setAlternatingRowColors(true);
    m_contributorsTable->setShowGrid(false);
    m_contributorsTable->setSortingEnabled(true);

    // Stat chips row (populated in loadContributors())
    auto *contribStatsLayout = new QHBoxLayout();
    contribStatsLayout->setContentsMargins(0, 0, 0, 0);
    contribStatsLayout->setSpacing(8);

    m_contribStatTotal = new QLabel(QStringLiteral("👥  — Authors"), contributorsPage);
    m_contribStatTotal->setObjectName(QStringLiteral("contribStatChip"));
    m_contribStatCommits = new QLabel(QStringLiteral("✦  — Commits"), contributorsPage);
    m_contribStatCommits->setObjectName(QStringLiteral("contribStatChip"));
    m_contribStatLatest = new QLabel(QStringLiteral("🗓  —"), contributorsPage);
    m_contribStatLatest->setObjectName(QStringLiteral("contribStatChip"));

    contribStatsLayout->addWidget(m_contribStatTotal);
    contribStatsLayout->addWidget(m_contribStatCommits);
    contribStatsLayout->addWidget(m_contribStatLatest);
    contribStatsLayout->addStretch(1);

    // Frame card wrapping the table
    auto *contribFrame = new QFrame(contributorsPage);
    contribFrame->setObjectName(QStringLiteral("contributorsTableFrame"));
    auto *contribFrameLayout = new QVBoxLayout(contribFrame);
    contribFrameLayout->setContentsMargins(0, 0, 0, 0);
    contribFrameLayout->setSpacing(0);
    contribFrameLayout->addWidget(m_contributorsTable);

    contributorsLayout->addLayout(contribHeaderLayout);
    contributorsLayout->addLayout(contribStatsLayout);
    contributorsLayout->addWidget(contribFrame, 1);

    m_contentStack->addWidget(contributorsPage); // index 2

    // --- Page 3: Hotspots ---
    auto *hotspotsPage = new QWidget();
    hotspotsPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *hotspotsLayout = new QVBoxLayout(hotspotsPage);
    hotspotsLayout->setContentsMargins(28, 24, 28, 24);
    hotspotsLayout->setSpacing(14);

    auto *hotspotsHeaderLayout = new QVBoxLayout();
    hotspotsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    hotspotsHeaderLayout->setSpacing(4);

    auto *hotspotsBadge = new QLabel(QStringLiteral("🔥 CODE CHURN & RISK ANALYSIS"), hotspotsPage);
    hotspotsBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *hotspotsBadgeRow = new QHBoxLayout();
    hotspotsBadgeRow->setContentsMargins(0, 0, 0, 0);
    hotspotsBadgeRow->addWidget(hotspotsBadge);
    hotspotsBadgeRow->addStretch(1);

    auto *hotspotsHeader = new QLabel(QStringLiteral("🔥 Hotspots"), hotspotsPage);
    hotspotsHeader->setObjectName(QStringLiteral("pageHeader"));

    auto *hotspotsDesc = new QLabel(
        QStringLiteral("Files changed most frequently across commit history. High change frequency "
                       "often highlights active development areas, shared dependencies, or code prone to churn."),
        hotspotsPage
    );
    hotspotsDesc->setObjectName(QStringLiteral("pageDescription"));
    hotspotsDesc->setWordWrap(true);

    hotspotsHeaderLayout->addLayout(hotspotsBadgeRow);
    hotspotsHeaderLayout->addWidget(hotspotsHeader);
    hotspotsHeaderLayout->addWidget(hotspotsDesc);

    m_hotspotsTable = new QTableWidget(0, 3, hotspotsPage);
    m_hotspotsTable->setObjectName(QStringLiteral("hotspotsTable"));
    m_hotspotsTable->setHorizontalHeaderLabels({
        QStringLiteral("File Path"),
        QStringLiteral("Changes"),
        QStringLiteral("Latest Change")
    });
    m_hotspotsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_hotspotsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_hotspotsTable->horizontalHeader()->resizeSection(1, 110);
    m_hotspotsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_hotspotsTable->verticalHeader()->setVisible(false);
    m_hotspotsTable->verticalHeader()->setDefaultSectionSize(40);
    m_hotspotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hotspotsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_hotspotsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_hotspotsTable->setAlternatingRowColors(true);
    m_hotspotsTable->setShowGrid(false);
    m_hotspotsTable->setSortingEnabled(true);

    // Stat chips row (populated in loadHotspots())
    auto *hotspotStatsLayout = new QHBoxLayout();
    hotspotStatsLayout->setContentsMargins(0, 0, 0, 0);
    hotspotStatsLayout->setSpacing(8);

    m_hotspotStatFiles = new QLabel(QStringLiteral("📁  — Files"), hotspotsPage);
    m_hotspotStatFiles->setObjectName(QStringLiteral("hotspotStatChip"));
    m_hotspotStatTopCount = new QLabel(QStringLiteral("🔥  — Changes"), hotspotsPage);
    m_hotspotStatTopCount->setObjectName(QStringLiteral("hotspotStatChip"));
    m_hotspotStatLatest = new QLabel(QStringLiteral("🗓  —"), hotspotsPage);
    m_hotspotStatLatest->setObjectName(QStringLiteral("hotspotStatChip"));

    hotspotStatsLayout->addWidget(m_hotspotStatFiles);
    hotspotStatsLayout->addWidget(m_hotspotStatTopCount);
    hotspotStatsLayout->addWidget(m_hotspotStatLatest);
    hotspotStatsLayout->addStretch(1);

    // Frame card wrapping the table
    auto *hotspotsFrame = new QFrame(hotspotsPage);
    hotspotsFrame->setObjectName(QStringLiteral("hotspotsTableFrame"));
    auto *hotspotsFrameLayout = new QVBoxLayout(hotspotsFrame);
    hotspotsFrameLayout->setContentsMargins(0, 0, 0, 0);
    hotspotsFrameLayout->setSpacing(0);
    hotspotsFrameLayout->addWidget(m_hotspotsTable);

    hotspotsLayout->addLayout(hotspotsHeaderLayout);
    hotspotsLayout->addLayout(hotspotStatsLayout);
    hotspotsLayout->addWidget(hotspotsFrame, 1);

    m_contentStack->addWidget(hotspotsPage); // index 3

    // --- Page 4: Repository Health ---
    auto *healthPage = new QWidget();
    healthPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *healthLayout = new QVBoxLayout(healthPage);
    healthLayout->setContentsMargins(28, 24, 28, 24);
    healthLayout->setSpacing(16);

    // --- Page header ---
    auto *healthHeaderLayout = new QVBoxLayout();
    healthHeaderLayout->setContentsMargins(0, 0, 0, 0);
    healthHeaderLayout->setSpacing(4);

    auto *healthBadge = new QLabel(QStringLiteral("🛡️ REPOSITORY INTEGRITY"), healthPage);
    healthBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *healthBadgeRow = new QHBoxLayout();
    healthBadgeRow->setContentsMargins(0, 0, 0, 0);
    healthBadgeRow->addWidget(healthBadge);
    healthBadgeRow->addStretch(1);

    auto *healthHeader = new QLabel(QStringLiteral("🛡️ Repository Health"), healthPage);
    healthHeader->setObjectName(QStringLiteral("pageHeader"));

    auto *healthDesc = new QLabel(
        QStringLiteral("Core repository integrity metrics, version control footprint, and working tree status."),
        healthPage
    );
    healthDesc->setObjectName(QStringLiteral("pageDescription"));
    healthDesc->setWordWrap(true);

    healthHeaderLayout->addLayout(healthBadgeRow);
    healthHeaderLayout->addWidget(healthHeader);
    healthHeaderLayout->addWidget(healthDesc);

    // --- Summary KPI cards row (keep existing 4 cards) ---
    auto *cardsRow = new QHBoxLayout();
    cardsRow->setContentsMargins(0, 0, 0, 0);
    cardsRow->setSpacing(12);

    auto createMetricCard = [healthPage](const QString &title, QLabel *&valLabel) {
        auto *card = new QFrame(healthPage);
        card->setObjectName(QStringLiteral("healthMetricCard"));
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto *cardShadow = new QGraphicsDropShadowEffect(card);
        cardShadow->setBlurRadius(14);
        cardShadow->setColor(QColor(50, 45, 38, 12));
        cardShadow->setOffset(0, 3);
        card->setGraphicsEffect(cardShadow);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(15, 12, 15, 12);
        cardLayout->setSpacing(4);

        auto *titleLbl = new QLabel(title, card);
        titleLbl->setObjectName(QStringLiteral("healthMetricTitle"));

        valLabel = new QLabel(QStringLiteral("—"), card);
        valLabel->setObjectName(QStringLiteral("healthMetricValue"));

        cardLayout->addWidget(titleLbl);
        cardLayout->addWidget(valLabel);
        return card;
    };

    cardsRow->addWidget(createMetricCard(QStringLiteral("⎇ TOTAL COMMITS"), m_healthCommitsValue));
    cardsRow->addWidget(createMetricCard(QStringLiteral("👥 CONTRIBUTORS"), m_healthContributorsValue));
    cardsRow->addWidget(createMetricCard(QStringLiteral("📁 TRACKED FILES"), m_healthFilesValue));
    cardsRow->addWidget(createMetricCard(QStringLiteral("🌿 CURRENT BRANCH"), m_healthBranchValue));

    // -----------------------------------------------------------------------
    // Two-column main content area
    // -----------------------------------------------------------------------
    auto *bodyRow = new QHBoxLayout();
    bodyRow->setContentsMargins(0, 0, 0, 0);
    bodyRow->setSpacing(14);

    // ---- LEFT: Repository Snapshot panel ----
    auto *snapshotFrame = new QFrame(healthPage);
    snapshotFrame->setObjectName(QStringLiteral("hrSnapshotFrame"));
    snapshotFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *snapshotOuterLayout = new QVBoxLayout(snapshotFrame);
    snapshotOuterLayout->setContentsMargins(0, 0, 0, 0);
    snapshotOuterLayout->setSpacing(0);

    // Panel title bar
    auto *snapshotTitleBar = new QWidget(snapshotFrame);
    snapshotTitleBar->setObjectName(QStringLiteral("hrPanelTitleBar"));
    auto *snapshotTitleBarLayout = new QHBoxLayout(snapshotTitleBar);
    snapshotTitleBarLayout->setContentsMargins(16, 10, 16, 10);
    auto *snapshotTitle = new QLabel(QStringLiteral("REPOSITORY SNAPSHOT"), snapshotTitleBar);
    snapshotTitle->setObjectName(QStringLiteral("hrPanelTitle"));
    snapshotTitleBarLayout->addWidget(snapshotTitle);
    snapshotTitleBarLayout->addStretch(1);
    snapshotOuterLayout->addWidget(snapshotTitleBar);

    // Separator
    auto *snapshotSep = new QFrame(snapshotFrame);
    snapshotSep->setObjectName(QStringLiteral("hrSeparator"));
    snapshotSep->setFixedHeight(1);
    snapshotOuterLayout->addWidget(snapshotSep);

    // Scrollable content area for metric rows
    auto *snapshotScroll = new QScrollArea(snapshotFrame);
    snapshotScroll->setObjectName(QStringLiteral("hrSnapshotScroll"));
    snapshotScroll->setWidgetResizable(true);
    snapshotScroll->setFrameShape(QFrame::NoFrame);
    snapshotScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *snapshotContent = new QWidget();
    snapshotContent->setObjectName(QStringLiteral("hrSnapshotContent"));
    auto *snapshotContentLayout = new QVBoxLayout(snapshotContent);
    snapshotContentLayout->setContentsMargins(0, 0, 0, 0);
    snapshotContentLayout->setSpacing(0);

    // Helper: create one metric row [icon | label | value | detail]
    // Stores the value label pointer so loadRepositoryHealth can update it.
    struct MetricRowDef { QString icon; QString label; QLabel **valPtr; };
    const QList<MetricRowDef> rowDefs = {
        { QStringLiteral("📂"), QStringLiteral("Repository Root"),       &m_hrRootVal   },
        { QStringLiteral("⎇"),  QStringLiteral("Current Branch"),        &m_hrBranchVal },
        { QStringLiteral("⏱"),  QStringLiteral("Total Commits"),         &m_hrCommitsVal },
        { QStringLiteral("👥"), QStringLiteral("Unique Contributors"),   &m_hrContribVal },
        { QStringLiteral("📁"), QStringLiteral("Tracked Files"),         &m_hrFilesVal  },
        { QStringLiteral("🌿"), QStringLiteral("Working Tree"),          &m_hrTreeVal   },
        { QStringLiteral("📌"), QStringLiteral("Latest Commit"),         &m_hrLatestVal },
    };

    bool isAlt = false;
    for (const MetricRowDef &def : rowDefs) {
        auto *rowFrame = new QFrame(snapshotContent);
        rowFrame->setObjectName(isAlt ? QStringLiteral("hrMetricRowAlt") : QStringLiteral("hrMetricRow"));
        auto *rowLayout = new QHBoxLayout(rowFrame);
        rowLayout->setContentsMargins(16, 12, 16, 12);
        rowLayout->setSpacing(10);

        auto *iconLbl = new QLabel(def.icon, rowFrame);
        iconLbl->setObjectName(QStringLiteral("hrRowIcon"));
        iconLbl->setFixedWidth(20);

        auto *nameLbl = new QLabel(def.label, rowFrame);
        nameLbl->setObjectName(QStringLiteral("hrRowLabel"));
        nameLbl->setFixedWidth(140);

        auto *valLbl = new QLabel(QStringLiteral("—"), rowFrame);
        valLbl->setObjectName(QStringLiteral("hrRowValue"));
        valLbl->setWordWrap(true);
        valLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        *def.valPtr = valLbl;

        rowLayout->addWidget(iconLbl);
        rowLayout->addWidget(nameLbl);
        rowLayout->addWidget(valLbl);
        snapshotContentLayout->addWidget(rowFrame);
        isAlt = !isAlt;
    }
    snapshotContentLayout->addStretch(1);

    // Status overlay for error / empty state
    m_hrStatusOverlay = new QLabel(QStringLiteral("No repository loaded."), snapshotContent);
    m_hrStatusOverlay->setObjectName(QStringLiteral("hrStatusOverlay"));
    m_hrStatusOverlay->setAlignment(Qt::AlignCenter);
    m_hrStatusOverlay->setWordWrap(true);
    m_hrStatusOverlay->hide();
    snapshotContentLayout->insertWidget(0, m_hrStatusOverlay);

    snapshotScroll->setWidget(snapshotContent);
    snapshotOuterLayout->addWidget(snapshotScroll, 1);

    // ---- RIGHT: Working Tree + Latest Commit stacked cards ----
    auto *rightCol = new QVBoxLayout();
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(12);

    // — Working Tree card —
    auto *wtCard = new QFrame(healthPage);
    wtCard->setObjectName(QStringLiteral("hrRightCard"));
    wtCard->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    wtCard->setFixedWidth(248);

    auto *wtCardLayout = new QVBoxLayout(wtCard);
    wtCardLayout->setContentsMargins(18, 16, 18, 16);
    wtCardLayout->setSpacing(8);

    auto *wtCardTitle = new QLabel(QStringLiteral("WORKING TREE"), wtCard);
    wtCardTitle->setObjectName(QStringLiteral("hrCardTitle"));

    m_hrWtStatus = new QLabel(QStringLiteral("—"), wtCard);
    m_hrWtStatus->setObjectName(QStringLiteral("hrWtStatusClean"));
    m_hrWtStatus->setWordWrap(true);

    auto *wtCardSep = new QFrame(wtCard);
    wtCardSep->setObjectName(QStringLiteral("hrCardSeparator"));
    wtCardSep->setFixedHeight(1);

    m_hrWtDetail = new QLabel(QStringLiteral("—"), wtCard);
    m_hrWtDetail->setObjectName(QStringLiteral("hrCardDetail"));
    m_hrWtDetail->setWordWrap(true);

    wtCardLayout->addWidget(wtCardTitle);
    wtCardLayout->addWidget(m_hrWtStatus);
    wtCardLayout->addWidget(wtCardSep);
    wtCardLayout->addWidget(m_hrWtDetail);
    wtCardLayout->addStretch(1);

    // — Latest Commit card —
    auto *lcCard = new QFrame(healthPage);
    lcCard->setObjectName(QStringLiteral("hrRightCard"));
    lcCard->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    lcCard->setFixedWidth(248);

    auto *lcCardLayout = new QVBoxLayout(lcCard);
    lcCardLayout->setContentsMargins(18, 16, 18, 16);
    lcCardLayout->setSpacing(6);

    auto *lcCardTitle = new QLabel(QStringLiteral("LATEST COMMIT"), lcCard);
    lcCardTitle->setObjectName(QStringLiteral("hrCardTitle"));

    m_hrCommitHash = new QLabel(QStringLiteral("—"), lcCard);
    m_hrCommitHash->setObjectName(QStringLiteral("hrCommitHash"));

    m_hrCommitDate = new QLabel(QStringLiteral("—"), lcCard);
    m_hrCommitDate->setObjectName(QStringLiteral("hrCommitDate"));

    auto *lcCardSep = new QFrame(lcCard);
    lcCardSep->setObjectName(QStringLiteral("hrCardSeparator"));
    lcCardSep->setFixedHeight(1);

    m_hrCommitMsg = new QLabel(QStringLiteral("—"), lcCard);
    m_hrCommitMsg->setObjectName(QStringLiteral("hrCommitMsg"));
    m_hrCommitMsg->setWordWrap(true);

    lcCardLayout->addWidget(lcCardTitle);
    lcCardLayout->addWidget(m_hrCommitHash);
    lcCardLayout->addWidget(m_hrCommitDate);
    lcCardLayout->addWidget(lcCardSep);
    lcCardLayout->addWidget(m_hrCommitMsg);
    lcCardLayout->addStretch(1);

    rightCol->addWidget(wtCard, 1);
    rightCol->addWidget(lcCard, 1);

    bodyRow->addWidget(snapshotFrame, 1);
    bodyRow->addLayout(rightCol, 0);

    // Hidden QTableWidget: kept for back-compat with loadRepositoryHealth() error paths
    m_healthTable = new QTableWidget(0, 3, healthPage);
    m_healthTable->setObjectName(QStringLiteral("healthTable"));
    m_healthTable->hide();

    // Also keep chip labels alive so loadRepositoryHealth compiles
    m_healthWorkingTreeChip = new QLabel(healthPage);
    m_healthWorkingTreeChip->hide();
    m_healthLatestCommitChip = new QLabel(healthPage);
    m_healthLatestCommitChip->hide();

    healthLayout->addLayout(healthHeaderLayout);
    healthLayout->addLayout(cardsRow);
    healthLayout->addLayout(bodyRow, 1);

    m_contentStack->addWidget(healthPage); // index 4

    rootLayout->addWidget(m_contentStack, 1);

    // ==========================================
    // 3. STATUS BAR
    // ==========================================
    m_statusLabel = new QLabel(QStringLiteral("No repository loaded"), this);
    m_statusLabel->setObjectName(QStringLiteral("footerStatusLabel"));
    ui->statusbar->addWidget(m_statusLabel);

    auto *readyLabel = new QLabel(QStringLiteral("Ready"), this);
    readyLabel->setObjectName(QStringLiteral("footerReadyLabel"));
    ui->statusbar->addPermanentWidget(readyLabel);
}

void MainWindow::onNavigationClicked(int activeIndex)
{
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == activeIndex);
    }
    if (m_contentStack) {
        m_contentStack->setCurrentIndex(activeIndex);
    }
    // Sync gear rotation to page selection
    updateGearState();
}

void MainWindow::onOpenRepositoryClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Open Git Repository"),
        m_repositoryPath.isEmpty() ? QString() : m_repositoryPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    // Silent cancellation if dialog was cancelled or closed
    if (dir.isEmpty()) {
        return;
    }

    validateAndOpenRepository(dir);
}

bool MainWindow::validateAndOpenRepository(const QString &dirPath)
{
    if (dirPath.trimmed().isEmpty()) {
        return false;
    }

    const QFileInfo dirInfo(dirPath);
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        QMessageBox::warning(
            this,
            tr("Invalid Repository"),
            tr("Please select a valid Git repository. The selected folder is not a valid Git working tree.")
        );
        return false;
    }

    if (!m_gitClient.isGitAvailable()) {
        QMessageBox::critical(
            this,
            tr("Git Not Found"),
            tr("Could not find git.exe on this system.\nPlease ensure Git is installed.")
        );
        return false;
    }

    // 1. Verify that dirPath is inside a valid Git work tree using actual git rev-parse check
    QString workTreeError;
    if (!m_gitClient.isInsideWorkTree(dirPath, &workTreeError)) {
        // Preserve previous repository state and existing analytics
        QMessageBox::warning(
            this,
            tr("Invalid Repository"),
            tr("Please select a valid Git repository. The selected folder is not a valid Git working tree.")
        );
        return false;
    }

    // 2. Resolve top-level root (supports worktrees, subfolders, and submodules)
    const QString repoRoot = m_gitClient.getTopLevel(dirPath);
    const QFileInfo repoRootInfo(repoRoot);
    if (repoRoot.trimmed().isEmpty() || !repoRootInfo.exists() || !repoRootInfo.isDir()) {
        QMessageBox::warning(
            this,
            tr("Invalid Repository"),
            tr("Please select a valid Git repository. The selected folder is not a valid Git working tree.")
        );
        return false;
    }

    const QString repoName = repoRootInfo.fileName();

    // 3. Query current branch name or detached HEAD
    const QString branch = m_gitClient.getCurrentBranch(repoRoot);

    // 4. Update repository state only after successful validation
    m_repositoryPath = repoRoot;
    m_currentBranch = branch;

    // 5. Update UI components consistently
    if (m_sidebarStatusLabel) {
        m_sidebarStatusLabel->setText(QStringLiteral("● %1 [%2]").arg(repoName, branch));
    }

    if (m_statusLabel) {
        m_statusLabel->setText(QStringLiteral("Repository: %1 | Branch: %2 | %3")
            .arg(repoName, branch, QDir::toNativeSeparators(repoRoot)));
    }

    // Runtime polymorphism: svc->refresh() calls the correct version at runtime --
    // RepositoryHealthService::refresh or GitAnalyticsService::refresh -- based on
    // the actual type each GitService* pointer points to.
    // 6. Refresh all services through the shared GitService interface (runtime polymorphism).
    // Both RepositoryHealthService and GitAnalyticsService are dispatched via GitService*.
    for (GitService *svc : m_services) {
        svc->refresh(m_repositoryPath);
    }

    loadCommitHistory();
    loadContributors();
    loadHotspots();
    loadRepositoryHealth();
    loadDashboard();

    // Automatically navigate to Dashboard (index 1) so user immediately sees intelligence
    onNavigationClicked(1);
    // updateGearState() is called inside onNavigationClicked above

    return true;
}

void MainWindow::onCommitFilterChanged(const QString &text)
{
    if (!m_commitsTable) {
        return;
    }

    const QString q = text.trimmed().toLower();
    int visibleCount = 0;
    const int total = m_commitsTable->rowCount();

    for (int r = 0; r < total; ++r) {
        bool match = false;
        if (q.isEmpty()) {
            match = true;
        } else {
            for (int c = 0; c < 4; ++c) {
                auto *item = m_commitsTable->item(r, c);
                if (item && item->text().toLower().contains(q)) {
                    match = true;
                    break;
                }
            }
        }
        m_commitsTable->setRowHidden(r, !match);
        if (match) {
            visibleCount++;
        }
    }

    if (m_commitsCountBadge) {
        if (q.isEmpty()) {
            m_commitsCountBadge->setText(QStringLiteral("%1 COMMITS").arg(total));
        } else {
            m_commitsCountBadge->setText(QStringLiteral("%1 / %2 MATCHES").arg(visibleCount).arg(total));
        }
    }
}

void MainWindow::loadCommitHistory()
{
    if (!m_commitsTable) {
        return;
    }

    m_commitsTable->setRowCount(0);

    auto makeMsgItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor(QStringLiteral("#a89ec0")));
        return item;
    };

    if (m_repositoryPath.isEmpty()) {
        m_commitsTable->setRowCount(1);
        m_commitsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No repository loaded. Open a Git repository to inspect commits.")));
        m_commitsTable->setSpan(0, 0, 1, 4);
        if (m_commitsCountBadge) m_commitsCountBadge->setText(QStringLiteral("0 COMMITS"));
        if (m_commitsBranchBadge) m_commitsBranchBadge->setText(QStringLiteral("● NO REPO"));
        if (m_commitsStatTotal) m_commitsStatTotal->setText(QStringLiteral("📦 0 Commits"));
        if (m_commitsStatAuthors) m_commitsStatAuthors->setText(QStringLiteral("👥 0 Authors"));
        if (m_commitsStatLatest) m_commitsStatLatest->setText(QStringLiteral("📅 --"));
        return;
    }

    if (!m_analyticsService.isLoaded() && !m_analyticsService.lastError().isEmpty()) {
        m_commitsTable->setRowCount(1);
        m_commitsTable->setItem(0, 0, makeMsgItem(QStringLiteral("Failed to retrieve commit history: %1").arg(m_analyticsService.lastError())));
        m_commitsTable->setSpan(0, 0, 1, 4);
        return;
    }

    const auto &commits = m_analyticsService.commits();
    if (commits.isEmpty()) {
        m_commitsTable->setRowCount(1);
        m_commitsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No commits found in this repository.")));
        m_commitsTable->setSpan(0, 0, 1, 4);
        if (m_commitsCountBadge) m_commitsCountBadge->setText(QStringLiteral("0 COMMITS"));
        return;
    }

    // Update Header Badges
    if (m_commitsCountBadge) {
        m_commitsCountBadge->setText(QStringLiteral("%1 COMMITS").arg(commits.size()));
    }
    if (m_commitsBranchBadge) {
        m_commitsBranchBadge->setText(QStringLiteral("● %1").arg(m_currentBranch.isEmpty() ? QStringLiteral("HEAD") : m_currentBranch));
    }

    // Update Quick Stat Chips
    if (m_commitsStatTotal) {
        m_commitsStatTotal->setText(QStringLiteral("📦 %1 Commits").arg(commits.size()));
    }
    if (m_commitsStatAuthors) {
        m_commitsStatAuthors->setText(QStringLiteral("👥 %1 Authors").arg(m_analyticsService.contributors().size()));
    }
    if (m_commitsStatLatest) {
        m_commitsStatLatest->setText(QStringLiteral("📅 %1").arg(commits.first().date));
    }

    QFont monoFont(QStringLiteral("Consolas"));
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(9);

    m_commitsTable->setRowCount(static_cast<int>(commits.size()));
    for (int row = 0; row < commits.size(); ++row) {
        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            return item;
        };

        auto *hashItem = makeItem(commits[row].hash);
        hashItem->setFont(monoFont);
        hashItem->setForeground(QColor(QStringLiteral("#ff2d95")));
        hashItem->setToolTip(QStringLiteral("Commit: %1\nClick to inspect").arg(commits[row].hash));

        auto *subjectItem = makeItem(commits[row].subject);
        subjectItem->setForeground(QColor(QStringLiteral("#f1f5f9")));
        subjectItem->setToolTip(commits[row].subject);

        const QString resolvedAuthor = GitAnalyticsService::canonicalContributorName(commits[row].author, commits[row].authorEmail);
        auto *authorItem = makeItem(resolvedAuthor);
        if (resolvedAuthor == QStringLiteral("Tanuja 658")) {
            authorItem->setForeground(QColor(QStringLiteral("#e000ff")));
        } else {
            authorItem->setForeground(QColor(QStringLiteral("#22d3ee")));
        }
        if (commits[row].author != resolvedAuthor && !commits[row].author.isEmpty()) {
            authorItem->setToolTip(QStringLiteral("Canonical: %1\nOriginal author: %2%3")
                .arg(resolvedAuthor, commits[row].author, commits[row].authorEmail.isEmpty() ? QString() : QStringLiteral(" <%1>").arg(commits[row].authorEmail)));
        } else {
            authorItem->setToolTip(resolvedAuthor);
        }

        auto *dateItem = makeItem(commits[row].date);
        dateItem->setForeground(QColor(QStringLiteral("#94a3b8")));
        dateItem->setToolTip(commits[row].date);

        m_commitsTable->setItem(row, 0, hashItem);
        m_commitsTable->setItem(row, 1, subjectItem);
        m_commitsTable->setItem(row, 2, authorItem);
        m_commitsTable->setItem(row, 3, dateItem);
    }

    if (m_commitsFilterInput && !m_commitsFilterInput->text().isEmpty()) {
        onCommitFilterChanged(m_commitsFilterInput->text());
    }
}

void MainWindow::loadContributors()
{
    if (!m_contributorsTable) {
        return;
    }

    m_contributorsTable->setRowCount(0);

    auto makeMsgItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor(QStringLiteral("#7a7d76")));
        return item;
    };

    if (m_repositoryPath.isEmpty()) {
        m_contributorsTable->setRowCount(1);
        m_contributorsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No commits found in this repository.")));
        m_contributorsTable->setSpan(0, 0, 1, 3);
        return;
    }

    if (!m_analyticsService.isLoaded() && !m_analyticsService.lastError().isEmpty()) {
        m_contributorsTable->setRowCount(1);
        m_contributorsTable->setItem(0, 0, makeMsgItem(QStringLiteral("Failed to retrieve contributor statistics: %1").arg(m_analyticsService.lastError())));
        m_contributorsTable->setSpan(0, 0, 1, 3);
        return;
    }

    const auto &contributors = m_analyticsService.contributors();
    if (contributors.isEmpty()) {
        m_contributorsTable->setRowCount(1);
        m_contributorsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No commits found in this repository.")));
        m_contributorsTable->setSpan(0, 0, 1, 3);
        return;
    }

    m_contributorsTable->setSortingEnabled(false);
    m_contributorsTable->setRowCount(static_cast<int>(contributors.size()));

    for (int row = 0; row < contributors.size(); ++row) {
        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            return item;
        };

        auto *authorItem = makeItem(contributors[row].name);
        authorItem->setForeground(QColor(QStringLiteral("#f1f5f9")));

        auto *countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, contributors[row].commits);
        countItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        countItem->setForeground(QColor(QStringLiteral("#22d3ee")));

        auto *dateItem = makeItem(contributors[row].latestDate);
        dateItem->setForeground(QColor(QStringLiteral("#94a3b8")));

        m_contributorsTable->setItem(row, 0, authorItem);
        m_contributorsTable->setItem(row, 1, countItem);
        m_contributorsTable->setItem(row, 2, dateItem);
    }

    m_contributorsTable->setSortingEnabled(true);
    m_contributorsTable->sortItems(1, Qt::DescendingOrder);

    // Update stat chips
    int totalCommits = 0;
    QString latestDate;
    for (const auto &c : contributors) {
        totalCommits += c.commits;
        if (latestDate.isEmpty() || c.latestDate > latestDate)
            latestDate = c.latestDate;
    }
    if (m_contribStatTotal)
        m_contribStatTotal->setText(
            QStringLiteral("👥  %1 Author%2").arg(contributors.size()).arg(contributors.size() == 1 ? "" : "s"));
    if (m_contribStatCommits)
        m_contribStatCommits->setText(
            QStringLiteral("✦  %1 Commit%2").arg(totalCommits).arg(totalCommits == 1 ? "" : "s"));
    if (m_contribStatLatest)
        m_contribStatLatest->setText(
            QStringLiteral("🗓  Latest: %1").arg(latestDate.isEmpty() ? QStringLiteral("—") : latestDate));
}

void MainWindow::loadHotspots()
{
    if (!m_hotspotsTable) {
        return;
    }

    m_hotspotsTable->setRowCount(0);

    auto makeMsgItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor(QStringLiteral("#7a7d76")));
        return item;
    };

    if (m_repositoryPath.isEmpty()) {
        m_hotspotsTable->setRowCount(1);
        m_hotspotsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No file change history found in this repository.")));
        m_hotspotsTable->setSpan(0, 0, 1, 3);
        return;
    }

    if (!m_analyticsService.isLoaded() && !m_analyticsService.lastError().isEmpty()) {
        m_hotspotsTable->setRowCount(1);
        m_hotspotsTable->setItem(0, 0, makeMsgItem(QStringLiteral("Failed to retrieve hotspot analysis: %1").arg(m_analyticsService.lastError())));
        m_hotspotsTable->setSpan(0, 0, 1, 3);
        return;
    }

    const auto &hotspots = m_analyticsService.hotspots();
    if (hotspots.isEmpty()) {
        m_hotspotsTable->setRowCount(1);
        m_hotspotsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No file change history found in this repository.")));
        m_hotspotsTable->setSpan(0, 0, 1, 3);
        return;
    }

    // Install delegate (Qt replaces any previously set delegate safely)
    m_hotspotsTable->setItemDelegate(new HotspotDelegate(m_hotspotsTable));

    // Compute max commit count for proportional heat bars
    int maxCommits = 0;
    QString latestDate;
    for (const auto &h : hotspots) {
        if (h.commits > maxCommits) maxCommits = h.commits;
        if (latestDate.isEmpty() || h.latestDate > latestDate)
            latestDate = h.latestDate;
    }

    m_hotspotsTable->setSortingEnabled(false);
    m_hotspotsTable->setRowCount(static_cast<int>(hotspots.size()));

    for (int row = 0; row < hotspots.size(); ++row) {
        // Col 0: file path (delegate renders dir prefix + bold filename)
        auto *fileItem = new QTableWidgetItem(hotspots[row].filePath);
        fileItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // Col 1: commit count + max stored for heat bar proportion
        auto *countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, hotspots[row].commits);
        countItem->setData(HotspotMaxRole, maxCommits);
        countItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Col 2: date (delegate renders pill)
        auto *dateItem = new QTableWidgetItem(hotspots[row].latestDate);
        dateItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_hotspotsTable->setItem(row, 0, fileItem);
        m_hotspotsTable->setItem(row, 1, countItem);
        m_hotspotsTable->setItem(row, 2, dateItem);
    }

    m_hotspotsTable->setSortingEnabled(true);
    m_hotspotsTable->sortItems(1, Qt::DescendingOrder);

    // Update stat chips
    if (m_hotspotStatFiles)
        m_hotspotStatFiles->setText(
            QStringLiteral("📁  %1 File%2").arg(hotspots.size()).arg(hotspots.size() == 1 ? "" : "s"));
    if (m_hotspotStatTopCount)
        m_hotspotStatTopCount->setText(
            QStringLiteral("🔥  %1 Changes (top file)").arg(maxCommits));
    if (m_hotspotStatLatest)
        m_hotspotStatLatest->setText(
            QStringLiteral("🗓  Latest: %1").arg(latestDate.isEmpty() ? QStringLiteral("—") : latestDate));
}

void MainWindow::loadRepositoryHealth()
{
    if (!m_healthTable) {
        return;
    }

    // Helper to reset all snapshot row labels to a placeholder
    auto resetSnapshotRows = [this](const QString &placeholder) {
        const QList<QLabel*> vals = {
            m_hrRootVal, m_hrBranchVal, m_hrCommitsVal,
            m_hrContribVal, m_hrFilesVal, m_hrTreeVal, m_hrLatestVal
        };
        for (QLabel *l : vals)
            if (l) l->setText(placeholder);
        if (m_hrWtStatus)    m_hrWtStatus->setText(placeholder);
        if (m_hrWtDetail)    m_hrWtDetail->setText(placeholder);
        if (m_hrCommitHash)  m_hrCommitHash->setText(placeholder);
        if (m_hrCommitDate)  m_hrCommitDate->setText(placeholder);
        if (m_hrCommitMsg)   m_hrCommitMsg->setText(placeholder);
    };

    if (m_repositoryPath.isEmpty()) {
        if (m_healthCommitsValue)      m_healthCommitsValue->setText(QStringLiteral("—"));
        if (m_healthContributorsValue) m_healthContributorsValue->setText(QStringLiteral("—"));
        if (m_healthFilesValue)        m_healthFilesValue->setText(QStringLiteral("—"));
        if (m_healthBranchValue)       m_healthBranchValue->setText(QStringLiteral("—"));
        resetSnapshotRows(QStringLiteral("—"));
        if (m_hrStatusOverlay) {
            m_hrStatusOverlay->setText(QStringLiteral("No repository loaded. Open a Git repository to view health metrics."));
            m_hrStatusOverlay->show();
        }
        // Keep hidden table consistent
        m_healthTable->setRowCount(0);
        return;
    }

    if (!m_healthService.isLoaded()) {
        if (m_healthCommitsValue)      m_healthCommitsValue->setText(QStringLiteral("Error"));
        if (m_healthContributorsValue) m_healthContributorsValue->setText(QStringLiteral("Error"));
        if (m_healthFilesValue)        m_healthFilesValue->setText(QStringLiteral("Error"));
        if (m_healthBranchValue)       m_healthBranchValue->setText(QStringLiteral("Error"));
        const QString errText = QStringLiteral("Failed to evaluate repository health metrics: %1")
                                    .arg(m_healthService.lastError());
        resetSnapshotRows(QStringLiteral("—"));
        if (m_hrStatusOverlay) {
            m_hrStatusOverlay->setText(errText);
            m_hrStatusOverlay->show();
        }
        m_healthTable->setRowCount(0);
        return;
    }

    // Hide overlay — we have data
    if (m_hrStatusOverlay) m_hrStatusOverlay->hide();

    const RepositoryHealthData &data = m_healthService.healthData();

    // --- KPI cards ---
    if (m_healthCommitsValue)
        m_healthCommitsValue->setText(QString::number(data.totalCommits));
    if (m_healthContributorsValue)
        m_healthContributorsValue->setText(QString::number(data.uniqueContributors));
    if (m_healthFilesValue)
        m_healthFilesValue->setText(QString::number(data.trackedFiles));
    if (m_healthBranchValue)
        m_healthBranchValue->setText(data.currentBranch.isEmpty() ? QStringLiteral("—") : data.currentBranch);

    // --- Repository Snapshot panel: per-row labels ---
    // metricsTable order: Root, Branch, Commits, Contributors, Files, WorkingTree, LatestCommit
    const QList<QLabel*> rowVals = {
        m_hrRootVal, m_hrBranchVal, m_hrCommitsVal,
        m_hrContribVal, m_hrFilesVal, m_hrTreeVal, m_hrLatestVal
    };
    for (int r = 0; r < data.metricsTable.size() && r < rowVals.size(); ++r) {
        if (!rowVals[r]) continue;
        const QString &val   = data.metricsTable[r].value;
        const QString &label = data.metricsTable[r].name;
        // Show "value — detail" for richer context in the snapshot panel
        const QString &detail = data.metricsTable[r].detail;
        rowVals[r]->setText(
            QStringLiteral("%1\n%2").arg(val, detail)
        );
        // Colour the Working Tree value
        if (label.contains(QStringLiteral("Working Tree"), Qt::CaseInsensitive)) {
            rowVals[r]->setStyleSheet(
                val.contains(QStringLiteral("Clean"), Qt::CaseInsensitive)
                    ? QStringLiteral("color: #34d399;")
                    : QStringLiteral("color: #ffb547;")
            );
        } else {
            rowVals[r]->setStyleSheet(QString());
        }
    }

    // --- Right panel: Working Tree card ---
    const bool isClean = data.workingTreeStatus.contains(QStringLiteral("Clean"), Qt::CaseInsensitive);
    if (m_hrWtStatus) {
        const QString wtIcon = isClean ? QStringLiteral("✅  ") : QStringLiteral("⚠️  ");
        m_hrWtStatus->setText(wtIcon + data.workingTreeStatus);
        m_hrWtStatus->setObjectName(isClean
            ? QStringLiteral("hrWtStatusClean")
            : QStringLiteral("hrWtStatusDirty"));
        m_hrWtStatus->style()->polish(m_hrWtStatus);
    }
    if (m_hrWtDetail)
        m_hrWtDetail->setText(data.workingTreeDetail);

    // --- Right panel: Latest Commit card ---
    // latestCommitText = "abc1234 (2026-10-01)"   latestCommitDetail = "commit message"
    if (data.totalCommits > 0 && !data.latestCommitText.isEmpty()) {
        // Split "hash (date)" into separate pieces
        const QString full = data.latestCommitText; // e.g. "abc1234 (2026-10-01)"
        const int paren = full.indexOf(u'(');
        if (paren != -1) {
            const QString hash = full.left(paren).trimmed();
            const QString date = full.mid(paren + 1).remove(u')').trimmed();
            if (m_hrCommitHash) m_hrCommitHash->setText(hash);
            if (m_hrCommitDate) m_hrCommitDate->setText(date);
        } else {
            if (m_hrCommitHash) m_hrCommitHash->setText(full);
            if (m_hrCommitDate) m_hrCommitDate->setText(QStringLiteral("—"));
        }
        if (m_hrCommitMsg) m_hrCommitMsg->setText(data.latestCommitDetail);
    } else {
        if (m_hrCommitHash) m_hrCommitHash->setText(QStringLiteral("—"));
        if (m_hrCommitDate) m_hrCommitDate->setText(QStringLiteral("No commits yet"));
        if (m_hrCommitMsg)  m_hrCommitMsg->setText(QStringLiteral("—"));
    }

    // Keep hidden table row count consistent (not shown, but keeps the widget valid)
    m_healthTable->setRowCount(0);
}

void MainWindow::applyTheme()
{
    Theme::applyTheme(this);
}

// ---------------------------------------------------------------------------
// Gear state management
// ---------------------------------------------------------------------------

void MainWindow::updateGearState()
{
    if (!m_gearWidget)
        return;

    // The gear rotates only when:
    //  1. A repository has been successfully loaded (m_repositoryPath is set), AND
    //  2. The Dashboard page (index 1 in m_contentStack) is the active page.
    const bool dashboardActive =
        m_contentStack && (m_contentStack->currentIndex() == 1);
    const bool repoLoaded = !m_repositoryPath.isEmpty();

    m_gearWidget->setActive(repoLoaded && dashboardActive);
}

void MainWindow::loadDashboard()
{
    if (!m_dashboardWidget) {
        return;
    }

    if (m_repositoryPath.isEmpty() || !m_healthService.isLoaded() || !m_analyticsService.isLoaded()) {
        m_dashboardWidget->clearData();
        return;
    }

    m_dashboardWidget->updateData(m_healthService.healthData(),
                                 m_analyticsService,
                                 m_repositoryPath,
                                 m_currentBranch);
}

