/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMSensorsComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMSensorsComponentController.h"
#include "APMAirframeComponent.h"

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
    return tr("The Sensors Component allows you to calibrate the sensors within your vehicle. "
              "Prior to flight you must calibrate the Magnetometer, Gyroscope and Accelerometer.");
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
    triggers << "COMPASS_DEV_ID" << "COMPASS_DEV_ID2" << "COMPASS_DEV_ID3"
             << "COMPASS_USE" << "COMPASS_USE2" << "COMPASS_USE3"
             << "COMPASS_OFS_X" << "COMPASS_OFS_X" << "COMPASS_OFS_X"
             << "COMPASS_OFS2_X" << "COMPASS_OFS2_X" << "COMPASS_OFS2_X"
             << "COMPASS_OFS3_X" << "COMPASS_OFS3_X" << "COMPASS_OFS3_X";

    // Accelerometer triggers
    triggers << "INS_ACCOFFS_X" << "INS_ACCOFFS_Y" << "INS_ACCOFFS_Z";

    if (_autopilot->parameterExists(FactSystem::defaultComponentId, QStringLiteral("INS_USE"))) {
        triggers << QStringLiteral("INS_USE") << QStringLiteral("INS_USE2") << QStringLiteral("INS_USE3");
        triggers << QStringLiteral("INS_ACC2OFFS_X") << QStringLiteral("INS_ACC2OFFS_Y") << QStringLiteral("INS_ACC2OFFS_Z");
        triggers << QStringLiteral("INS_ACC3OFFS_X") << QStringLiteral("INS_ACC3OFFS_Y") << QStringLiteral("INS_ACC3OFFS_Z");
    }

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

QString APMSensorsComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    
    return QString();
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
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, rgDevicesIds[i])->rawValue().toInt() != 0 &&
            _autopilot->getParameterFact(FactSystem::defaultComponentId, rgCompassUse[i])->rawValue().toInt() != 0) {
            for (size_t j=0; j<cOffset; j++) {
                if (_autopilot->getParameterFact(FactSystem::defaultComponentId, rgOffsets[i][j])->rawValue().toFloat() == 0.0f) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool APMSensorsComponent::accelSetupNeeded(void) const
{
    QStringList         rgUse;
    QStringList         rgOffsets;
    QList<QStringList>  rgAccels;

    // We always at a minimum test the first accel
    rgOffsets << QStringLiteral("INS_ACCOFFS_X") << QStringLiteral("INS_ACCOFFS_Y") << QStringLiteral("INS_ACCOFFS_Z");
    rgAccels << rgOffsets;
    rgOffsets.clear();

    // This parameter is not available in all firmware version. Specifically missing from older Solo firmware.
    if (_autopilot->parameterExists(FactSystem::defaultComponentId, QStringLiteral("INS_USE"))) {
        rgUse << QStringLiteral("INS_USE") << QStringLiteral("INS_USE2") << QStringLiteral("INS_USE3");

        // We have usage information for the remaining accels, so we can test them sa well
        rgOffsets << QStringLiteral("INS_ACC2OFFS_X") << QStringLiteral("INS_ACC2OFFS_Y") << QStringLiteral("INS_ACC2OFFS_Z");
        rgAccels << rgOffsets;
        rgOffsets.clear();

        rgOffsets << QStringLiteral("INS_ACC3OFFS_X") << QStringLiteral("INS_ACC3OFFS_Y") << QStringLiteral("INS_ACC3OFFS_Z");
        rgAccels << rgOffsets;
        rgOffsets.clear();
    }

    for (int i=0; i<rgAccels.count(); i++) {
        if (rgUse.count() == 0 || _autopilot->getParameterFact(FactSystem::defaultComponentId, rgUse[i])->rawValue().toInt() != 0) {
            for (int j=0; j<rgAccels[0].count(); j++) {
                if (_autopilot->getParameterFact(FactSystem::defaultComponentId, rgAccels[i][j])->rawValue().toFloat() == 0.0f) {
                    return true;
                }
            }
        }
    }

    return false;
}

