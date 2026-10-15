#ifndef GIT_CLIENT_H
#define GIT_CLIENT_H

#include <QString>
#include <QStringList>

/**
 * @brief Encapsulates the execution result of a Git CLI command.
 */
struct GitResult {
    bool success{false};
    QString stdOut;
    QString stdErr;
};

/**
 * @brief Low-level Git client responsible for locating the Git executable,
 * executing CLI subprocesses, enforcing timeouts, and resolving repository topology.
 */
class GitClient {
public:
    explicit GitClient();
    ~GitClient() = default;

    // Executable discovery & availability
    bool isGitAvailable() const;
    QString gitExecutablePath() const;

    // Command execution
    GitResult execute(const QString &workingDir, const QStringList &arguments, int timeoutMs = 3000) const;

    // Topology and validation helpers
    bool isInsideWorkTree(const QString &dirPath, QString *errorMessage = nullptr) const;
    QString getTopLevel(const QString &dirPath) const;
    QString getCurrentBranch(const QString &repoRoot) const;

private:
    static QString findGitExecutable();

    QString m_gitExecutable;
};

#endif // GIT_CLIENT_H
