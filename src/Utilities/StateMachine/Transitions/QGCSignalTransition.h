#pragma once

#include <QtStateMachine/QSignalTransition>
#include <QtCore/QString>

class QGCStateMachine;
class Vehicle;

/// Base class for signal-based transitions that need access to QGCStateMachine and Vehicle
class QGCSignalTransition : public QSignalTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCSignalTransition)

public:
    QGCSignalTransition(QState* sourceState = nullptr);
    QGCSignalTransition(const QObject* sender, const char* signal, QState* sourceState = nullptr);

    template<typename Func>
    QGCSignalTransition(const typename QtPrivate::FunctionPointer<Func>::Object* sender,
                        Func signal, QState* sourceState = nullptr)
        : QSignalTransition(sender, signal, sourceState)
    {
    }

    /// Get the QGCStateMachine this transition belongs to
    QGCStateMachine* machine() const;

    /// Get the Vehicle associated with the state machine
    Vehicle* vehicle() const;
};
