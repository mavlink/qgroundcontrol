#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

/// Delays that state machine for the specified time in milliseconds
class DelayState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(DelayState)

public:
    DelayState(QState* parentState, int delayMsecs);

signals:
    void delayComplete();

private:
    QTimer _delayTimer;
};
