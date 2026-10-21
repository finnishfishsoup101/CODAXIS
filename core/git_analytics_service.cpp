#include "git_analytics_service.h"
#include "git_client.h"

#include <QDateTime>
#include <QMap>
#include <algorithm>

GitAnalyticsService::GitAnalyticsService(GitClient *client)
    : GitService(client)
{
}

QString GitAnalyticsService::serviceName() const
{
    return QStringLiteral("Git Analytics Service");
}

QString GitAnalyticsService::lastError() const
{
    return m_lastError;
}

const QList<CommitRecord>& GitAnalyticsService::commits() const
{
    return m_commits;
}

const QList<ContributorRecord>& GitAnalyticsService::contributors() const
{
    return m_contributors;
}

const QList<HotspotRecord>& GitAnalyticsService::hotspots() const
{
    return m_hotspots;
}

bool GitAnalyticsService::isLoaded() const
{
    return m_isLoaded;
}

void GitAnalyticsService::clear()
{
    m_commits.clear();
    m_contributors.clear();
    m_hotspots.clear();
    m_lastError.clear();
    m_isLoaded = false;
}

bool GitAnalyticsService::refresh(const QString &repoPath)
{
    clear();

    if (!m_client) {
        m_lastError = QStringLiteral("Git client is null.");
        return false;
    }

    if (!m_client->isGitAvailable()) {
        m_lastError = QStringLiteral("Git executable is not available on this system.");
        return false;
    }

    if (repoPath.isEmpty()) {
        m_lastError = QStringLiteral("Repository path is empty.");
        return false;
    }

    QString workTreeErr;
    if (!m_client->isInsideWorkTree(repoPath, &workTreeErr)) {
        m_lastError = workTreeErr.isEmpty()
            ? QStringLiteral("The directory does not appear to be a Git repository.")
            : workTreeErr;
        return false;
    }

    if (!parseCommits(repoPath) || !parseContributors(repoPath) || !parseHotspots(repoPath)) {
        clear();
        return false;
    }

    m_isLoaded = true;
    m_lastError.clear();
    return true;
}

bool GitAnalyticsService::parseCommits(const QString &repoPath)
{
    // Fetch up to 500 commits: short-hash, subject, author name, ISO date.
    // Git processes %x1F natively as ASCII 0x1F unit separator.
    const GitResult logResult = m_client->execute(
        repoPath,
        {QStringLiteral("log"),
         QStringLiteral("--pretty=format:%h%x1F%s%x1F%an%x1F%ci"),
         QStringLiteral("-n"), QStringLiteral("500")},
        10000
    );

    if (!logResult.success) {
        // Distinguish empty / unborn repositories from actual execution failures
        if (logResult.stdErr.contains(QStringLiteral("does not have any commits")) ||
            logResult.stdErr.contains(QStringLiteral("unknown revision")) ||
            logResult.stdErr.contains(QStringLiteral("ambiguous argument 'HEAD'"))) {
            return true;
        }
        m_lastError = logResult.stdErr.isEmpty()
            ? QStringLiteral("Failed to retrieve commit history.")
            : logResult.stdErr;
        return false;
    }

    if (logResult.stdOut.isEmpty()) {
        return true;
    }

    const QStringList lines = logResult.stdOut.split(u'\n', Qt::SkipEmptyParts);
    m_commits.reserve(lines.size());

    for (const QString &line : lines) {
        const QStringList fields = line.split(QChar(0x1F));
        if (fields.size() < 4) {
            continue;
        }

        const QString hash = fields[0].trimmed();
        const QString subject = fields[1].trimmed();
        const QString author = fields[2].trimmed();
        const QDateTime dt = QDateTime::fromString(fields[3].trimmed(), Qt::ISODate);
        const QString date = dt.isValid()
            ? dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd"))
            : fields[3].trimmed().left(10);

        m_commits.append({hash, subject, author, date});
    }

    return true;
}

bool GitAnalyticsService::parseContributors(const QString &repoPath)
{
    const GitResult logResult = m_client->execute(
        repoPath,
        {QStringLiteral("log"),
         QStringLiteral("--pretty=format:%an%x1F%ad"),
         QStringLiteral("--date=short")},
        15000
    );

    if (!logResult.success) {
        if (logResult.stdErr.contains(QStringLiteral("does not have any commits")) ||
            logResult.stdErr.contains(QStringLiteral("unknown revision")) ||
            logResult.stdErr.contains(QStringLiteral("ambiguous argument 'HEAD'"))) {
            return true;
        }
        m_lastError = logResult.stdErr.isEmpty()
            ? QStringLiteral("Failed to retrieve contributor statistics.")
            : logResult.stdErr;
        return false;
    }

    if (logResult.stdOut.isEmpty()) {
        return true;
    }

    struct AuthorInfo {
        int commits{0};
        QString latestDate;
    };
    QMap<QString, AuthorInfo> authorMap;

    const QStringList lines = logResult.stdOut.split(u'\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList fields = line.split(QChar(0x1F));
        if (fields.size() < 2) {
            continue;
        }
        const QString author = fields[0].trimmed();
        const QString date   = fields[1].trimmed();
        if (author.isEmpty()) {
            continue;
        }
        AuthorInfo &info = authorMap[author];
        info.commits += 1;
        // First occurrence is most recent since git log is ordered newest-first
        if (info.latestDate.isEmpty()) {
            info.latestDate = date;
        }
    }

    m_contributors.reserve(authorMap.size());
    for (auto it = authorMap.cbegin(); it != authorMap.cend(); ++it) {
        m_contributors.append({it.key(), it.value().commits, it.value().latestDate});
    }

    // Sort descending by commit count
    std::sort(m_contributors.begin(), m_contributors.end(), [](const ContributorRecord &a, const ContributorRecord &b) {
        return a.commits > b.commits;
    });

    return true;
}

bool GitAnalyticsService::parseHotspots(const QString &repoPath)
{
    const GitResult logResult = m_client->execute(
        repoPath,
        {QStringLiteral("log"),
         QStringLiteral("--name-only"),
         QStringLiteral("--pretty=format:COMMIT%x1F%ad"),
         QStringLiteral("--date=short")},
        15000
    );

    if (!logResult.success) {
        if (logResult.stdErr.contains(QStringLiteral("does not have any commits")) ||
            logResult.stdErr.contains(QStringLiteral("unknown revision")) ||
            logResult.stdErr.contains(QStringLiteral("ambiguous argument 'HEAD'"))) {
            return true;
        }
        m_lastError = logResult.stdErr.isEmpty()
            ? QStringLiteral("Failed to retrieve hotspot analysis.")
            : logResult.stdErr;
        return false;
    }

    if (logResult.stdOut.isEmpty()) {
        return true;
    }

    struct HotspotInfo {
        int commits{0};
        QString latestDate;
    };
    QMap<QString, HotspotInfo> hotspotMap;

    const QString commitPrefix = QStringLiteral("COMMIT") + QChar(0x1F);
    QString currentCommitDate;

    const QStringList lines = logResult.stdOut.split(u'\n', Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (line.startsWith(commitPrefix)) {
            currentCommitDate = line.mid(commitPrefix.size()).trimmed();
        } else {
            HotspotInfo &info = hotspotMap[line];
            info.commits += 1;
            if (info.latestDate.isEmpty()) {
                info.latestDate = currentCommitDate;
            }
        }
    }

    m_hotspots.reserve(hotspotMap.size());
    for (auto it = hotspotMap.cbegin(); it != hotspotMap.cend(); ++it) {
        m_hotspots.append({it.key(), it.value().commits, it.value().latestDate});
    }

    // Sort descending by commit count
    std::sort(m_hotspots.begin(), m_hotspots.end(), [](const HotspotRecord &a, const HotspotRecord &b) {
        return a.commits > b.commits;
    });

    return true;
}
