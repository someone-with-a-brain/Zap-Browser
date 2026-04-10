#pragma once

/**
 * TorManager — Controls the bundled tor process.
 *
 * Responsibilities:
 *  - Launch/kill the tor.exe subprocess
 *  - Monitor the control port until tor bootstraps to 100%
 *  - Provide the SOCKS5 proxy address to inject into CEF settings
 *  - Expose running status to the UI for the status badge
 */

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

class TorManager : public QObject {
    Q_OBJECT

public:
    explicit TorManager(QObject* parent = nullptr);
    ~TorManager() override;

    /**
     * Starts the tor process. torBinaryPath defaults to assets/tor/tor.exe
     * on Windows. Signals torBootstrapped() when ready.
     */
    bool Start(const QString& torBinaryPath = QString());

    /**
     * Stops the tor process gracefully (SIGTERM → SIGKILL after timeout).
     */
    void Stop();

    /** Returns true if tor subprocess is running. */
    bool IsRunning() const;

    /** Returns the SOCKS5 proxy string: "127.0.0.1:9050" */
    QString GetSocksProxy() const { return "127.0.0.1:9050"; }

    /** Bootstrap percentage (0-100). */
    int BootstrapPercent() const { return bootstrapPercent_; }

signals:
    void torStarting();
    void torBootstrapped();            // 100% bootstrap complete
    void torBootstrapProgress(int pct, const QString& status);
    void torStopped();
    void torError(const QString& error);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onBootstrapTimeout();

private:
    QProcess* process_ = nullptr;
    QTimer*   bootstrapTimer_ = nullptr;
    int       bootstrapPercent_ = 0;
    bool      bootstrapped_ = false;

    QString defaultTorPath() const;
};
