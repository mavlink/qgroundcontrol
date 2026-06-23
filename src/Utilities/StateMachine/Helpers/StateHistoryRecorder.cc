#include "StateHistoryRecorder.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

#include <QtStateMachine/QAbstractState>

QJsonObject StateHistoryRecorder::HistoryEntry::toJson() const
{
    QJsonObject obj;
    obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
    obj["stateName"] = stateName;
    obj["reason"] = static_cast<int>(reason);
    if (!details.isEmpty()) {
        obj["details"] = details;
    }
    return obj;
}

QString StateHistoryRecorder::HistoryEntry::toString() const
{
    QString reasonStr;
    switch (reason) {
        case Entered: reasonStr = "ENTER"; break;
        case Exited: reasonStr = "EXIT"; break;
        case Timeout: reasonStr = "TIMEOUT"; break;
        case Error: reasonStr = "ERROR"; break;
        case Signal: reasonStr = "SIGNAL"; break;
        case Event: reasonStr = "EVENT"; break;
        default: reasonStr = "?"; break;
    }

    QString result = QStringLiteral("[%1] %2 %3")
                     .arg(timestamp.toString("HH:mm:ss.zzz"), reasonStr, stateName);
    if (!details.isEmpty()) {
        result += QStringLiteral(" (%1)").arg(details);
    }
    return result;
}

StateHistoryRecorder::StateHistoryRecorder(QGCStateMachine* machine, int maxEntries)
    : QObject(machine)
    , _machine(machine)
    , _maxEntries(maxEntries)
{
}

void StateHistoryRecorder::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;

    if (_enabled) {
        // Connect to all existing states
        const auto states = _machine->findChildren<QAbstractState*>();
        for (QAbstractState* state : states) {
            _connectToState(state);
        }
    } else {
        // Disconnect all
        for (const auto& conn : _connections) {
            disconnect(conn);
        }
        _connections.clear();
    }
}

void StateHistoryRecorder::_connectToState(QAbstractState* state)
{
    auto enterConn = connect(state, &QAbstractState::entered,
                             this, &StateHistoryRecorder::_onStateEntered);
    auto exitConn = connect(state, &QAbstractState::exited,
                            this, &StateHistoryRecorder::_onStateExited);
    _connections.append(enterConn);
    _connections.append(exitConn);
}

void StateHistoryRecorder::setMaxEntries(int max)
{
    _maxEntries = max;

    // Trim if necessary
    while (_history.size() > _maxEntries) {
        _history.removeFirst();
    }
}

void StateHistoryRecorder::clear()
{
    _history.clear();
}

QList<StateHistoryRecorder::HistoryEntry> StateHistoryRecorder::lastEntries(int n) const
{
    if (n >= _history.size()) {
        return _history;
    }
    return _history.mid(_history.size() - n);
}

QList<StateHistoryRecorder::HistoryEntry> StateHistoryRecorder::entriesForState(const QString& stateName) const
{
    QList<HistoryEntry> result;
    for (const auto& entry : _history) {
        if (entry.stateName == stateName) {
            result.append(entry);
        }
    }
    return result;
}

QString StateHistoryRecorder::dumpHistory() const
{
    QStringList lines;
    lines << QStringLiteral("=== State History: %1 ===").arg(_machine->objectName());
    lines << QStringLiteral("Entries: %1 / %2").arg(_history.size()).arg(_maxEntries);
    lines << QString();

    for (const auto& entry : _history) {
        lines << entry.toString();
    }

    return lines.join('\n');
}

QJsonArray StateHistoryRecorder::toJson() const
{
    QJsonArray array;
    for (const auto& entry : _history) {
        array.append(entry.toJson());
    }
    return array;
}

void StateHistoryRecorder::logHistory() const
{
    const auto lines = dumpHistory().split('\n');
    for (const QString& line : lines) {
        if (!line.isEmpty()) {
            qCDebug(QGCStateMachineLog) << line;
        }
    }
}

void StateHistoryRecorder::addEntry(const QString& stateName, TransitionReason reason,
                                     const QString& details)
{
    if (!_enabled) return;

    HistoryEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.stateName = stateName;
    entry.reason = reason;
    entry.details = details;

    _addEntry(entry);
}

void StateHistoryRecorder::_addEntry(const HistoryEntry& entry)
{
    _history.append(entry);

    // Maintain circular buffer
    while (_history.size() > _maxEntries) {
        _history.removeFirst();
    }
}

void StateHistoryRecorder::_onStateEntered()
{
    if (!_enabled) return;

    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    HistoryEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.stateName = state->objectName();
    entry.reason = Entered;

    _addEntry(entry);
}

void StateHistoryRecorder::_onStateExited()
{
    if (!_enabled) return;

    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    HistoryEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.stateName = state->objectName();
    entry.reason = Exited;

    _addEntry(entry);
}
