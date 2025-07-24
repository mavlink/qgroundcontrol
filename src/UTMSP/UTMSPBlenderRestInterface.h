/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UTMSPRestInterface.h"

#include <QPair>
#include <QString>

class UTMSPBlenderRestInterface: public UTMSPRestInterface
{
public:
    UTMSPBlenderRestInterface(QObject *parent = nullptr);

    QPair<int, std::string> setFlightPlan(const std::string& body);
    QPair<int, std::string> requestTelemetry(const std::string& body);
    QPair<int, std::string> updateFlightState(const std::string& body, const std::string& flightID);
    QPair<int, std::string> ping();

};
