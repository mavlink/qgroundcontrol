/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ActuatorOutputs.h"

#include <QDebug>

using namespace ActuatorOutputs;

void ChannelConfig::reevaluate()
{
    emit visibleChanged();
}

ActuatorOutputChannel::ActuatorOutputChannel(QObject *parent, const QString &label, int paramIndex,
        QmlObjectListModel &channelConfigs, ParameterManager* parameterManager, std::function<void(Fact*)> factAddedCb) :
        QObject(parent), _label(label), _paramIndex(paramIndex)
{
    for (int i = 0; i < channelConfigs.count(); ++i) {
        auto channelConfig = channelConfigs.value<ChannelConfig*>(i);
        QString param = channelConfig->parameter();
        QString sparamIndex = QString::number(paramIndex + channelConfig->indexOffset());
        param.replace("${i}", sparamIndex);

        Fact* fact = nullptr;
        if (parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
            fact = parameterManager->getParameter(FactSystem::defaultComponentId, param);
            if (channelConfig->displayOption() == Parameter::DisplayOption::Bitset) {
                fact = new FactBitset(channelConfig, fact, paramIndex + channelConfig->indexOffset());
            } else if (channelConfig->displayOption() == Parameter::DisplayOption::BoolTrueIfPositive) {
                fact = new FactFloatAsBool(channelConfig, fact);
            }
            factAddedCb(fact);
        } else {
            qCDebug(ActuatorsConfigLog) << "ActuatorOutputChannel: Param does not exist:" << param;
        }

        ChannelConfigInstance *instance = new ChannelConfigInstance(this, fact, *channelConfig);
        _configInstances->append(instance);
    }
}

void ActuatorOutputSubgroup::addChannelConfig(ChannelConfig *channelConfig)
{
    _channelConfigs->append(channelConfig);
    emit channelConfigsChanged();
}

void ActuatorOutputSubgroup::addChannel(ActuatorOutputChannel *channel)
{
    _channels->append(channel);
    emit channelsChanged();
}

void ActuatorOutputSubgroup::addConfigParam(ConfigParameter *param)
{
    if (param->function() == ConfigParameter::Function::Primary) {
        delete _primaryParam;
        _primaryParam = param;
    } else {
        _params->append(param);
    }
}

ActuatorOutput::ActuatorOutput(QObject* parent, const QString& label, const Condition& groupVisibilityCondition)
        : QObject(parent), _label(label), _groupVisibilityCondition(groupVisibilityCondition)
{
    if (_groupVisibilityCondition.fact()) {
        connect(_groupVisibilityCondition.fact(), &Fact::rawValueChanged, this, &ActuatorOutput::groupsVisibleChanged);
    }
}

void ActuatorOutput::addSubgroup(ActuatorOutputSubgroup *subgroup)
{
    _subgroups->append(subgroup);
    emit subgroupsChanged();
}

void ActuatorOutput::addConfigParam(ConfigParameter *param)
{
    if (param->function() == ConfigParameter::Function::Enable) {
        delete _enableParam;
        _enableParam = param;
    } else {
        _params->append(param);
    }
}

void ActuatorOutput::getAllChannelFunctions(QList<Fact*> &allFunctions) const
{
    forEachOutputFunction([&allFunctions](ActuatorOutputSubgroup*, ChannelConfigInstance*, Fact* fact) {
        allFunctions.append(fact);
    });
}

bool ActuatorOutput::hasExistingOutputFunctionParams() const
{
    bool hasExistingOutputFunction = false;
    forEachOutputFunction([&hasExistingOutputFunction](ActuatorOutputSubgroup*, ChannelConfigInstance*, Fact* fact) {
        hasExistingOutputFunction = true;
    });
    return hasExistingOutputFunction;
}

void ActuatorOutput::forEachOutputFunction(std::function<void(ActuatorOutputSubgroup*, ChannelConfigInstance*, Fact*)> callback) const
{
    for (int subgroupIdx = 0; subgroupIdx < _subgroups->count(); subgroupIdx++) {
        ActuatorOutputSubgroup *subgroup = qobject_cast<ActuatorOutputSubgroup*>(_subgroups->get(subgroupIdx));
        for (int channelIdx = 0; channelIdx < subgroup->channels()->count(); channelIdx++) {
            ActuatorOutputChannel *channel = qobject_cast<ActuatorOutputChannel*>(subgroup->channels()->get(channelIdx));
            for (int configIdx = 0; configIdx < channel->configInstances()->count(); configIdx++) {
                ChannelConfigInstance *configInstance = qobject_cast<ChannelConfigInstance*>(channel->configInstances()->get(configIdx));
                Fact* fact = configInstance->fact();
                if (configInstance->channelConfig()->function() == ChannelConfig::Function::OutputFunction && fact) {
                    callback(subgroup, configInstance, fact);
                }
            }
        }
    }
}
