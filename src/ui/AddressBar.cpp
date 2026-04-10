#include "AddressBar.h"

#include <QHBoxLayout>
#include <QCompleter>
#include <QUrl>
#include <QRegularExpression>
#include <QFocusEvent>
#include <QDebug>

AddressBar::AddressBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("address-bar-container");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(36);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    lockIcon_ = new QLabel("🔒", this);
    lockIcon_->setObjectName("lock-icon");
    lockIcon_->setFixedWidth(22);
    lockIcon_->setVisible(false);

    input_ = new QLineEdit(this);
    input_->setObjectName("address-input");
    input_->setPlaceholderText("Search or enter URL...");
    input_->setClearButtonEnabled(true);

    // Autocomplete from history
    histModel_  = new QStringListModel(this);
    completer_  = new QCompleter(histModel_, this);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setFilterMode(Qt::MatchContains);
    completer_->setMaxVisibleItems(8);
    input_->setCompleter(completer_);

    connect(input_, &QLineEdit::returnPressed,
            this, &AddressBar::onReturnPressed);

    layout->addWidget(lockIcon_);
    layout->addWidget(input_);
    setLayout(layout);
}

void AddressBar::SetUrl(const QString& url)
{
    input_->setText(url);
    SetSecure(url.startsWith("https://"));
}

QString AddressBar::CurrentUrl() const
{
    return input_->text();
}

void AddressBar::SetHistorySuggestions(const QStringList& suggestions)
{
    histModel_->setStringList(suggestions);
}

void AddressBar::SetSecure(bool secure)
{
    lockIcon_->setVisible(secure);
    lockIcon_->setText(secure ? "🔒" : "⚠");
    lockIcon_->setStyleSheet(
        secure ? "color: #4ecca3;" : "color: #ff6b6b;");
}

void AddressBar::onReturnPressed()
{
    QString raw = input_->text().trimmed();
    if (raw.isEmpty()) return;
    emit submitted(resolveInput(raw));
}

QString AddressBar::resolveInput(const QString& input)
{
    if (looksLikeUrl(input)) {
        // Ensure it has a scheme
        if (!input.contains("://")) {
            return "https://" + input;
        }
        return input;
    }
    // It's a search query — use DuckDuckGo
    return "https://duckduckgo.com/?q=" + QUrl::toPercentEncoding(input);
}

bool AddressBar::looksLikeUrl(const QString& input)
{
    // Has a scheme already
    if (input.contains("://")) return true;

    // Looks like a domain: contains a dot and no spaces, not a sentence
    static QRegularExpression domainRe(
        R"(^[a-zA-Z0-9-]+\.[a-zA-Z]{2,}(/.*)?$)");
    return domainRe.match(input).hasMatch();
}
