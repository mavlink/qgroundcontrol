#pragma once

#include <QtStateMachine/QAbstractTransition>
#include <QtCore/QString>

class QGCStateMachine;
class Vehicle;

/// Base class for custom transitions that need access to QGCStateMachine and Vehicle
class QGCAbstractTransition : public QAbstractTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCAbstractTransition)

public:
    explicit QGCAbstractTransition(QState* sourceState = nullptr);
    QGCAbstractTransition(QAbstractState* target, QState* sourceState = nullptr);

    /// Get the QGCStateMachine this transition belongs to
    QGCStateMachine* machine() const;

    /// Get the Vehicle associated with the state machine
    Vehicle* vehicle() const;

protected:
    // Subclasses must implement these
    bool eventTest(QEvent* event) override = 0;
    void onTransition(QEvent* event) override;
};
