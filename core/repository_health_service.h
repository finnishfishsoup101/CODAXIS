#ifndef REPOSITORY_HEALTH_SERVICE_H
#define REPOSITORY_HEALTH_SERVICE_H

#include "git_service.h"
#include "analytics_types.h"

/**
 * @brief Service responsible for auditing Git repository health,
 * revision statistics, contributor footprint, tracked files, and working tree state.
 */
// Inheritance: RepositoryHealthService extends GitService, so it gets the shared
// GitClient pointer and must implement all four virtual functions defined there.
class RepositoryHealthService : public GitService {
public:
    explicit RepositoryHealthService(GitClient *client);
    ~RepositoryHealthService() override = default;

    // GitService interface implementation
    QString serviceName() const override;
    bool refresh(const QString &repoPath) override;
    void clear() override;
    QString lastError() const override;

    // Domain data access
    const RepositoryHealthData& healthData() const;
    bool isLoaded() const;

    // Encapsulation: the data below is private -- nothing outside this class can
    // read or modify it directly, only through the public methods declared above.
private:
    RepositoryHealthData m_healthData;
    QString m_lastError;
    bool m_isLoaded{false};
};

#endif // REPOSITORY_HEALTH_SERVICE_H
