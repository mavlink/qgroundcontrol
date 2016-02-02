/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "APMRadioComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMRadioComponent::APMRadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Radio"))
{
    _mapParams << QStringLiteral("RCMAP_ROLL") << QStringLiteral("RCMAP_PITCH") << QStringLiteral("RCMAP_YAW") << QStringLiteral("RCMAP_THROTTLE");

    foreach (const QString& mapParam, _mapParams) {
        Fact* fact = _autopilot->getParameterFact(-1, mapParam);
        connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);
    }

    _connectSetupTriggers();
}

QString APMRadioComponent::name(void) const
{
    return _name;
}

QString APMRadioComponent::description(void) const
{
    return tr("The Radio Component is used to setup which channels on your RC Transmitter you will use for each vehicle control such as Roll, Pitch, Yaw and Throttle. "
              "It also allows you to assign switches and dials to the various flight modes. "
              "Prior to flight you must also calibrate the extents for all of your channels.");
}

QString APMRadioComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/RadioComponentIcon.png");
}

bool APMRadioComponent::requiresSetup(void) const
{
    return true;
}

bool APMRadioComponent::setupComplete(void) const
{
    // The best we can do to detect the need for a radio calibration is look for attitude
    // controls to be mapped as well as all attitude control rc min/max/trim still at defaults.
    QList<int> mapValues;

    // First check for all attitude controls mapped
    for (int i=0; i<_mapParams.count(); i++) {
        mapValues << _autopilot->getParameterFact(FactSystem::defaultComponentId, _mapParams[i])->rawValue().toInt();
        if (mapValues[i] <= 0) {
            return false;
        }
    }

    // Next check RC#_MIN/MAX/TRIM all at defaults
    foreach (const QString& mapParam, _mapParams) {
        int channel = _autopilot->getParameterFact(-1, mapParam)->rawValue().toInt();
        if (_autopilot->getParameterFact(-1, QString("RC%1_MIN").arg(channel))->rawValue().toInt() != 1100) {
            return true;
        }
        if (_autopilot->getParameterFact(-1, QString("RC%1_MAX").arg(channel))->rawValue().toInt() != 1900) {
            return true;
        }
        if (_autopilot->getParameterFact(-1, QString("RC%1_TRIM").arg(channel))->rawValue().toInt() != 1500) {
            return true;
        }
    }
    
    return false;
}

QStringList APMRadioComponent::setupCompleteChangedTriggerList(void) const
{
    // APMRadioComponent manages it's own triggers
    return QStringList();
}

QUrl APMRadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/RadioComponent.qml"));
}

QUrl APMRadioComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMRadioComponentSummary.qml"));
}

QString APMRadioComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    
    return QString();
}

void APMRadioComponent::_connectSetupTriggers(void)
{
    // Disconnect previous triggers
    foreach(Fact* fact, _triggerFacts) {
        disconnect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);
    }
    _triggerFacts.clear();

    // Get the channels for attitude controls and connect to those values for triggers
    foreach (const QString& mapParam, _mapParams) {
        int channel = _autopilot->getParameterFact(FactSystem::defaultComponentId, mapParam)->rawValue().toInt();

        Fact* fact = _autopilot->getParameterFact(-1, QString("RC%1_MIN").arg(channel));
        _triggerFacts << fact;
        connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);

        fact = _autopilot->getParameterFact(-1, QString("RC%1_MAX").arg(channel));
        _triggerFacts << fact;
        connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);

        fact = _autopilot->getParameterFact(-1, QString("RC%1_TRIM").arg(channel));
        _triggerFacts << fact;
        connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);
    }
}

void APMRadioComponent::_triggerChanged(void)
{
    emit setupCompleteChanged(setupComplete());

    // Control mapping may have changed so we need to reset triggers
    _connectSetupTriggers();
}
