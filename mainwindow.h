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
    QTableWidget *m_contributorsTable{nullptr};
    QTableWidget *m_hotspotsTable{nullptr};
    QLabel *m_healthCommitsValue{nullptr};
    QLabel *m_healthContributorsValue{nullptr};
    QLabel *m_healthFilesValue{nullptr};
    QLabel *m_healthBranchValue{nullptr};
    QTableWidget *m_healthTable{nullptr};

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
