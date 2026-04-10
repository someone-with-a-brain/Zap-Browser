#include "OllamaBridge.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDebug>

OllamaBridge::OllamaBridge(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{
    connect(nam_, &QNetworkAccessManager::finished,
            this, &OllamaBridge::onAvailabilityReply);
}

void OllamaBridge::CheckAvailability()
{
    QNetworkRequest req(QUrl(host_ + "/api/tags"));
    req.setTransferTimeout(3000);
    nam_->get(req);
}

void OllamaBridge::onAvailabilityReply(QNetworkReply* reply)
{
    bool ok = (reply->error() == QNetworkReply::NoError);
    if (ok != available_) {
        available_ = ok;
        emit availabilityChanged(available_);
    }

    if (ok) {
        // Parse model list from this response
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray modelsJson = doc.object()["models"].toArray();
        QList<OllamaModel> models;
        for (const auto& m : modelsJson) {
            QJsonObject obj = m.toObject();
            models.append({
                obj["name"].toString(),
                obj["modified_at"].toString(),
                obj["size"].toVariant().toLongLong()
            });
        }
        emit modelsListed(models);
    }
    reply->deleteLater();
}

void OllamaBridge::ListModels()
{
    QNetworkRequest req(QUrl(host_ + "/api/tags"));
    nam_->get(req);
}

void OllamaBridge::Generate(const QString& prompt,
                             const QString& model,
                             const QString& system)
{
    if (activeReply_) {
        activeReply_->abort();
        activeReply_ = nullptr;
    }
    accumulated_.clear();

    QNetworkRequest req(QUrl(host_ + "/api/generate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    activeReply_ = nam_->post(req, buildGeneratePayload(prompt, model, system));

    connect(activeReply_, &QNetworkReply::readyRead,
            this, &OllamaBridge::onGenerateReadyRead);
    connect(activeReply_, &QNetworkReply::finished,
            this, &OllamaBridge::onGenerateFinished);
}

void OllamaBridge::Abort()
{
    if (activeReply_) {
        activeReply_->abort();
        activeReply_ = nullptr;
    }
}

void OllamaBridge::onGenerateReadyRead()
{
    if (!activeReply_) return;

    // Ollama streams NDJSON — one JSON object per line
    while (activeReply_->canReadLine()) {
        QByteArray line = activeReply_->readLine().trimmed();
        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        QString token = obj["response"].toString();
        bool done = obj["done"].toBool();

        if (!token.isEmpty()) {
            accumulated_ += token;
            emit tokenReceived(token);
        }

        if (done) {
            emit responseComplete(accumulated_);
            accumulated_.clear();
            activeReply_ = nullptr;
            return;
        }
    }
}

void OllamaBridge::onGenerateFinished()
{
    if (!activeReply_) return;

    if (activeReply_->error() != QNetworkReply::NoError &&
        activeReply_->error() != QNetworkReply::OperationCanceledError)
    {
        emit requestError(activeReply_->errorString());
    }

    activeReply_->deleteLater();
    activeReply_ = nullptr;
}

QByteArray OllamaBridge::buildGeneratePayload(const QString& prompt,
                                              const QString& model,
                                              const QString& system) const
{
    QJsonObject payload;
    payload["model"]  = model;
    payload["prompt"] = prompt;
    payload["stream"] = true;

    if (!system.isEmpty()) {
        payload["system"] = system;
    }

    // Keep generation focused for sidebar use
    QJsonObject options;
    options["temperature"] = 0.7;
    options["num_predict"] = 1024;
    payload["options"] = options;

    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}
