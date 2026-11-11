#include "RTCMLogFile.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMLogFileLog, "GPS.Corrections.RTCMLogFile")

RTCMLogFile::RTCMLogFile(qint64 maxFileBytes)
    : _file(&RTCMLogFileLog, "Correction log", maxFileBytes)
{
    qCDebug(RTCMLogFileLog) << this;
}

RTCMLogFile::~RTCMLogFile()
{
    close();
    qCDebug(RTCMLogFileLog) << this;
}

QString RTCMLogFile::fileName(const QString& directory, const QDateTime& startedAt)
{
    return QDir(directory).filePath(startedAt.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmss.zzz'Z'")) +
                                    QStringLiteral(".rtcm3"));
}

bool RTCMLogFile::open(const QString& file)
{
    if (_file.open(file)) {
        qCDebug(RTCMLogFileLog) << "Logging corrections to" << file;
    }
    return isOpen();
}
