/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCTileLogger.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>

QGC_LOGGING_CATEGORY(QGCTileLoggerLog, "qgc.qtlocationplugin.tilelogger")

QGCTileLogger::QGCTileLogger()
{
    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tile_logs.jsonl";
    setLogFilePath(defaultPath);
}

QGCTileLogger::~QGCTileLogger()
{
    if (_logFile.isOpen()) {
        _logFile.close();
    }
}

QGCTileLogger* QGCTileLogger::instance()
{
    static QGCTileLogger *_instance = nullptr;
    if (!_instance) {
        _instance = new QGCTileLogger();
    }
    return _instance;
}

void QGCTileLogger::setLogFilePath(const QString &path)
{
    QMutexLocker locker(&_mutex);

    if (_logFile.isOpen()) {
        _logFile.close();
    }

    _logFilePath = path;

    const QFileInfo fileInfo(path);
    const QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    _logFile.setFileName(path);
    if (_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        _logStream.setDevice(&_logFile);
        qCDebug(QGCTileLoggerLog) << "Tile logger initialized:" << path;
    } else {
        qCWarning(QGCTileLoggerLog) << "Failed to open tile log file:" << path;
    }
}

void QGCTileLogger::logTileRequest(const QUuid &requestId, int mapId, int x, int y, int zoom, const QString &providerName)
{
    if (!_enabled) {
        return;
    }

    QJsonObject data;
    data["requestId"] = requestId.toString();
    data["mapId"] = mapId;
    data["x"] = x;
    data["y"] = y;
    data["zoom"] = zoom;
    data["provider"] = providerName;

    log(LogLevel::Info, "TileRequest", "Tile requested", data);
}

void QGCTileLogger::logTileResponse(const QUuid &requestId, bool success, int httpStatusCode, quint64 bytesReceived, qint64 durationMs, const QString &errorMessage)
{
    if (!_enabled) {
        return;
    }

    QJsonObject data;
    data["requestId"] = requestId.toString();
    data["success"] = success;
    data["httpStatusCode"] = httpStatusCode;
    data["bytesReceived"] = static_cast<qint64>(bytesReceived);
    data["durationMs"] = durationMs;
    if (!errorMessage.isEmpty()) {
        data["error"] = errorMessage;
    }

    log(success ? LogLevel::Info : LogLevel::Warning, "TileResponse", "Tile response received", data);
}

void QGCTileLogger::logCacheEvent(const QString &eventType, int x, int y, int zoom, const QJsonObject &additionalData)
{
    if (!_enabled) {
        return;
    }

    QJsonObject data = additionalData;
    data["eventType"] = eventType;
    data["x"] = x;
    data["y"] = y;
    data["zoom"] = zoom;

    log(LogLevel::Debug, "Cache", eventType, data);
}

void QGCTileLogger::logProviderHealth(int mapId, const QString &providerName, double healthScore, double successRate)
{
    if (!_enabled) {
        return;
    }

    QJsonObject data;
    data["mapId"] = mapId;
    data["provider"] = providerName;
    data["healthScore"] = healthScore;
    data["successRate"] = successRate;

    log(LogLevel::Info, "ProviderHealth", "Provider health updated", data);
}

void QGCTileLogger::logNetworkEvent(const QString &eventType, const QJsonObject &data)
{
    if (!_enabled) {
        return;
    }

    log(LogLevel::Info, "Network", eventType, data);
}

void QGCTileLogger::logError(const QString &component, const QString &message, const QJsonObject &context)
{
    if (!_enabled) {
        return;
    }

    log(LogLevel::Error, component, message, context);
}

void QGCTileLogger::log(LogLevel level, const QString &component, const QString &message, const QJsonObject &data)
{
    if (!_enabled) {
        return;
    }

    QJsonObject logEntry;
    logEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    logEntry["level"] = levelToString(level);
    logEntry["component"] = component;
    logEntry["message"] = message;

    if (!data.isEmpty()) {
        logEntry["data"] = data;
    }

    writeLog(logEntry);
}

void QGCTileLogger::writeLog(const QJsonObject &logEntry)
{
    QMutexLocker locker(&_mutex);

    if (!_logFile.isOpen()) {
        return;
    }

    const QJsonDocument doc(logEntry);
    _logStream << doc.toJson(QJsonDocument::Compact) << Qt::endl;

    // Check if rotation is needed
    if (_logFile.size() > kMaxLogFileSizeBytes) {
        locker.unlock();
        rotate();
    }
}

QString QGCTileLogger::levelToString(LogLevel level) const
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

void QGCTileLogger::flush()
{
    QMutexLocker locker(&_mutex);
    if (_logFile.isOpen()) {
        _logStream.flush();
        _logFile.flush();
    }
}

void QGCTileLogger::rotate()
{
    QMutexLocker locker(&_mutex);

    if (!_logFile.isOpen()) {
        return;
    }

    _logFile.close();

    const QString backupPath = _logFilePath + ".old";
    QFile::remove(backupPath);
    QFile::rename(_logFilePath, backupPath);

    if (_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        _logStream.setDevice(&_logFile);
        qCDebug(QGCTileLoggerLog) << "Log file rotated. Old log saved to:" << backupPath;
    } else {
        qCWarning(QGCTileLoggerLog) << "Failed to reopen log file after rotation";
    }
}
