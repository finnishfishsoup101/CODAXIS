#include <iostream>
#include <cassert>
#include <QDate>
#include <QList>
#include <QDir>
#include "core/analytics_types.h"
#include "core/git_analytics_service.h"
#include "core/repository_health_service.h"
#include "core/git_client.h"

int main()
{
    int testCount = 0;
    int passCount = 0;

    auto test = [&](const char *name, bool condition) {
        testCount++;
        if (condition) {
            passCount++;
            std::cout << "  [PASS] " << name << "\n";
        } else {
            std::cerr << "  [FAIL] " << name << "\n";
        }
    };

    std::cout << "Running focused date-bucketing analytics tests...\n";

    const QDate refDate(2026, 10, 21);

    // Test 1: Empty commits
    {
        const QList<CommitRecord> emptyList;
        const CommitActivityData result = GitAnalyticsService::aggregateCommitActivity(emptyList, 30, refDate);
        test("Empty commits produces empty buckets", result.buckets.isEmpty());
        test("Empty commits total is 0", result.totalCommits == 0);
        test("Empty commits is not truncated", !result.isTruncated);
    }

    // Test 2: Normal 30-day window with active and zero-commit days
    {
        QList<CommitRecord> commits = {
            {"c1", "Commit 1", "Author A", "2026-10-21"}, // today (refDate)
            {"c2", "Commit 2", "Author B", "2026-10-21"}, // today (refDate) -> 2 commits
            {"c3", "Commit 3", "Author A", "2026-10-19"}, // 2 days ago -> 1 commit
            {"c4", "Commit 4", "Author C", "2026-09-22"}, // 29 days ago (startDate for 30d) -> 1 commit
            {"c5", "Commit 5", "Author D", "2026-09-21"}  // 30 days ago (outside 30d window) -> 0 in window
        };

        const CommitActivityData result = GitAnalyticsService::aggregateCommitActivity(commits, 30, refDate);
        test("30-day window produces exactly 30 daily buckets", result.buckets.size() == 30);
        test("30-day window total commits in window is 4", result.totalCommits == 4);
        test("Oldest bucket date is 2026-09-22", result.buckets.first().date == QDate(2026, 9, 22));
        test("Newest bucket date is 2026-10-21", result.buckets.last().date == QDate(2026, 10, 21));
        test("Today has 2 commits", result.buckets.last().commitCount == 2);
        test("2026-10-20 (yesterday) has 0 commits", result.buckets[28].commitCount == 0);
        test("2026-10-19 has 1 commit", result.buckets[27].commitCount == 1);
        test("2026-09-22 (start day) has 1 commit", result.buckets[0].commitCount == 1);
        test("Not truncated under 500 commits", !result.isTruncated);
    }

    // Test 3: 90-day window
    {
        QList<CommitRecord> commits = {
            {"c1", "Commit 1", "Author A", "2026-10-21"},
            {"c2", "Commit 2", "Author A", "2026-08-01"}
        };

        const CommitActivityData result = GitAnalyticsService::aggregateCommitActivity(commits, 90, refDate);
        test("90-day window produces exactly 90 daily buckets", result.buckets.size() == 90);
        test("90-day window counts commits correctly", result.totalCommits == 2);
        test("90-day window start date is 2026-07-24", result.buckets.first().date == refDate.addDays(-89));
    }

    // Test 4: Malformed and invalid dates
    {
        QList<CommitRecord> commits = {
            {"c1", "Valid commit", "Author A", "2026-10-21"},
            {"c2", "Bad date 1", "Author B", "not-a-date"},
            {"c3", "Bad date 2", "Author C", ""}
        };

        const CommitActivityData result = GitAnalyticsService::aggregateCommitActivity(commits, 30, refDate);
        test("Malformed dates are safely ignored", result.totalCommits == 1);
        test("Buckets still generated cleanly for 30 days", result.buckets.size() == 30);
    }

    // Test 5: 500-commit truncation detection
    {
        // Simulate 500 commits where the oldest commit is 10 days ago (high-churn repo)
        QList<CommitRecord> highChurnCommits;
        highChurnCommits.reserve(500);
        for (int i = 0; i < 500; ++i) {
            // commits distributed across last 10 days
            const QDate d = refDate.addDays(-(i % 10));
            highChurnCommits.append({QString::number(i), "Msg", "Author", d.toString(Qt::ISODate)});
        }

        const CommitActivityData result = GitAnalyticsService::aggregateCommitActivity(highChurnCommits, 30, refDate);
        test("Truncation flag is true when 500 cap hit within window", result.isTruncated);
    }

    std::cout << "\nRunning contributor identity attribution regression tests...\n";

    // Test 6: Unified contributor identity mapping mirroring real repository history (15 commits)
    {
        QList<GitAnalyticsService::ContributorLogEntry> entries;
        // 3 recent commits as tanujaa658
        for (int i = 0; i < 3; ++i) {
            entries.append({"tanujaa658", "tanujasah09@gmail.com", "2026-10-21"});
        }
        // 1 commit as zen
        entries.append({"zen", "tanujasah09@gmail.com", "2026-10-15"});
        // 2 commits as sneha maurya
        entries.append({"sneha maurya", "snehamaurya7143@gmail.com", "2026-10-15"});
        entries.append({"sneha maurya", "snehamaurya7143@gmail.com", "2026-10-15"});
        // 1 commit as zen
        entries.append({"zen", "tanujasah09@gmail.com", "2026-10-15"});
        // 8 older commits as tanujaa658
        for (int i = 0; i < 8; ++i) {
            entries.append({"tanujaa658", "tanujasah09@gmail.com", "2026-10-15"});
        }

        test("Input commit entry count equals 15", entries.size() == 15);

        const QList<ContributorRecord> contributors = GitAnalyticsService::aggregateContributors(entries);
        test("Distinct logical contributors count is 2 (tanujaa658+zen merged, sneha separate)", contributors.size() == 2);

        int totalCommits = 0;
        for (const auto &c : contributors) {
            totalCommits += c.commits;
        }
        test("Total commits across logical contributors remains exactly 15", totalCommits == 15);

        // First contributor is Tanuja 658 with 13 commits (11 tanujaa658 + 2 zen)
        test("Primary contributor name is unified identity 'Tanuja 658'", contributors[0].name == "Tanuja 658");
        test("Primary contributor has 13 combined commits", contributors[0].commits == 13);
        test("Primary contributor latest commit date is 2026-10-21", contributors[0].latestDate == "2026-10-21");

        // Second contributor is sneha maurya with 2 commits
        test("Second contributor name is 'sneha maurya'", contributors[1].name == "sneha maurya");
        test("Second contributor has exactly 2 commits", contributors[1].commits == 2);
        test("Second contributor latest commit date is 2026-10-15", contributors[1].latestDate == "2026-10-15");
    }

    // Test 7: Alias fallback mapping when email metadata is empty
    {
        QList<GitAnalyticsService::ContributorLogEntry> entries = {
            {"zen", "", "2026-10-15"},
            {"tanujaa658", "", "2026-10-21"},
            {"zen", "", "2026-10-14"}
        };

        const QList<ContributorRecord> contributors = GitAnalyticsService::aggregateContributors(entries);
        test("Alias 'zen' without email merges into 'Tanuja 658'", contributors.size() == 1);
        test("Merged alias contributor name is 'Tanuja 658'", contributors[0].name == "Tanuja 658");
        test("Merged alias contributor commit count is 3", contributors[0].commits == 3);
        test("Merged alias contributor latest date is 2026-10-21", contributors[0].latestDate == "2026-10-21");
    }

    // Test 8: Protection against fragile substring matching on unrelated contributors
    {
        QList<GitAnalyticsService::ContributorLogEntry> entries = {
            {"zenith", "zenith@example.org", "2026-10-20"},
            {"zen", "tanujasah09@gmail.com", "2026-10-19"},
            {"tanuja", "tanuja@otherdomain.com", "2026-10-18"},
            {"tanujaa658", "tanujasah09@gmail.com", "2026-10-17"},
            {"sneha", "sneha@otherdomain.com", "2026-10-16"},
            {"sneha maurya", "snehamaurya7143@gmail.com", "2026-10-15"}
        };

        const QList<ContributorRecord> contributors = GitAnalyticsService::aggregateContributors(entries);
        // zenith, tanuja, sneha must NOT be merged into zen/tanujaa658/sneha maurya
        test("Substrings like 'zenith', 'tanuja', 'sneha' are not mistakenly merged (5 distinct contributors)", contributors.size() == 5);

        bool foundZenith = false;
        bool foundTanuja = false;
        bool foundSneha = false;
        int tanuja658Commits = 0;
        int snehaMauryaCommits = 0;

        for (const auto &c : contributors) {
            if (c.name == "zenith") foundZenith = true;
            if (c.name == "tanuja") foundTanuja = true;
            if (c.name == "sneha") foundSneha = true;
            if (c.name == "Tanuja 658") tanuja658Commits = c.commits;
            if (c.name == "sneha maurya") snehaMauryaCommits = c.commits;
        }

        test("Unrelated 'zenith' is preserved separately", foundZenith);
        test("Unrelated 'tanuja' is preserved separately", foundTanuja);
        test("Unrelated 'sneha' is preserved separately", foundSneha);
        test("Tanuja 658 has exactly 2 commits (zen + tanujaa658)", tanuja658Commits == 2);
        test("Sneha Maurya has exactly 1 commit", snehaMauryaCommits == 1);
    }

    // Test 9: Raw git log string parsing with 0x1F unit separator
    {
        const QString rawLog = QStringLiteral(
            "tanujaa658\x1Ftanujasah09@gmail.com\x1F" "2026-10-21\n"
            "zen\x1Ftanujasah09@gmail.com\x1F" "2026-10-15\n"
            "sneha maurya\x1Fsnehamaurya7143@gmail.com\x1F" "2026-10-15\n"
        );

        const QList<ContributorRecord> result = GitAnalyticsService::parseContributorLogOutput(rawLog);
        test("Raw log parsing yields 2 logical contributors", result.size() == 2);
        test("Raw log first contributor is Tanuja 658 with 2 commits", result[0].name == "Tanuja 658" && result[0].commits == 2);
        test("Raw log second contributor is sneha maurya with 1 commit", result[1].name == "sneha maurya" && result[1].commits == 1);
    }

    // Test 10: normalizeContributorKey and canonicalContributorName direct consistency
    {
        test("normalizeContributorKey maps tanujaa658 with email",
             GitAnalyticsService::normalizeContributorKey("tanujaa658", "tanujasah09@gmail.com") == "tanujasah09@gmail.com");
        test("normalizeContributorKey maps zen with email",
             GitAnalyticsService::normalizeContributorKey("zen", "tanujasah09@gmail.com") == "tanujasah09@gmail.com");
        test("normalizeContributorKey maps zen without email via alias",
             GitAnalyticsService::normalizeContributorKey("zen", "") == "tanujasah09@gmail.com");
        test("normalizeContributorKey maps tanujaa658 without email via alias",
             GitAnalyticsService::normalizeContributorKey("tanujaa658", "") == "tanujasah09@gmail.com");
        test("normalizeContributorKey maps Tanuja 658 without email via alias",
             GitAnalyticsService::normalizeContributorKey("Tanuja 658", "") == "tanujasah09@gmail.com");
        test("normalizeContributorKey preserves sneha maurya",
             GitAnalyticsService::normalizeContributorKey("sneha maurya", "snehamaurya7143@gmail.com") == "snehamaurya7143@gmail.com");
        test("canonicalContributorName returns 'Tanuja 658' for zen with email",
             GitAnalyticsService::canonicalContributorName("zen", "tanujasah09@gmail.com") == "Tanuja 658");
        test("canonicalContributorName returns 'Tanuja 658' for tanujaa658 with email",
             GitAnalyticsService::canonicalContributorName("tanujaa658", "tanujasah09@gmail.com") == "Tanuja 658");
        test("canonicalContributorName returns 'Tanuja 658' for zen without email",
             GitAnalyticsService::canonicalContributorName("zen", "") == "Tanuja 658");
        test("canonicalContributorName returns sneha maurya for sneha maurya",
             GitAnalyticsService::canonicalContributorName("sneha maurya", "snehamaurya7143@gmail.com") == "sneha maurya");
    }

    // Test 11: Real Git history verification on target repository CODAXIS-CLONE
    {
        const QString clonePath = QStringLiteral("C:/Users/asus/Desktop/CODAXIS-CLONE");
        GitClient client;
        if (client.isGitAvailable() && client.isInsideWorkTree(clonePath)) {
            GitAnalyticsService analytics(&client);
            const bool refreshed = analytics.refresh(clonePath);
            test("CODAXIS-CLONE analytics service refreshed successfully", refreshed);
            test("CODAXIS-CLONE total commits is 15", analytics.commits().size() == 15);
            test("CODAXIS-CLONE logical contributors count is exactly 2", analytics.contributors().size() == 2);
            if (analytics.contributors().size() == 2) {
                test("CODAXIS-CLONE contributor 1 is unified 'Tanuja 658' with 13 commits",
                     analytics.contributors()[0].name == "Tanuja 658" && analytics.contributors()[0].commits == 13);
                test("CODAXIS-CLONE contributor 2 is sneha maurya with 2 commits",
                     analytics.contributors()[1].name == "sneha maurya" && analytics.contributors()[1].commits == 2);
            }

            RepositoryHealthService health(&client);
            const bool healthRefreshed = health.refresh(clonePath);
            test("CODAXIS-CLONE health service refreshed successfully", healthRefreshed);
            test("CODAXIS-CLONE health service total commits is 15", health.healthData().totalCommits == 15);
            test("CODAXIS-CLONE health service unique contributors is exactly 2", health.healthData().uniqueContributors == 2);

            // Test 12: Verify commit bf27521 specifically for Recent Revisions resolution
            bool foundBf27521 = false;
            for (const auto &c : analytics.commits()) {
                if (c.hash.startsWith("bf27521")) {
                    foundBf27521 = true;
                    test("Commit bf27521 original author is preserved as 'zen'", c.author == "zen");
                    test("Commit bf27521 original authorEmail is preserved as 'tanujasah09@gmail.com'", c.authorEmail == "tanujasah09@gmail.com");
                    const QString resolved = GitAnalyticsService::canonicalContributorName(c.author, c.authorEmail);
                    test("Commit bf27521 resolved canonical identity is 'Tanuja 658'", resolved == "Tanuja 658");
                    break;
                }
            }
            test("Found commit bf27521 in parsed commit history", foundBf27521);
        }
    }

    // Test 13: Repository validation and topology tests
    {
        GitClient client;
        test("GitClient reports Git is available", client.isGitAvailable());

        // Empty path
        QString emptyErr;
        test("Empty path is rejected by isInsideWorkTree", !client.isInsideWorkTree(QString(), &emptyErr));
        test("Empty path produces descriptive error", !emptyErr.isEmpty());

        // Nonexistent path
        QString nonExistentErr;
        test("Nonexistent path is rejected by isInsideWorkTree", !client.isInsideWorkTree(QStringLiteral("C:/nonexistent_folder_xyz_123"), &nonExistentErr));
        test("Nonexistent path produces descriptive error", !nonExistentErr.isEmpty());

        // Ordinary directory with no git repository
        const QString tempDir = QDir::tempPath() + QStringLiteral("/codaxis_ordinary_test_folder");
        QDir().mkpath(tempDir);
        QString ordinaryErr;
        const bool ordinaryResult = client.isInsideWorkTree(tempDir, &ordinaryErr);
        test("Ordinary directory is rejected by isInsideWorkTree", !ordinaryResult);
        test("Ordinary directory produces error", !ordinaryErr.isEmpty());
        QDir().rmdir(tempDir);

        // Valid repository root
        const QString validRepo = QStringLiteral("C:/Users/asus/Desktop/CODAXIS-CLONE");
        if (QDir(validRepo).exists()) {
            QString validErr;
            test("Valid repository root passes isInsideWorkTree", client.isInsideWorkTree(validRepo, &validErr));
            test("Valid repository getTopLevel matches root", client.getTopLevel(validRepo).replace('\\', '/') == validRepo);

            // Nested directory inside valid repository
            const QString nestedDir = validRepo + QStringLiteral("/core");
            if (QDir(nestedDir).exists()) {
                test("Nested directory passes isInsideWorkTree", client.isInsideWorkTree(nestedDir));
                test("Nested directory getTopLevel resolves to parent repo root", client.getTopLevel(nestedDir).replace('\\', '/') == validRepo);
            }
        }
    }

    std::cout << "\nTest Summary: " << passCount << " / " << testCount << " passed.\n";
    return (passCount == testCount) ? 0 : 1;
}
