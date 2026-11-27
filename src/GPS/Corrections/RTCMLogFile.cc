#include "RTCMLogFile.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMLogFileLog, "GPS.Corrections.RTCMLogFile")

RTCMLogFile::RTCMLogFile(qint64 maxFileBytes)
    : _maxFileBytes(maxFileBytes)
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
    close();
    _file.setFileName(file);
    _bytes = 0;
    const QString directory = QFileInfo(_file).absolutePath();
    if (!QDir().mkpath(directory)) {
        _fail(QStringLiteral("Cannot create the folder %1").arg(directory));
    } else if (!_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        _fail(_file.errorString());
    } else {
        qCDebug(RTCMLogFileLog) << "Logging corrections to" << file;
    }
    return isOpen();
}

bool RTCMLogFile::write(const QByteArray& frame)
{
    if (!isOpen()) {
        return false;
    }
    // Whole frames only, so a log that reached its limit still ends with a complete frame.
    if (_bytes + frame.size() > _maxFileBytes) {
        qCWarning(RTCMLogFileLog) << "Correction log reached its size limit of" << _maxFileBytes
                                  << "bytes and stopped:" << _file.fileName();
        close();
    } else if (_file.write(frame) != frame.size()) {
        _fail(_file.errorString());
    } else {
        _bytes += frame.size();
    }
    return isOpen();
}

bool RTCMLogFile::flush()
{
    if (isOpen() && !_file.flush()) {
        _fail(_file.errorString());
    }
    return isOpen();
}

void RTCMLogFile::close()
{
    if (flush()) {
        _file.close();
    }
}

void RTCMLogFile::_fail(const QString& error)
{
    qCWarning(RTCMLogFileLog) << "Correction log stopped after a file error:" << _file.fileName() << error;
    _file.close();
}
