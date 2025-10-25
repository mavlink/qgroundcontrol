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
    FunctionState(const QString& stateName, QState* parentState, std::function<void()>);

private:
    std::function<void()> _function;
};
