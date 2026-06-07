// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLOGGER_P_H
#define QQMLJSLOGGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtqmlcompilerexports.h>

#include "qcoloroutput_p.h"
#include "qqmljsloggingutils_p.h"

#include <private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/QLoggingCategory>

#include <optional>

QT_BEGIN_NAMESPACE

/*!
    \internal
    Used to print the line containing the location of a certain error
 */
class Q_QMLCOMPILER_EXPORT IssueLocationWithContext
{
public:
    /*!
       \internal
       \param code: The whole text of a translation unit
       \param location: The location where an error occurred.
     */
    IssueLocationWithContext(QStringView code, const QQmlJS::SourceLocation &location) {
        quint32 before = qMax(0, code.lastIndexOf(QLatin1Char('\n'), location.offset));

        if (before != 0 && before < location.offset)
            before++;

        m_beforeText = code.mid(before, location.offset - before);
        m_issueText = code.mid(location.offset, location.length);
        int after = code.indexOf(QLatin1Char('\n'), location.offset + location.length);
        m_afterText = code.mid(location.offset + location.length,
                                  after - (location.offset+location.length));
    }

    // returns start of the line till first character of location
    QStringView beforeText() const { return m_beforeText; }
    // returns the text at location
    QStringView issueText() const { return m_issueText; }
    // returns any text after location until the end of the line is reached
    QStringView afterText() const { return m_afterText; }

private:
    QStringView m_beforeText;
    QStringView m_issueText;
    QStringView m_afterText;
};

class Q_QMLCOMPILER_EXPORT QQmlJSFixSuggestion
{
public:
    QQmlJSFixSuggestion() = default;
    QQmlJSFixSuggestion(const QString &fixDescription, const QQmlJS::SourceLocation &location,
                        const QString &replacement = QString());

    QString fixDescription() const { return m_fixDescription; }
    QQmlJS::SourceLocation location() const { return m_location; }
    QString replacement() const { return m_replacement; }

    void setFilename(const QString &filename) { m_filename = filename; }
    QString filename() const { return m_filename; }

    void setHint(const QString &hint) { m_hint = hint; }
    QString hint() const { return m_hint; }

    void setAutoApplicable(bool autoApply = true) { m_autoApplicable = autoApply; }
    bool isAutoApplicable() const { return m_autoApplicable; }

    bool operator==(const QQmlJSFixSuggestion &) const;
    bool operator!=(const QQmlJSFixSuggestion &) const;

private:
    QQmlJS::SourceLocation m_location;
    QString m_fixDescription;
    QString m_replacement;
    QString m_filename;
    QString m_hint;
    bool m_autoApplicable = false;
};

struct Message : public QQmlJS::DiagnosticMessage
{
    enum class CompilationStatus { Normal, Skip, Error };

    // This doesn't need to be an owning-reference since the string is expected to outlive any
    // Message object by virtue of coming from a LoggerWarningId.
    QAnyStringView id;
    std::optional<QQmlJSFixSuggestion> fixSuggestion;
    CompilationStatus compilationStatus = CompilationStatus::Normal;
    std::optional<quint32> customLineForDisabling = std::nullopt;

    quint32 lineForDisabling() const { return customLineForDisabling.value_or(loc.startLine); }
};

class Q_QMLCOMPILER_EXPORT QQmlJSLogger
{
    Q_DISABLE_COPY_MOVE(QQmlJSLogger)
public:
    QList<QQmlJS::LoggerCategory> categories() const;
    static const QList<QQmlJS::LoggerCategory> &defaultCategories();

    void registerCategory(const QQmlJS::LoggerCategory &category);

    QQmlJSLogger();
    ~QQmlJSLogger() = default;

    bool hasWarnings() const { return m_numWarnings > 0; }
    bool hasErrors() const { return m_numErrors > 0; }

    qsizetype numWarnings() const { return m_numWarnings; }
    qsizetype numErrors() const { return m_numErrors; }

    template<typename F>
    void iterateCurrentFunctionMessages(F &&f) const
    {
        for (const Message &msg : m_currentFunctionMessages)
            f(msg);
    }

    template<typename F>
    void iterateAllMessages(F &&f) const
    {
        for (const Message &msg : m_archivedMessages)
            f(msg);

        for (const Message &msg : m_currentFunctionMessages)
            f(msg);
    }

    QtMsgType categoryLevel(QQmlJS::LoggerWarningId id) const
    {
        return m_categoryLevels[id.name().toString()];
    }
    void setCategoryLevel(QQmlJS::LoggerWarningId id, QtMsgType level)
    {
        m_categoryLevels[id.name().toString()] = level;
        m_categoryChanged[id.name().toString()] = true;
    }

    bool isCategoryIgnored(QQmlJS::LoggerWarningId id) const
    {
        return m_categoryIgnored[id.name().toString()];
    }
    void setCategoryIgnored(QQmlJS::LoggerWarningId id, bool error)
    {
        m_categoryIgnored[id.name().toString()] = error;
        m_categoryChanged[id.name().toString()] = true;
    }

    bool isCategoryFatal(QQmlJS::LoggerWarningId id) const
    {
        return m_categoryFatal[id.name().toString()];
    }
    void setCategoryFatal(QQmlJS::LoggerWarningId id, bool error)
    {
        m_categoryFatal[id.name().toString()] = error;
        m_categoryChanged[id.name().toString()] = true;
    }

    bool wasCategoryChanged(QQmlJS::LoggerWarningId id) const
    {
        return m_categoryChanged[id.name().toString()];
    }

    QtMsgType compileErrorLevel() const { return m_compileErrorLevel; }
    void setCompileErrorLevel(QtMsgType level) { m_compileErrorLevel = level; }

    QString compileErrorPrefix() const { return m_compileErrorPrefix; }
    void setCompileErrorPrefix(const QString &prefix) { m_compileErrorPrefix = prefix; }

    QString compileSkipPrefix() const { return m_compileSkipPrefix; }
    void setCompileSkipPrefix(const QString &prefix) { m_compileSkipPrefix = prefix; }

    /*! \internal

        Logs \a message with severity deduced from \a category. Prefer using
        this function in most cases.

        \sa setCategoryLevel
    */
    void log(const QString &message, QQmlJS::LoggerWarningId id,
             const QQmlJS::SourceLocation &srcLocation, bool showContext = true,
             bool showFileName = true, const std::optional<QQmlJSFixSuggestion> &suggestion = {},
             const QString overrideFileName = QString(),
             std::optional<quint32> customLineForDisabling = std::nullopt)
    {
        log(Message {
                QQmlJS::DiagnosticMessage {
                    message,
                    m_categoryLevels[id.name().toString()],
                    srcLocation,
                },
                id.name(),
                suggestion,
                Message::CompilationStatus::Normal,
                customLineForDisabling
            }, showContext, showFileName, overrideFileName);
    }

    void logCompileError(const QString &message, const QQmlJS::SourceLocation &srcLocation)
    {
        if (m_inTransaction)
            m_hasPendingCompileError = true;
        else
            m_hasCompileError = true;

        log(Message {
                QQmlJS::DiagnosticMessage {
                    m_compileErrorPrefix + message,
                    m_compileErrorLevel,
                    srcLocation
                },
                qmlCompiler.name(),
                {},  // fixSuggestion
                Message::CompilationStatus::Error
            });

    }

    void logCompileSkip(const QString &message, const QQmlJS::SourceLocation &srcLocation)
    {
        m_hasCompileSkip = true;
        log(Message {
                QQmlJS::DiagnosticMessage {
                        m_compileSkipPrefix + message,
                        m_compileSkipLevel,
                        srcLocation
                },
                qmlCompiler.name(),
                {},  // fixSuggestion
                Message::CompilationStatus::Skip
        });
    }

    void processMessages(QSpan<const QQmlJS::DiagnosticMessage> messages,
                         const QQmlJS::LoggerWarningId id,
                         const QQmlJS::SourceLocation &sourceLocation = QQmlJS::SourceLocation{});

    void ignoreWarnings(uint32_t line, const QSet<QString> &categories)
    {
        m_ignoredWarnings[line] = categories;
    }

    void setSilent(bool silent) { m_output.setSilent(silent); }
    bool isSilent() const { return m_output.isSilent(); }

    /*!
    \internal
    The logger is disabled when warnings are not relevant, for example when the import visitor runs
    on a dependency of a linted file. In that case, the warnings should not be created, and
    expensive QQmlJSUtils::didYouMean call can be saved.

    setSilent() has a different behavior: a silent logger can still be used to process messages as
    JSON, for example, while a disabled logger won't contain any message.
    */
    void setIsDisabled(bool isDisabled) { m_isDisabled = isDisabled; }
    bool isDisabled() const { return m_isDisabled; }

    void setCode(const QString &code) { m_code = code; }
    QString code() const { return m_code; }

    void setFilePath(const QString &filePath) { m_filePath =  filePath; }
    QString filePath() const { return m_filePath; }

    bool currentFunctionHasCompileError() const
    {
        return m_hasCompileError || m_hasPendingCompileError;
    }

    bool currentFunctionWasSkipped() const
    {
        return m_hasCompileSkip;
    }

    bool currentFunctionHasErrorOrSkip() const
    {
        return currentFunctionHasCompileError() || currentFunctionWasSkipped();
    }

    QString currentFunctionCompileErrorMessage() const
    {
        for (const Message &message : m_currentFunctionMessages) {
            if (message.compilationStatus == Message::CompilationStatus::Error)
                return message.message;
        }

        return QString();
    }

    QString currentFunctionCompileSkipMessage() const
    {
        for (const Message &message : m_currentFunctionMessages) {
            if (message.compilationStatus == Message::CompilationStatus::Skip)
                return message.message;
        }

        return QString();
    }

    void startTransaction();
    void commit();
    void rollback();

    void finalizeFuction();

private:
    QMap<QString, QQmlJS::LoggerCategory> m_categories;

    void printContext(const QString &overrideFileName, const QQmlJS::SourceLocation &location);
    void printFix(const QQmlJSFixSuggestion &fix);

    void log(Message diagMsg, bool showContext = false, bool showFileName = true,
             const QString overrideFileName = QString());

    void countMessage(const Message &message);

    QString m_filePath;
    QString m_code;

    QColorOutput m_output;

    QHash<QString, QtMsgType> m_categoryLevels;
    QHash<QString, bool> m_categoryIgnored;

    // If true, triggers qFatal on documents with "pragma Strict"
    // TODO: Works only for qmlCompiler category so far.
    QHash<QString, bool> m_categoryFatal;

    QHash<QString, bool> m_categoryChanged;

    QList<Message> m_pendingMessages;
    QList<Message> m_currentFunctionMessages;
    QList<Message> m_archivedMessages;
    QHash<uint32_t, QSet<QString>> m_ignoredWarnings;

    QString m_compileErrorPrefix;
    QString m_compileSkipPrefix;

    qsizetype m_numWarnings = 0;
    qsizetype m_numErrors = 0;
    bool m_inTransaction = false;
    bool m_hasCompileError = false;
    bool m_hasPendingCompileError = false;
    bool m_hasCompileSkip = false;
    bool m_isDisabled = false;

    QtMsgType m_compileErrorLevel = QtWarningMsg;
    QtMsgType m_compileSkipLevel = QtInfoMsg;
};

QT_END_NAMESPACE

#endif // QQMLJSLOGGER_P_H
