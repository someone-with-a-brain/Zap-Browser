#pragma once

/**
 * ZapApp — CefApp + CefBrowserProcessHandler implementation.
 *
 * This class is the central hook into CEF's lifecycle for the main
 * (browser) process. It configures CEF command-line switches at startup
 * and handles browser-process-specific initialization.
 */

#ifdef ZAP_CEF_ENABLED
#include "include/cef_app.h"
#include "include/cef_browser_process_handler.h"
#include "include/cef_command_line.h"

class ZapApp : public CefApp, public CefBrowserProcessHandler {
public:
    ZapApp() = default;

    //─── CefApp ───────────────────────────────────────────────────────────────
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
        return this;
    }

    /**
     * Called before command-line processing. We use this to:
     * - Disable GPU sandbox (often needed on Windows)
     * - Set proxy switches if Tor is pre-enabled
     * - Disable crash reporting and metrics
     */
    void OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line) override;

    //─── CefBrowserProcessHandler ─────────────────────────────────────────────
    void OnContextInitialized() override;

private:
    IMPLEMENT_REFCOUNTING(ZapApp);
};
#else
// Stub when CEF is not available (for IDE / stub builds)
class ZapApp {};
#endif
