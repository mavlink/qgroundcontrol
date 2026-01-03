#pragma once

#include "QGCState.h"

#include <QTimer>

/// Delays that state machine for the specified time in milliseconds
class DelayState : public QGCState
{
    Q_OBJECT

public:
    DelayState(QState* parentState, int delayMsecs);

signals:
    void delayComplete();

private:
    QTimer _delayTimer;
};
