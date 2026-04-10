#include "AutoUpdater.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

AutoUpdater::AutoUpdater(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{
    connect(nam_, &QNetworkAccessManager::finished,
            this, &AutoUpdater::onReplyFinished);
}

void AutoUpdater::CheckForUpdates()
{
    QNetworkRequest req(QUrl(RELEASES_API_URL));
    req.setHeader(QNetworkRequest::UserAgentHeader, "ZapBrowser/1.0");
    req.setTransferTimeout(10000);
    nam_->get(req);
    qDebug() << "[AutoUpdater] Checking for updates...";
}

void AutoUpdater::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit checkFailed("Invalid response from releases API");
        return;
    }

    QJsonObject obj = doc.object();
    QString tagName = obj["tag_name"].toString();
    QString version = tagName.startsWith('v') ? tagName.mid(1) : tagName;

    if (version.isEmpty()) {
        emit checkFailed("No version tag in release");
        return;
    }

    ReleaseInfo info;
    info.version      = version;
    info.releaseUrl   = obj["html_url"].toString();
    info.releaseNotes = obj["body"].toString().left(500);  // Trim long notes

    // Find the download URL from assets
    QJsonArray assets = obj["assets"].toArray();
    for (const auto& asset : assets) {
        QString name = asset.toObject()["name"].toString();
        if (name.endsWith(".exe") || name.endsWith(".zip")) {
            info.downloadUrl = asset.toObject()["browser_download_url"].toString();
            break;
        }
    }

    info.isNewer = isNewerVersion(version, CURRENT_VERSION);

    if (info.isNewer) {
        qDebug() << "[AutoUpdater] Update available:" << version;
        emit updateAvailable(info);
    } else {
        qDebug() << "[AutoUpdater] Up to date (" << CURRENT_VERSION << ")";
        emit upToDate();
    }
}

bool AutoUpdater::isNewerVersion(const QString& latest, const QString& current)
{
    auto parse = [](const QString& v) -> QList<int> {
        QList<int> parts;
        for (const QString& p : v.split('.'))
            parts.append(p.toInt());
        while (parts.size() < 3) parts.append(0);
        return parts;
    };

    QList<int> l = parse(latest);
    QList<int> c = parse(current);

    for (int i = 0; i < 3; ++i) {
        if (l[i] > c[i]) return true;
        if (l[i] < c[i]) return false;
    }
    return false;
}
