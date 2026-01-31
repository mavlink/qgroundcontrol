#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#include <functional>

class QAbstractState;
class QAbstractTransition;
class QGCStateMachine;

/// Extended logging for state machines.
///
/// Provides configurable, structured logging with multiple output options.
///
/// Example usage:
/// @code
/// QGCStateMachine machine("MyMachine", nullptr);
/// StateMachineLogger logger(&machine);
///
/// logger.setLogLevel(StateMachineLogger::Verbose);
/// logger.setColoredOutput(true);
/// logger.setLogTimings(true);
/// logger.setEnabled(true);
///
/// // Or log to file
/// logger.setLogFile("/tmp/machine.log");
/// @endcode
class StateMachineLogger : public QObject
{
    Q_OBJECT

public:
    /// Log verbosity levels
    enum LogLevel {
        Silent = 0,     ///< No logging
        Error = 1,      ///< Only errors
        Normal = 2,     ///< Errors + state changes
        Verbose = 3,    ///< Normal + signals/transitions
        Trace = 4       ///< Everything including internal details
    };
    Q_ENUM(LogLevel)

    /// Types of log events
    enum LogEvent {
        EventNone           = 0,
        EventStateEntry     = 1 << 0,
        EventStateExit      = 1 << 1,
        EventTransition     = 1 << 2,
        EventSignal         = 1 << 3,
        EventTimeout        = 1 << 4,
        EventError          = 1 << 5,
        EventMachineStart   = 1 << 6,
        EventMachineStop    = 1 << 7,
        EventRetry          = 1 << 8,
        EventProgress       = 1 << 9,
        EventCustom         = 1 << 10,

        EventAll            = 0xFFFF,
        EventStateChanges   = EventStateEntry | EventStateExit,
        EventMachineEvents  = EventMachineStart | EventMachineStop,
        EventErrors         = EventError | EventTimeout
    };
    Q_DECLARE_FLAGS(LogEvents, LogEvent)

    /// Structured log entry
    struct LogEntry {
        QDateTime timestamp;
        qint64 elapsedMs = 0;
        LogLevel level = Normal;
        LogEvent event = EventNone;
        QString machine;
        QString state;
        QString previousState;
        QString message;
        QString transitionReason;
        qint64 stateDurationMs = 0;
        QJsonObject context;
        int depth = 0;

        QString toString(bool colored = false, bool showTiming = true, bool indent = true) const;
        QJsonObject toJson() const;
    };

    using LogHandler = std::function<void(const LogEntry&)>;

    explicit StateMachineLogger(QGCStateMachine* machine, QObject* parent = nullptr);
    ~StateMachineLogger() override;

    // -------------------------------------------------------------------------
    // Enable/Disable
    // -------------------------------------------------------------------------

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    // -------------------------------------------------------------------------
    // Log Level
    // -------------------------------------------------------------------------

    void setLogLevel(LogLevel level) { _logLevel = level; }
    LogLevel logLevel() const { return _logLevel; }

    /// Set log level override for a specific state
    void setStateLogLevel(const QString& stateName, LogLevel level);
    void clearStateLogLevel(const QString& stateName);

    // -------------------------------------------------------------------------
    // Event Filtering
    // -------------------------------------------------------------------------

    /// Set which events to log (default: all)
    void setLogFilter(LogEvents events) { _logFilter = events; }
    LogEvents logFilter() const { return _logFilter; }

    /// Exclude specific states from logging
    void excludeState(const QString& stateName);
    void includeState(const QString& stateName);
    void clearExclusions() { _excludedStates.clear(); }

    // -------------------------------------------------------------------------
    // Output Options
    // -------------------------------------------------------------------------

    /// Enable colored console output
    void setColoredOutput(bool enabled) { _coloredOutput = enabled; }
    bool coloredOutput() const { return _coloredOutput; }

    /// Enable timing annotations
    void setLogTimings(bool enabled) { _logTimings = enabled; }
    bool logTimings() const { return _logTimings; }

    /// Enable hierarchy indentation
    void setLogIndent(bool enabled) { _logIndent = enabled; }
    bool logIndent() const { return _logIndent; }

    /// Log transition reasons (signal name, timeout, etc.)
    void setLogTransitionReasons(bool enabled) { _logTransitionReasons = enabled; }
    bool logTransitionReasons() const { return _logTransitionReasons; }

    // -------------------------------------------------------------------------
    // Log Sinks
    // -------------------------------------------------------------------------

    /// Log to a file (in addition to console)
    bool setLogFile(const QString& filePath);
    void closeLogFile();

    /// Set a custom log handler
    void setLogHandler(LogHandler handler) { _customHandler = std::move(handler); }

    /// Enable crash log buffer (circular buffer for post-mortem analysis)
    void enableCrashLog(int maxEntries);
    void disableCrashLog();
    QString dumpCrashLog() const;
    QList<LogEntry> crashLogEntries() const { return _crashLog; }

    // -------------------------------------------------------------------------
    // Manual Logging
    // -------------------------------------------------------------------------

    /// Log a custom message
    void log(LogLevel level, const QString& message, const QJsonObject& context = QJsonObject());

    /// Log with specific event type
    void logEvent(LogEvent event, const QString& message, const QJsonObject& context = QJsonObject());

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    /// Get count of entries by event type
    QHash<LogEvent, int> eventCounts() const { return _eventCounts; }

    /// Reset statistics
    void resetStats() { _eventCounts.clear(); }

private slots:
    void _onMachineStarted();
    void _onMachineStopped();
    void _onStateEntered();
    void _onStateExited();

private:
    void _log(const LogEntry& entry);
    void _connectToState(QAbstractState* state);
    QString _colorize(const QString& text, const QString& colorCode) const;
    QString _eventColor(LogEvent event) const;
    LogLevel _effectiveLogLevel(const QString& stateName) const;
    QString _determineTransitionReason() const;

    QGCStateMachine* _machine = nullptr;
    bool _enabled = false;
    LogLevel _logLevel = Normal;
    LogEvents _logFilter = EventAll;

    bool _coloredOutput = false;
    bool _logTimings = true;
    bool _logIndent = true;
    bool _logTransitionReasons = true;

    QSet<QString> _excludedStates;
    QHash<QString, LogLevel> _stateLogLevels;

    QFile* _logFile = nullptr;
    QTextStream* _logStream = nullptr;
    LogHandler _customHandler;

    QList<LogEntry> _crashLog;
    int _crashLogMaxEntries = 0;

    QElapsedTimer _machineTimer;
    QHash<QString, qint64> _stateEntryTimes;
    QString _previousState;
    int _currentDepth = 0;

    QHash<LogEvent, int> _eventCounts;
    QList<QMetaObject::Connection> _connections;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(StateMachineLogger::LogEvents)
