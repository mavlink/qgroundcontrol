/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMFlightModesComponentController.h"
#include "QGCMAVLink.h"
#include "AutoPilotPluginManager.h"

#include <QVariant>
#include <QQmlProperty>

APMFlightModesComponentController::APMFlightModesComponentController(void)
    : _activeFlightMode(0)
    , _channelCount(Vehicle::cMaxRcChannels)
{
    QStringList usedParams;
    usedParams << QStringLiteral("FLTMODE1") << QStringLiteral("FLTMODE2") << QStringLiteral("FLTMODE3")
               << QStringLiteral("FLTMODE4") << QStringLiteral("FLTMODE5") << QStringLiteral("FLTMODE6");
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    _rgChannelOptionEnabled << QVariant(false) << QVariant(false) << QVariant(false) << QVariant(false) << QVariant(false) << QVariant(false);
    
    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &APMFlightModesComponentController::_rcChannelsChanged);
}

/// Connected to Vehicle::rcChannelsChanged signal
void APMFlightModesComponentController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    int flightModeChannel = 4;

    if (parameterExists(FactSystem::defaultComponentId, QStringLiteral("FLTMODE_CH"))) {
        flightModeChannel = getParameterFact(FactSystem::defaultComponentId, QStringLiteral("FLTMODE_CH"))->rawValue().toInt() - 1;
    }

    if (flightModeChannel >= channelCount) {
        return;
    }

    _activeFlightMode = 0;
    int channelValue = pwmValues[flightModeChannel];
    if (channelValue != -1) {
        bool found = false;
        int rgThreshold[] = { 1230, 1360, 1490, 1620, 1749 };
        for (int i=0; i<5; i++) {
            if (channelValue <= rgThreshold[i]) {
                _activeFlightMode = i + 1;
                found = true;
                break;
            }
        }
        if (!found) {
            _activeFlightMode = 6;
        }
    }
    emit activeFlightModeChanged(_activeFlightMode);

    for (int i=0; i<6; i++) {
        _rgChannelOptionEnabled[i] = QVariant(false);
        channelValue = pwmValues[i+6];
        if (channelValue != -1 && channelValue > 1800) {
            _rgChannelOptionEnabled[i] = QVariant(true);
        }
    }
    emit channelOptionEnabledChanged();
}
