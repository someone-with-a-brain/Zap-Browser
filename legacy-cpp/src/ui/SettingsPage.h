#pragma once

/**
 * SettingsPage — Full settings dialog for ZAP Browser.
 *
 * Tabbed dialog:
 *  - Privacy  : Tor, DoH, tracker blocking, clear-on-exit
 *  - AI       : Ollama toggle, model selector
 *  - Network  : Proxy config, Tor bridges
 *  - Appearance: Dark/light theme, compact mode
 *  - Performance: Tab sleep, hardware accel
 */

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>

class TorManager;
class OllamaBridge;

class SettingsPage : public QDialog {
    Q_OBJECT

public:
    explicit SettingsPage(TorManager* tor,
                          OllamaBridge* ollama,
                          QWidget* parent = nullptr);

signals:
    void themeChanged(const QString& theme);  // "dark" | "light"
    void torToggled(bool enabled);

private slots:
    void onSave();
    void onTorToggled(bool checked);
    void onThemeChanged(int index);
    void onClearHistoryClicked();

private:
    void buildPrivacyTab(QWidget* container);
    void buildAITab(QWidget* container);
    void buildNetworkTab(QWidget* container);
    void buildAppearanceTab(QWidget* container);
    void buildPerformanceTab(QWidget* container);

    void loadSettings();
    void saveSettings();

    TorManager*   tor_    = nullptr;
    OllamaBridge* ollama_ = nullptr;

    QTabWidget* tabs_ = nullptr;

    // Privacy tab widgets
    QCheckBox* torCheck_        = nullptr;
    QCheckBox* dohCheck_        = nullptr;
    QCheckBox* blockAdsCheck_   = nullptr;
    QCheckBox* clearOnExitCheck_= nullptr;

    // AI tab
    QCheckBox* ollamaCheck_     = nullptr;
    QComboBox* modelCombo_      = nullptr;

    // Network tab
    QLineEdit* proxyHost_       = nullptr;
    QSpinBox*  proxyPort_       = nullptr;
    QLineEdit* torBridges_      = nullptr;

    // Appearance tab
    QComboBox* themeCombo_      = nullptr;
    QCheckBox* compactModeCheck_= nullptr;
    QSpinBox*  fontSizeSpin_    = nullptr;

    // Performance tab
    QCheckBox* tabSleepCheck_   = nullptr;
    QSpinBox*  sleepDelaySpin_  = nullptr;
    QCheckBox* hwAccelCheck_    = nullptr;
};
