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

CommitActivityData GitAnalyticsService::commitActivity(int windowDays, const QDate &referenceDate) const
{
    return aggregateCommitActivity(m_commits, windowDays, referenceDate);
}

CommitActivityData GitAnalyticsService::aggregateCommitActivity(const QList<CommitRecord> &commits,
                                                              int windowDays,
                                                              const QDate &referenceDate)
{
    CommitActivityData result;
    if (windowDays <= 0 || commits.isEmpty()) {
        return result;
    }

    const QDate endDate = referenceDate.isValid() ? referenceDate : QDate::currentDate();
    const QDate startDate = endDate.addDays(-(windowDays - 1));

    QMap<QDate, int> dailyCounts;
    int totalInWindow = 0;
    QDate oldestParsedDate;

    for (const CommitRecord &c : commits) {
        const QDate d = QDate::fromString(c.date, Qt::ISODate);
        if (!d.isValid()) {
            continue;
        }

        if (!oldestParsedDate.isValid() || d < oldestParsedDate) {
            oldestParsedDate = d;
        }

        if (d >= startDate && d <= endDate) {
            dailyCounts[d] += 1;
            totalInWindow += 1;
        }
    }

    // Account for the 500-commit query limit:
    // If the commits collection reached the 500 cap and the oldest parsed commit
    // occurred more recently than the window start date, earlier days in this window
    // cannot be verified as inactive because earlier commits were truncated.
    if (commits.size() >= 500 && oldestParsedDate.isValid() && oldestParsedDate > startDate) {
        result.isTruncated = true;
    }

    result.buckets.reserve(windowDays);
    for (int i = 0; i < windowDays; ++i) {
        const QDate day = startDate.addDays(i);
        result.buckets.append({day, dailyCounts.value(day, 0)});
    }

    result.totalCommits = totalInWindow;
    return result;
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
    // Fetch up to 500 commits: short-hash, subject, author name, author email, ISO date.
    // Git processes %x1F natively as ASCII 0x1F unit separator.
    const GitResult logResult = m_client->execute(
        repoPath,
        {QStringLiteral("log"),
         QStringLiteral("--pretty=format:%h%x1F%s%x1F%an%x1F%ae%x1F%ci"),
         QStringLiteral("-n"), QStringLiteral("500")},
        5000
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
        QString authorEmail;
        QString rawDate;

        if (fields.size() >= 5) {
            authorEmail = fields[3].trimmed();
            rawDate = fields[4].trimmed();
        } else {
            rawDate = fields[3].trimmed();
        }

        const QDateTime dt = QDateTime::fromString(rawDate, Qt::ISODate);
        const QString date = dt.isValid()
            ? dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd"))
            : rawDate.left(10);

        m_commits.append({hash, subject, author, date, authorEmail});
    }

    return true;
}

QString GitAnalyticsService::normalizeContributorKey(const QString &name, const QString &email)
{
    const QString trimmedEmail = email.trimmed().toLower();
    const QString trimmedName = name.trimmed();

    // Check verified shared email for unified identity
    if (trimmedEmail == QStringLiteral("tanujasah09@gmail.com")) {
        return QStringLiteral("tanujasah09@gmail.com");
    }

    // Narrowly scoped alias fallback for known identities when email is missing or empty
    if (trimmedName.compare(QStringLiteral("zen"), Qt::CaseInsensitive) == 0 ||
        trimmedName.compare(QStringLiteral("tanujaa658"), Qt::CaseInsensitive) == 0 ||
        trimmedName.compare(QStringLiteral("tanuja 658"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("tanujasah09@gmail.com");
    }

    // Generic rule: use email if present
    if (!trimmedEmail.isEmpty()) {
        return trimmedEmail;
    }

    // Fallback to name
    return trimmedName.toLower();
}

QString GitAnalyticsService::canonicalContributorName(const QString &name, const QString &email)
{
    const QString key = normalizeContributorKey(name, email);
    if (key == QStringLiteral("tanujasah09@gmail.com")) {
        return QStringLiteral("Tanuja 658");
    }

    const QString trimmedName = name.trimmed();
    if (!trimmedName.isEmpty()) {
        return trimmedName;
    }

    return email.trimmed();
}

QList<ContributorRecord> GitAnalyticsService::aggregateContributors(const QList<ContributorLogEntry> &entries)
{
    if (entries.isEmpty()) {
        return {};
    }

    struct AuthorInfo {
        QString displayName;
        int commits{0};
        QString latestDate;
    };
    QMap<QString, AuthorInfo> authorMap;

    for (const auto &entry : entries) {
        const QString trimmedName = entry.name.trimmed();
        const QString trimmedEmail = entry.email.trimmed();
        const QString trimmedDate = entry.date.trimmed();

        if (trimmedName.isEmpty() && trimmedEmail.isEmpty()) {
            continue;
        }

        const QString key = normalizeContributorKey(trimmedName, trimmedEmail);
        AuthorInfo &info = authorMap[key];
        info.commits += 1;
        // Retain the latest commit date across all commits for this contributor
        if (info.latestDate.isEmpty() || (!trimmedDate.isEmpty() && trimmedDate > info.latestDate)) {
            info.latestDate = trimmedDate;
        }
        if (info.displayName.isEmpty()) {
            info.displayName = canonicalContributorName(trimmedName, trimmedEmail);
        }
    }

    QList<ContributorRecord> records;
    records.reserve(authorMap.size());
    for (auto it = authorMap.cbegin(); it != authorMap.cend(); ++it) {
        records.append({it.value().displayName, it.value().commits, it.value().latestDate});
    }

    // Sort descending by commit count
    std::sort(records.begin(), records.end(), [](const ContributorRecord &a, const ContributorRecord &b) {
        return a.commits > b.commits;
    });

    return records;
}

QList<ContributorRecord> GitAnalyticsService::parseContributorLogOutput(const QString &logOutput)
{
    if (logOutput.isEmpty()) {
        return {};
    }

    const QStringList lines = logOutput.split(u'\n', Qt::SkipEmptyParts);
    QList<ContributorLogEntry> entries;
    entries.reserve(lines.size());

    for (const QString &line : lines) {
        const QStringList fields = line.split(QChar(0x1F));
        if (fields.size() < 2) {
            continue;
        }

        ContributorLogEntry entry;
        if (fields.size() >= 3) {
            entry.name  = fields[0].trimmed();
            entry.email = fields[1].trimmed();
            entry.date  = fields[2].trimmed();
        } else {
            entry.name  = fields[0].trimmed();
            entry.email = QString();
            entry.date  = fields[1].trimmed();
        }

        entries.append(entry);
    }

    return aggregateContributors(entries);
}

bool GitAnalyticsService::parseContributors(const QString &repoPath)
{
    const GitResult logResult = m_client->execute(
        repoPath,
        {QStringLiteral("log"),
         QStringLiteral("--pretty=format:%an%x1F%ae%x1F%ad"),
         QStringLiteral("--date=short")},
        5000
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

    m_contributors = parseContributorLogOutput(logResult.stdOut);
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
        5000
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
