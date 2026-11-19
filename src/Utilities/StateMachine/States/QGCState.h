#pragma once

#include <functional>
#include <utility>

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtStateMachine/QState>

class QGCStateMachine;
class RuntimeScheduler;
class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

/// \brief Base class of the states of the module: a QState with a name for the log, the machine and the vehicle
/// of the state, an exit callback, and the signals advance() and error().
///
class QGCState : public QState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCState)

public:
    using ExitCallback = std::function<void()>;

    QGCState(const QString& stateName, QState* parentState);

    /// @return The machine of the state as a QGCStateMachine, nullptr for a state in a plain QStateMachine.
    ///         Takes the place of QAbstractState::machine(), which a state names in full when it wants any machine.
    QGCStateMachine* machine() const;  // NOLINT(bugprone-derived-method-shadowing-base-method)
    Vehicle* vehicle() const;
    QString stateName() const;

    /// Set a callback to be invoked when the state is exited, for what the state has started and is to end with it
    void setOnExit(ExitCallback callback) { _exitCallback = std::move(callback); }

    /// Simpler version of QState::addTransition which assumes the sender is this
    template <typename PointerToMemberFunction>
    QSignalTransition* addThisTransition(PointerToMemberFunction signal, QAbstractState* target)
    {
        return QState::addTransition(this, signal, target);
    }

signals:
    void advance();
    void error();

protected:
    /// @return vehicle(). Without one, a warning that names the state is logged to @a category, for a state that
    ///         fails because it has none.
    Vehicle* vehicleOrWarn(const QLoggingCategory& (*category)() = QGCStateMachineLog) const;

    /// @return The scheduler for the waits and delays of the state: that of its QGCStateMachine, one of the state's
    ///         own in a plain QStateMachine
    RuntimeScheduler* scheduler();

    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;

private:
    ExitCallback _exitCallback;
    RuntimeScheduler* _ownScheduler = nullptr;
};
