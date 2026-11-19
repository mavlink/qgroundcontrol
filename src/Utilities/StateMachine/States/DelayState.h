#pragma once

#include <optional>

#include "QGCState.h"
#include "ScheduledTask.h"

/// \brief Delays that state machine for the specified time in milliseconds
///
/// The delay starts when the state is entered and ends with advance(). It ends without it when the state is exited
/// before that, and when its machine stops. It runs on the scheduler of the state's machine.
///
class DelayState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(DelayState)

public:
    DelayState(QState* parentState, int delayMsecs);

private slots:
    void _startDelay();
    void _stopDelay();

private:
    int _delayMsecs = 0;
    /// Set for the scheduler of the machine that has entered the state, which is not known before
    std::optional<ScheduledTask> _delayTask;
};
