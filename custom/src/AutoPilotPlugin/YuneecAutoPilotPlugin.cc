/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "YuneecAutoPilotPlugin.h"
#include "GimbalComponent.h"
#include "ChannelComponent.h"
#include "HealthComponent.h"
#include "YuneecSafetyComponent.h"

#include "QGCApplication.h"
#include "QGCCorePlugin.h"

YuneecAutoPilotPlugin::YuneecAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : PX4AutoPilotPlugin(vehicle, parent)
    , _gimbalComponent(NULL)
    , _channelComponent(NULL)
    , _healthComponent(NULL)
    , _yuneecSafetyComponent(NULL)
{
    connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &YuneecAutoPilotPlugin::_advancedChanged);
}

void YuneecAutoPilotPlugin::_advancedChanged(bool advanced)
{
    _components.clear();
    if(!advanced && _channelComponent) {
        delete _channelComponent;
        _channelComponent = NULL;
    }
    emit vehicleComponentsChanged();
}

const QVariantList& YuneecAutoPilotPlugin::vehicleComponents(void)
{
    if (_components.count() == 0 && !_incorrectParameterVersion) {
        if (_vehicle->parameterManager()->parametersReady()) {

            if(!_sensorsComponent) {
                _sensorsComponent = new SensorsComponent(_vehicle, this);
                _sensorsComponent->setupTriggerSignals();
            }
            _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));

            if(!_yuneecSafetyComponent) {
                _yuneecSafetyComponent = new YuneecSafetyComponent(_vehicle, this);
                _yuneecSafetyComponent->setupTriggerSignals();
            }
            _components.append(QVariant::fromValue((VehicleComponent*)_yuneecSafetyComponent));

#if !defined (__planner__)
            if(!_gimbalComponent) {
                _gimbalComponent = new GimbalComponent(_vehicle, this);
                _gimbalComponent->setupTriggerSignals();
            }
#endif
            _components.append(QVariant::fromValue((VehicleComponent*)_gimbalComponent));

            if(!_healthComponent) {
                _healthComponent = new HealthComponent(_vehicle, this);
                _healthComponent->setupTriggerSignals();
            }
            _components.append(QVariant::fromValue((VehicleComponent*)_healthComponent));

#if !defined (__planner__)
            if(qgcApp()->toolbox()->corePlugin()->showAdvancedUI()) {
#endif
                if(!_channelComponent) {
                    _channelComponent = new ChannelComponent(_vehicle, this);
                    _channelComponent->setupTriggerSignals();
                }
                _components.append(QVariant::fromValue((VehicleComponent*)_channelComponent));
#if !defined (__planner__)
            }
#endif

        } else {
            qWarning() << "Call to vehicleComponents prior to parametersReady";
        }
    }

    return _components;
}
