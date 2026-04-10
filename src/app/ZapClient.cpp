#include "ZapClient.h"

#ifdef ZAP_CEF_ENABLED

#include "privacy/BlockList.h"
#include "include/cef_request.h"
#include "include/cef_menu_model.h"
#include "include/cef_resource_request_handler.h"

// ─── ZapResourceHandler ───────────────────────────────────────────────────────
// Inner class for per-request ad blocking decision.
class ZapResourceRequestHandler : public CefResourceRequestHandler {
public:
    explicit ZapResourceRequestHandler(bool blocked) : blocked_(blocked) {}

    ReturnValue OnBeforeResourceLoad(
        CefRefPtr<CefBrowser>,
        CefRefPtr<CefFrame>,
        CefRefPtr<CefRequest>,
        CefRefPtr<CefCallback> callback) override
    {
        return blocked_ ? RV_CANCEL : RV_CONTINUE;
    }

private:
    bool blocked_;
    IMPLEMENT_REFCOUNTING(ZapResourceRequestHandler);
};

// ─── ZapClient ────────────────────────────────────────────────────────────────

ZapClient::ZapClient(BlockList* blockList, QObject* parent)
    : QObject(parent), blockList_(blockList)
{}

//─── CefLifeSpanHandler ────────────────────────────────────────────────────────

void ZapClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    emit browserCreated(browser->GetIdentifier());
}

bool ZapClient::DoClose(CefRefPtr<CefBrowser> browser)
{
    // Returning false allows CEF to close.
    return false;
}

void ZapClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    emit browserClosed(browser->GetIdentifier());
}

//─── CefLoadHandler ───────────────────────────────────────────────────────────

void ZapClient::OnLoadStart(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             TransitionType)
{
    if (frame->IsMain()) {
        emit loadStarted(browser->GetIdentifier());
    }
}

void ZapClient::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           int httpStatusCode)
{
    if (frame->IsMain()) {
        emit loadFinished(browser->GetIdentifier(), httpStatusCode);
    }
}

void ZapClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl)
{
    if (errorCode == ERR_ABORTED) return; // user navigated away, ignore

    // Inject a simple error page
    std::string html =
        "<html><body style='font-family:sans-serif;padding:40px;background:#1a1a2e;color:#e0e0e0;'>"
        "<h1>⚡ ZAP — Page Not Available</h1>"
        "<p>Could not load: <code>" + std::string(failedUrl) + "</code></p>"
        "<p>Error: " + std::string(errorText) + "</p>"
        "</body></html>";

    frame->LoadURL("data:text/html;charset=UTF-8," + CefURIEncode(html, false).ToString());
}

//─── CefDisplayHandler ────────────────────────────────────────────────────────

void ZapClient::OnTitleChange(CefRefPtr<CefBrowser> browser,
                               const CefString& title)
{
    emit titleChanged(browser->GetIdentifier(),
                      QString::fromStdWString(title.ToWString()));
}

void ZapClient::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 const CefString& url)
{
    if (frame->IsMain()) {
        emit urlChanged(browser->GetIdentifier(),
                        QString::fromStdWString(url.ToWString()));
    }
}

//─── CefRequestHandler ────────────────────────────────────────────────────────

CefRefPtr<CefResourceRequestHandler> ZapClient::GetResourceRequestHandler(
    CefRefPtr<CefBrowser>,
    CefRefPtr<CefFrame>,
    CefRefPtr<CefRequest> request,
    bool, bool, const CefString&, bool&)
{
    if (blockList_) {
        std::string url = request->GetURL().ToString();
        if (blockList_->IsBlocked(url)) {
            return new ZapResourceRequestHandler(true);
        }
    }
    return nullptr; // proceed normally
}

//─── CefContextMenuHandler ────────────────────────────────────────────────────

void ZapClient::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model)
{
    // Remove developer tools and other unwanted default items
    model->Remove(MENU_ID_VIEW_SOURCE);

    // Add ZAP-specific items
    model->AddSeparator();
    model->AddItem(50001, "⚡ ZAP AI — Explain Selection");
    model->AddItem(50002, "📋 Copy Link");
    model->AddItem(50003, "🔍 Search with DuckDuckGo");
}

#endif // ZAP_CEF_ENABLED
