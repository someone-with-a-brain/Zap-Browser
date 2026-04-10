#pragma once

/**
 * DohResolver — DNS over HTTPS for ZAP Browser.
 *
 * Strategy: System-level DoH is handled by configuring CEF's network layer
 * to use an encrypted DNS resolver. This is done by:
 *  - Setting DoH server via CEF's command-line switch:
 *    --use-doh=https://cloudflare-dns.com/dns-query (Chromium 89+)
 *  - OR: overriding DNS at the OS level for the process (platform-specific)
 *
 * This class manages the CEF command-line injection at startup and provides
 * a runtime toggle interface for the settings page.
 *
 * Supported providers: Cloudflare (1.1.1.1), Quad9, Google.
 */

#include <QObject>
#include <QString>
#include <QStringList>

struct DohProvider {
    QString name;
    QString url;
    QString description;
};

class DohResolver : public QObject {
    Q_OBJECT

public:
    // Known DoH providers
    static const QList<DohProvider> KnownProviders;

    explicit DohResolver(QObject* parent = nullptr);

    /** Returns the CEF command-line switch to inject at startup. */
    static QString CefSwitch(const QString& dohUrl =
        "https://cloudflare-dns.com/dns-query");

    /** Enable DoH — injects into CEF prefs at runtime (requires restart for full effect). */
    void Enable(const QString& provider = "cloudflare");
    void Disable();

    bool IsEnabled() const { return enabled_; }
    QString CurrentProvider() const { return currentProvider_; }

signals:
    void stateChanged(bool enabled, const QString& provider);

private:
    bool    enabled_         = false;
    QString currentProvider_ = "cloudflare";
};
