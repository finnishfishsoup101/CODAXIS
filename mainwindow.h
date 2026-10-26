#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <vector>
#include "core/git_client.h"
#include "core/git_service.h"
#include "core/repository_health_service.h"
#include "core/git_analytics_service.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QPushButton;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QTableWidget;
QT_END_NAMESPACE

class GearWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    bool validateAndOpenRepository(const QString &dirPath);

private slots:
    void onOpenRepositoryClicked();
    void onNavigationClicked(int index);
    void onCommitFilterChanged(const QString &text);

private:
    void setupShellUi();
    void applyTheme();
    void loadCommitHistory();
    void loadContributors();
    void loadHotspots();
    void loadRepositoryHealth();
    void loadDashboard();
    /** Drive the gear: rotating when repo loaded + Dashboard active. */
    void updateGearState();

    Ui::MainWindow *ui;
    QList<QPushButton*> m_navButtons;
    QLabel *m_sidebarStatusLabel{nullptr};
    QLabel *m_statusLabel{nullptr};
    QStackedWidget *m_contentStack{nullptr};
    class GearWidget *m_gearWidget{nullptr};
    class DashboardWidget *m_dashboardWidget{nullptr};
    QTableWidget *m_commitsTable{nullptr};
    QLineEdit *m_commitsFilterInput{nullptr};
    QLabel *m_commitsCountBadge{nullptr};
    QLabel *m_commitsBranchBadge{nullptr};
    QLabel *m_commitsStatTotal{nullptr};
    QLabel *m_commitsStatAuthors{nullptr};
    QLabel *m_commitsStatLatest{nullptr};
    QTableWidget *m_contributorsTable{nullptr};
    QLabel *m_contribStatTotal{nullptr};
    QLabel *m_contribStatCommits{nullptr};
    QLabel *m_contribStatLatest{nullptr};
    QTableWidget *m_hotspotsTable{nullptr};
    QLabel *m_hotspotStatFiles{nullptr};
    QLabel *m_hotspotStatTopCount{nullptr};
    QLabel *m_hotspotStatLatest{nullptr};
    QLabel *m_healthCommitsValue{nullptr};
    QLabel *m_healthContributorsValue{nullptr};
    QLabel *m_healthFilesValue{nullptr};
    QLabel *m_healthBranchValue{nullptr};
    QLabel *m_healthWorkingTreeChip{nullptr};
    QLabel *m_healthLatestCommitChip{nullptr};
    QTableWidget *m_healthTable{nullptr};
    // Per-row labels for the Repository Snapshot panel (left column)
    QLabel *m_hrRootVal{nullptr};
    QLabel *m_hrBranchVal{nullptr};
    QLabel *m_hrCommitsVal{nullptr};
    QLabel *m_hrContribVal{nullptr};
    QLabel *m_hrFilesVal{nullptr};
    QLabel *m_hrTreeVal{nullptr};
    QLabel *m_hrLatestVal{nullptr};
    // Right panel — Working Tree card
    QLabel *m_hrWtStatus{nullptr};
    QLabel *m_hrWtDetail{nullptr};
    // Right panel — Latest Commit card
    QLabel *m_hrCommitHash{nullptr};
    QLabel *m_hrCommitDate{nullptr};
    QLabel *m_hrCommitMsg{nullptr};
    // Status overlay label (shown when no/error state)
    QLabel *m_hrStatusOverlay{nullptr};

    QString m_repositoryPath;
    QString m_currentBranch;
    // Classes and objects: the three declarations below are objects -- each one is
    // an instance of its class, created automatically when MainWindow is constructed.
    GitClient m_gitClient;
    RepositoryHealthService m_healthService;
    GitAnalyticsService m_analyticsService;

    // Non-owning base-class pointers for polymorphic lifecycle dispatch.
    // Points into the value members above; lifetime is safe for the duration of MainWindow.
    std::vector<GitService*> m_services;
};
#endif // MAINWINDOW_H
