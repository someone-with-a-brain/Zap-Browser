#pragma once

/**
 * CookieIsolator — Per-tab cookie store isolation.
 *
 * Each browser tab gets its own CefCookieManager instance with
 * a separate on-disk profile. This prevents cross-tab cookie sharing
 * and provides container-tab-style isolation without requiring sign-out.
 *
 * Integration:
 *  - Called by ZapClient when creating a new CefBrowser
 *  - Returns a unique CefRequestContext per tab
 *  - Optional: ephemeral (in-memory) mode for private tabs
 */

#include <QObject>
#include <QString>
#include <QMap>

#ifdef ZAP_CEF_ENABLED
#include "include/cef_request_context.h"
#include "include/cef_cookie.h"
#endif

class CookieIsolator : public QObject {
    Q_OBJECT

public:
    explicit CookieIsolator(const QString& profileBaseDir,
                             QObject* parent = nullptr);

    /**
     * Returns a CefRequestContext for the given tab ID.
     * Creates a new isolated context if one doesn't exist.
     *
     * @param tabId     Unique tab identifier
     * @param ephemeral If true, use in-memory (private) context
     */
#ifdef ZAP_CEF_ENABLED
    CefRefPtr<CefRequestContext> GetContextForTab(int tabId,
                                                   bool ephemeral = false);
    void ReleaseContextForTab(int tabId);
    void ClearAllCookies();
#endif

    /** Delete disk profile for a specific tab. */
    void DeleteTabProfile(int tabId);

private:
    QString profileBaseDir_;

#ifdef ZAP_CEF_ENABLED
    QMap<int, CefRefPtr<CefRequestContext>> contexts_;
    CefRefPtr<CefRequestContext> createIsolatedContext(
        const QString& profilePath) const;
#endif
};
