#pragma once

/**
 * ZapClient — The central hub of all CEF browser events.
 *
 * Implements every CEF handler interface we need:
 *  - CefLifeSpanHandler  : created/closed browser events
 *  - CefLoadHandler      : page load start/end/error
 *  - CefDisplayHandler   : title, URL, favicon changes
 *  - CefRequestHandler   : request interception (ad blocking)
 *  - CefContextMenuHandler: right-click customization
 *  - CefKeyboardHandler  : keyboard shortcuts
 */

#ifdef ZAP_CEF_ENABLED
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_display_handler.h"
#include "include/cef_request_handler.h"
#include "include/cef_context_menu_handler.h"
#include "include/cef_keyboard_handler.h"
#include "include/cef_resource_request_handler.h"

#include <functional>
#include <string>
#endif

#include <QObject>
#include <QString>
#include <functional>

// Forward declarations
class BlockList;
class MainWindow;

#ifdef ZAP_CEF_ENABLED

class ZapClient : public QObject,
                  public CefClient,
                  public CefLifeSpanHandler,
                  public CefLoadHandler,
                  public CefDisplayHandler,
                  public CefRequestHandler,
                  public CefContextMenuHandler,
                  public CefKeyboardHandler
{
    Q_OBJECT

public:
    explicit ZapClient(BlockList* blockList, QObject* parent = nullptr);
    ~ZapClient() override = default;

    //─── CefClient handler getters ────────────────────────────────────────────
    CefRefPtr<CefLifeSpanHandler>    GetLifeSpanHandler()    override { return this; }
    CefRefPtr<CefLoadHandler>        GetLoadHandler()        override { return this; }
    CefRefPtr<CefDisplayHandler>     GetDisplayHandler()     override { return this; }
    CefRefPtr<CefRequestHandler>     GetRequestHandler()     override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
    CefRefPtr<CefKeyboardHandler>    GetKeyboardHandler()    override { return this; }

    //─── CefLifeSpanHandler ───────────────────────────────────────────────────
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    //─── CefLoadHandler ───────────────────────────────────────────────────────
    void OnLoadStart(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     TransitionType transition_type) override;

    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   int httpStatusCode) override;

    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override;

    //─── CefDisplayHandler ────────────────────────────────────────────────────
    void OnTitleChange(CefRefPtr<CefBrowser> browser,
                       const CefString& title) override;

    void OnAddressChange(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         const CefString& url) override;

    bool OnTooltip(CefRefPtr<CefBrowser> browser,
                   CefString& text) override { return false; }

    //─── CefRequestHandler ────────────────────────────────────────────────────
    /**
     * Called before every resource request. This is where we block
     * ads and trackers by checking the BlockList.
     */
    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool is_navigation,
        bool is_download,
        const CefString& request_initiator,
        bool& disable_default_handling) override;

    bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefRequest> request,
                        bool user_gesture,
                        bool is_redirect) override { return false; }

    //─── CefContextMenuHandler ────────────────────────────────────────────────
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefContextMenuParams> params,
                             CefRefPtr<CefMenuModel> model) override;

signals:
    void titleChanged(int browserId, const QString& title);
    void urlChanged(int browserId, const QString& url);
    void loadStarted(int browserId);
    void loadFinished(int browserId, int httpStatus);
    void browserCreated(int browserId);
    void browserClosed(int browserId);

private:
    BlockList* blockList_ = nullptr;

    IMPLEMENT_REFCOUNTING(ZapClient);
};

#else
// Stub for builds without CEF
class ZapClient : public QObject {
    Q_OBJECT
public:
    explicit ZapClient(void* blockList, QObject* parent = nullptr) : QObject(parent) {}
signals:
    void titleChanged(int browserId, const QString& title);
    void urlChanged(int browserId, const QString& url);
    void loadStarted(int browserId);
    void loadFinished(int browserId, int httpStatus);
    void browserCreated(int browserId);
    void browserClosed(int browserId);
};
#endif
