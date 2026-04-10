#pragma once

/**
 * PageSummarizer — Extracts page text from CEF and feeds it to OllamaBridge.
 *
 * Workflow:
 *  1. Caller requests summarization for a specific CefBrowser
 *  2. PageSummarizer injects JS to extract document.body.innerText
 *  3. Text is truncated to fit Ollama context window
 *  4. Sent to OllamaBridge.Generate() with a summarization system prompt
 *  5. AISidebar displays the streaming result
 */

#include <QObject>
#include <QString>

class OllamaBridge;

class PageSummarizer : public QObject {
    Q_OBJECT

public:
    explicit PageSummarizer(OllamaBridge* bridge, QObject* parent = nullptr);

    /**
     * Call this when you already have the page text extracted.
     * (Called from MainWindow after JS execution via CEF.)
     */
    void SummarizeText(const QString& pageText,
                       const QString& pageTitle = QString(),
                       const QString& pageUrl   = QString());

    /** Max characters to send to Ollama (roughly 3000 tokens). */
    static constexpr int MAX_CHARS = 12000;

signals:
    void summarizationStarted();
    void summarizationComplete(const QString& summary);
    void summarizationError(const QString& error);

private:
    OllamaBridge* bridge_ = nullptr;

    QString buildPrompt(const QString& text,
                        const QString& title,
                        const QString& url) const;
};
