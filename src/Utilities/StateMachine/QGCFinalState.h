/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

 #pragma once

#include <QFinalState>

/// Final state for a QGCStateMachine
///     Same as QFinalState but with logging
class QGCFinalState : public QFinalState
{
    Q_OBJECT

public:
    QGCFinalState(QState* parent = nullptr);
};