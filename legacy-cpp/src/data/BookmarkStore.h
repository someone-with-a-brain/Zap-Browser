#pragma once

/**
 * BookmarkStore — JSON-based local bookmark manager.
 *
 * Stored at: %APPDATA%/ZapBrowser/bookmarks.json
 * Format:
 * {
 *   "bookmarks": [
 *     {"title": "Example", "url": "https://...", "added": 1712345678, "tags": []}
 *   ]
 * }
 */

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>

struct Bookmark {
    QString  title;
    QString  url;
    QDateTime added;
    QStringList tags;
};

class BookmarkStore : public QObject {
    Q_OBJECT

public:
    explicit BookmarkStore(QObject* parent = nullptr);

    bool Load(const QString& filePath);
    bool Save();

    void Add(const QString& url, const QString& title,
             const QStringList& tags = {});
    void Remove(const QString& url);
    bool Contains(const QString& url) const;

    QList<Bookmark> All() const { return bookmarks_; }
    QList<Bookmark> Search(const QString& query) const;

signals:
    void bookmarkAdded(const Bookmark& b);
    void bookmarkRemoved(const QString& url);

private:
    QList<Bookmark> bookmarks_;
    QString         filePath_;
};
