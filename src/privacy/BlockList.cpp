#include "BlockList.h"

#include <QFile>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>
#include <algorithm>
#include <regex>

BlockList::BlockList(QObject* parent)
    : QObject(parent)
    , refreshTimer_(new QTimer(this))
{
    connect(refreshTimer_, &QTimer::timeout,
            this, &BlockList::onAutoRefresh);
}

void BlockList::LoadFromDirectory(const QString& dir)
{
    listDir_ = dir;
    QStringList files = { dir + "/ads.txt", dir + "/trackers.txt" };
    for (const QString& f : files) {
        if (QFile::exists(f)) LoadFile(f);
    }
    qDebug() << "[BlockList] Loaded" << Count() << "blocked domains";
    emit listsUpdated(Count());
}

void BlockList::LoadFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BlockList] Cannot open:" << path;
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        parseEasyListLine(in.readLine().toStdString());
    }
    file.close();
}

bool BlockList::IsBlocked(const QString& url) const
{
    return IsBlocked(url.toStdString());
}

bool BlockList::IsBlocked(const std::string& url) const
{
    std::string host = extractHostname(url);
    if (host.empty()) return false;

    // Check exact match
    if (blockedDomains_.count(host)) return true;

    // Check parent domains (e.g. "ads.example.com" → "example.com")
    std::string::size_type pos = host.find('.');
    while (pos != std::string::npos) {
        std::string parent = host.substr(pos + 1);
        if (blockedDomains_.count(parent)) return true;
        pos = parent.find('.');
        if (pos != std::string::npos) {
            host = parent;
            pos = host.find('.');
        } else {
            break;
        }
    }
    return false;
}

void BlockList::ScheduleAutoRefresh(int intervalMs)
{
    refreshTimer_->setInterval(intervalMs);
    refreshTimer_->start();
}

void BlockList::onAutoRefresh()
{
    if (listDir_.isEmpty()) return;
    qDebug() << "[BlockList] Auto-refreshing from remote sources...";

    QStringList paths = { listDir_ + "/ads.txt", listDir_ + "/trackers.txt" };
    for (int i = 0; i < remoteSources_.size() && i < paths.size(); ++i) {
        downloadList(remoteSources_[i], paths[i]);
    }
}

void BlockList::downloadList(const QString& url, const QString& savePath)
{
    auto* nam = new QNetworkAccessManager(this);
    auto* reply = nam->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile f(savePath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                blockedDomains_.clear();
                LoadFromDirectory(listDir_);
            }
        }
        reply->deleteLater();
        nam->deleteLater();
    });
}

void BlockList::parseEasyListLine(const std::string& line)
{
    if (line.empty() || line[0] == '!' || line[0] == '[') return; // comment / header

    // Hosts-file format: "0.0.0.0 ads.example.com"
    if (line.substr(0, 7) == "0.0.0.0" || line.substr(0, 9) == "127.0.0.1") {
        std::istringstream iss(line);
        std::string ip, domain;
        iss >> ip >> domain;
        if (!domain.empty()) {
            // Remove www. prefix
            if (domain.substr(0, 4) == "www.") domain = domain.substr(4);
            blockedDomains_.insert(domain);
        }
        return;
    }

    // EasyList filter: "||ads.example.com^" → extract domain
    if (line.substr(0, 2) == "||") {
        std::string rule = line.substr(2);
        // Find end of domain
        auto end = rule.find_first_of("^/");
        if (end != std::string::npos) rule = rule.substr(0, end);
        if (!rule.empty() && rule.find('*') == std::string::npos) {
            blockedDomains_.insert(rule);
        }
        return;
    }

    // Plain domain (one per line)
    if (line.find(' ') == std::string::npos &&
        line.find('/') == std::string::npos &&
        line.find('#') == std::string::npos)
    {
        blockedDomains_.insert(line);
    }
}

std::string BlockList::extractHostname(const std::string& url) const
{
    // Find "://" then extract up to next "/" or ":"
    auto schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) return url;
    std::string rest = url.substr(schemeEnd + 3);
    auto hostEnd = rest.find_first_of("/:?#");
    std::string host = (hostEnd == std::string::npos) ? rest : rest.substr(0, hostEnd);
    // Strip "www."
    if (host.substr(0, 4) == "www.") host = host.substr(4);
    return host;
}
