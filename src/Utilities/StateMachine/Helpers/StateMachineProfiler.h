#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>

class QAbstractState;
class QGCStateMachine;

/// Performance profiler for state machines.
///
/// Tracks time spent in each state and provides profiling data.
///
/// Usage:
/// @code
/// QGCStateMachine machine("MyMachine", nullptr);
/// StateMachineProfiler profiler(&machine);
/// profiler.setEnabled(true);
///
/// // ... run state machine ...
///
/// qDebug() << profiler.summary();
/// qDebug() << profiler.toJson();
/// @endcode
class StateMachineProfiler : public QObject
{
    Q_OBJECT

public:
    struct StateProfile {
        QString name;
        int entryCount = 0;
        qint64 totalTimeMs = 0;
        qint64 minTimeMs = std::numeric_limits<qint64>::max();
        qint64 maxTimeMs = 0;
        qint64 lastEntryTime = 0;

        double averageTimeMs() const {
            return entryCount > 0 ? static_cast<double>(totalTimeMs) / entryCount : 0.0;
        }
    };

    explicit StateMachineProfiler(QGCStateMachine* machine);
    ~StateMachineProfiler() override = default;

    /// Enable or disable profiling
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    /// Reset all profiling data
    void reset();

    /// Get profile data for a specific state
    StateProfile profile(const QString& stateName) const;

    /// Get profile data for all states
    QHash<QString, StateProfile> allProfiles() const { return _profiles; }

    /// Get total machine runtime
    qint64 totalRuntimeMs() const { return _totalRuntimeMs; }

    /// Get the number of state transitions
    int transitionCount() const { return _transitionCount; }

    /// Get a human-readable summary
    QString summary() const;

    /// Export profile data as JSON
    QJsonObject toJson() const;

    /// Log the profile to debug output
    void logProfile() const;

private slots:
    void _onMachineStarted();
    void _onMachineStopped();
    void _onStateEntered();
    void _onStateExited();

private:
    QGCStateMachine* _machine = nullptr;
    bool _enabled = false;

    QHash<QString, StateProfile> _profiles;
    QElapsedTimer _machineTimer;
    QElapsedTimer _stateTimer;
    qint64 _totalRuntimeMs = 0;
    int _transitionCount = 0;

    QString _currentStateName;
    QList<QMetaObject::Connection> _stateConnections;
};
