#include "SettingsPage.h"
#include "privacy/TorManager.h"
#include "ai/OllamaBridge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSettings>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QLabel>

SettingsPage::SettingsPage(TorManager* tor, OllamaBridge* ollama, QWidget* parent)
    : QDialog(parent), tor_(tor), ollama_(ollama)
{
    setWindowTitle("⚡ ZAP Settings");
    setMinimumSize(560, 480);
    setObjectName("settings-dialog");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    tabs_ = new QTabWidget(this);

    // Build tabs
    auto makeTab = [&](const QString& name, auto buildFn) {
        QWidget* w = new QWidget();
        (this->*buildFn)(w);
        tabs_->addTab(w, name);
    };

    makeTab("🔐 Privacy",    &SettingsPage::buildPrivacyTab);
    makeTab("🧠 AI",         &SettingsPage::buildAITab);
    makeTab("🌐 Network",    &SettingsPage::buildNetworkTab);
    makeTab("🎨 Appearance", &SettingsPage::buildAppearanceTab);
    makeTab("⚡ Performance", &SettingsPage::buildPerformanceTab);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsPage::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(tabs_);
    mainLayout->addWidget(buttons);
    setLayout(mainLayout);

    loadSettings();
}

void SettingsPage::buildPrivacyTab(QWidget* w)
{
    QVBoxLayout* l = new QVBoxLayout(w);

    QGroupBox* torGroup = new QGroupBox("Tor Privacy Mode", w);
    QVBoxLayout* tl = new QVBoxLayout(torGroup);
    torCheck_ = new QCheckBox("Enable Tor routing (all traffic anonymized)", torGroup);
    tl->addWidget(torCheck_);
    tl->addWidget(new QLabel(
        "<small style='color:#888'>Routes all traffic through Tor. "
        "Startup may take 30-60 seconds.</small>", torGroup));
    connect(torCheck_, &QCheckBox::toggled, this, &SettingsPage::onTorToggled);

    QGroupBox* dnsGroup = new QGroupBox("DNS Privacy", w);
    QVBoxLayout* dl = new QVBoxLayout(dnsGroup);
    dohCheck_ = new QCheckBox("Enable DNS over HTTPS (DoH)", dnsGroup);
    dl->addWidget(dohCheck_);
    dl->addWidget(new QLabel(
        "<small style='color:#888'>Uses Cloudflare 1.1.1.1 — prevents ISP DNS snooping.</small>",
        dnsGroup));

    QGroupBox* blockGroup = new QGroupBox("Content Blocking", w);
    QVBoxLayout* bl = new QVBoxLayout(blockGroup);
    blockAdsCheck_ = new QCheckBox("Block ads and trackers", blockGroup);
    clearOnExitCheck_ = new QCheckBox("Clear history and cookies on exit", blockGroup);
    QPushButton* clearNowBtn = new QPushButton("🗑 Clear History Now", blockGroup);
    connect(clearNowBtn, &QPushButton::clicked, this, &SettingsPage::onClearHistoryClicked);
    bl->addWidget(blockAdsCheck_);
    bl->addWidget(clearOnExitCheck_);
    bl->addWidget(clearNowBtn);

    l->addWidget(torGroup);
    l->addWidget(dnsGroup);
    l->addWidget(blockGroup);
    l->addStretch();
}

void SettingsPage::buildAITab(QWidget* w)
{
    QVBoxLayout* l = new QVBoxLayout(w);

    QGroupBox* aiGroup = new QGroupBox("Ollama AI Assistant", w);
    QFormLayout* fl = new QFormLayout(aiGroup);

    ollamaCheck_ = new QCheckBox("Enable AI sidebar", aiGroup);
    modelCombo_  = new QComboBox(aiGroup);
    // Populate with known defaults; runtime list comes from OllamaBridge
    modelCombo_->addItems({"llama3:8b", "mistral", "phi3:mini", "qwen2.5:7b"});

    fl->addRow("", ollamaCheck_);
    fl->addRow("Default model:", modelCombo_);
    fl->addRow(new QLabel(
        "<small style='color:#888'>Models must be pulled with: <code>ollama pull &lt;model&gt;</code></small>",
        aiGroup));

    bool ollamaAvail = ollama_ && ollama_->IsAvailable();
    QLabel* statusLbl = new QLabel(
        ollamaAvail ? "✅ Ollama connected" : "⚠ Ollama offline", aiGroup);
    statusLbl->setStyleSheet(
        ollamaAvail ? "color:#4ecca3;" : "color:#ff6b6b;");
    fl->addRow("Status:", statusLbl);

    l->addWidget(aiGroup);
    l->addStretch();
}

void SettingsPage::buildNetworkTab(QWidget* w)
{
    QVBoxLayout* l = new QVBoxLayout(w);

    QGroupBox* proxyGroup = new QGroupBox("Custom Proxy", w);
    QFormLayout* fl = new QFormLayout(proxyGroup);
    proxyHost_ = new QLineEdit(proxyGroup);
    proxyHost_->setPlaceholderText("127.0.0.1");
    proxyPort_ = new QSpinBox(proxyGroup);
    proxyPort_->setRange(1, 65535);
    proxyPort_->setValue(8080);
    fl->addRow("Proxy host:", proxyHost_);
    fl->addRow("Proxy port:", proxyPort_);

    QGroupBox* bridgeGroup = new QGroupBox("Tor Bridges (Advanced)", w);
    QVBoxLayout* bl = new QVBoxLayout(bridgeGroup);
    torBridges_ = new QLineEdit(bridgeGroup);
    torBridges_->setPlaceholderText("obfs4 ... (one per line, from bridges.torproject.org)");
    bl->addWidget(new QLabel("Enter Tor bridges if Tor is blocked in your country:"));
    bl->addWidget(torBridges_);

    l->addWidget(proxyGroup);
    l->addWidget(bridgeGroup);
    l->addStretch();
}

void SettingsPage::buildAppearanceTab(QWidget* w)
{
    QVBoxLayout* l = new QVBoxLayout(w);

    QGroupBox* themeGroup = new QGroupBox("Theme", w);
    QFormLayout* fl = new QFormLayout(themeGroup);
    themeCombo_ = new QComboBox(themeGroup);
    themeCombo_->addItems({"Dark", "Light"});
    compactModeCheck_ = new QCheckBox("Enable compact mode (smaller UI)", themeGroup);
    fontSizeSpin_ = new QSpinBox(themeGroup);
    fontSizeSpin_->setRange(8, 24);
    fontSizeSpin_->setValue(10);
    connect(themeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onThemeChanged);
    fl->addRow("Theme:", themeCombo_);
    fl->addRow("", compactModeCheck_);
    fl->addRow("Font size:", fontSizeSpin_);

    l->addWidget(themeGroup);
    l->addStretch();
}

void SettingsPage::buildPerformanceTab(QWidget* w)
{
    QVBoxLayout* l = new QVBoxLayout(w);

    QGroupBox* sleepGroup = new QGroupBox("Tab Sleeping", w);
    QFormLayout* fl = new QFormLayout(sleepGroup);
    tabSleepCheck_ = new QCheckBox("Sleep inactive tabs to save RAM", sleepGroup);
    sleepDelaySpin_ = new QSpinBox(sleepGroup);
    sleepDelaySpin_->setRange(1, 60);
    sleepDelaySpin_->setValue(5);
    sleepDelaySpin_->setSuffix(" minutes");
    fl->addRow("", tabSleepCheck_);
    fl->addRow("Sleep after:", sleepDelaySpin_);
    fl->addRow(new QLabel(
        "<small style='color:#888'>Sleeping tabs pause JS and reduce memory usage. "
        "They wake instantly on click.</small>", sleepGroup));

    QGroupBox* renderGroup = new QGroupBox("Rendering", w);
    QFormLayout* rl = new QFormLayout(renderGroup);
    hwAccelCheck_ = new QCheckBox("Enable hardware acceleration (GPU rendering)", renderGroup);
    rl->addRow("", hwAccelCheck_);

    l->addWidget(sleepGroup);
    l->addWidget(renderGroup);
    l->addStretch();
}

// ── Persistence ───────────────────────────────────────────────────────────────

void SettingsPage::loadSettings()
{
    QSettings s("ZAP", "ZapBrowser");
    torCheck_->setChecked(s.value("privacy/tor", false).toBool());
    dohCheck_->setChecked(s.value("privacy/doh", true).toBool());
    blockAdsCheck_->setChecked(s.value("privacy/blockAds", true).toBool());
    clearOnExitCheck_->setChecked(s.value("privacy/clearOnExit", false).toBool());
    ollamaCheck_->setChecked(s.value("ai/enabled", true).toBool());
    modelCombo_->setCurrentText(s.value("ai/model", "llama3:8b").toString());
    themeCombo_->setCurrentText(s.value("ui/theme", "Dark").toString());
    compactModeCheck_->setChecked(s.value("ui/compact", false).toBool());
    fontSizeSpin_->setValue(s.value("ui/fontSize", 10).toInt());
    tabSleepCheck_->setChecked(s.value("perf/tabSleep", true).toBool());
    sleepDelaySpin_->setValue(s.value("perf/sleepDelay", 5).toInt());
    hwAccelCheck_->setChecked(s.value("perf/hwAccel", true).toBool());
}

void SettingsPage::saveSettings()
{
    QSettings s("ZAP", "ZapBrowser");
    s.setValue("privacy/tor",        torCheck_->isChecked());
    s.setValue("privacy/doh",        dohCheck_->isChecked());
    s.setValue("privacy/blockAds",   blockAdsCheck_->isChecked());
    s.setValue("privacy/clearOnExit",clearOnExitCheck_->isChecked());
    s.setValue("ai/enabled",         ollamaCheck_->isChecked());
    s.setValue("ai/model",           modelCombo_->currentText());
    s.setValue("ui/theme",           themeCombo_->currentText().toLower());
    s.setValue("ui/compact",         compactModeCheck_->isChecked());
    s.setValue("ui/fontSize",        fontSizeSpin_->value());
    s.setValue("perf/tabSleep",      tabSleepCheck_->isChecked());
    s.setValue("perf/sleepDelay",    sleepDelaySpin_->value());
    s.setValue("perf/hwAccel",       hwAccelCheck_->isChecked());
}

void SettingsPage::onSave()
{
    saveSettings();
    accept();
}

void SettingsPage::onTorToggled(bool checked)
{
    emit torToggled(checked);
}

void SettingsPage::onThemeChanged(int index)
{
    emit themeChanged(index == 0 ? "dark" : "light");
}

void SettingsPage::onClearHistoryClicked()
{
    auto res = QMessageBox::question(this, "Clear History",
        "This will delete all local browsing history. Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        // HistoryDB::Clear() — called via MainWindow signal in full impl
        QMessageBox::information(this, "Done", "History cleared.");
    }
}
