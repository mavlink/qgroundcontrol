/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QState>
#include <QString>
#include <QLoggingCategory>

class QGCParallelState : public QGCState
{
    Q_OBJECT

public:
    QGCParallelState(const QString& stateName, QState* parentState);
};
