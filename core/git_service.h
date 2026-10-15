#ifndef GIT_SERVICE_H
#define GIT_SERVICE_H

#include <QString>

class GitClient;

/**
 * @brief Abstract base class for domain services operating on a local Git repository.
 * Encapsulates the GitClient dependency and defines the uniform lifecycle contract.
 */
class GitService {
protected:
    GitClient *m_client{nullptr};

public:
    explicit GitService(GitClient *client);
    virtual ~GitService() = default;

    GitClient* client() const { return m_client; }

    // Abstraction: the four functions below form the interface every service must
    // implement. GitService doesn't know (or care) how each one works internally.
    // Core polymorphic service contract
    virtual QString serviceName() const = 0;
    virtual bool refresh(const QString &repoPath) = 0;
    virtual void clear() = 0;
    virtual QString lastError() const = 0;
};

#endif // GIT_SERVICE_H
