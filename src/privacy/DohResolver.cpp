#include "DohResolver.h"
#include <QDebug>

const QList<DohProvider> DohResolver::KnownProviders = {
    {
        "cloudflare",
        "https://cloudflare-dns.com/dns-query",
        "Cloudflare 1.1.1.1 — fast, no logging"
    },
    {
        "quad9",
        "https://dns.quad9.net/dns-query",
        "Quad9 — malware blocking, no logging"
    },
    {
        "google",
        "https://dns.google/dns-query",
        "Google Public DNS"
    },
    {
        "mullvad",
        "https://dns.mullvad.net/dns-query",
        "Mullvad — ad/tracker blocking, no logging"
    }
};

DohResolver::DohResolver(QObject* parent)
    : QObject(parent)
{}

QString DohResolver::CefSwitch(const QString& dohUrl)
{
    // Chromium 89+ supports DoH natively via this switch.
    // Inject in ZapApp::OnBeforeCommandLineProcessing:
    //   command_line->AppendSwitchWithValue("use-doh", dohUrl.toStdString());
    return dohUrl;
}

void DohResolver::Enable(const QString& provider)
{
    for (const auto& p : KnownProviders) {
        if (p.name == provider) {
            currentProvider_ = provider;
            enabled_ = true;
            qDebug() << "[DoH] Enabled:" << p.url;
            emit stateChanged(true, provider);
            return;
        }
    }
    qWarning() << "[DoH] Unknown provider:" << provider;
}

void DohResolver::Disable()
{
    enabled_ = false;
    qDebug() << "[DoH] Disabled";
    emit stateChanged(false, currentProvider_);
}
