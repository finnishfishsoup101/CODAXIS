#include "repository_health_service.h"
#include "git_client.h"

#include <QDir>
#include <QStringList>
#include <QChar>

RepositoryHealthService::RepositoryHealthService(GitClient *client)
    : GitService(client)
{
}

QString RepositoryHealthService::serviceName() const
{
    return QStringLiteral("Repository Health Service");
}

QString RepositoryHealthService::lastError() const
{
    return m_lastError;
}

const RepositoryHealthData& RepositoryHealthService::healthData() const
{
    return m_healthData;
}

bool RepositoryHealthService::isLoaded() const
{
    return m_isLoaded;
}

void RepositoryHealthService::clear()
{
    m_healthData = RepositoryHealthData();
    m_lastError.clear();
    m_isLoaded = false;
}

bool RepositoryHealthService::refresh(const QString &repoPath)
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

    const QString branch = m_client->getCurrentBranch(repoPath);

    // 1. Total commits on HEAD
    int totalCommits = 0;
    const GitResult revCountResult = m_client->execute(
        repoPath,
        {QStringLiteral("rev-list"), QStringLiteral("--count"), QStringLiteral("HEAD")}
    );
    if (revCountResult.success && !revCountResult.stdOut.isEmpty()) {
        totalCommits = revCountResult.stdOut.trimmed().toInt();
    }

    // 2. Unique contributors on HEAD
    int uniqueContributors = 0;
    const GitResult shortlogResult = m_client->execute(
        repoPath,
        {QStringLiteral("shortlog"), QStringLiteral("-s"), QStringLiteral("-n"), QStringLiteral("HEAD")}
    );
    if (shortlogResult.success && !shortlogResult.stdOut.isEmpty()) {
        const QStringList authorLines = shortlogResult.stdOut.split(u'\n', Qt::SkipEmptyParts);
        uniqueContributors = static_cast<int>(authorLines.size());
    }

    // 3. Tracked files (git ls-files)
    int trackedFiles = 0;
    const GitResult lsResult = m_client->execute(
        repoPath,
        {QStringLiteral("ls-files")}
    );
    if (lsResult.success && !lsResult.stdOut.isEmpty()) {
        const QStringList fileLines = lsResult.stdOut.split(u'\n', Qt::SkipEmptyParts);
        trackedFiles = static_cast<int>(fileLines.size());
    }

    // 4. Working tree status (git status --porcelain)
    QString workingTreeStatus = QStringLiteral("Clean");
    QString workingTreeDetail = QStringLiteral("Working directory matches HEAD with no uncommitted changes.");
    const GitResult statusResult = m_client->execute(
        repoPath,
        {QStringLiteral("status"), QStringLiteral("--porcelain")}
    );
    if (statusResult.success) {
        if (!statusResult.stdOut.isEmpty()) {
            const QStringList dirtyLines = statusResult.stdOut.split(u'\n', Qt::SkipEmptyParts);
            workingTreeStatus = QStringLiteral("%1 uncommitted item(s)").arg(dirtyLines.size());
            workingTreeDetail = QStringLiteral("Working directory has unstaged or untracked modifications.");
        }
    } else {
        workingTreeStatus = QStringLiteral("Unavailable");
        workingTreeDetail = statusResult.stdErr;
    }

    // 5. Latest commit details
    QString latestCommitText = QStringLiteral("No commits yet");
    QString latestCommitDetail = QStringLiteral("Repository has an unborn or empty commit history.");
    if (totalCommits > 0) {
        const GitResult lastCommitResult = m_client->execute(
            repoPath,
            {QStringLiteral("log"), QStringLiteral("-1"),
             QStringLiteral("--pretty=format:%h%x1F%ad%x1F%s"),
             QStringLiteral("--date=short")}
        );
        if (lastCommitResult.success && !lastCommitResult.stdOut.isEmpty()) {
            const QStringList fields = lastCommitResult.stdOut.split(QChar(0x1F));
            if (fields.size() >= 3) {
                latestCommitText = QStringLiteral("%1 (%2)").arg(fields[0].trimmed(), fields[1].trimmed());
                latestCommitDetail = fields[2].trimmed();
            }
        }
    }

    // 6. Detailed metrics table
    const QList<HealthMetricItem> rows = {
        {QStringLiteral("Repository Root"), QDir::toNativeSeparators(repoPath), QStringLiteral("Active local repository filesystem directory")},
        {QStringLiteral("Current Branch"), branch.isEmpty() ? QStringLiteral("unborn") : branch, QStringLiteral("Currently active branch ref or HEAD checkout")},
        {QStringLiteral("Total Commits"), QString::number(totalCommits), totalCommits == 0 ? QStringLiteral("No commits recorded yet on this branch") : QStringLiteral("Total revisions recorded in current branch history")},
        {QStringLiteral("Unique Contributors"), QString::number(uniqueContributors), QStringLiteral("Distinct Git author identities in repository log")},
        {QStringLiteral("Tracked Files"), QString::number(trackedFiles), QStringLiteral("Files currently registered under version control")},
        {QStringLiteral("Working Tree"), workingTreeStatus, workingTreeDetail},
        {QStringLiteral("Latest Commit"), latestCommitText, latestCommitDetail}
    };

    m_healthData.totalCommits = totalCommits;
    m_healthData.uniqueContributors = uniqueContributors;
    m_healthData.trackedFiles = trackedFiles;
    m_healthData.currentBranch = branch;
    m_healthData.workingTreeStatus = workingTreeStatus;
    m_healthData.workingTreeDetail = workingTreeDetail;
    m_healthData.latestCommitText = latestCommitText;
    m_healthData.latestCommitDetail = latestCommitDetail;
    m_healthData.metricsTable = rows;

    m_lastError.clear();
    m_isLoaded = true;
    return true;
}
