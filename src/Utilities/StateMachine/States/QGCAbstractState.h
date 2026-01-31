#pragma once

#include <QtStateMachine/QAbstractState>
#include <QtCore/QString>

#include <functional>

class QGCStateMachine;
class Vehicle;

/// Lightweight base class for simple states that don't need child states or outgoing transitions.
///
/// ## When to use QGCAbstractState vs QGCState
///
/// Use **QGCAbstractState** when:
/// - The state is a terminal state or marker state
/// - The state only receives transitions, never initiates them
/// - You don't need child states (sub-states)
/// - You want minimal overhead
///
/// Use **QGCState** when:
/// - You need to add outgoing transitions using addTransition()
/// - You need child states (hierarchical state machine)
/// - You need local error state handling (setLocalErrorState())
/// - You need property assignment support
///
/// ## Limitations of QGCAbstractState
/// - Cannot have local error states (only global error state via machine)
/// - Cannot add outgoing transitions directly (transitions must be created externally)
/// - Cannot have child states
class QGCAbstractState : public QAbstractState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCAbstractState)

public:
    using EntryCallback = std::function<void()>;
    using ExitCallback = std::function<void()>;
    using EventHandler = std::function<bool(QEvent*)>;

    QGCAbstractState(const QString& stateName, QState* parent);

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
    void setOnEntry(EntryCallback callback) { _entryCallback = std::move(callback); }

    /// Set a callback to be invoked when the state is exited
    void setOnExit(ExitCallback callback) { _exitCallback = std::move(callback); }

    /// Set both entry and exit callbacks
    void setCallbacks(EntryCallback onEntry, ExitCallback onExit = nullptr);

    // -------------------------------------------------------------------------
    // Event Handling
    // -------------------------------------------------------------------------

    /// Set a custom event handler for this state
    void setEventHandler(EventHandler handler) { _eventHandler = std::move(handler); }

signals:
    void advance();
    void error();

protected:
    /// Override to perform actions on state entry
    virtual void onEnter() {}

    /// Override to perform actions on state exit
    virtual void onLeave() {}

    // QAbstractState overrides
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    EntryCallback _entryCallback;
    ExitCallback _exitCallback;
    EventHandler _eventHandler;
};
