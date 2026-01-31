#include "SensorsComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"

SensorsComponent::SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : SensorsComponentBase(vehicle, autopilot, AutoPilotPlugin::KnownSensorsVehicleComponent, parent)
{
    _deviceIds = QStringList({QStringLiteral("CAL_GYRO0_ID"), QStringLiteral("CAL_ACC0_ID") });

    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        _airspeedCalTriggerParams << "SENS_DPRES_OFF";
        if (_vehicle->firmwareMajorVersion() >= 1 && _vehicle->firmwareMinorVersion() >= 14) {
            _airspeedCalTriggerParams << "SYS_HAS_NUM_ASPD";
        } else {
            _airspeedCalTriggerParams << "FW_ARSP_MODE" << "CBRK_AIRSPD_CHK";
        }
    }
}

bool SensorsComponent::setupComplete() const
{
    for (const QString &triggerParam : std::as_const(_deviceIds)) {
        if (parameterValue(triggerParam).toFloat() == 0.0f) {
            return false;
        }
    }

    bool magEnabled = true;
    if (parameterExists(_magEnabledParam)) {
        magEnabled = parameterValue(_magEnabledParam).toBool();
    }

    if (magEnabled && parameterValue(_magCalParam).toFloat() == 0.0f) {
        return false;
    }

    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        if (_vehicle->firmwareMajorVersion() > 1 || (_vehicle->firmwareMajorVersion() == 1 && _vehicle->firmwareMinorVersion() > 14)) {
            if (parameterValue("SYS_HAS_NUM_ASPD").toBool() &&
                    parameterValue("SENS_DPRES_OFF").toFloat() == 0.0f) {
                return false;
            }
        } else {
            if (!parameterValue("FW_ARSP_MODE").toBool() &&
                    parameterValue("CBRK_AIRSPD_CHK").toInt() != 162128 &&
                    parameterValue("SENS_DPRES_OFF").toFloat() == 0.0f) {
                return false;
            }
        }
    }

    return true;
}

QStringList SensorsComponent::setupCompleteChangedTriggerList() const
{
    QStringList triggers;

    triggers << _deviceIds << _magCalParam << _magEnabledParam;
    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        triggers << _airspeedCalTriggerParams;
    }

    return triggers;
}

QUrl SensorsComponent::setupSource() const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/SensorsComponent.qml");
}

QUrl SensorsComponent::summaryQmlSource() const
{
    QString summaryQml;

    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        summaryQml = "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/SensorsComponentSummaryFixedWing.qml";
    } else {
        summaryQml = "qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/SensorsComponentSummary.qml";
    }

    return QUrl::fromUserInput(summaryQml);
}

 bool SensorsComponent::_airspeedCalSupported() const
 {
    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        if (_vehicle->firmwareMajorVersion() > 1 || (_vehicle->firmwareMajorVersion() == 1 && _vehicle->firmwareMinorVersion() > 14)) {
            if (parameterValue("SYS_HAS_NUM_ASPD").toBool()) {
                return true;
            }
        } else {
            if (!parameterValue("FW_ARSP_MODE").toBool() &&
                    parameterValue("CBRK_AIRSPD_CHK").toInt() != 162128) {
                return true;
            }
        }
    }

    return false;
 }

 bool SensorsComponent::_airspeedCalRequired() const
 {
    return _airspeedCalSupported() && parameterValue("SENS_DPRES_OFF").toFloat() == 0.0f;
 }
