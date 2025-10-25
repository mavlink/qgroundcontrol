/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
