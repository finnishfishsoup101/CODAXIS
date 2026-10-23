#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include <QWidget>
#include <QList>
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QSplineSeries>
#include <QAreaSeries>
#include <QScatterSeries>
#include <QPieSeries>
#include <QPieSlice>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include "core/analytics_types.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QTableWidget;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;
class QScrollArea;
class QGraphicsDropShadowEffect;
QT_END_NAMESPACE

class GitAnalyticsService;

/**
 * @brief Snapping-tooltip chart view for the commit activity timeline.
 *
 * Finds the nearest plotted data point in pixel space to avoid "0 commits"
 * tooltip errors from unreliable chart-coordinate rounding.
 */
class ActivityChartView : public QChartView {
    Q_OBJECT
public:
    explicit ActivityChartView(QWidget *parent = nullptr);
    void setActivityData(const CommitActivityData &data, QLineSeries *lineSeries);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    CommitActivityData m_data;
    QLineSeries *m_lineSeries{nullptr};
    QScatterSeries *m_highlightSeries{nullptr};
};

/**
 * @brief Interactive KPI card with hover border glow and lift effect.
 */
class KpiCard : public QFrame {
    Q_OBJECT
public:
    KpiCard(const QString &icon, const QString &title, const QColor &accentColor, QWidget *parent = nullptr);
    void setValue(const QString &val);
    void setSubText(const QString &sub);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QColor m_accentColor;
    QLabel *m_valLabel{nullptr};
    QLabel *m_subLabel{nullptr};
    QGraphicsDropShadowEffect *m_shadow{nullptr};
};

/**
 * @brief Interactive file hotspot row with hover border, tooltip, and gradient progress bar.
 */
class HotspotRowWidget : public QFrame {
    Q_OBJECT
public:
    HotspotRowWidget(int rank, const HotspotRecord &hotspot, int maxChurn, QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

/**
 * @brief Prism Glass analytics dashboard for CODAXIS.
 * Shows donut + bar simultaneously, contributor-activity-over-time chart,
 * commit timeline with snapping date tooltips, file hotspots, and recent revisions.
 */
class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget() override = default;

    void updateData(const RepositoryHealthData &health,
                    const GitAnalyticsService &analytics,
                    const QString &repoPath,
                    const QString &branch);
    void clearData();

private slots:
    void onWindow30Clicked();
    void onWindow90Clicked();

private:
    void setupUi();
    void updateKpiCards(const RepositoryHealthData &health, const GitAnalyticsService &analytics);
    void updateCommitChart();
    void updateContributorChart(const QList<ContributorRecord> &contributors);
    void updateContributorActivityChart(const QList<CommitRecord> &commits,
                                        const QList<ContributorRecord> &contributors);
    void updateHotspotsList(const QList<HotspotRecord> &hotspots);
    void updateRecentCommitsTable(const QList<CommitRecord> &commits);

    // Repository header
    QLabel *m_repoPathLabel{nullptr};
    QLabel *m_branchBadge{nullptr};
    QLabel *m_statusPill{nullptr};

    // KPI Cards
    KpiCard *m_commitsCard{nullptr};
    KpiCard *m_contributorsCard{nullptr};
    KpiCard *m_filesCard{nullptr};
    KpiCard *m_workingTreeCard{nullptr};

    // Commit activity timeline
    QPushButton *m_btnWindow30{nullptr};
    QPushButton *m_btnWindow90{nullptr};
    ActivityChartView *m_commitChartView{nullptr};
    QLabel *m_commitChartEmptyLabel{nullptr};
    QLabel *m_commitChartSubLabel{nullptr};
    int m_currentWindowDays{30};

    // Contributor distribution — donut + bar simultaneously (no toggle)
    QChartView *m_contribDonutView{nullptr};
    QChartView *m_contribBarView{nullptr};
    QWidget *m_contribChartsWidget{nullptr};
    QLabel *m_contributorChartEmptyLabel{nullptr};
    QVBoxLayout *m_contribLegendLayout{nullptr};
    QWidget *m_contribLegendWidget{nullptr};

    // Contributor activity over time
    QChartView *m_contribActivityView{nullptr};
    QLabel *m_contribActivityEmptyLabel{nullptr};

    // Hotspots container
    QVBoxLayout *m_hotspotsContainerLayout{nullptr};
    QLabel *m_hotspotsEmptyLabel{nullptr};

    // Recent commits table
    QTableWidget *m_recentCommitsTable{nullptr};

    // Cached data
    QList<CommitRecord> m_cachedCommits;
    QList<ContributorRecord> m_cachedContributors;
    bool m_hasLoadedRepo{false};
};

#endif // DASHBOARD_WIDGET_H
