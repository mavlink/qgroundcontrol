#include "StateMachineLogger.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtStateMachine/QAbstractState>
#include <QtStateMachine/QAbstractTransition>
#include <QtStateMachine/QSignalTransition>

// ANSI color codes
namespace Colors {
    const char* Reset   = "\033[0m";
    const char* Red     = "\033[31m";
    const char* Green   = "\033[32m";
    const char* Yellow  = "\033[33m";
    const char* Blue    = "\033[34m";
    const char* Magenta = "\033[35m";
    const char* Cyan    = "\033[36m";
    const char* White   = "\033[37m";
    const char* BoldRed = "\033[1;31m";
    const char* Gray    = "\033[90m";
}

// -----------------------------------------------------------------------------
// LogEntry
// -----------------------------------------------------------------------------

QString StateMachineLogger::LogEntry::toString(bool colored, bool showTiming, bool indent) const
{
    QString result;

    // Timing prefix
    if (showTiming) {
        QString timeStr = QStringLiteral("[+%1]").arg(elapsedMs / 1000.0, 7, 'f', 3);
        if (colored) {
            result += QStringLiteral("%1%2%3 ").arg(Colors::Gray, timeStr, Colors::Reset);
        } else {
            result += timeStr + " ";
        }
    }

    // Indentation
    if (indent && depth > 0) {
        result += QString(depth * 2, ' ');
    }

    // Event-specific formatting
    QString eventStr;
    QString colorCode = Colors::White;

    switch (event) {
    case StateMachineLogger::EventStateEntry:
        eventStr = QStringLiteral("▶ Entered %1").arg(state);
        colorCode = Colors::Green;
        break;

    case StateMachineLogger::EventStateExit:
        eventStr = QStringLiteral("◀ Exited %1").arg(state);
        if (stateDurationMs > 0) {
            eventStr += QStringLiteral(" (duration: %1ms)").arg(stateDurationMs);
        }
        colorCode = Colors::Blue;
        break;

    case StateMachineLogger::EventTransition:
        eventStr = QStringLiteral("→ Transition %1 -> %2").arg(previousState, state);
        if (!transitionReason.isEmpty()) {
            eventStr += QStringLiteral(" [%1]").arg(transitionReason);
        }
        colorCode = Colors::Cyan;
        break;

    case StateMachineLogger::EventTimeout:
        eventStr = QStringLiteral("⏱ Timeout in %1").arg(state);
        colorCode = Colors::Yellow;
        break;

    case StateMachineLogger::EventError:
        eventStr = QStringLiteral("✖ Error in %1").arg(state);
        if (!message.isEmpty()) {
            eventStr += QStringLiteral(": %1").arg(message);
        }
        colorCode = Colors::BoldRed;
        break;

    case StateMachineLogger::EventMachineStart:
        eventStr = QStringLiteral("▷ Machine started: %1").arg(machine);
        colorCode = Colors::Green;
        break;

    case StateMachineLogger::EventMachineStop:
        eventStr = QStringLiteral("□ Machine stopped: %1").arg(machine);
        colorCode = Colors::Blue;
        break;

    case StateMachineLogger::EventRetry:
        eventStr = QStringLiteral("↻ Retry in %1").arg(state);
        if (!message.isEmpty()) {
            eventStr += QStringLiteral(": %1").arg(message);
        }
        colorCode = Colors::Yellow;
        break;

    case StateMachineLogger::EventSignal:
        eventStr = QStringLiteral("⚡ Signal: %1").arg(message);
        colorCode = Colors::Magenta;
        break;

    case StateMachineLogger::EventProgress:
        eventStr = QStringLiteral("◐ Progress: %1").arg(message);
        colorCode = Colors::Cyan;
        break;

    default:
        eventStr = message;
        break;
    }

    if (colored) {
        result += QStringLiteral("%1%2%3").arg(colorCode, eventStr, Colors::Reset);
    } else {
        result += eventStr;
    }

    return result;
}

QJsonObject StateMachineLogger::LogEntry::toJson() const
{
    QJsonObject obj;
    obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
    obj["elapsed_ms"] = elapsedMs;
    obj["level"] = static_cast<int>(level);
    obj["event"] = static_cast<int>(event);
    obj["machine"] = machine;

    if (!state.isEmpty()) {
        obj["state"] = state;
    }
    if (!previousState.isEmpty()) {
        obj["previous_state"] = previousState;
    }
    if (!message.isEmpty()) {
        obj["message"] = message;
    }
    if (!transitionReason.isEmpty()) {
        obj["transition_reason"] = transitionReason;
    }
    if (stateDurationMs > 0) {
        obj["state_duration_ms"] = stateDurationMs;
    }
    if (!context.isEmpty()) {
        obj["context"] = context;
    }
    if (depth > 0) {
        obj["depth"] = depth;
    }

    return obj;
}

// -----------------------------------------------------------------------------
// StateMachineLogger
// -----------------------------------------------------------------------------

StateMachineLogger::StateMachineLogger(QGCStateMachine* machine, QObject* parent)
    : QObject(parent ? parent : machine)
    , _machine(machine)
{
}

StateMachineLogger::~StateMachineLogger()
{
    closeLogFile();
}

void StateMachineLogger::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;

    if (_enabled) {
        // Connect to machine events
        _connections.append(connect(_machine, &QStateMachine::started,
                                    this, &StateMachineLogger::_onMachineStarted));
        _connections.append(connect(_machine, &QStateMachine::stopped,
                                    this, &StateMachineLogger::_onMachineStopped));

        // Connect to all existing states
        const auto states = _machine->findChildren<QAbstractState*>();
        for (QAbstractState* state : states) {
            _connectToState(state);
        }

        _machineTimer.start();
    } else {
        // Disconnect all
        for (const auto& conn : _connections) {
            disconnect(conn);
        }
        _connections.clear();
    }
}

void StateMachineLogger::_connectToState(QAbstractState* state)
{
    auto entryConn = connect(state, &QAbstractState::entered,
                             this, &StateMachineLogger::_onStateEntered);
    auto exitConn = connect(state, &QAbstractState::exited,
                            this, &StateMachineLogger::_onStateExited);
    _connections.append(entryConn);
    _connections.append(exitConn);
}

void StateMachineLogger::setStateLogLevel(const QString& stateName, LogLevel level)
{
    _stateLogLevels[stateName] = level;
}

void StateMachineLogger::clearStateLogLevel(const QString& stateName)
{
    _stateLogLevels.remove(stateName);
}

void StateMachineLogger::excludeState(const QString& stateName)
{
    _excludedStates.insert(stateName);
}

void StateMachineLogger::includeState(const QString& stateName)
{
    _excludedStates.remove(stateName);
}

bool StateMachineLogger::setLogFile(const QString& filePath)
{
    closeLogFile();

    _logFile = new QFile(filePath, this);
    if (!_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete _logFile;
        _logFile = nullptr;
        return false;
    }

    _logStream = new QTextStream(_logFile);
    return true;
}

void StateMachineLogger::closeLogFile()
{
    if (_logStream) {
        delete _logStream;
        _logStream = nullptr;
    }
    if (_logFile) {
        _logFile->close();
        delete _logFile;
        _logFile = nullptr;
    }
}

void StateMachineLogger::enableCrashLog(int maxEntries)
{
    _crashLogMaxEntries = maxEntries;
    _crashLog.clear();
}

void StateMachineLogger::disableCrashLog()
{
    _crashLogMaxEntries = 0;
    _crashLog.clear();
}

QString StateMachineLogger::dumpCrashLog() const
{
    QStringList lines;
    lines << QStringLiteral("=== Crash Log: %1 ===").arg(_machine->objectName());
    lines << QStringLiteral("Entries: %1").arg(_crashLog.size());
    lines << QString();

    for (const auto& entry : _crashLog) {
        lines << entry.toString(false, true, true);
    }

    return lines.join('\n');
}

void StateMachineLogger::log(LogLevel level, const QString& message, const QJsonObject& context)
{
    if (!_enabled || level > _logLevel) {
        return;
    }

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.elapsedMs = _machineTimer.elapsed();
    entry.level = level;
    entry.event = EventCustom;
    entry.machine = _machine->objectName();
    entry.message = message;
    entry.context = context;
    entry.depth = _currentDepth;

    _log(entry);
}

void StateMachineLogger::logEvent(LogEvent event, const QString& message, const QJsonObject& context)
{
    if (!_enabled || !(_logFilter & event)) {
        return;
    }

    LogLevel requiredLevel = Normal;
    if (event == EventSignal || event == EventProgress) {
        requiredLevel = Verbose;
    }

    if (requiredLevel > _logLevel) {
        return;
    }

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.elapsedMs = _machineTimer.elapsed();
    entry.level = requiredLevel;
    entry.event = event;
    entry.machine = _machine->objectName();
    entry.message = message;
    entry.context = context;
    entry.depth = _currentDepth;

    _log(entry);
}

void StateMachineLogger::_log(const LogEntry& entry)
{
    // Update statistics
    _eventCounts[entry.event]++;

    // Add to crash log if enabled
    if (_crashLogMaxEntries > 0) {
        _crashLog.append(entry);
        while (_crashLog.size() > _crashLogMaxEntries) {
            _crashLog.removeFirst();
        }
    }

    // Format the message
    QString formatted = entry.toString(_coloredOutput, _logTimings, _logIndent);

    // Output to console
    qCDebug(QGCStateMachineLog).noquote() << formatted;

    // Output to file
    if (_logStream) {
        *_logStream << entry.toString(false, _logTimings, _logIndent) << "\n";
        _logStream->flush();
    }

    // Output to custom handler
    if (_customHandler) {
        _customHandler(entry);
    }
}

StateMachineLogger::LogLevel StateMachineLogger::_effectiveLogLevel(const QString& stateName) const
{
    if (_stateLogLevels.contains(stateName)) {
        return _stateLogLevels[stateName];
    }
    return _logLevel;
}

void StateMachineLogger::_onMachineStarted()
{
    if (!_enabled || !(_logFilter & EventMachineStart)) {
        return;
    }

    _machineTimer.restart();
    _currentDepth = 0;
    _previousState.clear();
    _stateEntryTimes.clear();

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.elapsedMs = 0;
    entry.level = Normal;
    entry.event = EventMachineStart;
    entry.machine = _machine->objectName();

    _log(entry);
}

void StateMachineLogger::_onMachineStopped()
{
    if (!_enabled || !(_logFilter & EventMachineStop)) {
        return;
    }

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.elapsedMs = _machineTimer.elapsed();
    entry.level = Normal;
    entry.event = EventMachineStop;
    entry.machine = _machine->objectName();

    _log(entry);
}

void StateMachineLogger::_onStateEntered()
{
    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    QString stateName = state->objectName();

    // Check exclusions
    if (_excludedStates.contains(stateName)) {
        return;
    }

    // Check log level
    if (_effectiveLogLevel(stateName) < Normal) {
        return;
    }

    if (!(_logFilter & EventStateEntry)) {
        // Still track for duration calculation
        _stateEntryTimes[stateName] = _machineTimer.elapsed();
        _previousState = stateName;
        return;
    }

    qint64 now = _machineTimer.elapsed();
    _stateEntryTimes[stateName] = now;

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.elapsedMs = now;
    entry.level = Normal;
    entry.event = EventStateEntry;
    entry.machine = _machine->objectName();
    entry.state = stateName;
    entry.previousState = _previousState;
    entry.depth = _currentDepth;

    // Try to determine transition reason
    if (_logTransitionReasons && !_previousState.isEmpty()) {
        entry.transitionReason = _determineTransitionReason();
    }

    _log(entry);

    _previousState = stateName;
    _currentDepth++;
}

void StateMachineLogger::_onStateExited()
{
    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    QString stateName = state->objectName();

    // Check exclusions
    if (_excludedStates.contains(stateName)) {
        return;
    }

    _currentDepth = qMax(0, _currentDepth - 1);

    // Check log level
    if (_effectiveLogLevel(stateName) < Normal) {
        return;
    }

    if (!(_logFilter & EventStateExit)) {
        return;
    }

    qint64 now = _machineTimer.elapsed();
    qint64 duration = 0;
    if (_stateEntryTimes.contains(stateName)) {
        duration = now - _stateEntryTimes[stateName];
        _stateEntryTimes.remove(stateName);
    }

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.elapsedMs = now;
    entry.level = Normal;
    entry.event = EventStateExit;
    entry.machine = _machine->objectName();
    entry.state = stateName;
    entry.stateDurationMs = duration;
    entry.depth = _currentDepth;

    _log(entry);
}

QString StateMachineLogger::_determineTransitionReason() const
{
    // This is a simplified version - in practice you'd need to track
    // which signal/event triggered the transition
    return QString();  // Would need signal tracking infrastructure
}

QString StateMachineLogger::_colorize(const QString& text, const QString& colorCode) const
{
    if (!_coloredOutput) {
        return text;
    }
    return colorCode + text + Colors::Reset;
}

QString StateMachineLogger::_eventColor(LogEvent event) const
{
    switch (event) {
    case EventStateEntry:
    case EventMachineStart:
        return Colors::Green;
    case EventStateExit:
    case EventMachineStop:
        return Colors::Blue;
    case EventTransition:
        return Colors::Cyan;
    case EventSignal:
        return Colors::Magenta;
    case EventTimeout:
    case EventRetry:
        return Colors::Yellow;
    case EventError:
        return Colors::BoldRed;
    default:
        return Colors::White;
    }
}
