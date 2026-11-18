#pragma once

#include "QGCAbstractState.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

/// \brief Full-featured base class for QGroundControl state machine states.
///
/// Extends QGCAbstractState with:
/// - Local per-state error handling (setLocalErrorState)
/// - Convenience transition helper (addThisTransition)
///
/// Use QGCAbstractState when you only need callback-driven entry/exit.
/// Use QGCState when you need the extra features listed above.
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

    // -------------------------------------------------------------------------
    // Error Handling
    // -------------------------------------------------------------------------

    /// Set a per-state error handler that overrides the global error state, whether the state is registered with
    /// its machine before or after. The last state set is the one used.
    /// @param errorState The state to transition to on error for this state only, nullptr for the global one again
    void setLocalErrorState(QAbstractState* errorState);

protected:
    /// @return vehicle(). Without one, a warning that names the state is logged to @a category, for a state that
    ///         fails because it has none.
    Vehicle* vehicleOrWarn(const QLoggingCategory& (*category)() = QGCStateMachineLog) const;

private:
    friend class QGCStateMachine;

    /// Called by QGCStateMachine::registerState() with the global error state of the machine
    void _setGlobalErrorState(QAbstractState* errorState);

    /// Points the one transition that error() triggers at the local error state, or else at the global one
    void _updateErrorTransition();

    QAbstractState* _localErrorState = nullptr;
    QAbstractState* _globalErrorState = nullptr;
    QSignalTransition* _errorTransition = nullptr;
};
