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
