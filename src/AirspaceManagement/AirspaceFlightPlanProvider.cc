/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceFlightPlanProvider.h"

AirspaceFlightInfo::AirspaceFlightInfo(QObject *parent)
    : QObject(parent)
    , _selected(false)
{
}

AirspaceFlightPlanProvider::AirspaceFlightPlanProvider(QObject *parent)
    : QObject(parent)
{
}
