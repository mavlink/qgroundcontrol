#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <QObject>
#include <QString>

#if defined(Q_OS_WIN)
#include "client/windows/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#include "client/linux/handler/exception_handler.h"
#else
#error Crash Handler: Unsupported platform
#endif

#define CLOG_CRASH_HANDLER_STR  "breakpad.crashHandler"

class CrashHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString dumpPath READ dumpPath CONSTANT)

public:
    explicit CrashHandler(QObject *parent = nullptr);

    QString dumpPath() const { return m_dumpPath; }
    void setDumpPath(const QString &path);

    QString namePrefix() const { return m_namePrefix; }
    void setNamePrefix(const QString &s) { m_namePrefix = s; }

    QString nameSuffix() const { return m_nameSuffix; }
    void setNameSuffix(const QString &s) { m_nameSuffix = s; }

signals:
    void minidumpCreated(const QString &dumpFilePath);

public slots:
    void install();

private:
    static inline void minidumpCreated(const CrashHandler *crashHandler,
                                       const QString &dumpFilePath);

#if defined(Q_OS_WIN)
    static bool minidumpCallback(const wchar_t *dump_path,
                                 const wchar_t *minidump_id,
                                 void *context,
                                 EXCEPTION_POINTERS *exinfo,
                                 MDRawAssertionInfo *assertion,
                                 bool succeeded);
#elif defined(Q_OS_LINUX)
    static bool minidumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                                 void *context,
                                 bool succeeded);
#endif

private:
    QString m_dumpPath;
    QString m_namePrefix;
    QString m_nameSuffix;
};

#endif // CRASHHANDLER_H
