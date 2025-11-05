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

#include <QString>

/// Display an application message to the user

class ShowAppMessageState : public QGCState
{
    Q_OBJECT

public:
    ShowAppMessageState(QState* parentState, const QString& appMessage);

private:
    QString _appMessage;
};
