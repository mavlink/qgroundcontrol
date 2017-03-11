/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MixersComponent.h"
#include "ParameterManager.h"

MixersComponent::MixersComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Mixers"))
{
}

QString MixersComponent::name(void) const
{
    return _name;
}

QString MixersComponent::description(void) const
{
    return tr("Mixers tuning is used to blah, blah, blah... [WIP]");
}

QString MixersComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/TuningComponentIcon.png");
}

bool MixersComponent::requiresSetup(void) const
{
    return false;
}

bool MixersComponent::setupComplete(void) const
{
    return true;
}

QStringList MixersComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl MixersComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/MixersComponent.qml"));
}

QUrl MixersComponent::summaryQmlSource(void) const
{
    return QUrl();
}
