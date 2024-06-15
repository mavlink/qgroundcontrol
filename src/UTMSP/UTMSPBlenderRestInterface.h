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

#include <QPair>
#include <QString>

class UTMSPBlenderRestInterface: public UTMSPRestInterface
{
public:
    UTMSPBlenderRestInterface(QObject *parent = nullptr);

    QPair<int, QString> setFlightPlan(const QString& body);
    QPair<int, QString> requestTelemetry(const QString& body);
    QPair<int, QString> ping();

};
