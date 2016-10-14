/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "APMSafetyComponent.h"
#include "QGCQmlWidgetHolder.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMSafetyComponent::APMSafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Safety"))
{
}

QString APMSafetyComponent::name(void) const
{
    return _name;
}

QString APMSafetyComponent::description(void) const
{
    return tr("The Safety Component is used to setup triggers for Return to Land as well as the settings for Return to Land itself.");
}

QString APMSafetyComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/SafetyComponentIcon.png");
}

bool APMSafetyComponent::requiresSetup(void) const
{
    return false;
}

bool APMSafetyComponent::setupComplete(void) const
{
    // FIXME: What aboout invalid settings?
    return true;
}

QStringList APMSafetyComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMSafetyComponent::setupSource(void) const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
            qmlFile = QStringLiteral("qrc:/qml/APMSafetyComponentPlane.qml");
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = QStringLiteral("qrc:/qml/APMSafetyComponentCopter.qml");
            break;
        case MAV_TYPE_GROUND_ROVER:
            qmlFile = QStringLiteral("qrc:/qml/APMSafetyComponentRover.qml");
            break;
        default:
            qmlFile = QStringLiteral("qrc:/qml/APMNotSupported.qml");
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl APMSafetyComponent::summaryQmlSource(void) const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
            qmlFile = QStringLiteral("qrc:/qml/APMSafetyComponentSummaryPlane.qml");
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = QStringLiteral("qrc:/qml/APMSafetyComponentSummaryCopter.qml");
            break;
        case MAV_TYPE_GROUND_ROVER:
            qmlFile = QStringLiteral("qrc:/qml/APMSafetyComponentSummaryRover.qml");
            break;
        default:
            qmlFile = QStringLiteral("qrc:/qml/APMNotSupported.qml");
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QString APMSafetyComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }

    return QString();
}
