#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ui/theme.h"
#include "ui/repository_canvas.h"
#include <QGraphicsDropShadowEffect>

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
    auto *brandLogo = new QLabel(QStringLiteral("CODAXIS"), sidebarWidget);
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
        QStringLiteral("Overview"),
        QStringLiteral("Commits"),
        QStringLiteral("Contributors"),
        QStringLiteral("Hotspots"),
        QStringLiteral("Repository Health")
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
    cardShadow->setBlurRadius(20);
    cardShadow->setColor(QColor(50, 45, 38, 16));
    cardShadow->setOffset(0, 4);
    welcomeCard->setGraphicsEffect(cardShadow);

    auto *welcomeLayout = new QVBoxLayout(welcomeCard);
    welcomeLayout->setContentsMargins(34, 30, 34, 30);
    welcomeLayout->setSpacing(0);

    // 1. Eyebrow badge
    auto *badgeRow = new QHBoxLayout();
    badgeRow->setContentsMargins(0, 0, 0, 0);
    auto *badge = new QLabel(QStringLiteral("REPOSITORY ARCHITECTURE & HEALTH"), welcomeCard);
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
    auto *openRepoBtn = new QPushButton(QStringLiteral("Open Repository"), welcomeCard);
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

    pillarsRow->addWidget(createPillar(QStringLiteral("CHURN & HOTSPOTS"),
                                       QStringLiteral("Track high-frequency file changes and active development areas.")));
    pillarsRow->addWidget(createPillar(QStringLiteral("CONTRIBUTORS"),
                                       QStringLiteral("Aggregate authorship patterns and collaborative dynamics.")));
    pillarsRow->addWidget(createPillar(QStringLiteral("INTEGRITY"),
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

    // --- Page 1: Commits ---
    auto *commitsPage = new QWidget();
    commitsPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *commitsPageLayout = new QVBoxLayout(commitsPage);
    commitsPageLayout->setContentsMargins(28, 24, 28, 24);
    commitsPageLayout->setSpacing(14);

    auto *commitsHeaderLayout = new QVBoxLayout();
    commitsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    commitsHeaderLayout->setSpacing(4);

    auto *commitsBadge = new QLabel(QStringLiteral("VERSION CONTROL TIMELINE"), commitsPage);
    commitsBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *commitsBadgeRow = new QHBoxLayout();
    commitsBadgeRow->setContentsMargins(0, 0, 0, 0);
    commitsBadgeRow->addWidget(commitsBadge);
    commitsBadgeRow->addStretch(1);

    auto *commitsHeader = new QLabel(QStringLiteral("Commit History"), commitsPage);
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

    m_commitsTable = new QTableWidget(0, 4, commitsPage);
    m_commitsTable->setObjectName(QStringLiteral("commitsTable"));
    m_commitsTable->setHorizontalHeaderLabels({
        QStringLiteral("Hash"),
        QStringLiteral("Message"),
        QStringLiteral("Author"),
        QStringLiteral("Date")
    });
    m_commitsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_commitsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_commitsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_commitsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_commitsTable->verticalHeader()->setVisible(false);
    m_commitsTable->verticalHeader()->setDefaultSectionSize(32);
    m_commitsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commitsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_commitsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_commitsTable->setAlternatingRowColors(true);
    m_commitsTable->setShowGrid(false);
    m_commitsTable->setSortingEnabled(false);

    commitsPageLayout->addLayout(commitsHeaderLayout);
    commitsPageLayout->addWidget(m_commitsTable, 1);

    m_contentStack->addWidget(commitsPage); // index 1

    // --- Page 2: Contributors ---
    auto *contributorsPage = new QWidget();
    contributorsPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *contributorsLayout = new QVBoxLayout(contributorsPage);
    contributorsLayout->setContentsMargins(28, 24, 28, 24);
    contributorsLayout->setSpacing(14);

    auto *contribHeaderLayout = new QVBoxLayout();
    contribHeaderLayout->setContentsMargins(0, 0, 0, 0);
    contribHeaderLayout->setSpacing(4);

    auto *contributorsBadge = new QLabel(QStringLiteral("AUTHORSHIP & COLLABORATION"), contributorsPage);
    contributorsBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *contribBadgeRow = new QHBoxLayout();
    contribBadgeRow->setContentsMargins(0, 0, 0, 0);
    contribBadgeRow->addWidget(contributorsBadge);
    contribBadgeRow->addStretch(1);

    auto *contributorsHeader = new QLabel(QStringLiteral("Contributors"), contributorsPage);
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
    m_contributorsTable->setObjectName(QStringLiteral("commitsTable"));
    m_contributorsTable->setHorizontalHeaderLabels({
        QStringLiteral("Author"),
        QStringLiteral("Commits"),
        QStringLiteral("Latest Commit")
    });
    m_contributorsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_contributorsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_contributorsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_contributorsTable->verticalHeader()->setVisible(false);
    m_contributorsTable->verticalHeader()->setDefaultSectionSize(32);
    m_contributorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_contributorsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_contributorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_contributorsTable->setAlternatingRowColors(true);
    m_contributorsTable->setShowGrid(false);
    m_contributorsTable->setSortingEnabled(true);

    contributorsLayout->addLayout(contribHeaderLayout);
    contributorsLayout->addWidget(m_contributorsTable, 1);

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

    auto *hotspotsBadge = new QLabel(QStringLiteral("CODE CHURN & RISK ANALYSIS"), hotspotsPage);
    hotspotsBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *hotspotsBadgeRow = new QHBoxLayout();
    hotspotsBadgeRow->setContentsMargins(0, 0, 0, 0);
    hotspotsBadgeRow->addWidget(hotspotsBadge);
    hotspotsBadgeRow->addStretch(1);

    auto *hotspotsHeader = new QLabel(QStringLiteral("Hotspots"), hotspotsPage);
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
    m_hotspotsTable->setObjectName(QStringLiteral("commitsTable"));
    m_hotspotsTable->setHorizontalHeaderLabels({
        QStringLiteral("File Path"),
        QStringLiteral("Commits"),
        QStringLiteral("Latest Change")
    });
    m_hotspotsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_hotspotsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_hotspotsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_hotspotsTable->verticalHeader()->setVisible(false);
    m_hotspotsTable->verticalHeader()->setDefaultSectionSize(32);
    m_hotspotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hotspotsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_hotspotsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_hotspotsTable->setAlternatingRowColors(true);
    m_hotspotsTable->setShowGrid(false);
    m_hotspotsTable->setSortingEnabled(true);

    hotspotsLayout->addLayout(hotspotsHeaderLayout);
    hotspotsLayout->addWidget(m_hotspotsTable, 1);

    m_contentStack->addWidget(hotspotsPage); // index 3

    // --- Page 4: Repository Health ---
    auto *healthPage = new QWidget();
    healthPage->setObjectName(QStringLiteral("mainAreaWidget"));

    auto *healthLayout = new QVBoxLayout(healthPage);
    healthLayout->setContentsMargins(28, 24, 28, 24);
    healthLayout->setSpacing(14);

    auto *healthHeaderLayout = new QVBoxLayout();
    healthHeaderLayout->setContentsMargins(0, 0, 0, 0);
    healthHeaderLayout->setSpacing(4);

    auto *healthBadge = new QLabel(QStringLiteral("REPOSITORY INTEGRITY"), healthPage);
    healthBadge->setObjectName(QStringLiteral("pageBadge"));
    auto *healthBadgeRow = new QHBoxLayout();
    healthBadgeRow->setContentsMargins(0, 0, 0, 0);
    healthBadgeRow->addWidget(healthBadge);
    healthBadgeRow->addStretch(1);

    auto *healthHeader = new QLabel(QStringLiteral("Repository Health"), healthPage);
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

    // Summary metric cards row
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

    cardsRow->addWidget(createMetricCard(QStringLiteral("TOTAL COMMITS"), m_healthCommitsValue));
    cardsRow->addWidget(createMetricCard(QStringLiteral("CONTRIBUTORS"), m_healthContributorsValue));
    cardsRow->addWidget(createMetricCard(QStringLiteral("TRACKED FILES"), m_healthFilesValue));
    cardsRow->addWidget(createMetricCard(QStringLiteral("CURRENT BRANCH"), m_healthBranchValue));

    // Detailed metrics table
    m_healthTable = new QTableWidget(0, 3, healthPage);
    m_healthTable->setObjectName(QStringLiteral("commitsTable"));
    m_healthTable->setHorizontalHeaderLabels({
        QStringLiteral("Metric"),
        QStringLiteral("Value"),
        QStringLiteral("Details")
    });
    m_healthTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_healthTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_healthTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_healthTable->verticalHeader()->setVisible(false);
    m_healthTable->verticalHeader()->setDefaultSectionSize(32);
    m_healthTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_healthTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_healthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_healthTable->setAlternatingRowColors(true);
    m_healthTable->setShowGrid(false);

    healthLayout->addLayout(healthHeaderLayout);
    healthLayout->addLayout(cardsRow);
    healthLayout->addWidget(m_healthTable, 1);

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
    if (!m_gitClient.isGitAvailable()) {
        QMessageBox::critical(
            this,
            tr("Git Not Found"),
            tr("Could not find git.exe on this system.\nPlease ensure Git is installed.")
        );
        return false;
    }

    // 1. Verify that dirPath is inside a valid Git work tree
    QString workTreeError;
    if (!m_gitClient.isInsideWorkTree(dirPath, &workTreeError)) {
        m_repositoryPath.clear();
        m_currentBranch.clear();
        // Polymorphic clear: dispatch through the shared GitService interface.
        for (GitService *svc : m_services) {
            svc->clear();
        }

        if (m_sidebarStatusLabel) {
            m_sidebarStatusLabel->setText(QStringLiteral("● No repository loaded"));
        }
        if (m_statusLabel) {
            m_statusLabel->setText(QStringLiteral("No repository loaded"));
        }
        loadCommitHistory();
        loadContributors();
        loadHotspots();
        loadRepositoryHealth();

        const QString detail = workTreeError.isEmpty()
            ? tr("The directory does not appear to be a Git repository.")
            : workTreeError;

        QMessageBox::warning(
            this,
            tr("Invalid Repository"),
            tr("Cannot open '%1':\n\n%2").arg(QDir::toNativeSeparators(dirPath), detail)
        );
        return false;
    }

    // 2. Resolve top-level root (supports worktrees, subfolders, and submodules)
    const QString repoRoot = m_gitClient.getTopLevel(dirPath);
    const QString repoName = QFileInfo(repoRoot).fileName();

    // 3. Query current branch name or detached HEAD
    const QString branch = m_gitClient.getCurrentBranch(repoRoot);

    // 4. Update repository state
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

    return true;
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
        item->setForeground(QColor(QStringLiteral("#7a7d76")));
        return item;
    };

    if (m_repositoryPath.isEmpty()) {
        m_commitsTable->setRowCount(1);
        m_commitsTable->setItem(0, 0, makeMsgItem(QStringLiteral("No commits found in this repository.")));
        m_commitsTable->setSpan(0, 0, 1, 4);
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
        return;
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
        hashItem->setForeground(QColor(QStringLiteral("#45563d")));

        auto *subjectItem = makeItem(commits[row].subject);
        subjectItem->setForeground(QColor(QStringLiteral("#242724")));

        auto *authorItem = makeItem(commits[row].author);
        authorItem->setForeground(QColor(QStringLiteral("#4a4d46")));

        auto *dateItem = makeItem(commits[row].date);
        dateItem->setForeground(QColor(QStringLiteral("#686b63")));

        m_commitsTable->setItem(row, 0, hashItem);
        m_commitsTable->setItem(row, 1, subjectItem);
        m_commitsTable->setItem(row, 2, authorItem);
        m_commitsTable->setItem(row, 3, dateItem);
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
        authorItem->setForeground(QColor(QStringLiteral("#202320")));

        auto *countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, contributors[row].commits);
        countItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        countItem->setForeground(QColor(QStringLiteral("#384d31")));

        auto *dateItem = makeItem(contributors[row].latestDate);
        dateItem->setForeground(QColor(QStringLiteral("#686b63")));

        m_contributorsTable->setItem(row, 0, authorItem);
        m_contributorsTable->setItem(row, 1, countItem);
        m_contributorsTable->setItem(row, 2, dateItem);
    }

    m_contributorsTable->setSortingEnabled(true);
    m_contributorsTable->sortItems(1, Qt::DescendingOrder);
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

    QFont monoFont(QStringLiteral("Consolas"));
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(9);

    m_hotspotsTable->setSortingEnabled(false);
    m_hotspotsTable->setRowCount(static_cast<int>(hotspots.size()));

    for (int row = 0; row < hotspots.size(); ++row) {
        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            return item;
        };

        auto *fileItem = makeItem(hotspots[row].filePath);
        fileItem->setFont(monoFont);
        fileItem->setForeground(QColor(QStringLiteral("#242724")));

        auto *countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, hotspots[row].commits);
        countItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        countItem->setForeground(QColor(QStringLiteral("#526649")));

        auto *dateItem = makeItem(hotspots[row].latestDate);
        dateItem->setForeground(QColor(QStringLiteral("#686b63")));

        m_hotspotsTable->setItem(row, 0, fileItem);
        m_hotspotsTable->setItem(row, 1, countItem);
        m_hotspotsTable->setItem(row, 2, dateItem);
    }

    m_hotspotsTable->setSortingEnabled(true);
    m_hotspotsTable->sortItems(1, Qt::DescendingOrder);
}

void MainWindow::loadRepositoryHealth()
{
    if (!m_healthTable) {
        return;
    }

    auto makeMsgItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor(QStringLiteral("#7a7d76")));
        return item;
    };

    if (m_repositoryPath.isEmpty()) {
        // No repository: services were already cleared via the polymorphic loop in
        // validateAndOpenRepository. Just reset the UI labels.
        if (m_healthCommitsValue) m_healthCommitsValue->setText(QStringLiteral("—"));
        if (m_healthContributorsValue) m_healthContributorsValue->setText(QStringLiteral("—"));
        if (m_healthFilesValue) m_healthFilesValue->setText(QStringLiteral("—"));
        if (m_healthBranchValue) m_healthBranchValue->setText(QStringLiteral("—"));

        m_healthTable->setRowCount(1);
        m_healthTable->setItem(0, 0, makeMsgItem(QStringLiteral("No repository loaded. Open a Git repository to view health metrics.")));
        m_healthTable->setSpan(0, 0, 1, 3);
        return;
    }

    // Refresh is dispatched through the GitService* collection in validateAndOpenRepository.
    // Here we only read the data that the services loop has already populated.
    if (!m_healthService.isLoaded()) {
        if (m_healthCommitsValue) m_healthCommitsValue->setText(QStringLiteral("Error"));
        if (m_healthContributorsValue) m_healthContributorsValue->setText(QStringLiteral("Error"));
        if (m_healthFilesValue) m_healthFilesValue->setText(QStringLiteral("Error"));
        if (m_healthBranchValue) m_healthBranchValue->setText(QStringLiteral("Error"));

        m_healthTable->setRowCount(1);
        const QString errText = QStringLiteral("Failed to evaluate repository health metrics: %1")
                                    .arg(m_healthService.lastError());
        m_healthTable->setItem(0, 0, makeMsgItem(errText));
        m_healthTable->setSpan(0, 0, 1, 3);
        return;
    }

    const RepositoryHealthData &data = m_healthService.healthData();

    // Update summary card labels
    if (m_healthCommitsValue) {
        m_healthCommitsValue->setText(QString::number(data.totalCommits));
    }
    if (m_healthContributorsValue) {
        m_healthContributorsValue->setText(QString::number(data.uniqueContributors));
    }
    if (m_healthFilesValue) {
        m_healthFilesValue->setText(QString::number(data.trackedFiles));
    }
    if (m_healthBranchValue) {
        m_healthBranchValue->setText(data.currentBranch.isEmpty() ? QStringLiteral("—") : data.currentBranch);
    }

    // Populate metrics table
    m_healthTable->setSortingEnabled(false);
    m_healthTable->setRowCount(static_cast<int>(data.metricsTable.size()));

    auto makeItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };

    for (int r = 0; r < data.metricsTable.size(); ++r) {
        auto *nameItem = makeItem(data.metricsTable[r].name);
        nameItem->setForeground(QColor(QStringLiteral("#202320")));

        auto *valItem = makeItem(data.metricsTable[r].value);
        valItem->setForeground(QColor(QStringLiteral("#202320")));

        // Visual emphasis grounded strictly in actual data:
        if (data.metricsTable[r].name.contains(QStringLiteral("Working Tree"), Qt::CaseInsensitive)) {
            if (data.metricsTable[r].value.contains(QStringLiteral("Clean"), Qt::CaseInsensitive)) {
                valItem->setForeground(QColor(QStringLiteral("#526649")));
            }
        }

        auto *detailItem = makeItem(data.metricsTable[r].detail);
        detailItem->setForeground(QColor(QStringLiteral("#565954")));

        m_healthTable->setItem(r, 0, nameItem);
        m_healthTable->setItem(r, 1, valItem);
        m_healthTable->setItem(r, 2, detailItem);
    }
}

void MainWindow::applyTheme()
{
    Theme::applyTheme(this);
}
