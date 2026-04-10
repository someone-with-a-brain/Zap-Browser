#pragma once

/**
 * HistoryDB — SQLite3 local browsing history database.
 *
 * Stores: URL, title, timestamp, visit count, tab_id.
 * Never sent anywhere. Optional clear-on-exit.
 * Thread-safe: all DB operations run on a background QThread.
 */

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QThread>
#include <QMutex>

struct HistoryEntry {
    int      id         = 0;
    QString  url;
    QString  title;
    QDateTime timestamp;
    int      visitCount = 1;
    int      tabId      = -1;
};

class HistoryDB : public QObject {
    Q_OBJECT

public:
    explicit HistoryDB(QObject* parent = nullptr);
    ~HistoryDB() override;

    /**
     * Open (or create) the history database at the given path.
     * Call once at startup.
     */
    bool Open(const QString& dbPath);

    /** Close and flush. */
    void Close();

    /** Record a page visit. Increments visit_count if URL seen before. */
    void RecordVisit(const QString& url,
                     const QString& title,
                     int tabId = -1);

    /** Search history by URL/title fragment. */
    QList<HistoryEntry> Search(const QString& query, int limit = 20) const;

    /** Returns recent entries. */
    QList<HistoryEntry> Recent(int limit = 50) const;

    /** Delete all history. */
    void Clear();

    /** Delete entries older than `days` days. */
    void PurgeOlderThan(int days);

    bool IsOpen() const { return dbOpen_; }

signals:
    void visitRecorded(const HistoryEntry& entry);

private:
    struct sqlite3* db_ = nullptr;
    bool   dbOpen_ = false;
    mutable QMutex mutex_;

    bool createSchema();
    void execSQL(const QString& sql);
};
