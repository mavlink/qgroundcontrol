/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPBlenderRestInterface.h"

UTMSPBlenderRestInterface::UTMSPBlenderRestInterface():
    UTMSPRestInterface("blender.utm.dev.airoplatform.com")
{

}

std::pair<int, std::string> UTMSPBlenderRestInterface::setFlightPlan(const std::string& body)
{
    // Post Flight plan
    const std::string setFlightPlanTarget = "/flight_declaration_ops/set_flight_declaration";
    modifyRequest(setFlightPlanTarget, http::verb::post, body);

    return executeRequest();
}

std::pair<int, std::string> UTMSPBlenderRestInterface::requestTelemetry(const std::string& body)
{
    // Post RID data
    const std::string target = "/flight_stream/set_telemetry";
    modifyRequest(target, http::verb::put, body);

    return executeRequest();
}

std::pair<int, std::string> UTMSPBlenderRestInterface::ping()
{
    const std::string target = "/ping";
    modifyRequest(target, http::verb::get);

    return executeRequest();
}
