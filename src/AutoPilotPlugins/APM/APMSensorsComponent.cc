/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMSensorsComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMSensorsComponentController.h"
#include "APMAirframeComponent.h"
#include "ParameterManager.h"

// These two list must be kept in sync

APMSensorsComponent::APMSensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Sensors"))
{

}

QString APMSensorsComponent::name(void) const
{
    return _name;
}

QString APMSensorsComponent::description(void) const
{
    return tr("Sensors Setup is used to calibrate the sensors within your vehicle.");
}

QString APMSensorsComponent::iconResource(void) const
{
    return "/qmlimages/SensorsComponentIcon.png";
}

bool APMSensorsComponent::requiresSetup(void) const
{
    return true;
}

bool APMSensorsComponent::setupComplete(void) const
{
    return !compassSetupNeeded() && !accelSetupNeeded();
}

QStringList APMSensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    // Compass triggers
    triggers << QStringLiteral("COMPASS_DEV_ID") << QStringLiteral("COMPASS_DEV_ID2") << QStringLiteral("COMPASS_DEV_ID3")
             << QStringLiteral("COMPASS_USE") << QStringLiteral("COMPASS_USE2") << QStringLiteral("COMPASS_USE3")
             << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_X")
             << QStringLiteral("COMPASS_OFS2_X") << QStringLiteral("COMPASS_OFS2_X") << QStringLiteral("COMPASS_OFS2_X")
             << QStringLiteral("COMPASS_OFS3_X") << QStringLiteral("COMPASS_OFS3_X") << QStringLiteral("COMPASS_OFS3_X");

    // Accelerometer triggers
    triggers << QStringLiteral("INS_ACCOFFS_X") << QStringLiteral("INS_ACCOFFS_Y") << QStringLiteral("INS_ACCOFFS_Z");

    return triggers;
}

QUrl APMSensorsComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMSensorsComponent.qml");
}

QUrl APMSensorsComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMSensorsComponentSummary.qml");
}

bool APMSensorsComponent::compassSetupNeeded(void) const
{
    const size_t cCompass = 3;
    const size_t cOffset = 3;
    QStringList rgDevicesIds;
    QStringList rgCompassUse;
    QStringList rgOffsets[cCompass];

    rgDevicesIds << QStringLiteral("COMPASS_DEV_ID") << QStringLiteral("COMPASS_DEV_ID2") << QStringLiteral("COMPASS_DEV_ID3");
    rgCompassUse << QStringLiteral("COMPASS_USE") << QStringLiteral("COMPASS_USE2") << QStringLiteral("COMPASS_USE3");
    rgOffsets[0] << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_Y") << QStringLiteral("COMPASS_OFS_Z");
    rgOffsets[1] << QStringLiteral("COMPASS_OFS2_X") << QStringLiteral("COMPASS_OFS2_Y") << QStringLiteral("COMPASS_OFS2_Z");
    rgOffsets[2] << QStringLiteral("COMPASS_OFS3_X") << QStringLiteral("COMPASS_OFS3_Y") << QStringLiteral("COMPASS_OFS3_Z");

    for (size_t i=0; i<cCompass; i++) {
        if (_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, rgDevicesIds[i])->rawValue().toInt() != 0 &&
            _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, rgCompassUse[i])->rawValue().toInt() != 0) {
            for (size_t j=0; j<cOffset; j++) {
                if (_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, rgOffsets[i][j])->rawValue().toFloat() == 0.0f) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool APMSensorsComponent::accelSetupNeeded(void) const
{
    QStringList rgOffsets;

    // The best we can do is test the first accel which will always be there. We don't have enough information to know
    // whether any of the other accels are available.
    rgOffsets << QStringLiteral("INS_ACCOFFS_X") << QStringLiteral("INS_ACCOFFS_Y") << QStringLiteral("INS_ACCOFFS_Z");

    int zeroCount = 0;
    for (int i=0; i<rgOffsets.count(); i++) {
        if (_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, rgOffsets[i])->rawValue().toFloat() == 0.0f) {
            zeroCount++;
        }
    }

    return zeroCount == rgOffsets.count();
}
