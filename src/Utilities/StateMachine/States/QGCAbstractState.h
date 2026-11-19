#pragma once

#include <QtStateMachine/QState>
#include <QtCore/QString>

#include <functional>

class QGCStateMachine;
class Vehicle;

/// \brief Base class of the states of the module: a QState with a name for the log, the machine and the vehicle
/// of the state, an exit callback, and the signals advance() and error().
///
/// QGCState adds a transition helper (addThisTransition()) and a check for the vehicle.
///
class QGCAbstractState : public QState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCAbstractState)

public:
    using ExitCallback = std::function<void()>;

    QGCAbstractState(const QString& stateName, QState* parent);

    /// @return The machine of the state as a QGCStateMachine, nullptr for a state in a plain QStateMachine.
    ///         Takes the place of QAbstractState::machine(), which a state names in full when it wants any machine.
    QGCStateMachine* machine() const;  // NOLINT(bugprone-derived-method-shadowing-base-method)
    Vehicle* vehicle() const;
    QString stateName() const;

    /// Set a callback to be invoked when the state is exited, for what the state has started and is to end with it
    void setOnExit(ExitCallback callback) { _exitCallback = std::move(callback); }

signals:
    void advance();
    void error();

protected:
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;

private:
    ExitCallback _exitCallback;
};
