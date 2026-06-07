// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOGGING_H
#define QLOGGING_H

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qcontainerfwd.h>

#if 0
// header is automatically included in qglobal.h
#pragma qt_no_master_include
#pragma qt_class(QtLogging)
#endif

QT_BEGIN_NAMESPACE

/*
  Forward declarations only.

  In order to use the qDebug() stream, you must #include<QDebug>
*/
class QDebug;
class QNoDebug;


enum QtMsgType {
    QtDebugMsg,
    QT7_ONLY(QtInfoMsg,)
    QtWarningMsg,
    QtCriticalMsg,
    QtFatalMsg,
    QT6_ONLY(QtInfoMsg,)
#if QT_DEPRECATED_SINCE(6, 7)
    QtSystemMsg Q_DECL_ENUMERATOR_DEPRECATED_X("Use QtCriticalMsg instead.") = QtCriticalMsg
#endif
};

class QInternalMessageLogContext;
class QMessageLogContext
{
    Q_DISABLE_COPY(QMessageLogContext)
public:
    static constexpr int CurrentVersion = 2;
    constexpr QMessageLogContext() noexcept = default;
    constexpr QMessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName) noexcept
        : line(lineNumber), file(fileName), function(functionName), category(categoryName) {}

    int version = CurrentVersion;
    int line = 0;
    const char *file = nullptr;
    const char *function = nullptr;
    const char *category = nullptr;

private:
    QMessageLogContext &copyContextFrom(const QMessageLogContext &logContext) noexcept;

    friend class QInternalMessageLogContext;
    friend class QMessageLogger;
};

class QLoggingCategory;

#if defined(Q_CC_MSVC_ONLY)
#  define QT_MESSAGE_LOGGER_NORETURN
#else
#  define QT_MESSAGE_LOGGER_NORETURN Q_NORETURN
#endif

class Q_CORE_EXPORT QMessageLogger
{
    Q_DISABLE_COPY(QMessageLogger)
public:
    constexpr QMessageLogger() : context() {}
    constexpr QMessageLogger(const char *file, int line, const char *function)
        : context(file, line, function, "default") {}
    constexpr QMessageLogger(const char *file, int line, const char *function, const char *category)
        : context(file, line, function, category) {}

    void debug(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void noDebug(const char *, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3)
    {}
    void info(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    Q_DECL_COLD_FUNCTION
    void warning(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    Q_DECL_COLD_FUNCTION
    void critical(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    QT_MESSAGE_LOGGER_NORETURN Q_DECL_COLD_FUNCTION
    void fatal(const char *msg, ...) const noexcept Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

    typedef const QLoggingCategory &(*CategoryFunction)();

    void debug(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void debug(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void info(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void info(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    Q_DECL_COLD_FUNCTION
    void warning(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    Q_DECL_COLD_FUNCTION
    void warning(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    Q_DECL_COLD_FUNCTION
    void critical(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    Q_DECL_COLD_FUNCTION
    void critical(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    QT_MESSAGE_LOGGER_NORETURN Q_DECL_COLD_FUNCTION
    void fatal(const QLoggingCategory &cat, const char *msg, ...) const noexcept Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    QT_MESSAGE_LOGGER_NORETURN Q_DECL_COLD_FUNCTION
    void fatal(CategoryFunction catFunc, const char *msg, ...) const noexcept Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);

#ifndef QT_NO_DEBUG_STREAM
    QDebug debug() const;
    QDebug debug(const QLoggingCategory &cat) const;
    QDebug debug(CategoryFunction catFunc) const;
    QDebug info() const;
    QDebug info(const QLoggingCategory &cat) const;
    QDebug info(CategoryFunction catFunc) const;
    Q_DECL_COLD_FUNCTION
    QDebug warning() const;
    Q_DECL_COLD_FUNCTION
    QDebug warning(const QLoggingCategory &cat) const;
    Q_DECL_COLD_FUNCTION
    QDebug warning(CategoryFunction catFunc) const;
    Q_DECL_COLD_FUNCTION
    QDebug critical() const;
    Q_DECL_COLD_FUNCTION
    QDebug critical(const QLoggingCategory &cat) const;
    Q_DECL_COLD_FUNCTION
    QDebug critical(CategoryFunction catFunc) const;
    Q_DECL_COLD_FUNCTION
    QDebug fatal() const;
    Q_DECL_COLD_FUNCTION
    QDebug fatal(const QLoggingCategory &cat) const;
    Q_DECL_COLD_FUNCTION
    QDebug fatal(CategoryFunction catFunc) const;
#endif // QT_NO_DEBUG_STREAM

#  if QT_CORE_REMOVED_SINCE(6, 10)
    QNoDebug noDebug() const noexcept;
#  endif
    inline QNoDebug noDebug(...) const noexcept;    // in qdebug.h

private:
    QMessageLogContext context;
};

#undef QT_MESSAGE_LOGGER_NORETURN

#if !defined(QT_MESSAGELOGCONTEXT) && !defined(QT_NO_MESSAGELOGCONTEXT)
#  if defined(QT_NO_DEBUG)
#    define QT_NO_MESSAGELOGCONTEXT
#  else
#    define QT_MESSAGELOGCONTEXT
#  endif
#endif

#ifdef QT_MESSAGELOGCONTEXT
  #define QT_MESSAGELOG_FILE static_cast<const char *>(__FILE__)
  #define QT_MESSAGELOG_LINE __LINE__
  #define QT_MESSAGELOG_FUNC static_cast<const char *>(Q_FUNC_INFO)
#else
  #define QT_MESSAGELOG_FILE nullptr
  #define QT_MESSAGELOG_LINE 0
  #define QT_MESSAGELOG_FUNC nullptr
#endif

#define qDebug QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug
#define qInfo QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).info
#define qWarning QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).warning
#define qCritical QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).critical
#define qFatal QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).fatal

Q_CORE_EXPORT Q_DECL_COLD_FUNCTION void qErrnoWarning(int code, const char *msg, ...);
Q_CORE_EXPORT Q_DECL_COLD_FUNCTION void qErrnoWarning(const char *msg, ...);

#define QT_NO_QDEBUG_MACRO while (false) QMessageLogger().noDebug

#if defined(QT_NO_DEBUG_OUTPUT)
#  undef qDebug
#  define qDebug QT_NO_QDEBUG_MACRO
#endif
#if defined(QT_NO_INFO_OUTPUT)
#  undef qInfo
#  define qInfo QT_NO_QDEBUG_MACRO
#endif
#if defined(QT_NO_WARNING_OUTPUT)
#  undef qWarning
#  define qWarning QT_NO_QDEBUG_MACRO
#  define qErrnoWarning QT_NO_QDEBUG_MACRO
#endif

Q_CORE_EXPORT void qt_message_output(QtMsgType, const QMessageLogContext &context,
                                     const QString &message);

typedef void (*QtMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
Q_CORE_EXPORT QtMessageHandler qInstallMessageHandler(QtMessageHandler);

Q_CORE_EXPORT void qSetMessagePattern(const QString &messagePattern);
Q_CORE_EXPORT QString qFormatLogMessage(QtMsgType type, const QMessageLogContext &context,
                                        const QString &buf);

Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT QString qt_error_string(int errorCode = -1);

QT_END_NAMESPACE
#endif // QLOGGING_H
