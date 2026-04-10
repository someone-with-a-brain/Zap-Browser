#include "MainWindow.h"
#include "TabBar.h"
#include "AddressBar.h"
#include "AISidebar.h"
#include "SettingsPage.h"

#include "privacy/TorManager.h"
#include "privacy/BlockList.h"
#include "ai/OllamaBridge.h"
#include "data/HistoryDB.h"
#include "data/BookmarkStore.h"
#include "update/AutoUpdater.h"

#ifdef ZAP_CEF_ENABLED
#include "app/ZapClient.h"
#include "include/cef_browser.h"
#include "include/cef_browser_host.h"
#endif

#include <QApplication>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QKeySequence>
#include <QShortcut>
#include <QFile>
#include <QStandardPaths>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>

#ifdef WIN32
#include <windows.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("⚡ ZAP Browser");
    setMinimumSize(900, 600);
    resize(1280, 800);

    //─── Subsystems ──────────────────────────────────────────────────────────
    blockList_     = new BlockList(this);
    torManager_    = new TorManager(this);
    ollamaBridge_  = new OllamaBridge(this);
    historyDB_     = new HistoryDB(this);
    bookmarkStore_ = new BookmarkStore(this);
    autoUpdater_   = new AutoUpdater(this);

    QString dataPath = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);

    // Load block lists
    blockList_->LoadFromDirectory(
        QApplication::applicationDirPath() + "/assets/blocklists");
    blockList_->ScheduleAutoRefresh();

    // Open history DB
    historyDB_->Open(dataPath + "/history.db");
    bookmarkStore_->Load(dataPath + "/bookmarks.json");

    // Connect Tor signals
    connect(torManager_, &TorManager::torBootstrapped,
            this, &MainWindow::onTorBootstrapped);
    connect(torManager_, &TorManager::torBootstrapProgress,
            this, &MainWindow::onTorProgress);
    connect(torManager_, &TorManager::torError,
            this, &MainWindow::onTorError);

    // Connect Ollama
    ollamaBridge_->CheckAvailability();

#ifdef ZAP_CEF_ENABLED
    zapClient_ = new ZapClient(blockList_, this);
    connect(zapClient_, &ZapClient::titleChanged,
            this, &MainWindow::onTitleChanged);
    connect(zapClient_, &ZapClient::urlChanged,
            this, &MainWindow::onUrlChanged);
    connect(zapClient_, &ZapClient::loadStarted,
            this, &MainWindow::onLoadStarted);
    connect(zapClient_, &ZapClient::loadFinished,
            this, &MainWindow::onLoadFinished);
#endif

    //─── UI ───────────────────────────────────────────────────────────────────
    setupToolbar();
    setupStatusBar();
    setupShortcuts();
    applyTheme("dark");

    // Central widget: splitter → [tab stack | AI sidebar]
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);

    QWidget* browserArea = new QWidget(this);
    QVBoxLayout* browserLayout = new QVBoxLayout(browserArea);
    browserLayout->setContentsMargins(0, 0, 0, 0);
    browserLayout->setSpacing(0);
    browserLayout->addWidget(tabBar_);

    tabStack_ = new QStackedWidget(this);
    browserLayout->addWidget(tabStack_);
    browserArea->setLayout(browserLayout);

    aiSidebar_ = new AISidebar(ollamaBridge_, this);
    aiSidebar_->setVisible(false);
    aiSidebar_->setFixedWidth(340);

    connect(ollamaBridge_, &OllamaBridge::modelsListed, this, [this](const QList<OllamaModel>& models) {
        QStringList names;
        for (auto& m : models) names << m.name;
        aiSidebar_->SetAvailableModels(names);
    });

    // Summarize page coordinator
    connect(aiSidebar_, &AISidebar::summarizeRequested,
            this, &MainWindow::onPageSummarizeRequested);

    mainSplitter_->addWidget(browserArea);
    mainSplitter_->addWidget(aiSidebar_);
    mainSplitter_->setStretchFactor(0, 1);
    mainSplitter_->setStretchFactor(1, 0);

    setCentralWidget(mainSplitter_);

    // Open default tab
    newTab("https://duckduckgo.com");

    // Check for updates silently
    autoUpdater_->CheckForUpdates();
}

MainWindow::~MainWindow() = default;

// ─── Tab Management ──────────────────────────────────────────────────────────

void MainWindow::newTab(const QString& url)
{
    TabData tab;
    tab.url   = url;
    tab.title = "New Tab";

    int idx = tabBar_->AddTab("New Tab");
    tab.browserId = idx; // placeholder until CEF assigns real ID

    QWidget* container = new QWidget(tabStack_);
    container->setObjectName("cef-container");
    tab.container = container;
    tabStack_->addWidget(container);

#ifdef ZAP_CEF_ENABLED
    CefWindowInfo windowInfo;
    HWND hwnd = (HWND)container->winId();
    windowInfo.SetAsChild(hwnd, CefRect(0, 0, container->width(), container->height()));

    CefBrowserSettings browserSettings;
    browserSettings.background_color = 0xFF1A1A2E;

    CefRefPtr<ZapClient> client(zapClient_);
    CefBrowserHost::CreateBrowser(windowInfo, client,
        url.toStdString(), browserSettings, nullptr, nullptr);
#else
    // Stub: just show a placeholder label
    QLabel* placeholder = new QLabel(
        QString("<center><h2 style='color:#7c83fd'>⚡ ZAP</h2>"
                "<p style='color:#aaa'>Loading: %1</p></center>").arg(url),
        container);
    QVBoxLayout* l = new QVBoxLayout(container);
    l->addWidget(placeholder);
#endif

    tabs_[tab.browserId] = tab;
    tabBar_->SetCurrentIndex(idx);
    tabStack_->setCurrentWidget(container);
    activeTabIndex_ = idx;
}

void MainWindow::closeTab(int index)
{
    if (tabBar_->Count() <= 1) {
        close(); // last tab → close window
        return;
    }
    tabBar_->RemoveTab(index);
    QWidget* w = tabStack_->widget(index);
    tabStack_->removeWidget(w);
    w->deleteLater();
}

// ─── Navigation ──────────────────────────────────────────────────────────────

void MainWindow::navigateTo(const QString& urlOrSearch)
{
#ifdef ZAP_CEF_ENABLED
    int bid = currentBrowserId();
    // Find the browser by ID and navigate
    // (CefBrowserHost::GetBrowser lookup via ZapClient registry omitted for brevity)
    // In full impl: zapClient_->GetBrowser(bid)->GetMainFrame()->LoadURL(...)
#else
    addressBar_->SetUrl(urlOrSearch);
#endif
    historyDB_->RecordVisit(urlOrSearch, urlOrSearch);
}

void MainWindow::goBack()
{
#ifdef ZAP_CEF_ENABLED
    // browser->GoBack();
#endif
}

void MainWindow::goForward()
{
#ifdef ZAP_CEF_ENABLED
    // browser->GoForward();
#endif
}

void MainWindow::reload()
{
#ifdef ZAP_CEF_ENABLED
    // browser->Reload();
#endif
}

// ─── Privacy ─────────────────────────────────────────────────────────────────

void MainWindow::toggleTor(bool enabled)
{
    torEnabled_ = enabled;
    if (enabled) {
        torButton_->setText("🧅 Tor: Starting...");
        torButton_->setEnabled(false);
        torManager_->Start();
    } else {
        torManager_->Stop();
        torButton_->setText("🌐 Tor: Off");
        statusLabel_->setText("Tor disabled");
    }
}

void MainWindow::onTorProgress(int pct, const QString& status)
{
    torButton_->setText(QString("🧅 Tor: %1%").arg(pct));
    statusLabel_->setText(QString("Tor bootstrapping: %1% — %2").arg(pct).arg(status));
}

void MainWindow::onTorBootstrapped()
{
    torButton_->setText("🧅 Tor: ON");
    torButton_->setEnabled(true);
    statusLabel_->setText("🔒 Tor active — All traffic anonymized");
}

void MainWindow::onTorError(const QString& msg)
{
    torButton_->setText("🌐 Tor: Error");
    torButton_->setEnabled(true);
    statusLabel_->setText("Tor error: " + msg);
}

// ─── AI ──────────────────────────────────────────────────────────────────────

void MainWindow::toggleAI(bool visible)
{
    aiSidebar_->setVisible(visible);
    if (visible) ollamaBridge_->ListModels();
}

void MainWindow::onPageSummarizeRequested()
{
#ifdef ZAP_CEF_ENABLED
    // Execute JS to extract page text, then pass to sidebar
    // int bid = currentBrowserId();
    // browser->GetMainFrame()->ExecuteJavaScript(
    //   "document.body.innerText.substring(0, 4000)",
    //   ..., page_text_callback);
#else
    aiSidebar_->SummarizePage("(Page text extraction requires CEF)");
#endif
}

// ─── Slot handlers ───────────────────────────────────────────────────────────

void MainWindow::onTabChanged(int index)
{
    activeTabIndex_ = index;
    tabStack_->setCurrentIndex(index);
}

void MainWindow::onTabCloseRequested(int index)
{
    closeTab(index);
}

void MainWindow::onUrlChanged(int browserId, const QString& url)
{
    if (tabs_.contains(browserId)) {
        tabs_[browserId].url = url;
        addressBar_->SetUrl(url);
        addressBar_->SetSecure(url.startsWith("https://"));
    }
}

void MainWindow::onTitleChanged(int browserId, const QString& title)
{
    if (tabs_.contains(browserId)) {
        tabs_[browserId].title = title;
        // Find tab index by browserId
        for (int i = 0; i < tabBar_->Count(); ++i) {
            if (tabs_.contains(browserId)) {
                tabBar_->SetTabTitle(i, title);
                break;
            }
        }
        setWindowTitle(title + " — ⚡ ZAP");
    }
}

void MainWindow::onLoadStarted(int)
{
    loadProgress_->setVisible(true);
    loadProgress_->setValue(0);
    loadProgress_->setMaximum(0); // indeterminate
}

void MainWindow::onLoadFinished(int browserId, int status)
{
    loadProgress_->setVisible(false);
    if (status >= 400) {
        statusLabel_->setText(QString("Error %1").arg(status));
    }
}

void MainWindow::onAddressBarSubmit(const QString& input)
{
    navigateTo(input);
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void MainWindow::setupToolbar()
{
    QToolBar* toolbar = addToolBar("Navigation");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setObjectName("main-toolbar");

    // Navigation buttons
    QPushButton* backBtn    = new QPushButton("←", this);
    QPushButton* fwdBtn     = new QPushButton("→", this);
    QPushButton* reloadBtn  = new QPushButton("↺", this);

    backBtn->setObjectName("nav-btn");
    fwdBtn->setObjectName("nav-btn");
    reloadBtn->setObjectName("nav-btn");
    backBtn->setFixedSize(32, 32);
    fwdBtn->setFixedSize(32, 32);
    reloadBtn->setFixedSize(32, 32);

    connect(backBtn,   &QPushButton::clicked, this, &MainWindow::goBack);
    connect(fwdBtn,    &QPushButton::clicked, this, &MainWindow::goForward);
    connect(reloadBtn, &QPushButton::clicked, this, &MainWindow::reload);

    // Tab bar
    tabBar_ = new TabBar(this);
    connect(tabBar_, &TabBar::tabChanged,         this, &MainWindow::onTabChanged);
    connect(tabBar_, &TabBar::tabCloseRequested,  this, &MainWindow::onTabCloseRequested);
    connect(tabBar_, &TabBar::newTabRequested,     this, [this]{ newTab(); });

    // Address bar
    addressBar_ = new AddressBar(this);
    connect(addressBar_, &AddressBar::submitted,
            this, &MainWindow::onAddressBarSubmit);

    // Tor toggle button
    torButton_ = new QPushButton("🌐 Tor: Off", this);
    torButton_->setObjectName("tor-btn");
    torButton_->setCheckable(true);
    torButton_->setFixedHeight(30);
    connect(torButton_, &QPushButton::toggled,
            this, &MainWindow::toggleTor);

    // AI toggle button
    aiButton_ = new QPushButton("🧠 AI", this);
    aiButton_->setObjectName("ai-btn");
    aiButton_->setCheckable(true);
    aiButton_->setFixedHeight(30);
    connect(aiButton_, &QPushButton::toggled,
            this, &MainWindow::toggleAI);

    // Settings button
    QPushButton* settingsBtn = new QPushButton("⚙", this);
    settingsBtn->setObjectName("settings-btn");
    settingsBtn->setFixedSize(30, 30);
    connect(settingsBtn, &QPushButton::clicked,
            this, &MainWindow::openSettings);

    toolbar->addWidget(backBtn);
    toolbar->addWidget(fwdBtn);
    toolbar->addWidget(reloadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(addressBar_);
    toolbar->addSeparator();
    toolbar->addWidget(torButton_);
    toolbar->addWidget(aiButton_);
    toolbar->addWidget(settingsBtn);
}

void MainWindow::setupStatusBar()
{
    statusLabel_  = new QLabel("Ready", this);
    loadProgress_ = new QProgressBar(this);
    loadProgress_->setFixedWidth(120);
    loadProgress_->setFixedHeight(14);
    loadProgress_->setVisible(false);

    statusBar()->addWidget(statusLabel_);
    statusBar()->addPermanentWidget(loadProgress_);
    statusBar()->setObjectName("status-bar");
}

void MainWindow::setupShortcuts()
{
    auto addShortcut = [this](const QKeySequence& key, auto slot) {
        auto* sc = new QShortcut(key, this);
        connect(sc, &QShortcut::activated, this, slot);
    };

    addShortcut(QKeySequence("Ctrl+T"),  [this]{ newTab(); });
    addShortcut(QKeySequence("Ctrl+W"),  [this]{ closeTab(tabBar_->CurrentIndex()); });
    addShortcut(QKeySequence("Ctrl+L"),  [this]{ addressBar_->setFocus(); });
    addShortcut(QKeySequence("Ctrl+R"),  [this]{ reload(); });
    addShortcut(QKeySequence("Alt+Left"),[this]{ goBack(); });
    addShortcut(QKeySequence("Alt+Right"),[this]{ goForward(); });
    addShortcut(QKeySequence("F12"),     [this]{ toggleAI(!aiSidebar_->isVisible()); });
}

void MainWindow::applyTheme(const QString& theme)
{
    currentTheme_ = theme;
    QString qssPath = QString("%1/assets/themes/%2.qss")
        .arg(QApplication::applicationDirPath())
        .arg(theme);

    QFile f(qssPath);
    if (f.open(QIODevice::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    }
}

void MainWindow::openSettings()
{
    auto* dialog = new SettingsPage(torManager_, ollamaBridge_, this);
    connect(dialog, &SettingsPage::themeChanged,
            this, &MainWindow::applyTheme);
    connect(dialog, &SettingsPage::torToggled,
            this, &MainWindow::toggleTor);
    dialog->exec();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    // Resize any native CEF windows embedded in tab containers
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    torManager_->Stop();
    historyDB_->Close();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    QMainWindow::keyPressEvent(event);
}

int MainWindow::currentBrowserId() const
{
    if (activeTabIndex_ >= 0 && !tabs_.isEmpty()) {
        auto ids = tabs_.keys();
        if (activeTabIndex_ < ids.size()) return ids[activeTabIndex_];
    }
    return -1;
}
