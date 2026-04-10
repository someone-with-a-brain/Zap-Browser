#pragma once

/**
 * AddressBar — Smart URL/search input widget.
 *
 * Behaviors:
 *  - Auto-detects URL vs. search query
 *  - Shows security padlock (https vs http)
 *  - Autocomplete from local HistoryDB
 *  - Focus highlight animation
 */

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QCompleter>
#include <QStringListModel>

class AddressBar : public QWidget {
    Q_OBJECT

public:
    explicit AddressBar(QWidget* parent = nullptr);

    /** Set the displayed URL (called on navigation). */
    void SetUrl(const QString& url);

    QString CurrentUrl() const;

    /** Update autocomplete suggestions from history. */
    void SetHistorySuggestions(const QStringList& suggestions);

    /** Show/hide the security padlock icon. */
    void SetSecure(bool secure);

signals:
    /** Emitted when user presses Enter. raw = what user typed. */
    void submitted(const QString& raw);

private slots:
    void onReturnPressed();

private:
    /** Returns normalized URL or DuckDuckGo search URL. */
    static QString resolveInput(const QString& input);
    static bool looksLikeUrl(const QString& input);

    QLineEdit*         input_       = nullptr;
    QLabel*            lockIcon_    = nullptr;
    QStringListModel*  histModel_   = nullptr;
    QCompleter*        completer_   = nullptr;
};
