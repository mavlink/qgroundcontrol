#include "PX4FlightModesController.h"

#include "PX4FirmwarePlugin.h"
#include "StandardModes.h"
#include "Vehicle.h"

PX4FlightModesController::PX4FlightModesController(QObject* parent)
    : FactPanelController(parent)
{
    // Long enough for a PX4 RC_CHANNELS stream as slow as 1 Hz
    _rcTimeoutTimer.setInterval(2000);
    _rcTimeoutTimer.setSingleShot(true);
    (void) connect(&_rcTimeoutTimer, &QTimer::timeout, this, &PX4FlightModesController::_clearRcChannelValues);

    if (!_vehicle) {
        return;
    }

    (void) connect(_vehicle, &Vehicle::flightModesChanged, this, &PX4FlightModesController::_updateExternalModeNames);
    _updateExternalModeNames();
}

void PX4FlightModesController::setMonitorRc(bool monitorRc)
{
    if (monitorRc == _monitorRc) {
        return;
    }
    _monitorRc = monitorRc;

    if (_vehicle) {
        // Raw rather than clamped: PX4 scales PWM against RC#_MIN/RC#_MAX, which can lie outside 1000-2000 us
        if (_monitorRc) {
            (void) connect(_vehicle, &Vehicle::rcChannelsRawChanged, this,
                           &PX4FlightModesController::_rcChannelsChanged);
        } else {
            (void) disconnect(_vehicle, &Vehicle::rcChannelsRawChanged, this,
                              &PX4FlightModesController::_rcChannelsChanged);
        }
    }
    if (!_monitorRc) {
        _rcTimeoutTimer.stop();
        _clearRcChannelValues();
    }
    emit monitorRcChanged();
}

void PX4FlightModesController::_rcChannelsChanged(QVector<int> pwmValues)
{
    _rcChannelValues.clear();
    _rcChannelValues.reserve(pwmValues.size());
    for (const int pwm : pwmValues) {
        _rcChannelValues.append(pwm);
    }
    _rcTimeoutTimer.start();
    emit rcChannelValuesChanged();
}

void PX4FlightModesController::_clearRcChannelValues()
{
    if (_rcChannelValues.isEmpty()) {
        return;
    }
    _rcChannelValues.clear();
    emit rcChannelValuesChanged();
}

void PX4FlightModesController::_updateExternalModeNames()
{
    if (!_vehicle || !_vehicle->standardModes()) {
        return;
    }

    const FlightModeList& modeList = _vehicle->standardModes()->modeList();
    QStringList names;
    for (int externalIndex = 1; externalIndex <= PX4FirmwarePlugin::kExternalFlightModeCount; externalIndex++) {
        names.append(PX4FirmwarePlugin::externalFlightModeName(modeList, externalIndex));
    }
    if (names != _externalModeNames) {
        _externalModeNames = names;
        emit externalModeNamesChanged();
    }
}
