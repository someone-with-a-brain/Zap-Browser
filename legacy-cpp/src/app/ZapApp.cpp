#include "ZapApp.h"

#ifdef ZAP_CEF_ENABLED

void ZapApp::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line)
{
    // Only modify the main browser process flags
    if (process_type.empty()) {
        // ── Disable Google services / telemetry ──────────────────────────────
        command_line->AppendSwitch("disable-sync");
        command_line->AppendSwitch("disable-translate");
        command_line->AppendSwitch("disable-background-networking");
        command_line->AppendSwitch("disable-client-side-phishing-detection");
        command_line->AppendSwitch("disable-component-update");
        command_line->AppendSwitch("disable-default-apps");
        command_line->AppendSwitch("disable-extensions");
        command_line->AppendSwitch("no-first-run");
        command_line->AppendSwitch("no-default-browser-check");
        command_line->AppendSwitch("disable-breakpad");     // No crash reporting
        command_line->AppendSwitch("disable-logging");

        // ── Performance ──────────────────────────────────────────────────────
        command_line->AppendSwitchWithValue("renderer-process-limit", "4");
        command_line->AppendSwitch("enable-gpu-rasterization");
        command_line->AppendSwitch("enable-zero-copy");

        // ── Sandbox workaround (Windows, may be needed) ──────────────────────
        // Only enable if needed for your environment:
        // command_line->AppendSwitch("no-sandbox");
    }
}

void ZapApp::OnContextInitialized()
{
    // Browser process context is ready.
    // Custom scheme handlers or DevTools registration go here.
}

#endif // ZAP_CEF_ENABLED
