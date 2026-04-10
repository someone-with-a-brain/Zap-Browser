#include "CookieIsolator.h"

#include <QDir>
#include <QDebug>

#ifdef ZAP_CEF_ENABLED
#include "include/cef_request_context_handler.h"
#endif

CookieIsolator::CookieIsolator(const QString& profileBaseDir, QObject* parent)
    : QObject(parent), profileBaseDir_(profileBaseDir)
{
    QDir().mkpath(profileBaseDir_);
}

#ifdef ZAP_CEF_ENABLED

CefRefPtr<CefRequestContext> CookieIsolator::GetContextForTab(
    int tabId, bool ephemeral)
{
    if (contexts_.contains(tabId)) {
        return contexts_[tabId];
    }

    CefRefPtr<CefRequestContext> ctx;

    if (ephemeral) {
        // In-memory context — no disk writes (private/incognito tab)
        CefRequestContextSettings settings;
        ctx = CefRequestContext::CreateContext(settings, nullptr);
        qDebug() << "[CookieIsolator] Created ephemeral context for tab" << tabId;
    } else {
        // Disk-backed isolated context per tab
        QString profilePath = profileBaseDir_ + QString("/tab_%1").arg(tabId);
        QDir().mkpath(profilePath);
        ctx = createIsolatedContext(profilePath);
        qDebug() << "[CookieIsolator] Created isolated context for tab"
                 << tabId << "at" << profilePath;
    }

    contexts_[tabId] = ctx;
    return ctx;
}

void CookieIsolator::ReleaseContextForTab(int tabId)
{
    contexts_.remove(tabId);
}

void CookieIsolator::ClearAllCookies()
{
    for (auto& ctx : contexts_) {
        ctx->GetCookieManager(nullptr)->DeleteCookies(
            CefString(), CefString(), nullptr);
    }
    qDebug() << "[CookieIsolator] Cleared cookies for all" << contexts_.size() << "tabs";
}

CefRefPtr<CefRequestContext> CookieIsolator::createIsolatedContext(
    const QString& profilePath) const
{
    CefRequestContextSettings settings;
    CefString(&settings.cache_path) = profilePath.toStdWString();
    settings.persist_session_cookies = false; // Don't persist session cookies
    settings.persist_user_preferences = false;
    return CefRequestContext::CreateContext(settings, nullptr);
}

#endif // ZAP_CEF_ENABLED

void CookieIsolator::DeleteTabProfile(int tabId)
{
#ifdef ZAP_CEF_ENABLED
    ReleaseContextForTab(tabId);
#endif
    QString profilePath = profileBaseDir_ + QString("/tab_%1").arg(tabId);
    QDir dir(profilePath);
    if (dir.exists()) {
        dir.removeRecursively();
        qDebug() << "[CookieIsolator] Deleted profile for tab" << tabId;
    }
}
