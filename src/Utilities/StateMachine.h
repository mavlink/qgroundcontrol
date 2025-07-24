/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(StateMachineLog)

class StateMachine : public QObject
{
    Q_OBJECT

public:
    StateMachine(QObject *parent = nullptr);
    ~StateMachine();

    typedef void (*StateFn)(StateMachine *stateMachine);

    /// Start the state machine with the first step
    void start();

    /// Advance the state machine to the next state and call the state function
    virtual void advance();

    /// Move the state machine to the specified state and call the state function
    void move(StateFn stateFn);

    StateFn currentState() const;

    /// @return The number of states in the rgStates array
    virtual int stateCount() const = 0;

    /// @return Array of states to execute
    virtual const StateFn *rgStates() const = 0;

    /// Called when all states have completed
    virtual void statesCompleted() const {};

    bool active() const { return _active; }

protected:
    bool _active = false;
    int _stateIndex = -1;
};
