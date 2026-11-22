#pragma once

#include <QtCore/QLocale>
#include <QtCore/QString>

/// Friendly zh-Hans translation of firmware STATUSTEXT and event text messages.
///
/// Display layer only: the corpus is matched against the complete firmware text
/// (after chunked STATUSTEXT reassembly). Anything unmatched, or when the ui
/// language is not chinese, is returned unchanged - messages are never dropped.
class StatusTextTranslator
{
public:
    /// Installs the application ui locale. Called from QGCApplication::setLanguage().
    /// Checking this locale explicitly (instead of a default-constructed QLocale)
    /// is required because setLanguage() skips QLocale::setDefault() for en_US,
    /// which would otherwise leave the gate on the system language.
    static void setLocale(const QLocale &locale);

    /// Translates a firmware status text for display. Returns the raw text on
    /// any mismatch, corpus load failure or error.
    static QString translate(const QString &raw);

private:
    StatusTextTranslator() = delete;
};
