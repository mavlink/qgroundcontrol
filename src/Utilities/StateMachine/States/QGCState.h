#pragma once

#include "QGCAbstractState.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

/// \brief QGCAbstractState with a transition helper (addThisTransition()) and a check for the vehicle
/// (vehicleOrWarn()). The other states of the module derive from it.
///
class QGCState : public QGCAbstractState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCState)

public:
    QGCState(const QString& stateName, QState* parentState);

    /// Simpler version of QState::addTransition which assumes the sender is this
    template <typename PointerToMemberFunction>
    QSignalTransition* addThisTransition(PointerToMemberFunction signal, QAbstractState* target)
    {
        return QState::addTransition(this, signal, target);
    }

protected:
    /// @return vehicle(). Without one, a warning that names the state is logged to @a category, for a state that
    ///         fails because it has none.
    Vehicle* vehicleOrWarn(const QLoggingCategory& (*category)() = QGCStateMachineLog) const;
};
