#ifndef GIT_ANALYTICS_SERVICE_H
#define GIT_ANALYTICS_SERVICE_H

#include "git_service.h"
#include "analytics_types.h"

#include <QList>

/**
 * @brief Service responsible for parsing Git commit history,
 * aggregating contributor distributions, and identifying file churn hotspots.
 */
class GitAnalyticsService : public GitService {
public:
    explicit GitAnalyticsService(GitClient *client);
    ~GitAnalyticsService() override = default;

    // GitService interface implementation
    QString serviceName() const override;
    bool refresh(const QString &repoPath) override;
    void clear() override;
    QString lastError() const override;

    // Domain data accessors
    const QList<CommitRecord>& commits() const;
    const QList<ContributorRecord>& contributors() const;
    const QList<HotspotRecord>& hotspots() const;
    bool isLoaded() const;

    // Time-window commit activity aggregation
    static CommitActivityData aggregateCommitActivity(const QList<CommitRecord> &commits,
                                                      int windowDays = 30,
                                                      const QDate &referenceDate = QDate::currentDate());
    CommitActivityData commitActivity(int windowDays = 30,
                                      const QDate &referenceDate = QDate::currentDate()) const;

    // Contributor aggregation and identity normalization
    struct ContributorLogEntry {
        QString name;
        QString email;
        QString date;
    };
    static QString normalizeContributorKey(const QString &name, const QString &email);
    static QString canonicalContributorName(const QString &name, const QString &email);
    static QList<ContributorRecord> aggregateContributors(const QList<ContributorLogEntry> &entries);
    static QList<ContributorRecord> parseContributorLogOutput(const QString &logOutput);

private:
    bool parseCommits(const QString &repoPath);
    bool parseContributors(const QString &repoPath);
    bool parseHotspots(const QString &repoPath);

    QList<CommitRecord> m_commits;
    QList<ContributorRecord> m_contributors;
    QList<HotspotRecord> m_hotspots;
    QString m_lastError;
    bool m_isLoaded{false};
};

#endif // GIT_ANALYTICS_SERVICE_H
