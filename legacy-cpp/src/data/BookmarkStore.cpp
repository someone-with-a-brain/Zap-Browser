#include "BookmarkStore.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

BookmarkStore::BookmarkStore(QObject* parent)
    : QObject(parent)
{}

bool BookmarkStore::Load(const QString& filePath)
{
    filePath_ = filePath;
    QFile f(filePath);
    if (!f.exists()) return true; // first run, empty store

    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("[Bookmarks] Cannot open: %s", qPrintable(filePath));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonArray arr = doc.object()["bookmarks"].toArray();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        Bookmark b;
        b.url   = obj["url"].toString();
        b.title = obj["title"].toString();
        b.added = QDateTime::fromSecsSinceEpoch(obj["added"].toInt());
        for (const auto& t : obj["tags"].toArray())
            b.tags << t.toString();
        bookmarks_.append(b);
    }
    return true;
}

bool BookmarkStore::Save()
{
    if (filePath_.isEmpty()) return false;

    QJsonArray arr;
    for (const auto& b : bookmarks_) {
        QJsonObject obj;
        obj["url"]   = b.url;
        obj["title"] = b.title;
        obj["added"] = (qint64)b.added.toSecsSinceEpoch();
        QJsonArray tags;
        for (const auto& t : b.tags) tags.append(t);
        obj["tags"]  = tags;
        arr.append(obj);
    }

    QJsonObject root;
    root["bookmarks"] = arr;

    QFile f(filePath_);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("[Bookmarks] Cannot write: %s", qPrintable(filePath_));
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

void BookmarkStore::Add(const QString& url, const QString& title,
                        const QStringList& tags)
{
    if (Contains(url)) return; // no duplicates
    Bookmark b;
    b.url   = url;
    b.title = title;
    b.added = QDateTime::currentDateTime();
    b.tags  = tags;
    bookmarks_.prepend(b);
    Save();
    emit bookmarkAdded(b);
}

void BookmarkStore::Remove(const QString& url)
{
    auto it = std::remove_if(bookmarks_.begin(), bookmarks_.end(),
        [&](const Bookmark& b){ return b.url == url; });
    if (it != bookmarks_.end()) {
        bookmarks_.erase(it, bookmarks_.end());
        Save();
        emit bookmarkRemoved(url);
    }
}

bool BookmarkStore::Contains(const QString& url) const
{
    return std::any_of(bookmarks_.begin(), bookmarks_.end(),
        [&](const Bookmark& b){ return b.url == url; });
}

QList<Bookmark> BookmarkStore::Search(const QString& query) const
{
    QList<Bookmark> results;
    for (const auto& b : bookmarks_) {
        if (b.title.contains(query, Qt::CaseInsensitive) ||
            b.url.contains(query, Qt::CaseInsensitive))
        {
            results.append(b);
        }
    }
    return results;
}
