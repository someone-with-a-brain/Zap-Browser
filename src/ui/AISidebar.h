#pragma once

/**
 * AISidebar — Collapsible AI assistant panel.
 *
 * Features:
 *  - Streaming chat display (tokens appear word-by-word)
 *  - "Summarize Page" shortcut button
 *  - Model selector dropdown
 *  - Abort in-progress generation
 *  - Chat history within session (no persistence)
 */

#include <QWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>

class OllamaBridge;

class AISidebar : public QWidget {
    Q_OBJECT

public:
    explicit AISidebar(OllamaBridge* bridge, QWidget* parent = nullptr);

    /** Called by MainWindow when page text is ready for summarization. */
    void SummarizePage(const QString& pageText);

    /** Update the model selector list. */
    void SetAvailableModels(const QStringList& models);

    /** Show/hide with animation. */
    void SetVisible(bool visible);

    QString CurrentModel() const;

signals:
    void summarizeRequested();

public slots:
    void onTokenReceived(const QString& token);
    void onResponseComplete(const QString& full);
    void onOllamaError(const QString& error);
    void onOllamaAvailabilityChanged(bool available);

private slots:
    void onSendClicked();
    void onSummarizeClicked();
    void onAbortClicked();
    void onInputReturnPressed();

private:
    void buildUI();
    void appendUserMessage(const QString& text);
    void appendAssistantToken(const QString& token);
    void appendSystemMessage(const QString& text);
    void startAssistantBlock();
    void endAssistantBlock();
    void scrollToBottom();

    OllamaBridge* bridge_  = nullptr;
    bool          generating_ = false;

    //─── UI elements ──────────────────────────────────────────────────────────
    QLabel*       headerLabel_    = nullptr;
    QComboBox*    modelSelector_  = nullptr;
    QTextBrowser* chatDisplay_    = nullptr;
    QLineEdit*    inputLine_      = nullptr;
    QPushButton*  sendButton_     = nullptr;
    QPushButton*  summarizeBtn_   = nullptr;
    QPushButton*  abortButton_    = nullptr;
    QLabel*       statusLabel_    = nullptr;

    QString currentAssistantText_;
};
