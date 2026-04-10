#pragma once

/**
 * BlockList — Ad and tracker URL blocking.
 *
 * Loads domain-based block lists (EasyList format) from disk and
 * provides an O(1) lookup used inside CefRequestHandler to cancel
 * blocked resource requests before they hit the network.
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <unordered_set>
#include <string>

class BlockList : public QObject {
    Q_OBJECT

public:
    explicit BlockList(QObject* parent = nullptr);

    /**
     * Loads block list files from the given directory.
     * Expects: ads.txt, trackers.txt in EasyList/hosts format.
     */
    void LoadFromDirectory(const QString& dir);

    /**
     * Load a specific block list file (one domain per line or EasyList).
     */
    void LoadFile(const QString& path);

    /**
     * Returns true if the given URL's hostname is in the block list.
     * Optimized: O(1) hash lookup.
     */
    bool IsBlocked(const QString& url) const;
    bool IsBlocked(const std::string& url) const;

    /** Number of blocked domains currently loaded. */
    int Count() const { return static_cast<int>(blockedDomains_.size()); }

    /**
     * Schedule auto-refresh from remote URLs every intervalMs ms.
     * (Default: 24 hours)
     */
    void ScheduleAutoRefresh(int intervalMs = 86400000);

signals:
    void listsUpdated(int domainCount);

private slots:
    void onAutoRefresh();
    void downloadList(const QString& url, const QString& savePath);

private:
    std::unordered_set<std::string> blockedDomains_;

    void parseEasyListLine(const std::string& line);
    std::string extractHostname(const std::string& url) const;

    QStringList remoteSources_ = {
        "https://easylist.to/easylist/easylist.txt",
        "https://easylist.to/easylist/easyprivacy.txt"
    };
    QString listDir_;
    QTimer* refreshTimer_ = nullptr;
};
