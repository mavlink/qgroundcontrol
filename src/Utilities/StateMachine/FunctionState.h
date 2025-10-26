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

#include <functional>

class FunctionState : public QGCState
{
    Q_OBJECT

public:
    using Function = std::function<void(FunctionState *state)>;

    FunctionState(const QString& stateName, Function function, QState* parentState);

private:
    Function _function;
};
