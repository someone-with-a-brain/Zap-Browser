#include "TorManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

TorManager::TorManager(QObject* parent)
    : QObject(parent)
    , process_(new QProcess(this))
    , bootstrapTimer_(new QTimer(this))
{
    connect(process_, &QProcess::readyReadStandardOutput,
            this, &TorManager::onProcessReadyRead);
    connect(process_, &QProcess::readyReadStandardError,
            this, &TorManager::onProcessReadyRead);
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TorManager::onProcessFinished);

    bootstrapTimer_->setSingleShot(true);
    bootstrapTimer_->setInterval(60000); // 60 second bootstrap timeout
    connect(bootstrapTimer_, &QTimer::timeout,
            this, &TorManager::onBootstrapTimeout);
}

TorManager::~TorManager()
{
    Stop();
}

bool TorManager::Start(const QString& torBinaryPath)
{
    if (IsRunning()) return true;

    QString binary = torBinaryPath.isEmpty() ? defaultTorPath() : torBinaryPath;
    if (!QFileInfo::exists(binary)) {
        emit torError(QString("Tor binary not found: %1").arg(binary));
        return false;
    }

    // Build tor config args
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/tor_data";
    QDir().mkpath(dataDir);

    QStringList args = {
        "--SocksPort",      "9050",
        "--ControlPort",    "9051",
        "--DataDirectory",  dataDir,
        "--Log",            "notice stdout",
        "--ClientOnly",     "1",    // don't act as relay
        "--GeoIPFile",      QFileInfo(binary).dir().filePath("geoip"),
        "--GeoIPv6File",    QFileInfo(binary).dir().filePath("geoip6"),
    };

    bootstrapPercent_ = 0;
    bootstrapped_     = false;

    emit torStarting();
    process_->start(binary, args);

    if (!process_->waitForStarted(5000)) {
        emit torError("Failed to start tor process");
        return false;
    }

    bootstrapTimer_->start();
    return true;
}

void TorManager::Stop()
{
    bootstrapTimer_->stop();
    if (process_->state() != QProcess::NotRunning) {
        process_->terminate();
        if (!process_->waitForFinished(5000)) {
            process_->kill();
        }
    }
    bootstrapPercent_ = 0;
    bootstrapped_     = false;
    emit torStopped();
}

bool TorManager::IsRunning() const
{
    return process_->state() == QProcess::Running;
}

void TorManager::onProcessReadyRead()
{
    // Read all available output (both stdout and stderr)
    QByteArray output = process_->readAllStandardOutput()
                      + process_->readAllStandardError();
    QString text = QString::fromUtf8(output);

    // Parse bootstrap progress: "Bootstrapped 75% (conn_or): ..."
    static QRegularExpression re(
        R"(Bootstrapped (\d+)%[^:]*:\s*(.+))");
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        int pct = match.captured(1).toInt();
        QString status = match.captured(2).trimmed();
        bootstrapPercent_ = pct;
        emit torBootstrapProgress(pct, status);

        if (pct == 100 && !bootstrapped_) {
            bootstrapped_ = true;
            bootstrapTimer_->stop();
            emit torBootstrapped();
            qDebug() << "[Tor] Bootstrapped 100% — ready";
        }
    }
}

void TorManager::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    qDebug() << "[Tor] Process exited with code" << exitCode;
    bootstrapped_ = false;
    bootstrapTimer_->stop();
    emit torStopped();
}

void TorManager::onBootstrapTimeout()
{
    emit torError(
        QString("Tor bootstrap timed out at %1%").arg(bootstrapPercent_));
}

QString TorManager::defaultTorPath() const
{
#ifdef Q_OS_WIN
    return QCoreApplication::applicationDirPath() + "/assets/tor/tor.exe";
#else
    // On Linux, check if system tor is available
    QString systemTor = "/usr/bin/tor";
    if (QFileInfo::exists(systemTor)) return systemTor;
    return QCoreApplication::applicationDirPath() + "/assets/tor/tor";
#endif
}
