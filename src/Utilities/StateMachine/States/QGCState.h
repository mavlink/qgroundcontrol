#pragma once

#include <QtStateMachine/QState>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

#include <functional>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

class QGCStateMachine;
class Vehicle;

/// Base class for all QGroundControl state machine states
class QGCState : public QState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCState)

public:
    using EntryCallback = std::function<void()>;
    using ExitCallback = std::function<void()>;
    using EventHandler = std::function<bool(QEvent*)>;

    QGCState(const QString& stateName, QState* parentState);

    /// Simpler version of QState::addTransition which assumes the sender is this
    template <typename PointerToMemberFunction> QSignalTransition *addThisTransition(PointerToMemberFunction signal, QAbstractState *target)
        { return QState::addTransition(this, signal, target); };

    QGCStateMachine* machine() const;
    Vehicle* vehicle() const;
    QString stateName() const;

    /// Get the state machine's context for inter-state data passing
    /// @return Pointer to the context, or nullptr if not in a QGCStateMachine
    class StateContext* context() const;

    // -------------------------------------------------------------------------
    // Entry/Exit Callbacks
    // -------------------------------------------------------------------------

    /// Set a callback to be invoked when the state is entered
    /// @param callback Function to call on entry
    void setOnEntry(EntryCallback callback) { _entryCallback = std::move(callback); }

    /// Set a callback to be invoked when the state is exited
    /// @param callback Function to call on exit
    void setOnExit(ExitCallback callback) { _exitCallback = std::move(callback); }

    /// Set both entry and exit callbacks
    void setCallbacks(EntryCallback onEntry, ExitCallback onExit = nullptr);

    // -------------------------------------------------------------------------
    // Event Handling
    // -------------------------------------------------------------------------

    /// Set a custom event handler for this state
    /// @param handler Function that receives events, returns true if handled
    void setEventHandler(EventHandler handler) { _eventHandler = std::move(handler); }

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

signals:
    void advance();     ///< Signal to indicate state is complete and machine should advance to next state
    void error();       ///< Signal to indicate an error has occurred

protected:
    /// Override to perform actions on state entry
    /// Called after entry callback but before any transitions are evaluated
    virtual void onEnter() {}

    /// Override to perform actions on state exit
    /// Called before exit callback
    virtual void onLeave() {}

    // QState overrides
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    QAbstractState* _localErrorState = nullptr;
    EntryCallback _entryCallback;
    ExitCallback _exitCallback;
    EventHandler _eventHandler;
};
