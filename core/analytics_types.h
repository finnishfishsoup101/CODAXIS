#ifndef ANALYTICS_TYPES_H
#define ANALYTICS_TYPES_H

#include <QString>
#include <QList>
#include <QDate>

/**
 * @brief Represents commit count on a specific calendar date within an activity window.
 */
struct CommitActivityBucket {
    QDate date;
    int commitCount{0};
};

/**
 * @brief Aggregated commit activity series across a time window.
 */
struct CommitActivityData {
    QList<CommitActivityBucket> buckets;
    int totalCommits{0};
    bool isTruncated{false}; // True if older commits in the window were omitted due to query capping
};

/**
 * @brief Represents an individual commit record parsed from the Git log.
 */
struct CommitRecord {
    QString hash;
    QString subject;
    QString author;
    QString date;
    QString authorEmail{};
};

/**
 * @brief Represents aggregated statistics for a repository contributor.
 */
struct ContributorRecord {
    QString name;
    int commits{0};
    QString latestDate;
};

/**
 * @brief Represents change frequency (churn) analysis for a repository file.
 */
struct HotspotRecord {
    QString filePath;
    int commits{0};
    QString latestDate;
};

/**
 * @brief Represents a single key-value metric entry in the detailed health table.
 */
struct HealthMetricItem {
    QString name;
    QString value;
    QString detail;
};

/**
 * @brief Comprehensive repository health and integrity state.
 * Contains top-level summary metrics (cards) and detailed table rows.
 */
struct RepositoryHealthData {
    int totalCommits{0};
    int uniqueContributors{0};
    int trackedFiles{0};
    QString currentBranch;
    QString workingTreeStatus{QStringLiteral("Clean")};
    QString workingTreeDetail{QStringLiteral("Working directory matches HEAD with no uncommitted changes.")};
    QString latestCommitText{QStringLiteral("No commits yet")};
    QString latestCommitDetail{QStringLiteral("Repository has an unborn or empty commit history.")};
    QList<HealthMetricItem> metricsTable;
};

#endif // ANALYTICS_TYPES_H
