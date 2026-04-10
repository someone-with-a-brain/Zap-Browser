#include "PageSummarizer.h"
#include "OllamaBridge.h"

#include <QDebug>

PageSummarizer::PageSummarizer(OllamaBridge* bridge, QObject* parent)
    : QObject(parent), bridge_(bridge)
{
    connect(bridge_, &OllamaBridge::responseComplete,
            this, &PageSummarizer::summarizationComplete);
    connect(bridge_, &OllamaBridge::requestError,
            this, &PageSummarizer::summarizationError);
}

void PageSummarizer::SummarizeText(const QString& pageText,
                                    const QString& pageTitle,
                                    const QString& pageUrl)
{
    if (!bridge_ || !bridge_->IsAvailable()) {
        emit summarizationError("Ollama is not available");
        return;
    }

    if (pageText.trimmed().isEmpty()) {
        emit summarizationError("Page appears to have no readable text");
        return;
    }

    QString truncated = pageText.length() > MAX_CHARS
        ? pageText.left(MAX_CHARS) + "\n\n[...content truncated for length...]"
        : pageText;

    QString prompt = buildPrompt(truncated, pageTitle, pageUrl);

    qDebug() << "[PageSummarizer] Summarizing" << pageText.length()
             << "characters from:" << pageUrl;

    emit summarizationStarted();
    bridge_->Generate(
        prompt,
        bridge_->IsAvailable() ? "llama3:8b" : OllamaBridge::DEFAULT_MODEL,
        "You are a concise web page summarizer. "
        "Provide a clear, structured summary with key points. "
        "Be objective and informative. Use bullet points where appropriate."
    );
}

QString PageSummarizer::buildPrompt(const QString& text,
                                     const QString& title,
                                     const QString& url) const
{
    QString prompt;

    if (!title.isEmpty()) {
        prompt += QString("Page Title: %1\n").arg(title);
    }
    if (!url.isEmpty()) {
        prompt += QString("URL: %1\n").arg(url);
    }
    prompt += "\n";
    prompt += "Please summarize the following webpage content:\n\n";
    prompt += "---\n";
    prompt += text;
    prompt += "\n---\n\n";
    prompt += "Provide:\n"
              "1. A 1-2 sentence overview\n"
              "2. Key points or main topics (bullet list)\n"
              "3. Any important facts, numbers, or conclusions";

    return prompt;
}
