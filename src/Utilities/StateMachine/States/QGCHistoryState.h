#pragma once

#include <QtStateMachine/QHistoryState>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

class QGCStateMachine;
class Vehicle;

/// QGroundControl wrapper around QHistoryState for consistency with other QGC state classes
/// History states remember which child state was active and return to it
class QGCHistoryState : public QHistoryState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCHistoryState)

public:
    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param historyType ShallowHistory (default) remembers immediate child,
    ///                    DeepHistory remembers entire nested configuration
    QGCHistoryState(const QString& stateName, QState* parent, HistoryType historyType = ShallowHistory);

    QGCStateMachine* machine() const;
    Vehicle* vehicle() const;
    QString stateName() const;
};
