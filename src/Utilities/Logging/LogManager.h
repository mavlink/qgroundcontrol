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
#include "LogFormatter.h"

class QJSEngine;
class QQmlEngine;
class LogModel;
class LogRemoteSink;
class LogStore;
class LogStoreQueryModel;
class QGCFileWriter;

class LogManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("LogModel.h")
    Q_MOC_INCLUDE("LogRemoteSink.h")
    Q_MOC_INCLUDE("LogStore.h")
    Q_MOC_INCLUDE("LogStoreQueryModel.h")
    Q_PROPERTY(LogModel* model READ model CONSTANT)
    Q_PROPERTY(LogRemoteSink* remoteSink READ remoteSink CONSTANT)
    Q_PROPERTY(LogStore* logStore READ logStore CONSTANT)
    Q_PROPERTY(LogStoreQueryModel* historyModel READ historyModel CONSTANT)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(
        bool diskLoggingEnabled READ isDiskLoggingEnabled WRITE setDiskLoggingEnabled NOTIFY diskLoggingEnabledChanged)
    Q_PROPERTY(bool diskCompressionEnabled READ isDiskCompressionEnabled WRITE setDiskCompressionEnabled NOTIFY
                   diskCompressionEnabledChanged)
    Q_PROPERTY(int flushOnLevel READ flushOnLevel WRITE setFlushOnLevel NOTIFY flushOnLevelChanged)

public:
    enum ExportFormat
    {
        PlainText = LogFormatter::PlainText,
        JSON = LogFormatter::JSON,
        CSV = LogFormatter::CSV,
        JSONLines = LogFormatter::JSONLines,
    };
    Q_ENUM(ExportFormat)

    ~LogManager();

    static LogManager* instance();
    static LogManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static void installHandler();
    static void applyEnvironmentLogLevel();

    [[nodiscard]] LogModel* model() { return _model; }

    [[nodiscard]] LogRemoteSink* remoteSink() { return _remoteSink; }

    [[nodiscard]] LogStore* logStore() { return _logStore; }

    [[nodiscard]] LogStoreQueryModel* historyModel() { return _historyModel; }

    [[nodiscard]] bool hasError() const { return _ioError; }

    [[nodiscard]] QString lastError() const { return _lastError; }

    void setLogDirectory(const QString& path);

    [[nodiscard]] QString logDirectory() const { return _logDirectory; }

    [[nodiscard]] bool isDiskLoggingEnabled() const { return _diskLoggingEnabled; }

    void setDiskLoggingEnabled(bool enabled);

    [[nodiscard]] bool isDiskCompressionEnabled() const { return _diskCompressionEnabled; }

    void setDiskCompressionEnabled(bool enabled);

    [[nodiscard]] int flushOnLevel() const { return _flushOnLevel; }

    void setFlushOnLevel(int level);

    Q_INVOKABLE void writeMessages(const QString& destFile, ExportFormat format = PlainText);
    Q_INVOKABLE void writeFilteredMessages(const QString& destFile, ExportFormat format = PlainText);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void flush();

    Q_INVOKABLE static QStringList categoryLogLevelNames();
    Q_INVOKABLE static QVariantList categoryLogLevelValues();

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
    void diskLoggingEnabledChanged();
    void diskCompressionEnabledChanged();
    void flushOnLevelChanged();
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
    void _rotateLogs();
    void _setIoError(const QString& message);
    void _exportEntries(QList<LogEntry> entries, const QString& destFile, ExportFormat format);
    const QString& _internCategory(const QString& category);

    LogModel* _model = nullptr;
    LogRemoteSink* _remoteSink = nullptr;
    LogStore* _logStore = nullptr;
    LogStoreQueryModel* _historyModel = nullptr;
    QGCFileWriter* _fileWriter = nullptr;

    struct RateBucket
    {
        qint64 lastRefillMs = 0;
        int tokens = 0;
        int suppressed = 0;
    };

    bool _rateLimitCheck(const LogEntry& entry);
    void _emitSuppressedSummary(const QString& category, int count);

    QFuture<void> _exportFuture;
    QTimer _flushTimer;
    QList<LogEntry> _pendingDiskWrites;
    QSet<QString> _internedCategories;
    QHash<QString, RateBucket> _rateBuckets;
    QString _logDirectory;
    bool _ioError = false;
    QString _lastError;
    bool _diskLoggingEnabled = false;
    bool _diskCompressionEnabled = false;
    int _flushOnLevel = LogEntry::Warning;

    static constexpr int kMaxLogFileSize = 10 * 1024 * 1024;
    static constexpr int kMaxBackupFiles = 5;
    static constexpr int kFlushIntervalMSecs = 1000;
    static constexpr int kRateTokensPerSecond = 100;
    static constexpr int kRateMaxTokens = 200;
};
