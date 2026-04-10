#include "AISidebar.h"
#include "ai/OllamaBridge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QFont>
#include <QDateTime>
#include <QDebug>

AISidebar::AISidebar(OllamaBridge* bridge, QWidget* parent)
    : QWidget(parent), bridge_(bridge)
{
    buildUI();

    if (bridge_) {
        connect(bridge_, &OllamaBridge::tokenReceived,
                this, &AISidebar::onTokenReceived);
        connect(bridge_, &OllamaBridge::responseComplete,
                this, &AISidebar::onResponseComplete);
        connect(bridge_, &OllamaBridge::requestError,
                this, &AISidebar::onOllamaError);
        connect(bridge_, &OllamaBridge::availabilityChanged,
                this, &AISidebar::onOllamaAvailabilityChanged);
    }
}

void AISidebar::buildUI()
{
    setObjectName("ai-sidebar");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── Header ────────────────────────────────────────────────────────────
    headerLabel_ = new QLabel("🧠 ZAP AI", this);
    headerLabel_->setObjectName("ai-header");
    QFont hf = headerLabel_->font();
    hf.setPointSize(14);
    hf.setBold(true);
    headerLabel_->setFont(hf);

    // ── Model selector ────────────────────────────────────────────────────
    modelSelector_ = new QComboBox(this);
    modelSelector_->setObjectName("model-selector");
    modelSelector_->addItem("llama3:8b");
    modelSelector_->setToolTip("Select Ollama model");

    // ── Status bar ────────────────────────────────────────────────────────
    statusLabel_ = new QLabel("Checking Ollama...", this);
    statusLabel_->setObjectName("ai-status");
    statusLabel_->setWordWrap(true);

    // ── Chat display ──────────────────────────────────────────────────────
    chatDisplay_ = new QTextBrowser(this);
    chatDisplay_->setObjectName("chat-display");
    chatDisplay_->setOpenLinks(false);
    chatDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chatDisplay_->setReadOnly(true);

    // ── Summarize button ──────────────────────────────────────────────────
    summarizeBtn_ = new QPushButton("📄 Summarize Page", this);
    summarizeBtn_->setObjectName("summarize-btn");
    connect(summarizeBtn_, &QPushButton::clicked,
            this, &AISidebar::onSummarizeClicked);

    // ── Input area ────────────────────────────────────────────────────────
    inputLine_ = new QLineEdit(this);
    inputLine_->setObjectName("chat-input");
    inputLine_->setPlaceholderText("Ask anything...");
    connect(inputLine_, &QLineEdit::returnPressed,
            this, &AISidebar::onInputReturnPressed);

    sendButton_  = new QPushButton("Send", this);
    sendButton_->setObjectName("send-btn");
    connect(sendButton_, &QPushButton::clicked, this, &AISidebar::onSendClicked);

    abortButton_ = new QPushButton("⏹ Stop", this);
    abortButton_->setObjectName("abort-btn");
    abortButton_->setVisible(false);
    connect(abortButton_, &QPushButton::clicked, this, &AISidebar::onAbortClicked);

    QHBoxLayout* inputRow = new QHBoxLayout();
    inputRow->addWidget(inputLine_);
    inputRow->addWidget(sendButton_);
    inputRow->addWidget(abortButton_);

    layout->addWidget(headerLabel_);
    layout->addWidget(modelSelector_);
    layout->addWidget(statusLabel_);
    layout->addWidget(chatDisplay_, 1);
    layout->addWidget(summarizeBtn_);
    layout->addLayout(inputRow);

    setLayout(layout);
}

void AISidebar::SummarizePage(const QString& pageText)
{
    if (!bridge_ || !bridge_->IsAvailable()) {
        appendSystemMessage("⚠ Ollama not available. Start Ollama first.");
        return;
    }
    appendSystemMessage("📄 Summarizing page...");
    startAssistantBlock();
    generating_ = true;
    sendButton_->setVisible(false);
    abortButton_->setVisible(true);

    QString prompt = "Please provide a concise, clear summary of the following webpage content:\n\n"
                   + pageText.left(4000);
    bridge_->Generate(prompt, CurrentModel(),
        "You are a helpful web page summarizer. Be concise and informative.");
}

void AISidebar::SetAvailableModels(const QStringList& models)
{
    modelSelector_->clear();
    for (const QString& m : models) modelSelector_->addItem(m);
    if (models.isEmpty()) modelSelector_->addItem("No models found");
}

QString AISidebar::CurrentModel() const
{
    return modelSelector_->currentText();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void AISidebar::onSendClicked()
{
    onInputReturnPressed();
}

void AISidebar::onInputReturnPressed()
{
    QString text = inputLine_->text().trimmed();
    if (text.isEmpty() || generating_) return;

    appendUserMessage(text);
    inputLine_->clear();
    startAssistantBlock();
    generating_ = true;
    sendButton_->setVisible(false);
    abortButton_->setVisible(true);

    bridge_->Generate(text, CurrentModel());
}

void AISidebar::onSummarizeClicked()
{
    emit summarizeRequested();
}

void AISidebar::onAbortClicked()
{
    if (bridge_) bridge_->Abort();
    endAssistantBlock();
    appendSystemMessage("⏹ Generation stopped.");
    generating_ = false;
    sendButton_->setVisible(true);
    abortButton_->setVisible(false);
}

void AISidebar::onTokenReceived(const QString& token)
{
    currentAssistantText_ += token;
    // Update the last paragraph in chatDisplay_ with growing text
    QTextCursor cursor = chatDisplay_->textCursor();
    cursor.movePosition(QTextCursor::End);
    chatDisplay_->setTextCursor(cursor);
    chatDisplay_->insertPlainText(token);
    scrollToBottom();
}

void AISidebar::onResponseComplete(const QString&)
{
    endAssistantBlock();
    generating_ = false;
    sendButton_->setVisible(true);
    abortButton_->setVisible(false);
    currentAssistantText_.clear();
}

void AISidebar::onOllamaError(const QString& error)
{
    endAssistantBlock();
    appendSystemMessage("⚠ Error: " + error);
    generating_ = false;
    sendButton_->setVisible(true);
    abortButton_->setVisible(false);
}

void AISidebar::onOllamaAvailabilityChanged(bool available)
{
    if (available) {
        statusLabel_->setText("✅ Ollama connected");
        statusLabel_->setStyleSheet("color: #4ecca3;");
    } else {
        statusLabel_->setText("⚠ Ollama offline — start Ollama to enable AI");
        statusLabel_->setStyleSheet("color: #ff6b6b;");
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

void AISidebar::appendUserMessage(const QString& text)
{
    chatDisplay_->append(
        QString("<div style='color:#7c83fd;font-weight:bold;margin-top:8px;'>"
                "You:</div>"
                "<div style='color:#e0e0e0;margin-left:4px;'>%1</div>")
        .arg(text.toHtmlEscaped()));
    scrollToBottom();
}

void AISidebar::startAssistantBlock()
{
    chatDisplay_->append(
        "<div style='color:#4ecca3;font-weight:bold;margin-top:8px;'>"
        "⚡ ZAP AI:</div>"
        "<div id='response' style='color:#c8c8c8;margin-left:4px;'>");
}

void AISidebar::endAssistantBlock()
{
    chatDisplay_->append("</div>");
}

void AISidebar::appendSystemMessage(const QString& text)
{
    chatDisplay_->append(
        QString("<div style='color:#888;font-style:italic;margin-top:4px;'>%1</div>")
        .arg(text.toHtmlEscaped()));
    scrollToBottom();
}

void AISidebar::scrollToBottom()
{
    chatDisplay_->verticalScrollBar()->setValue(
        chatDisplay_->verticalScrollBar()->maximum());
}
