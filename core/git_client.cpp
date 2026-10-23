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

#include <QElapsedTimer>
#include <QCoreApplication>

GitResult GitClient::execute(const QString &workingDir, const QStringList &arguments, int timeoutMs) const
{
    const QString gitExe = gitExecutablePath();
    if (gitExe.isEmpty()) {
        return {false, QString(), QStringLiteral("Git executable not found on this system.")};
    }

    if (workingDir.trimmed().isEmpty()) {
        return {false, QString(), QStringLiteral("Working directory path is empty.")};
    }

    const QFileInfo dirInfo(workingDir);
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        return {false, QString(), QStringLiteral("Working directory does not exist or is not a directory.")};
    }

    QProcess process;
    process.setWorkingDirectory(workingDir);
    process.start(gitExe, arguments);

    const int startTimeout = std::min(timeoutMs > 0 ? timeoutMs : 3000, 3000);
    if (!process.waitForStarted(startTimeout)) {
        return {false, QString(), QStringLiteral("Failed to launch Git process.")};
    }

    QByteArray stdOut;
    QByteArray stdErr;
    QElapsedTimer timer;
    timer.start();

    const int effectiveTimeout = timeoutMs > 0 ? timeoutMs : 5000;

    while (process.state() != QProcess::NotRunning) {
        if (process.waitForReadyRead(50)) {
            stdOut.append(process.readAllStandardOutput());
            stdErr.append(process.readAllStandardError());
        }
        // Pump Qt paint and system events while keeping user input blocked
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        if (timer.hasExpired(effectiveTimeout)) {
            process.kill();
            process.waitForFinished(500);
            return {false, QString(), QStringLiteral("Git operation timed out.")};
        }
    }

    // Drain any remaining output buffers
    stdOut.append(process.readAllStandardOutput());
    stdErr.append(process.readAllStandardError());

    const bool ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    return {
        ok,
        QString::fromUtf8(stdOut).trimmed(),
        QString::fromUtf8(stdErr).trimmed()
    };
}

bool GitClient::isInsideWorkTree(const QString &dirPath, QString *errorMessage) const
{
    if (dirPath.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Path cannot be empty.");
        }
        return false;
    }

    const QFileInfo info(dirPath);
    if (!info.exists() || !info.isDir()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Path does not exist or is not a directory.");
        }
        return false;
    }

    const GitResult workTreeCheck = execute(
        dirPath,
        {QStringLiteral("rev-parse"), QStringLiteral("--is-inside-work-tree")},
        3000
    );

    if (!workTreeCheck.success || workTreeCheck.stdOut != QStringLiteral("true")) {
        if (errorMessage) {
            *errorMessage = workTreeCheck.stdErr.isEmpty()
                ? QStringLiteral("The selected folder is not a valid Git working tree.")
                : workTreeCheck.stdErr;
        }
        return false;
    }
    return true;
}

QString GitClient::getTopLevel(const QString &dirPath) const
{
    if (dirPath.trimmed().isEmpty()) {
        return dirPath;
    }

    const GitResult topLevelCheck = execute(
        dirPath,
        {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")},
        3000
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
