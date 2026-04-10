#include "HistoryDB.h"

#include <sqlite3.h>
#include <QDebug>
#include <QMutexLocker>

HistoryDB::HistoryDB(QObject* parent)
    : QObject(parent)
{}

HistoryDB::~HistoryDB()
{
    Close();
}

bool HistoryDB::Open(const QString& dbPath)
{
    QMutexLocker lock(&mutex_);
    int rc = sqlite3_open(dbPath.toUtf8().constData(), &db_);
    if (rc != SQLITE_OK) {
        qCritical("[HistoryDB] Failed to open: %s", sqlite3_errmsg(db_));
        db_ = nullptr;
        return false;
    }
    dbOpen_ = true;
    return createSchema();
}

void HistoryDB::Close()
{
    QMutexLocker lock(&mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_    = nullptr;
        dbOpen_ = false;
    }
}

bool HistoryDB::createSchema()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS history (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            url         TEXT NOT NULL,
            title       TEXT DEFAULT '',
            timestamp   INTEGER DEFAULT (strftime('%s', 'now')),
            visit_count INTEGER DEFAULT 1,
            tab_id      INTEGER DEFAULT -1
        );
        CREATE INDEX IF NOT EXISTS idx_history_url ON history(url);
        CREATE INDEX IF NOT EXISTS idx_history_ts  ON history(timestamp DESC);
    )";

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        qCritical("[HistoryDB] Schema error: %s", errmsg);
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

void HistoryDB::RecordVisit(const QString& url, const QString& title, int tabId)
{
    if (!dbOpen_) return;
    QMutexLocker lock(&mutex_);

    // Upsert: increment visit_count if URL exists, else insert
    const char* sql = R"(
        INSERT INTO history (url, title, timestamp, visit_count, tab_id)
        VALUES (?, ?, strftime('%s','now'), 1, ?)
        ON CONFLICT(url) DO UPDATE SET
            visit_count = visit_count + 1,
            timestamp   = strftime('%s','now'),
            title       = excluded.title;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, url.toUtf8().constData(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, title.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,  3, tabId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    HistoryEntry e;
    e.url   = url;
    e.title = title;
    e.tabId = tabId;
    e.timestamp = QDateTime::currentDateTime();
    emit visitRecorded(e);
}

QList<HistoryEntry> HistoryDB::Search(const QString& query, int limit) const
{
    QList<HistoryEntry> results;
    if (!dbOpen_) return results;
    QMutexLocker lock(&mutex_);

    const char* sql = R"(
        SELECT id, url, title, timestamp, visit_count, tab_id
        FROM history
        WHERE url LIKE ? OR title LIKE ?
        ORDER BY timestamp DESC
        LIMIT ?;
    )";

    QString pattern = "%" + query + "%";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return results;

    sqlite3_bind_text(stmt, 1, pattern.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry e;
        e.id         = sqlite3_column_int(stmt, 0);
        e.url        = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
        e.title      = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
        e.timestamp  = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 3));
        e.visitCount = sqlite3_column_int(stmt, 4);
        e.tabId      = sqlite3_column_int(stmt, 5);
        results.append(e);
    }
    sqlite3_finalize(stmt);
    return results;
}

QList<HistoryEntry> HistoryDB::Recent(int limit) const
{
    QList<HistoryEntry> results;
    if (!dbOpen_) return results;
    QMutexLocker lock(&mutex_);

    const char* sql = R"(
        SELECT id, url, title, timestamp, visit_count, tab_id
        FROM history
        ORDER BY timestamp DESC
        LIMIT ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return results;

    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry e;
        e.id         = sqlite3_column_int(stmt, 0);
        e.url        = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
        e.title      = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
        e.timestamp  = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 3));
        e.visitCount = sqlite3_column_int(stmt, 4);
        e.tabId      = sqlite3_column_int(stmt, 5);
        results.append(e);
    }
    sqlite3_finalize(stmt);
    return results;
}

void HistoryDB::Clear()
{
    if (!dbOpen_) return;
    QMutexLocker lock(&mutex_);
    sqlite3_exec(db_, "DELETE FROM history;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "VACUUM;", nullptr, nullptr, nullptr);
}

void HistoryDB::PurgeOlderThan(int days)
{
    if (!dbOpen_) return;
    QMutexLocker lock(&mutex_);

    const char* sql = R"(
        DELETE FROM history
        WHERE timestamp < strftime('%s','now','-' || ? || ' days');
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, days);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}
