#pragma once

#include "QGCAbstractState.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

class QGCStateMachine;
class Vehicle;

/// Full-featured base class for QGroundControl state machine states.
///
/// Extends QGCAbstractState with:
/// - Local per-state error handling (setLocalErrorState)
/// - Property assignment support (setProperty, setEnabled, setVisible)
/// - Convenience transition helper (addThisTransition)
///
/// Use QGCAbstractState when you only need callback-driven entry/exit.
/// Use QGCState when you need the extra features listed above.
class QGCState : public QGCAbstractState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCState)

public:
    QGCState(const QString& stateName, QState* parentState);

    /// Simpler version of QState::addTransition which assumes the sender is this
    template <typename PointerToMemberFunction> QSignalTransition *addThisTransition(PointerToMemberFunction signal, QAbstractState *target)
        { return QState::addTransition(this, signal, target); };

    // -------------------------------------------------------------------------
    // Error Handling
    // -------------------------------------------------------------------------

    /// Set a per-state error handler that overrides the global error state
    /// @param errorState The state to transition to on error for this state only
    void setLocalErrorState(QAbstractState* errorState);

    /// Get the per-state error state (nullptr if using global)
    QAbstractState* localErrorState() const { return _localErrorState; }

    // -------------------------------------------------------------------------
    // Property Assignment
    // -------------------------------------------------------------------------

    /// Assign a property value when this state is entered
    /// If RestoreProperties policy is set on the machine, the value is restored on exit
    /// @param object The QObject to modify
    /// @param name The property name
    /// @param value The value to assign
    void setProperty(QObject* object, const char* name, const QVariant& value);

    /// Convenience overload for setting enabled state on widgets/controls
    void setEnabled(QObject* object, bool enabled);

    /// Convenience overload for setting visible state on widgets/controls
    void setVisible(QObject* object, bool visible);

private:
    QAbstractState* _localErrorState = nullptr;
};
