/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MockConfiguration.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MockConfigurationLog, "Comms.MockLink.MockConfiguration")

MockConfiguration::MockConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(MockConfigurationLog) << this;
}

MockConfiguration::MockConfiguration(const MockConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _firmwareType(copy->firmwareType())
    , _vehicleType(copy->vehicleType())
    , _sendStatusText(copy->sendStatusText())
    , _incrementVehicleId(copy->incrementVehicleId())
    , _failureMode(copy->failureMode())
{
    qCDebug(MockConfigurationLog) << this;
}

MockConfiguration::~MockConfiguration()
{
    qCDebug(MockConfigurationLog) << this;
}

void MockConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const MockConfiguration *mockLinkSource = qobject_cast<const MockConfiguration*>(source);

    setFirmwareType(mockLinkSource->firmwareType());
    setVehicleType(mockLinkSource->vehicleType());
    setSendStatusText(mockLinkSource->sendStatusText());
    setIncrementVehicleId(mockLinkSource->incrementVehicleId());
    setFailureMode(mockLinkSource->failureMode());
}

void MockConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setFirmwareType(static_cast<MAV_AUTOPILOT>(settings.value(_firmwareTypeKey, static_cast<int>(MAV_AUTOPILOT_PX4)).toInt()));
    setVehicleType(static_cast<MAV_TYPE>(settings.value(_vehicleTypeKey, static_cast<int>(MAV_TYPE_QUADROTOR)).toInt()));
    setSendStatusText(settings.value(_sendStatusTextKey, false).toBool());
    setIncrementVehicleId(settings.value(_incrementVehicleIdKey, true).toBool());
    setFailureMode(static_cast<FailureMode_t>(settings.value(_failureModeKey, static_cast<int>(FailNone)).toInt()));

    settings.endGroup();
}

void MockConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue(_firmwareTypeKey, firmwareType());
    settings.setValue(_vehicleTypeKey, vehicleType());
    settings.setValue(_sendStatusTextKey, sendStatusText());
    settings.setValue(_incrementVehicleIdKey, incrementVehicleId());
    settings.setValue(_failureModeKey, failureMode());

    settings.endGroup();
}
