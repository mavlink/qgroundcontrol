/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPBlenderRestInterface.h"

UTMSPBlenderRestInterface::UTMSPBlenderRestInterface(QObject *parent):
    UTMSPRestInterface(parent)
{
    setHost("BlenderClient");
}

QPair<int, std::string> UTMSPBlenderRestInterface::setFlightPlan(const std::string& body)
{
    // Post Flight plan
    QString setFlightPlanTarget = "/flight_declaration_ops/set_flight_declaration";
    modifyRequest(setFlightPlanTarget, QNetworkAccessManager::PostOperation, QString::fromStdString(body));

    return executeRequest();
}

QPair<int, std::string> UTMSPBlenderRestInterface::requestTelemetry(const std::string& body)
{
    // Post RID data
    QString target = "/flight_stream/set_telemetry";
    modifyRequest(target, QNetworkAccessManager::PutOperation, QString::fromStdString(body));

    return executeRequest();
}

QPair<int, std::string> UTMSPBlenderRestInterface::updateFlightState(const std::string& body, const std::string &flightID)
{
    // Post RID data
    QString target = "/flight_declaration_ops/flight_declaration_state/" + QString::fromStdString(flightID);
    modifyRequest(target, QNetworkAccessManager::PutOperation, QString::fromStdString(body));

    return executeRequest();
}

QPair<int, std::string> UTMSPBlenderRestInterface::ping()
{
    QString target = "/ping";
    modifyRequest(target, QNetworkAccessManager::GetOperation);

    return executeRequest();
}
