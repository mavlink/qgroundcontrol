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

QPair<int, QString> UTMSPBlenderRestInterface::setFlightPlan(const QString& body)
{
    // Post Flight plan
    QString setFlightPlanTarget = "/flight_declaration_ops/set_flight_declaration";
    modifyRequest(setFlightPlanTarget, QNetworkAccessManager::PostOperation, body);

    return executeRequest();
}

QPair<int, QString> UTMSPBlenderRestInterface::requestTelemetry(const QString& body)
{
    // Post RID data
    QString target = "/flight_stream/set_telemetry";
    modifyRequest(target, QNetworkAccessManager::PutOperation, body);

    return executeRequest();
}

QPair<int, QString> UTMSPBlenderRestInterface::ping()
{
    QString target = "/ping";
    modifyRequest(target, QNetworkAccessManager::GetOperation);

    return executeRequest();
}
