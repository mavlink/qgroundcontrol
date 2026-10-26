#include "SensorsComponent.h"

#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

SensorsComponent::SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownSensorsVehicleComponent, parent)
    , _name(tr("Sensors"))
{
    _deviceIds = QStringList({QStringLiteral("CAL_GYRO0_ID"), QStringLiteral("CAL_ACC0_ID") });
}

QString SensorsComponent::name(void) const
{
    return _name;
}

QString SensorsComponent::description(void) const
{
    return tr(
        "Calibrate the compass, gyroscope, accelerometer, barometer and airspeed sensors, and set how they are "
        "mounted.");
}

QStringList SensorsComponent::searchKeywords() const
{
    return {
        tr("Compass"),     tr("Magnetometer"), tr("Gyroscope"),      tr("Accelerometer"),
        tr("Barometer"),   tr("Airspeed"),     tr("Level horizon"),  tr("Orientation"),
        tr("Calibration"), tr("Priority"),     tr("Board rotation"),
    };
}

QString SensorsComponent::iconResource(void) const
{
    return "/qmlimages/SensorsComponentIcon.png";
}

bool SensorsComponent::requiresSetup(void) const
{
    return true;
}

bool SensorsComponent::setupComplete(void) const
{
    for (const QString &triggerParam : std::as_const(_deviceIds)) {
        if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, triggerParam)->rawValue().toFloat() == 0.0f) {
            return false;
        }
    }
    bool magEnabled = true;
    if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, _magEnabledParam)) {
        magEnabled = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, _magEnabledParam)->rawValue().toBool();
    }

    if (magEnabled && _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, _magCalParam)->rawValue().toFloat() == 0.0f) {
        return false;
    }

    if (_airspeedCalRequired()) {
        return false;
    }

    return true;
}

QStringList SensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;

    triggers << _deviceIds << _magCalParam << _magEnabledParam;
    if (_airspeedVehicle()) {
        triggers << _airspeedCalParam;
        if (_airspeedLegacyFirmware()) {
            triggers << _airspeedModeParam << _airspeedCheckBreakerParam;
        } else {
            triggers << _airspeedPresentParam;
        }
    }

    return triggers;
}

QUrl SensorsComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/SensorsComponent.qml");
}

QStringList SensorsComponent::summaryChangedTriggerList(void) const
{
    return {QStringLiteral("CAL_MAG1_ID"), QStringLiteral("CAL_MAG2_ID")};
}

QList<VehicleSummaryItem> SensorsComponent::summaryItems(void) const
{
    QList<VehicleSummaryItem> items;
    const auto addCalibration = [this, &items](const QString& param, const QString& caption, bool optional) {
        const Fact* const fact = _parameterFact(param);
        if (!fact || (optional && fact->rawValue().toInt() == 0)) {
            return;
        }
        if (fact->rawValue().toInt() == 0) {
            items.append({caption, tr("Setup required"), VehicleSummaryItem::Attention});
        } else {
            items.append({caption, tr("Ready"), VehicleSummaryItem::Normal});
        }
    };

    addCalibration(QStringLiteral("CAL_MAG0_ID"), tr("Compass 0"), false);
    addCalibration(QStringLiteral("CAL_MAG1_ID"), tr("Compass 1"), true);
    addCalibration(QStringLiteral("CAL_MAG2_ID"), tr("Compass 2"), true);
    addCalibration(QStringLiteral("CAL_GYRO0_ID"), tr("Gyro"), false);
    addCalibration(QStringLiteral("CAL_ACC0_ID"), tr("Accelerometer"), false);

    if (_airspeedCalSupported()) {
        if (_airspeedCalRequired()) {
            items.append({tr("Airspeed"), tr("Setup required"), VehicleSummaryItem::Attention});
        } else {
            items.append({tr("Airspeed"), tr("Ready"), VehicleSummaryItem::Normal});
        }
    }

    return items;
}

bool SensorsComponent::_airspeedVehicle(void) const
{
    return _vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship();
}

bool SensorsComponent::_airspeedParamExists(const char* name) const
{
    return _vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, name);
}

Fact* SensorsComponent::_airspeedParam(const char* name) const
{
    return _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, name);
}

bool SensorsComponent::_airspeedLegacyFirmware(void) const
{
    return !(_vehicle->firmwareMajorVersion() > 1 ||
             (_vehicle->firmwareMajorVersion() == 1 && _vehicle->firmwareMinorVersion() > 14));
}

bool SensorsComponent::_airspeedCalSupported(void) const
{
    if (!_airspeedVehicle()) {
        return false;
    }

    if (!_airspeedLegacyFirmware()) {
        return _airspeedParamExists(_airspeedPresentParam) &&
               _airspeedParam(_airspeedPresentParam)->rawValue().toBool();
    }

    return _airspeedParamExists(_airspeedModeParam) && _airspeedParamExists(_airspeedCheckBreakerParam) &&
           !_airspeedParam(_airspeedModeParam)->rawValue().toBool() &&
           _airspeedParam(_airspeedCheckBreakerParam)->rawValue().toInt() != _airspeedCheckBreakerDisabled;
}

bool SensorsComponent::_airspeedCalRequired(void) const
{
    return _airspeedCalSupported() && _airspeedParamExists(_airspeedCalParam) &&
           _airspeedParam(_airspeedCalParam)->rawValue().toFloat() == 0.0f;
}
