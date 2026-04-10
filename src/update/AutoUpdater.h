#pragma once

/**
 * AutoUpdater — Silent ZAP Browser update checker.
 *
 * Checks GitHub Releases API for a newer version of ZAP.
 * Does NOT auto-install. Shows a non-intrusive notification in the
 * status bar when an update is available. User manually downloads.
 *
 * Endpoint: https://api.github.com/repos/youruser/zap-browser/releases/latest
 */

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct ReleaseInfo {
    QString version;
    QString releaseUrl;
    QString downloadUrl;
    QString releaseNotes;
    bool    isNewer = false;
};

class AutoUpdater : public QObject {
    Q_OBJECT

public:
    static constexpr const char* CURRENT_VERSION = "1.0.0";
    // Change to your actual repo:
    static constexpr const char* RELEASES_API_URL =
        "https://api.github.com/repos/youruser/zap-browser/releases/latest";

    explicit AutoUpdater(QObject* parent = nullptr);

    /** Fire-and-forget update check. Emits updateAvailable() if newer version found. */
    void CheckForUpdates();

signals:
    void updateAvailable(const ReleaseInfo& info);
    void upToDate();
    void checkFailed(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* nam_;

    static bool isNewerVersion(const QString& latest, const QString& current);
};
