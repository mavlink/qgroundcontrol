#include "PX4FlightModesController.h"

#include <QtCore/QMap>

#include "PX4FirmwarePlugin.h"
#include "PX4FlightModeConfig.h"
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

    const QStringList names = PX4FirmwarePlugin::externalFlightModeNames(_vehicle->standardModes()->modeList());
    if (names != _externalModeNames) {
        _externalModeNames = names;
        emit externalModeNamesChanged();
    }
}

QString PX4FlightModesController::selectionMethod(int modeChannel, int buttonMask) const
{
    switch (PX4FlightModeConfig::selectionMethod(modeChannel, buttonMask)) {
    case PX4FlightModeConfig::SelectionMethod::Switch:
        return QStringLiteral("switch");
    case PX4FlightModeConfig::SelectionMethod::Buttons:
        return QStringLiteral("buttons");
    case PX4FlightModeConfig::SelectionMethod::None:
        break;
    }

    return QStringLiteral("none");
}

QVariantList PX4FlightModesController::buttonChannels(int buttonMask) const
{
    return _toVariantList(PX4FlightModeConfig::buttonChannels(buttonMask));
}

QVariantList PX4FlightModesController::ignoredButtonChannels(int buttonMask) const
{
    return _toVariantList(PX4FlightModeConfig::ignoredButtonChannels(buttonMask));
}

QVariantList PX4FlightModesController::modeChoices(const QString& paramName, const QStringList& enumStrings,
                                                   const QVariantList& enumValues, const QStringList& externalNames,
                                                   int currentValue) const
{
    QVariantList choices;
    for (const PX4FlightModeConfig::Choice& choice :
         PX4FlightModeConfig::modeChoices(paramName, enumStrings, enumValues, externalNames, currentValue)) {
        choices.append(QVariantMap{{QStringLiteral("label"), choice.label}, {QStringLiteral("value"), choice.value}});
    }

    return choices;
}

QString PX4FlightModesController::modeName(const QString& paramName, const QStringList& enumStrings,
                                           const QVariantList& enumValues, const QStringList& externalNames,
                                           int value) const
{
    return PX4FlightModeConfig::modeName(paramName, enumStrings, enumValues, externalNames, value);
}

QVariantList PX4FlightModesController::_toVariantList(const QList<int>& values)
{
    QVariantList list;
    for (const int value : values) {
        list.append(value);
    }

    return list;
}
