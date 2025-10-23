/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FunctionState.h"

#include <QTimer>

/// Executes a function when the state is entered
FunctionState::FunctionState(const QString& stateName, QState* parentState, std::function<void()> function)
    : QGCState   (stateName, parentState)
    , _function     (function)
{
    connect(this, &QState::entered, this, [this] () {
        _function();
        emit advance();
    });
}
