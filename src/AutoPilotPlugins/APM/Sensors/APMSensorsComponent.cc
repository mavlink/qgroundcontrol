#include "APMSensorsComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"

APMSensorsComponent::APMSensorsComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : SensorsComponentBase(vehicle, autopilot, AutoPilotPlugin::KnownSensorsVehicleComponent, parent)
{
}

QStringList APMSensorsComponent::setupCompleteChangedTriggerList() const
{
    static const QStringList triggers = {
        QStringLiteral("COMPASS_DEV_ID"), QStringLiteral("COMPASS_DEV_ID2"), QStringLiteral("COMPASS_DEV_ID3"),
        QStringLiteral("COMPASS_USE"), QStringLiteral("COMPASS_USE2"), QStringLiteral("COMPASS_USE3"),
        QStringLiteral("COMPASS_OFS_X"), QStringLiteral("COMPASS_OFS_Y"), QStringLiteral("COMPASS_OFS_Z"),
        QStringLiteral("COMPASS_OFS2_X"), QStringLiteral("COMPASS_OFS2_Y"), QStringLiteral("COMPASS_OFS2_Z"),
        QStringLiteral("COMPASS_OFS3_X"), QStringLiteral("COMPASS_OFS3_Y"), QStringLiteral("COMPASS_OFS3_Z"),
        QStringLiteral("INS_ACCOFFS_X"), QStringLiteral("INS_ACCOFFS_Y"), QStringLiteral("INS_ACCOFFS_Z")
    };

    return triggers;
}

bool APMSensorsComponent::compassSetupNeeded() const
{
    static const QStringList rgDevicesIds = {
        QStringLiteral("COMPASS_DEV_ID"),
        QStringLiteral("COMPASS_DEV_ID2"),
        QStringLiteral("COMPASS_DEV_ID3")
    };

    static const QStringList rgCompassUse = {
        QStringLiteral("COMPASS_USE"),
        QStringLiteral("COMPASS_USE2"),
        QStringLiteral("COMPASS_USE3")
    };

    static const QList<QStringList> rgOffsets = {
        { QStringLiteral("COMPASS_OFS_X"), QStringLiteral("COMPASS_OFS_Y"), QStringLiteral("COMPASS_OFS_Z") },
        { QStringLiteral("COMPASS_OFS2_X"), QStringLiteral("COMPASS_OFS2_Y"), QStringLiteral("COMPASS_OFS2_Z") },
        { QStringLiteral("COMPASS_OFS3_X"), QStringLiteral("COMPASS_OFS3_Y"), QStringLiteral("COMPASS_OFS3_Z") }
    };

    for (qsizetype i = 0; i < rgDevicesIds.length(); i++) {
        if (parameterValue(rgDevicesIds[i]).toInt() == 0) {
            continue;
        }
        if (parameterValue(rgCompassUse[i]).toInt() == 0) {
            continue;
        }

        const QStringList &offsets = rgOffsets[i];
        for (const QString &offset : offsets) {
            if (parameterValue(offset).toFloat() == 0.0f) {
                return true;
            }
        }
    }

    return false;
}

bool APMSensorsComponent::accelSetupNeeded() const
{
    static const QStringList rgOffsets = {
        QStringLiteral("INS_ACCOFFS_X"),
        QStringLiteral("INS_ACCOFFS_Y"),
        QStringLiteral("INS_ACCOFFS_Z")
    };

    int zeroCount = 0;
    for (const QString &offset: rgOffsets) {
        if (parameterValue(offset).toFloat() == 0.0f) {
            zeroCount++;
        }
    }

    return (zeroCount == rgOffsets.count());
}
