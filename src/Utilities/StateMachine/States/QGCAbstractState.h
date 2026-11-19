#pragma once

#include <QtStateMachine/QState>
#include <QtCore/QString>

#include <functional>

class QGCStateMachine;
class Vehicle;

/// \brief Base class of the states of the module: a QState with a name for the log, the machine and the vehicle
/// of the state, entry and exit callbacks, and the signals advance() and error().
///
/// QGCState adds a local error state (setLocalErrorState()) and a check for the vehicle.
///
class QGCAbstractState : public QState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCAbstractState)

public:
    using EntryCallback = std::function<void()>;
    using ExitCallback = std::function<void()>;

    QGCAbstractState(const QString& stateName, QState* parent);

    /// @return The machine of the state as a QGCStateMachine, nullptr for a state in a plain QStateMachine.
    ///         Takes the place of QAbstractState::machine(), which a state names in full when it wants any machine.
    QGCStateMachine* machine() const;  // NOLINT(bugprone-derived-method-shadowing-base-method)
    Vehicle* vehicle() const;
    QString stateName() const;

    // -------------------------------------------------------------------------
    // Entry/Exit Callbacks
    // -------------------------------------------------------------------------

    /// Set a callback to be invoked when the state is entered
    void setOnEntry(EntryCallback callback) { _entryCallback = std::move(callback); }

    /// Set a callback to be invoked when the state is exited
    void setOnExit(ExitCallback callback) { _exitCallback = std::move(callback); }

signals:
    void advance();
    void error();

protected:
    /// Override to perform actions on state entry
    virtual void onEnter() {}

    /// Override to perform actions on state exit
    virtual void onLeave() {}

    // QState overrides
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;

private:
    EntryCallback _entryCallback;
    ExitCallback _exitCallback;
};
