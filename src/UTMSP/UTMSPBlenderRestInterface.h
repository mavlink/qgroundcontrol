/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UTMSPRestInterface.h"

class UTMSPBlenderRestInterface: public UTMSPRestInterface
{
public:
    UTMSPBlenderRestInterface();

    std::pair<int, std::string> setFlightPlan(const std::string& body);
    std::pair<int, std::string> requestTelemetry(const std::string& body);
    std::pair<int, std::string> ping();

private:
    http::request<http::string_body> _request;
};
