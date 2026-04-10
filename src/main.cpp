/**
 * ZAP Browser — main.cpp
 * Entry point: initializes CEF subprocess check, creates Qt app, launches UI.
 *
 * CEF and Qt must coexist carefully on Windows:
 *  - CefExecuteProcess must be called BEFORE QApplication is created.
 *  - CEF message loop is driven via CefRunMessageLoop() or manually pumped.
 *  - We use Qt's event loop and manually pump CEF with a QTimer.
 */

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef ZAP_CEF_ENABLED
#include "include/cef_app.h"
#include "include/cef_base.h"
#endif

#include "app/ZapApp.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    //──────────────────────────────────────────────────────────────────────────
    // 1. CEF Subprocess Check (Windows)
    //    CEF launches sub-processes (renderer, GPU, network) by re-invoking
    //    the same executable with special command-line flags. If this is a
    //    subprocess, CefExecuteProcess handles it and returns >= 0 immediately.
    //──────────────────────────────────────────────────────────────────────────
#ifdef ZAP_CEF_ENABLED
    CefMainArgs main_args(GetModuleHandle(nullptr));
    CefRefPtr<ZapApp> zapCefApp = new ZapApp();
    int exit_code = CefExecuteProcess(main_args, zapCefApp, nullptr);
    if (exit_code >= 0) {
        return exit_code; // this is a CEF subprocess — exit normally
    }
#endif

    //──────────────────────────────────────────────────────────────────────────
    // 2. Qt Application Setup
    //──────────────────────────────────────────────────────────────────────────
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("ZAP Browser");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ZAP");

    // Ensure app data directory exists
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);

    //──────────────────────────────────────────────────────────────────────────
    // 3. CEF Initialization
    //──────────────────────────────────────────────────────────────────────────
#ifdef ZAP_CEF_ENABLED
    CefSettings settings;
    settings.no_sandbox = true;
    settings.multi_threaded_message_loop = false; // We pump CEF manually

    // Cache & user-data paths (local, never synced)
    CefString(&settings.cache_path) =
        (dataPath + "/cache").toStdWString();
    CefString(&settings.user_data_path) =
        (dataPath + "/userdata").toStdWString();

    // Disable CEF telemetry / crash reporting
    settings.remote_debugging_port = 0;
    CefString(&settings.log_file) = (dataPath + "/cef.log").toStdWString();
    settings.log_severity = LOGSEVERITY_WARNING;

    if (!CefInitialize(main_args, settings, zapCefApp, nullptr)) {
        qCritical("Failed to initialize CEF");
        return 1;
    }
#endif

    //──────────────────────────────────────────────────────────────────────────
    // 4. Launch Main Window
    //──────────────────────────────────────────────────────────────────────────
    MainWindow window;
    window.show();

#ifdef ZAP_CEF_ENABLED
    // Pump CEF message loop every 10ms alongside Qt
    QTimer cefTimer;
    QObject::connect(&cefTimer, &QTimer::timeout, []() {
        CefDoMessageLoopWork();
    });
    cefTimer.start(10);
#endif

    int result = app.exec();

    //──────────────────────────────────────────────────────────────────────────
    // 5. Shutdown CEF cleanly
    //──────────────────────────────────────────────────────────────────────────
#ifdef ZAP_CEF_ENABLED
    CefShutdown();
#endif

    return result;
}
