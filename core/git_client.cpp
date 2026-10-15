#include "git_client.h"

#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

GitClient::GitClient()
    : m_gitExecutable(findGitExecutable())
{
}

bool GitClient::isGitAvailable() const
{
    return !gitExecutablePath().isEmpty();
}

QString GitClient::gitExecutablePath() const
{
    if (m_gitExecutable.isEmpty()) {
        const_cast<GitClient*>(this)->m_gitExecutable = findGitExecutable();
    }
    return m_gitExecutable;
}

QString GitClient::findGitExecutable()
{
    QString gitPath = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (!gitPath.isEmpty()) {
        return gitPath;
    }

    const QStringList commonLocations = {
        QStringLiteral("C:/Program Files/Git/cmd/git.exe"),
        QStringLiteral("C:/Program Files/Git/bin/git.exe"),
        QStringLiteral("C:/Program Files (x86)/Git/cmd/git.exe")
    };

    for (const QString &candidate : commonLocations) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

GitResult GitClient::execute(const QString &workingDir, const QStringList &arguments, int timeoutMs) const
{
    const QString gitExe = gitExecutablePath();
    if (gitExe.isEmpty()) {
        return {false, QString(), QStringLiteral("Git executable not found on this system.")};
    }

    QProcess process;
    process.setWorkingDirectory(workingDir);
    process.start(gitExe, arguments);

    if (!process.waitForStarted(timeoutMs)) {
        return {false, QString(), QStringLiteral("Failed to launch Git process.")};
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        return {false, QString(), QStringLiteral("Git operation timed out.")};
    }

    const bool ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    return {
        ok,
        QString::fromUtf8(process.readAllStandardOutput()).trimmed(),
        QString::fromUtf8(process.readAllStandardError()).trimmed()
    };
}

bool GitClient::isInsideWorkTree(const QString &dirPath, QString *errorMessage) const
{
    const GitResult workTreeCheck = execute(
        dirPath,
        {QStringLiteral("rev-parse"), QStringLiteral("--is-inside-work-tree")}
    );

    if (!workTreeCheck.success || workTreeCheck.stdOut != QStringLiteral("true")) {
        if (errorMessage) {
            *errorMessage = workTreeCheck.stdErr;
        }
        return false;
    }
    return true;
}

QString GitClient::getTopLevel(const QString &dirPath) const
{
    const GitResult topLevelCheck = execute(
        dirPath,
        {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")}
    );

    return topLevelCheck.success ? topLevelCheck.stdOut : dirPath;
}

QString GitClient::getCurrentBranch(const QString &repoRoot) const
{
    const GitResult branchCheck = execute(
        repoRoot,
        {QStringLiteral("branch"), QStringLiteral("--show-current")}
    );

    QString branch = branchCheck.stdOut;
    if (branch.isEmpty()) {
        const GitResult headCheck = execute(
            repoRoot,
            {QStringLiteral("rev-parse"), QStringLiteral("--short"), QStringLiteral("HEAD")}
        );
        if (headCheck.success && !headCheck.stdOut.isEmpty()) {
            branch = QStringLiteral("detached (%1)").arg(headCheck.stdOut);
        } else {
            branch = QStringLiteral("unborn");
        }
    }
    return branch;
}
