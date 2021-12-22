/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Mixer.h"

#include <QDebug>

#include <cmath>

using namespace Mixer;

MixerChannel::MixerChannel(QObject *parent, const QString &label, int actuatorFunction, int paramIndex, int actuatorTypeIndex,
        QmlObjectListModel &channelConfigs, ParameterManager* parameterManager, const Rule* rule,
        std::function<void(Function, Fact*)> factAddedCb)
    : QObject(parent), _label(label), _actuatorFunction(actuatorFunction), _paramIndex(paramIndex),
      _actuatorTypeIndex(actuatorTypeIndex), _rule(rule)
{
    for (int i = 0; i < channelConfigs.count(); ++i) {
        auto channelConfig = channelConfigs.value<ChannelConfig*>(i);
        QString param = channelConfig->parameter();
        int usedParamIndex;
        if (channelConfig->isActuatorTypeConfig()) {
            usedParamIndex = actuatorTypeIndex + channelConfig->indexOffset();
        } else {
            usedParamIndex = paramIndex + channelConfig->indexOffset();
        }
        param.replace("${i}", QString::number(usedParamIndex));

        Fact* fact = nullptr;
        if (param == "" && !channelConfig->isActuatorTypeConfig()) { // constant value
            float value = 0.f;
            if (channelConfig->fixedValues().size() == 1) {
                value = channelConfig->fixedValues()[0];
            } else if (paramIndex < channelConfig->fixedValues().size()) {
                value = channelConfig->fixedValues()[paramIndex];
            }

            FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeFloat, "", this);
            metaData->setReadOnly(true);
            metaData->setDecimalPlaces(4);
            fact = new Fact("", metaData, this);
            fact->setRawValue(value);

        } else if (parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
            fact = parameterManager->getParameter(FactSystem::defaultComponentId, param);
            if (channelConfig->displayOption() == Parameter::DisplayOption::Bitset) {
                fact = new FactBitset(channelConfig, fact, usedParamIndex);
            } else if (channelConfig->displayOption() == Parameter::DisplayOption::BoolTrueIfPositive) {
                fact = new FactFloatAsBool(channelConfig, fact);
            }
            factAddedCb(channelConfig->function(), fact);
        } else {
            qCDebug(ActuatorsConfigLog) << "ActuatorOutputChannel: Param does not exist:" << param;
        }

        // if we have a valid rule, check the identifiers
        int applyIdentifierIdx = -1;
        if (rule) {
            if (channelConfig->identifier() == rule->selectIdentifier) {
                _ruleSelectIdentifierIdx = _configInstances->count();
                if (fact) {
                    _currentSelectIdentifierValue = fact->rawValue().toInt();
                }
            } else {
                for (int i = 0; i < rule->applyIdentifiers.size(); ++i) {
                    if (channelConfig->identifier() == rule->applyIdentifiers[i]) {
                        applyIdentifierIdx = i;
                    }
                }
            }

            if (fact) {
                connect(fact, &Fact::rawValueChanged, this, [this]() { applyRule(); });
            }
        }

        ChannelConfigInstance* instance = new ChannelConfigInstance(this, fact, *channelConfig, applyIdentifierIdx);
        _configInstances->append(instance);
    }

    applyRule(true);
}

void MixerChannel::applyRule(bool noConstraints)
{
    if (!_rule || _ruleSelectIdentifierIdx == -1 || _applyingRule) {
        return;
    }
    _applyingRule = true; // prevent recursive signals

    Fact* selectFact = _configInstances->value<ChannelConfigInstance*>(_ruleSelectIdentifierIdx)->fact();
    if (selectFact && selectFact->type() == FactMetaData::valueTypeInt32) {
        const int value = selectFact->rawValue().toInt();
        if (_rule->items.contains(value)) {
            bool valueChanged = value != _currentSelectIdentifierValue;
            const auto& items = _rule->items[value];
            for (int i = 0; i < _configInstances->count(); ++i) {
                ChannelConfigInstance* configInstance = _configInstances->value<ChannelConfigInstance*>(i);
                if (configInstance->fact() && configInstance->ruleApplyIdentifierIdx() >= 0) {
                    const Rule::RuleItem& ruleItem = items[configInstance->ruleApplyIdentifierIdx()];
                    double factValue = configInstance->fact()->rawValue().toDouble();
                    bool changed = false;
                    if (ruleItem.hasMin && factValue < ruleItem.min) {
                        factValue = ruleItem.min;
                        changed = true;
                    }
                    if (ruleItem.hasMax && factValue > ruleItem.max) {
                        factValue = ruleItem.max;
                        changed = true;
                    }
                    if (valueChanged && ruleItem.hasDefault) {
                        factValue = ruleItem.defaultVal;
                        changed = true;
                    }
                    if (changed && !noConstraints) {
                        // here we could notify the user that a constraint got applied...
                        configInstance->fact()->setRawValue(factValue);
                    }
                    configInstance->setVisible(!ruleItem.hidden);
                    configInstance->setEnabled(!ruleItem.disabled);
                }
            }

        } else {
            // no rule set for this value, just reset
            for (int i = 0; i < _configInstances->count(); ++i) {
                ChannelConfigInstance* configInstance = _configInstances->value<ChannelConfigInstance*>(i);
                configInstance->setVisible(true);
                configInstance->setEnabled(true);
            }
        }
        _currentSelectIdentifierValue = value;
    }
    _applyingRule = false;
}

bool MixerChannel::getGeometry(const ActuatorTypes& actuatorTypes, const MixerOption::ActuatorGroup& group,
        ActuatorGeometry& geometry) const
{
    geometry.type = ActuatorGeometry::typeFromStr(group.actuatorType);
    const auto iter = actuatorTypes.find(group.actuatorType);
    if (iter == actuatorTypes.end()) {
        return false;
    }
    geometry.index = _actuatorTypeIndex;
    geometry.labelIndexOffset = iter->labelIndexOffset;
    int numPositionAxis = 0;

    for (int i = 0; i < _configInstances->count(); ++i) {
        ChannelConfigInstance* configInstance = _configInstances->value<ChannelConfigInstance*>(i);
        if (!configInstance->fact()) {
            continue;
        }
        switch (configInstance->channelConfig()->function()) {
            case Function::PositionX:
                geometry.position.setX(configInstance->fact()->rawValue().toFloat());
                ++numPositionAxis;
                break;
            case Function::PositionY:
                geometry.position.setY(configInstance->fact()->rawValue().toFloat());
                ++numPositionAxis;
                break;
            case Function::PositionZ:
                geometry.position.setZ(configInstance->fact()->rawValue().toFloat());
                ++numPositionAxis;
                break;
            case Function::SpinDirection:
                geometry.spinDirection = configInstance->fact()->rawValue().toBool() ?
					ActuatorGeometry::SpinDirection::CounterClockWise : ActuatorGeometry::SpinDirection::ClockWise;
                break;
            case Function::AxisX:
            case Function::AxisY:
            case Function::AxisZ:
            case Function::Type:
            case Function::Unspecified:
                break;
        }
    }

    return numPositionAxis == 3;
}

Fact* MixerChannel::getFact(Function function) const
{
    for (int i = 0; i < _configInstances->count(); ++i) {
        ChannelConfigInstance* configInstance = _configInstances->value<ChannelConfigInstance*>(i);
        if (configInstance->channelConfig()->function() == Function::Type) {
            return configInstance->fact();
        }
    }
    return nullptr;
}

void MixerConfigGroup::addChannelConfig(ChannelConfig* channelConfig)
{
    _channelConfigs->append(channelConfig);
    emit channelConfigsChanged();
}

void MixerConfigGroup::addChannel(MixerChannel* channel)
{
    _channels->append(channel);
    emit channelsChanged();
}

void MixerConfigGroup::addConfigParam(ConfigParameter* param)
{
    _params->append(param);
}

void Mixers::reset(const ActuatorTypes& actuatorTypes, const MixerOptions& mixerOptions,
        const QMap<int, OutputFunction>& functions, const Rules& rules)
{
    _groups->clearAndDeleteContents();
    _actuatorTypes = actuatorTypes;
    _mixerOptions = mixerOptions;
    _functions = functions;
    _functionsSpecificLabel.clear();
    _rules = rules;
    _mixerConditions.clear();
    for (const auto& mixerOption : _mixerOptions) {
        _mixerConditions.append(Condition(mixerOption.option, _parameterManager));
    }
    update();
}

void Mixers::update()
{
    // clear first
    _groups->clearAndDeleteContents();
    _functionsSpecificLabel.clear();

    unsubscribeFacts();

    // find configured mixer
    _selectedMixer = -1;
    for (int i = 0; i < _mixerConditions.size(); ++i) {
        if (_mixerConditions[i].evaluate()) {
            _selectedMixer = i;
            break;
        }
    }


    if (_selectedMixer != -1) {

        subscribeFact(_mixerConditions[_selectedMixer].fact());

        qCDebug(ActuatorsConfigLog) << "selected mixer index:" << _selectedMixer;

        const auto& actuatorGroups = _mixerOptions[_selectedMixer].actuators;
        QMap<QString, int> actuatorTypeCount;
        for (const auto &actuatorGroup : actuatorGroups) {
            int count = actuatorGroup.fixedCount;
            if (actuatorGroup.count != "") {
                Fact* countFact = getFact(actuatorGroup.count);
                if (countFact) {
                    count = countFact->rawValue().toInt();
                }
            }

            int actuatorTypeIndex = actuatorTypeCount.value(actuatorGroup.actuatorType, 0);

            MixerConfigGroup* currentMixerGroup = new MixerConfigGroup(this, actuatorGroup);

            // params
            const auto actuatorType = _actuatorTypes.find(actuatorGroup.actuatorType);
            if (actuatorType != _actuatorTypes.end()) {
                for (const auto& perItemParam : actuatorType->perItemParams) {
                    MixerParameter param{};
                    param.param = perItemParam;
                    currentMixerGroup->addChannelConfig(new ChannelConfig(currentMixerGroup, param, true));
                }
            }

            const Rule* selectedRule{nullptr}; // at most 1 rule can be applied
            for (const auto& perItemParam : actuatorGroup.perItemParameters) {
                currentMixerGroup->addChannelConfig(new ChannelConfig(currentMixerGroup, perItemParam, false));

                if (!perItemParam.identifier.isEmpty()) {
                    for (const auto& rule : _rules) {
                        if (rule.selectIdentifier == perItemParam.identifier) {
                            selectedRule = &rule;
                        }
                    }
                }
            }

            // 'count' param
            if (actuatorGroup.count != "") {
                currentMixerGroup->setCountParam(new ConfigParameter(currentMixerGroup, getFact(actuatorGroup.count),
                        "", false));
            }

            for (int actuatorIdx = 0; actuatorIdx < count; ++actuatorIdx) {
                QString label = "";
                int actuatorFunction = 0;
                if (actuatorType != _actuatorTypes.end()) {
                    actuatorFunction = actuatorType->functionMin + actuatorTypeIndex;
                    label = _functions.value(actuatorFunction).label;
                    if (label == "") {
                        qCWarning(ActuatorsConfigLog) << "No label for output function" << actuatorFunction;
                    }
                    QString itemLabelPrefix{};
                    if (actuatorGroup.itemLabelPrefix.size() == 1) {
                        QString paramIndex = QString::number(actuatorIdx + 1);
                        itemLabelPrefix = actuatorGroup.itemLabelPrefix[0];
                        itemLabelPrefix.replace("${i}", paramIndex);
                    } else if (actuatorIdx < actuatorGroup.itemLabelPrefix.size()) {
                        itemLabelPrefix = actuatorGroup.itemLabelPrefix[actuatorIdx];
                    }
                    if (itemLabelPrefix != "") {
                        label = itemLabelPrefix + " (" + label + ")";
                        _functionsSpecificLabel[actuatorFunction] = label;
                    }
                }
                auto factAdded = [this](Function function, Fact* fact) {
                    // Type might change more than the geometry
                    subscribeFact(fact, function != Function::Type);
                };
                MixerChannel* channel = new MixerChannel(currentMixerGroup, label, actuatorFunction, actuatorIdx, actuatorTypeIndex,
                        *currentMixerGroup->channelConfigs(), _parameterManager, selectedRule, factAdded);
                currentMixerGroup->addChannel(channel);
                ++actuatorTypeIndex;
            }

            // config params
            for (const auto& parameter : actuatorGroup.parameters) {
                currentMixerGroup->addConfigParam(new ConfigParameter(currentMixerGroup, getFact(parameter.name),
                        parameter.label, parameter.advanced));
            }

            _groups->append(currentMixerGroup);
            actuatorTypeCount[actuatorGroup.actuatorType] = actuatorTypeIndex;
        }
    }

    emit groupsChanged();

}

QString Mixers::getSpecificLabelForFunction(int function) const
{
    // Try to get it from the actuator type param
    for (int mixerGroupIdx = 0; mixerGroupIdx < _groups->count(); ++mixerGroupIdx) {
        Mixer::MixerConfigGroup* mixerGroup = _groups->value<Mixer::MixerConfigGroup*>(mixerGroupIdx);
        for (int mixerChannelIdx = 0; mixerChannelIdx < mixerGroup->channels()->count(); ++mixerChannelIdx) {
            Mixer::MixerChannel* mixerChannel = mixerGroup->channels()->value<Mixer::MixerChannel*>(mixerChannelIdx);

            if (mixerChannel->actuatorFunction() != function) {
                continue;
            }

            Fact* typeFact = mixerChannel->getFact(Function::Type);
            if (typeFact) {
                return typeFact->enumOrValueString() + " (" + _functions.value(function).label +")";
            }
        }
    }

    const auto iter = _functionsSpecificLabel.find(function);
    if (iter == _functionsSpecificLabel.end()) {
        return _functions.value(function).label;
    }
    return *iter;
}

QSet<int> Mixers::requiredFunctions() const
{
    QSet<int> ret;
    for (int mixerGroupIdx = 0; mixerGroupIdx < _groups->count(); ++mixerGroupIdx) {
        Mixer::MixerConfigGroup* mixerGroup = _groups->value<Mixer::MixerConfigGroup*>(mixerGroupIdx);
        if (mixerGroup->group().required) {
            for (int mixerChannelIdx = 0; mixerChannelIdx < mixerGroup->channels()->count(); ++mixerChannelIdx) {
                const Mixer::MixerChannel* mixerChannel = mixerGroup->channels()->value<Mixer::MixerChannel*>(mixerChannelIdx);
                if (mixerChannel->actuatorFunction() != 0) {
                    ret.insert(mixerChannel->actuatorFunction());
                }
            }
        }
    }

    return ret;
}

QString Mixers::configuredType() const
{
    if (_selectedMixer == -1) {
        return "";
    }
    return _mixerOptions[_selectedMixer].type;
}

Fact* Mixers::getFact(const QString& paramName)
{
    if (!_parameterManager->parameterExists(FactSystem::defaultComponentId, paramName)) {
        qCDebug(ActuatorsConfigLog) << "Mixers: Param does not exist:" << paramName;
        return nullptr;
    }
    Fact* fact = _parameterManager->getParameter(FactSystem::defaultComponentId, paramName);
	subscribeFact(fact);
	return fact;
}

void Mixers::subscribeFact(Fact* fact, bool geometry)
{
    if (geometry) {
        if (fact && !_subscribedFactsGeometry.contains(fact)) {
            connect(fact, &Fact::rawValueChanged, this, &Mixers::geometryParamChanged);
            _subscribedFactsGeometry.insert(fact);
        }
    } else {
        if (fact && !_subscribedFacts.contains(fact)) {
            connect(fact, &Fact::rawValueChanged, this, &Mixers::paramChanged);
            _subscribedFacts.insert(fact);
        }
    }
}

void Mixers::unsubscribeFacts()
{
    for (Fact* fact : _subscribedFacts) {
        disconnect(fact, &Fact::rawValueChanged, this, &Mixers::paramChanged);
    }
    _subscribedFacts.clear();

    for (Fact* fact : _subscribedFactsGeometry) {
        disconnect(fact, &Fact::rawValueChanged, this, &Mixers::geometryParamChanged);
    }
    _subscribedFactsGeometry.clear();
}
