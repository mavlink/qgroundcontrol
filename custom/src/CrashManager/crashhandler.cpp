#include "crashhandler.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QStandardPaths>

Q_DECLARE_LOGGING_CATEGORY(CLOG_CRASH_HANDLER)
Q_LOGGING_CATEGORY(CLOG_CRASH_HANDLER, CLOG_CRASH_HANDLER_STR)

CrashHandler::CrashHandler(QObject *parent) :
    QObject(parent),
    m_dumpPath(QStandardPaths::writableLocation(
                   QStandardPaths::TempLocation))
{
}

void CrashHandler::setDumpPath(const QString &path)
{
    m_dumpPath = path;

    QDir().mkpath(path);
}

void CrashHandler::install()
{
    static google_breakpad::ExceptionHandler *g_eh = nullptr;

    if (g_eh) return;

#if defined(Q_OS_WIN)
    g_eh = new google_breakpad::ExceptionHandler(
                m_dumpPath.toStdWString(), nullptr,
                &minidumpCallback, this,
                google_breakpad::ExceptionHandler::HANDLER_ALL);
#elif defined(Q_OS_LINUX)
    google_breakpad::MinidumpDescriptor descriptor(m_dumpPath.toStdString());

    g_eh = new google_breakpad::ExceptionHandler(
                descriptor, nullptr,
                &minidumpCallback, this,
                true, -1);
#endif

    if (g_eh) {
        qInfo(CLOG_CRASH_HANDLER()) << "Dump Path:" << dumpPath();
    }
}

void CrashHandler::minidumpCreated(const CrashHandler *crashHandler,
                                   const QString &dumpFilePath)
{
    const QString namePrefix = crashHandler->namePrefix();
    const QString nameSuffix = crashHandler->nameSuffix();

    const QString newFileName =
            (namePrefix.isEmpty() ? QString() : namePrefix + '-')
            + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss_zzz")
            + (nameSuffix.isEmpty() ? QString() : '-' + nameSuffix)
            + ".dmp";

    const QFileInfo fi(dumpFilePath);
    QDir dumpDir = fi.dir();

    // Rename mini-dump file name
    const QString newFilePath =
            dumpDir.rename(fi.fileName(), newFileName)
            ? dumpDir.filePath(newFileName) : dumpFilePath;

    qInfo(CLOG_CRASH_HANDLER()) << "Dump Created:" << newFilePath;
}

#if defined(Q_OS_WIN)

bool CrashHandler::minidumpCallback(const wchar_t *dump_path,
                                    const wchar_t *minidump_id,
                                    void *context,
                                    EXCEPTION_POINTERS *exinfo,
                                    MDRawAssertionInfo *assertion,
                                    bool succeeded)
{
    Q_UNUSED(exinfo)
    Q_UNUSED(assertion)

    if (succeeded) {
        const QString dumpFilePath = QString::fromStdWString(dump_path)
                + '/' + QString::fromStdWString(minidump_id) + ".dmp";

        minidumpCreated(static_cast<CrashHandler *>(context),
                        dumpFilePath);
    }
    return succeeded;
}

#elif defined(Q_OS_LINUX)

bool CrashHandler::minidumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                                    void *context,
                                    bool succeeded)
{
    if (succeeded) {
        const QString dumpFilePath = QString::fromUtf8(descriptor.path());

        minidumpCreated(static_cast<CrashHandler *>(context),
                        dumpFilePath);
    }
    return succeeded;
}

#endif
