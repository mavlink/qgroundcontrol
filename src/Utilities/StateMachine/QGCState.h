/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QState>
#include <QString>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

class QGCStateMachine;
class Vehicle;

/// Base class for all QGroundControl state machine states
class QGCState : public QState
{
    Q_OBJECT

public:
    QGCState(const QString& stateName, QState* parentState);

    /// Simpler version of QState::addTransition which assumes the sender is this
    template <typename PointerToMemberFunction> QSignalTransition *addThisTransition(PointerToMemberFunction signal, QAbstractState *target)
        { return QState::addTransition(this, signal, target); };

    QGCStateMachine* machine() const;
    Vehicle *vehicle();
    QString stateName() const;

signals:
    void advance();     ///< Signal to indicate state is complete and machine should advance to next state
    void error();       ///< Signal to indicate an error has occurred
};
