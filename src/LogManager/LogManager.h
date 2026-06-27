#pragma once

#include <QtCore/QFuture>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LogEntry.h"

class QJSEngine;
class QQmlEngine;
class LogModel;
class QGCFileWriter;

class LogManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_MOC_INCLUDE("LogModel.h")

    Q_PROPERTY(LogModel*    model       READ model      CONSTANT)
    Q_PROPERTY(bool         hasError    READ hasError   NOTIFY hasErrorChanged)
    Q_PROPERTY(QString      lastError   READ lastError  NOTIFY lastErrorChanged)

public:
    ~LogManager();

    static LogManager* instance();
    static LogManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static void installHandler(bool logOutput);
    static void applyEnvironmentLogLevel();

    void init();

    [[nodiscard]] LogModel* model() { return _model; }

    [[nodiscard]] bool hasError() const { return _ioError; }

    [[nodiscard]] QString lastError() const { return _lastError; }

    void setLogDirectory(const QString& path);

    [[nodiscard]] QString logDirectory() const { return _logDirectory; }

    Q_INVOKABLE void writeMessages(const QString& destFile);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void flush();

    static void setCaptureEnabled(bool enabled);
    static void clearCapturedMessages();
    [[nodiscard]] static QList<LogEntry> capturedMessages(const QString& category = {});
    [[nodiscard]] static bool hasCapturedMessage(const QString& category, LogEntry::Level level);
    [[nodiscard]] static bool hasCapturedWarning(const QString& category);
    [[nodiscard]] static bool hasCapturedCritical(const QString& category);
    [[nodiscard]] static bool hasCapturedUncategorizedMessage();
    static void captureIfEnabled(QtMsgType type, const QMessageLogContext& context, const QString& msg);

signals:
    void hasErrorChanged();
    void lastErrorChanged();
    void writeStarted();
    void writeFinished(bool success);

private slots:
    void _handleEntry(const LogEntry& entry);
    void _flushToDisk();

private:
    explicit LogManager(QObject* parent = nullptr);

    static void msgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    void log(QtMsgType type, const QMessageLogContext& context, const QString& message);
    static LogEntry buildEntry(QtMsgType type, const QMessageLogContext& context, const QString& message);

    void _dispatchToSinks(const LogEntry& entry);
    void _replayEarlyEntries();
    void _setDiskLoggingEnabled(bool enabled);
    void _rotateLogs();
    void _setIoError(const QString& message);
    void _exportEntries(QList<LogEntry> entries, const QString& destFile);
    const QString& _internCategory(const QString& category);

    // Rate limiting infrastructure — currently disabled (_rateLimitingEnabled = false)
    struct RateBucket
    {
        qint64 lastRefillMs = 0;
        int tokens = 0;
        int suppressed = 0;
    };

    bool _rateLimitCheck(const LogEntry& entry);
    void _emitSuppressedSummary(const QString& category, int count);

    LogModel* _model = nullptr;
    QGCFileWriter* _fileWriter = nullptr;

    QFuture<void> _exportFuture;
    QTimer _flushTimer;
    QList<LogEntry> _pendingDiskWrites;
    QSet<QString> _internedCategories;
    QHash<QString, RateBucket> _rateBuckets;
    QString _logDirectory;
    bool _ioError = false;
    QString _lastError;
    bool _initialized = false;
    bool _diskLoggingEnabled = false;
    bool _rateLimitingEnabled = false;
    int _maxLogFileSize = 10 * 1024 * 1024;
    int _maxBackupFiles = 5;

    static constexpr int kFlushIntervalMSecs = 1000;
    static constexpr int kRateTokensPerSecond = 100;
    static constexpr int kRateMaxTokens = 200;
};
