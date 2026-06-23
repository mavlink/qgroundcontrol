#include "StateMachineProfiler.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtStateMachine/QAbstractState>

StateMachineProfiler::StateMachineProfiler(QGCStateMachine* machine)
    : QObject(machine)
    , _machine(machine)
{
    connect(_machine, &QStateMachine::started, this, &StateMachineProfiler::_onMachineStarted);
    connect(_machine, &QStateMachine::stopped, this, &StateMachineProfiler::_onMachineStopped);
}

void StateMachineProfiler::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;

    if (_enabled) {
        // Connect to all existing states
        const auto states = _machine->findChildren<QAbstractState*>();
        for (QAbstractState* state : states) {
            auto enterConn = connect(state, &QAbstractState::entered, this, &StateMachineProfiler::_onStateEntered);
            auto exitConn = connect(state, &QAbstractState::exited, this, &StateMachineProfiler::_onStateExited);
            _stateConnections.append(enterConn);
            _stateConnections.append(exitConn);
        }
    } else {
        // Disconnect all state connections
        for (const auto& conn : _stateConnections) {
            disconnect(conn);
        }
        _stateConnections.clear();
    }
}

void StateMachineProfiler::reset()
{
    _profiles.clear();
    _totalRuntimeMs = 0;
    _transitionCount = 0;
    _currentStateName.clear();
}

StateMachineProfiler::StateProfile StateMachineProfiler::profile(const QString& stateName) const
{
    return _profiles.value(stateName);
}

QString StateMachineProfiler::summary() const
{
    QStringList lines;
    lines << QStringLiteral("=== State Machine Profile: %1 ===").arg(_machine->objectName());
    lines << QStringLiteral("Total runtime: %1 ms").arg(_totalRuntimeMs);
    lines << QStringLiteral("Transitions: %1").arg(_transitionCount);
    lines << QString();

    // Sort by total time descending
    QList<StateProfile> sortedProfiles = _profiles.values();
    std::sort(sortedProfiles.begin(), sortedProfiles.end(),
              [](const StateProfile& a, const StateProfile& b) {
                  return a.totalTimeMs > b.totalTimeMs;
              });

    lines << QStringLiteral("%1 %2 %3 %4 %5 %6")
             .arg("State", -30)
             .arg("Count", 8)
             .arg("Total(ms)", 10)
             .arg("Avg(ms)", 10)
             .arg("Min(ms)", 10)
             .arg("Max(ms)", 10);
    lines << QString(80, '-');

    for (const auto& p : sortedProfiles) {
        lines << QStringLiteral("%1 %2 %3 %4 %5 %6")
                 .arg(p.name.left(30), -30)
                 .arg(p.entryCount, 8)
                 .arg(p.totalTimeMs, 10)
                 .arg(p.averageTimeMs(), 10, 'f', 1)
                 .arg(p.minTimeMs == std::numeric_limits<qint64>::max() ? 0 : p.minTimeMs, 10)
                 .arg(p.maxTimeMs, 10);
    }

    return lines.join('\n');
}

QJsonObject StateMachineProfiler::toJson() const
{
    QJsonObject root;
    root["machineName"] = _machine->objectName();
    root["totalRuntimeMs"] = _totalRuntimeMs;
    root["transitionCount"] = _transitionCount;

    QJsonArray statesArray;
    for (auto it = _profiles.constBegin(); it != _profiles.constEnd(); ++it) {
        QJsonObject stateObj;
        stateObj["name"] = it.value().name;
        stateObj["entryCount"] = it.value().entryCount;
        stateObj["totalTimeMs"] = it.value().totalTimeMs;
        stateObj["averageTimeMs"] = it.value().averageTimeMs();
        stateObj["minTimeMs"] = it.value().minTimeMs == std::numeric_limits<qint64>::max()
                                ? 0 : it.value().minTimeMs;
        stateObj["maxTimeMs"] = it.value().maxTimeMs;
        statesArray.append(stateObj);
    }
    root["states"] = statesArray;

    return root;
}

void StateMachineProfiler::logProfile() const
{
    qCDebug(QGCStateMachineLog).noquote() << summary();
}

void StateMachineProfiler::_onMachineStarted()
{
    if (!_enabled) return;

    _machineTimer.start();
    qCDebug(QGCStateMachineLog) << "Profiler: Machine started -" << _machine->objectName();
}

void StateMachineProfiler::_onMachineStopped()
{
    if (!_enabled) return;

    _totalRuntimeMs = _machineTimer.elapsed();
    qCDebug(QGCStateMachineLog) << "Profiler: Machine stopped -" << _machine->objectName()
                                 << "total runtime:" << _totalRuntimeMs << "ms";
}

void StateMachineProfiler::_onStateEntered()
{
    if (!_enabled) return;

    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    _currentStateName = state->objectName();
    _stateTimer.start();

    StateProfile& profile = _profiles[_currentStateName];
    profile.name = _currentStateName;
    profile.entryCount++;
    profile.lastEntryTime = _machineTimer.elapsed();

    _transitionCount++;
}

void StateMachineProfiler::_onStateExited()
{
    if (!_enabled) return;

    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    QString stateName = state->objectName();
    qint64 elapsed = _stateTimer.elapsed();

    StateProfile& profile = _profiles[stateName];
    profile.totalTimeMs += elapsed;
    profile.minTimeMs = qMin(profile.minTimeMs, elapsed);
    profile.maxTimeMs = qMax(profile.maxTimeMs, elapsed);
}
