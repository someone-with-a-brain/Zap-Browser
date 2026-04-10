#pragma once

/**
 * OllamaBridge — Async HTTP bridge to the local Ollama API.
 *
 * Supports:
 *  - Streaming chat completions (token-by-token via NDJSON)
 *  - Model listing from /api/tags
 *  - Single-shot generate requests
 *
 * All network I/O is async via Qt's QNetworkAccessManager.
 * Tokens are emitted as they arrive so the UI can stream them.
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

struct OllamaModel {
    QString name;
    QString modifiedAt;
    qint64  size = 0;
};

class OllamaBridge : public QObject {
    Q_OBJECT

public:
    static constexpr const char* DEFAULT_HOST = "http://localhost:11434";
    static constexpr const char* DEFAULT_MODEL = "llama3:8b";

    explicit OllamaBridge(QObject* parent = nullptr);

    /** Check if Ollama is reachable at the configured host. */
    void CheckAvailability();

    /** List all locally available models. */
    void ListModels();

    /**
     * Stream a chat response from Ollama.
     * Emits tokenReceived() for each streamed token.
     * Emits responseComplete() when the stream ends.
     *
     * @param model  Model name e.g. "llama3:8b"
     * @param prompt Full prompt text
     * @param system Optional system prompt
     */
    void Generate(const QString& prompt,
                  const QString& model  = DEFAULT_MODEL,
                  const QString& system = QString());

    /** Abort any in-progress generation. */
    void Abort();

    void SetHost(const QString& host) { host_ = host; }
    QString Host() const { return host_; }

    bool IsAvailable() const { return available_; }

signals:
    void availabilityChanged(bool available);
    void modelsListed(const QList<OllamaModel>& models);
    void tokenReceived(const QString& token);
    void responseComplete(const QString& fullResponse);
    void requestError(const QString& error);

private slots:
    void onGenerateReadyRead();
    void onGenerateFinished();
    void onAvailabilityReply(QNetworkReply* reply);

private:
    QNetworkAccessManager* nam_;
    QNetworkReply*         activeReply_ = nullptr;
    QString                host_        = DEFAULT_HOST;
    bool                   available_   = false;
    QString                accumulated_;

    QByteArray buildGeneratePayload(const QString& prompt,
                                    const QString& model,
                                    const QString& system) const;
};
