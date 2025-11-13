/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QUuid>

Q_DECLARE_LOGGING_CATEGORY(QGCTileLoggerLog)

class QGCTileLogger
{
public:
    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    static QGCTileLogger* instance();

    void enable(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    void setLogFilePath(const QString &path);
    QString logFilePath() const { return _logFilePath; }

    // Structured logging methods
    void logTileRequest(const QUuid &requestId, int mapId, int x, int y, int zoom, const QString &providerName);
    void logTileResponse(const QUuid &requestId, bool success, int httpStatusCode, quint64 bytesReceived, qint64 durationMs, const QString &errorMessage = QString());
    void logCacheEvent(const QString &eventType, int x, int y, int zoom, const QJsonObject &additionalData = QJsonObject());
    void logProviderHealth(int mapId, const QString &providerName, double healthScore, double successRate);
    void logNetworkEvent(const QString &eventType, const QJsonObject &data);
    void logError(const QString &component, const QString &message, const QJsonObject &context = QJsonObject());

    // Generic structured log
    void log(LogLevel level, const QString &component, const QString &message, const QJsonObject &data = QJsonObject());

    void flush();
    void rotate();  // Rotate log file if too large

private:
    QGCTileLogger();
    ~QGCTileLogger();

    void writeLog(const QJsonObject &logEntry);
    QString levelToString(LogLevel level) const;

    bool _enabled = false;
    QString _logFilePath;
    QFile _logFile;
    QTextStream _logStream;
    mutable QMutex _mutex;

    static constexpr quint64 kMaxLogFileSizeBytes = 50 * 1024 * 1024;  // 50 MB
};
