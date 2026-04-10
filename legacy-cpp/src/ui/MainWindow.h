#pragma once

/**
 * MainWindow — The ZAP Browser main window.
 *
 * Hosts:
 *  - Top toolbar (navigation, address bar, privacy badge, AI toggle)
 *  - Custom TabBar + tab content area (CEF browser widgets)
 *  - Collapsible AISidebar
 *  - Bottom status bar (Tor status, page load progress)
 *
 * Owns all top-level subsystems: TorManager, BlockList,
 * OllamaBridge, HistoryDB, and the ZapClient (CEF handler).
 */

#include <QMainWindow>
#include <QStackedWidget>
#include <QSplitter>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QMap>

class TabBar;
class AddressBar;
class AISidebar;
class SettingsPage;
class TorManager;
class BlockList;
class OllamaBridge;
class HistoryDB;
class BookmarkStore;
class ZapClient;
class AutoUpdater;

#ifdef ZAP_CEF_ENABLED
#include "include/cef_browser.h"
#endif

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    void newTab(const QString& url = "https://duckduckgo.com");
    void closeTab(int index);
    void navigateTo(const QString& urlOrSearch);
    void goBack();
    void goForward();
    void reload();
    void toggleTor(bool enabled);
    void toggleAI(bool visible);
    void openSettings();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event)   override;
    void keyPressEvent(QKeyEvent* event)  override;

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onUrlChanged(int browserId, const QString& url);
    void onTitleChanged(int browserId, const QString& title);
    void onLoadStarted(int browserId);
    void onLoadFinished(int browserId, int status);
    void onTorBootstrapped();
    void onTorProgress(int pct, const QString& status);
    void onTorError(const QString& msg);
    void onAddressBarSubmit(const QString& input);
    void onPageSummarizeRequested();

private:
    //─── UI Construction ──────────────────────────────────────────────────────
    void setupToolbar();
    void setupStatusBar();
    void setupShortcuts();
    void applyTheme(const QString& theme);  // "dark" | "light"

    //─── Tab / CEF management ─────────────────────────────────────────────────
    struct TabData {
        int     browserId = -1;
        QString url;
        QString title;
        bool    sleeping  = false;
        QWidget* container = nullptr;
    };

    int  currentBrowserId() const;
    void buildBrowserContainer(TabData& tab, const QString& url);
    void repositionCefWidget(int tabIndex);

    //─── Subsystems ───────────────────────────────────────────────────────────
    TorManager*    torManager_    = nullptr;
    BlockList*     blockList_     = nullptr;
    OllamaBridge*  ollamaBridge_  = nullptr;
    HistoryDB*     historyDB_     = nullptr;
    BookmarkStore* bookmarkStore_ = nullptr;
    AutoUpdater*   autoUpdater_   = nullptr;

#ifdef ZAP_CEF_ENABLED
    ZapClient*     zapClient_     = nullptr;
#endif

    //─── UI Elements ──────────────────────────────────────────────────────────
    TabBar*        tabBar_        = nullptr;
    QStackedWidget* tabStack_     = nullptr;
    AddressBar*    addressBar_    = nullptr;
    AISidebar*     aiSidebar_     = nullptr;
    QSplitter*     mainSplitter_  = nullptr;

    QPushButton*   torButton_     = nullptr;
    QPushButton*   aiButton_      = nullptr;
    QLabel*        statusLabel_   = nullptr;
    QProgressBar*  loadProgress_  = nullptr;

    QMap<int, TabData> tabs_; // browserId → TabData
    int activeTabIndex_ = -1;
    QString currentTheme_ = "dark";

    bool torEnabled_ = false;
};
