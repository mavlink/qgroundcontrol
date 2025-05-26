/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "VehicleComponent.h"

class ESP8266Component : public VehicleComponent
{
    Q_OBJECT

public:
    explicit ESP8266Component(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }
    QString name() const final { return _name; }
    QString description() const final { return tr("The ESP8266 WiFi Bridge Component is used to setup the WiFi link."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/wifi.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput("qrc:/qml/ESP8266Component.qml"); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput("qrc:/qml/ESP8266ComponentSummary.qml"); }

private:
    const QString _name;
    QVariantList _summaryItems;
};
