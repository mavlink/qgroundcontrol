/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Mixer.h"
#include "ParameterManager.h"

using namespace Mixer;

ChannelConfigInstance* ChannelConfig::instantiate(int paramIndex, int actuatorTypeIndex,
        ParameterManager* parameterManager, std::function<void(Function, Fact*)> factAddedCb)
{
    QString param = _parameter.param.name;
    int usedParamIndex;
    if (_isActuatorTypeConfig) {
        usedParamIndex = actuatorTypeIndex + indexOffset();
    } else {
        usedParamIndex = paramIndex + indexOffset();
    }
    param.replace("${i}", QString::number(usedParamIndex));

    Fact* fact = nullptr;
    if (param == "" && !_isActuatorTypeConfig) { // constant value
        float value = 0.f;
        if (fixedValues().size() == 1) {
            value = fixedValues()[0];
        } else if (paramIndex < fixedValues().size()) {
            value = fixedValues()[paramIndex];
        }

        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeFloat, "", this);
        metaData->setReadOnly(true);
        metaData->setDecimalPlaces(4);
        fact = new Fact("", metaData, this);
        fact->setRawValue(value);

    } else if (parameterManager->parameterExists(ParameterManager::defaultComponentId, param)) {
        fact = parameterManager->getParameter(ParameterManager::defaultComponentId, param);
        if (displayOption() == Parameter::DisplayOption::Bitset) {
            fact = new FactBitset(this, fact, usedParamIndex);
        } else if (displayOption() == Parameter::DisplayOption::BoolTrueIfPositive) {
            fact = new FactFloatAsBool(this, fact);
        }
        factAddedCb(function(), fact);
    } else {
        qCDebug(ActuatorsConfigLog) << "ActuatorOutputChannel: Param does not exist:" << param;
    }

    ChannelConfigInstance* instance = new ChannelConfigInstance(this, fact, *this);
    channelInstanceCreated(instance);
    return instance;
}

void ChannelConfig::channelInstanceCreated(ChannelConfigInstance* instance)
{
    _instances.append(instance);
    connect(instance, &ChannelConfigInstance::visibleChanged,
            this, &ChannelConfig::instanceVisibleChanged);
}

void ChannelConfig::instanceVisibleChanged()
{
    bool visible = false;
    for (const auto& instance : _instances) {
        if (instance->visible()) {
            visible = true;
        }
    }

    if (visible != _visible) {
        _visible = visible;
        emit visibleChanged();
    }
}

ChannelConfigInstance *ChannelConfigVirtualAxis::instantiate(
    [[maybe_unused]] int paramIndex, [[maybe_unused]] int actuatorTypeIndex,
    [[maybe_unused]] ParameterManager *parameterManager,
    [[maybe_unused]] std::function<void(Function, Fact *)> factAddedCb)
{
    ChannelConfigInstance* instance = new ChannelConfigInstanceVirtualAxis(this, *this);
    channelInstanceCreated(instance);
    return instance;
}

void ChannelConfigInstanceVirtualAxis::allInstancesInitialized(QmlObjectListModel* configInstances)
{
    for (int i = 0; i < configInstances->count(); ++i) {
        auto channelConfigInstance = configInstances->value<ChannelConfigInstance*>(i);
        if (channelConfigInstance->channelConfig()->function() == Function::AxisX) {
            _axes[0] = channelConfigInstance;
        } else if (channelConfigInstance->channelConfig()->function() == Function::AxisY) {
            _axes[1] = channelConfigInstance;
        } else if (channelConfigInstance->channelConfig()->function() == Function::AxisZ) {
            _axes[2] = channelConfigInstance;
        }
    }
    Q_ASSERT(_axes[0] && _axes[1] && _axes[2]);

    for (int i = 0; i < 3; ++i) {
        if (!_axes[i]->fact())
            return;
    }

    // Initialize fact
    QStringList enumStrings{tr("Custom"), tr("Upwards"), tr("Downwards"), tr("Forwards"), tr("Backwards"),
        tr("Leftwards"), tr("Rightwards")};
    QVariantList enumValues{0, 1, 2, 3, 4, 5, 6};
    FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
    metaData->setEnumInfo(enumStrings, enumValues);
    _fact = new Fact("", metaData, this);
    setFactFromAxes();

    connect(_fact, &Fact::rawValueChanged, this, &ChannelConfigInstanceVirtualAxis::setAxesFromFact);
    for (int i=0; i < 3; ++i) {
        connect(_axes[i]->fact(), &Fact::rawValueChanged,
                this, [this](){ ChannelConfigInstanceVirtualAxis::setFactFromAxes(true); });
    }
    // Inherit visibility & enabled from the first axis
    connect(_axes[0], &ChannelConfigInstance::visibleChanged,
            this, &ChannelConfigInstanceVirtualAxis::axisVisibleChanged);
    connect(_axes[0], &ChannelConfigInstance::enabledChanged,
            this, &ChannelConfigInstanceVirtualAxis::axisEnableChanged);
    axisVisibleChanged();
    axisEnableChanged();
}

void ChannelConfigInstanceVirtualAxis::axisVisibleChanged()
{
    if (_axes[0]->visibleRule() != visibleRule()) {
        setVisibleRule(_axes[0]->visibleRule());
    }
}

void ChannelConfigInstanceVirtualAxis::axisEnableChanged()
{
    if (_axes[0]->enabledRule() != enabledRule()) {
        setEnabledRule(_axes[0]->enabledRule());
    }
}

void ChannelConfigInstanceVirtualAxis::setFactFromAxes(bool keepVisible)
{
    if (_ignoreChange) {
        return;
    }
    _ignoreChange = true;
    float x = _axes[0]->fact()->rawValue().toFloat();
    float y = _axes[1]->fact()->rawValue().toFloat();
    float z = _axes[2]->fact()->rawValue().toFloat();
    Direction direction{Direction::Custom}; // set to custom if no match
    const float eps = 0.00001f;
    if (fabsf(x) < eps && fabsf(y) < eps && fabsf(z + 1.f) < eps) {
        direction = Direction::Upwards;
    } else if (fabsf(x) < eps && fabsf(y) < eps && fabsf(z - 1.f) < eps) {
        direction = Direction::Downwards;
    } else if (fabsf(x - 1.f) < eps && fabsf(y) < eps && fabsf(z) < eps) {
        direction = Direction::Forwards;
    } else if (fabsf(x + 1.f) < eps && fabsf(y) < eps && fabsf(z) < eps) {
        direction = Direction::Backwards;
    } else if (fabsf(x) < eps && fabsf(y + 1.f) < eps && fabsf(z) < eps) {
        direction = Direction::Leftwards;
    } else if (fabsf(x) < eps && fabsf(y - 1.f) < eps && fabsf(z) < eps) {
        direction = Direction::Rightwards;
    }
    _fact->setRawValue((uint32_t)direction);

    bool visible = direction == Direction::Custom || keepVisible;
    for(int i=0; i < 3; ++i) {
        _axes[i]->setVisibleAxis(visible);
    }
    _ignoreChange = false;
}

void ChannelConfigInstanceVirtualAxis::setAxesFromFact()
{
    if (_ignoreChange) {
        return;
    }
    _ignoreChange = true;
    int directionIdx = _fact->rawValue().toInt();

    if (directionIdx > 0) {
        Direction direction = (Direction)directionIdx;
        float x{}, y{}, z{};
        switch (direction) {
            case Direction::Upwards:
                x = 0.f; y = 0.f; z = -1.f;
                break;
            case Direction::Downwards:
                x = 0.f; y = 0.f; z = 1.f;
                break;
            case Direction::Forwards:
                x = 1.f; y = 0.f; z = 0.f;
                break;
            case Direction::Backwards:
                x = -1.f; y = 0.f; z = 0.f;
                break;
            case Direction::Leftwards:
                x = 0.f; y = -1.f; z = 0.f;
                break;
            case Direction::Rightwards:
                x = 0.f; y = 1.f; z = 0.f;
                break;
            case Direction::Custom:
                break;
        }
        _axes[0]->fact()->setRawValue(x);
        _axes[1]->fact()->setRawValue(y);
        _axes[2]->fact()->setRawValue(z);
    }


    bool visible = directionIdx == 0;
    for(int i=0; i < 3; ++i) {
        _axes[i]->setVisibleAxis(visible);
    }
    _ignoreChange = false;
}

MixerChannel::MixerChannel(QObject *parent, const QString &label, int actuatorFunction, int paramIndex, int actuatorTypeIndex,
        QmlObjectListModel &channelConfigs, ParameterManager* parameterManager, const Rule* rule,
        std::function<void(Function, Fact*)> factAddedCb)
    : QObject(parent), _label(label), _actuatorFunction(actuatorFunction), _paramIndex(paramIndex),
      _actuatorTypeIndex(actuatorTypeIndex), _rule(rule)
{
    for (int i = 0; i < channelConfigs.count(); ++i) {
        auto channelConfig = channelConfigs.value<ChannelConfig*>(i);

        ChannelConfigInstance* instance = channelConfig->instantiate(paramIndex, actuatorTypeIndex, parameterManager, factAddedCb);
        Fact* fact = instance->fact();

        // if we have a valid rule, check the identifiers
        if (rule) {
            if (channelConfig->identifier() == rule->selectIdentifier) {
                _ruleSelectIdentifierIdx = _configInstances->count();
                if (fact) {
                    _currentSelectIdentifierValue = fact->rawValue().toInt();
                }
            } else {
                for (int i = 0; i < rule->applyIdentifiers.size(); ++i) {
                    if (channelConfig->identifier() == rule->applyIdentifiers[i]) {
                        instance->setRuleApplyIdentifierIdx(i);
                    }
                }
            }

            if (fact) {
                connect(fact, &Fact::rawValueChanged, this, [this]() { applyRule(); });
            }
        }

        _configInstances->append(instance);
    }

    for (int i = 0; i < _configInstances->count(); ++i) {
        auto channelConfigInstance = _configInstances->value<ChannelConfigInstance*>(i);
        channelConfigInstance->allInstancesInitialized(_configInstances);
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
                    configInstance->setVisibleRule(!ruleItem.hidden);
                    configInstance->setEnabledRule(!ruleItem.disabled);
                }
            }

        } else {
            // no rule set for this value, just reset
            for (int i = 0; i < _configInstances->count(); ++i) {
                ChannelConfigInstance* configInstance = _configInstances->value<ChannelConfigInstance*>(i);
                configInstance->setVisibleRule(true);
                configInstance->setEnabledRule(true);
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

Fact* MixerChannel::getFact([[maybe_unused]] Function function) const
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
            int axisIdx[3]{-1, -1, -1};
            for (const auto& perItemParam : actuatorGroup.perItemParameters) {
                currentMixerGroup->addChannelConfig(new ChannelConfig(currentMixerGroup, perItemParam, false));

                if (perItemParam.function == Function::AxisX) {
                    axisIdx[0] = currentMixerGroup->channelConfigs()->count() - 1;
                } else if (perItemParam.function == Function::AxisY) {
                    axisIdx[1] = currentMixerGroup->channelConfigs()->count() - 1;
                } else if (perItemParam.function == Function::AxisZ) {
                    axisIdx[2] = currentMixerGroup->channelConfigs()->count() - 1;
                }

                if (!perItemParam.identifier.isEmpty()) {
                    for (const auto& rule : _rules) {
                        if (rule.selectIdentifier == perItemParam.identifier) {
                            selectedRule = &rule;
                        }
                    }
                }
            }

            // Add virtual axis dropdown configuration param if all 3 axes are found
            if (axisIdx[0] >= 0 && axisIdx[1] >= 0 && axisIdx[2] >= 0) {
                ChannelConfig* axisXConfig = currentMixerGroup->channelConfigs()->value<ChannelConfig*>(axisIdx[0]);
                MixerParameter parameter = axisXConfig->config(); // use axisX as base (somewhat arbitrary)
                parameter.function = Function::Unspecified;
                parameter.param.name = "";
                parameter.param.label = tr("Axis");
                parameter.identifier = "";
                ChannelConfig* virtualChannelConfig = new ChannelConfigVirtualAxis(currentMixerGroup, parameter);
                currentMixerGroup->channelConfigs()->insert(axisIdx[0], virtualChannelConfig);
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
                        _functionsSpecificLabel[actuatorFunction] = itemLabelPrefix;
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
    Fact* typeFact = nullptr;
    for (int mixerGroupIdx = 0; !typeFact && mixerGroupIdx < _groups->count(); ++mixerGroupIdx) {
        Mixer::MixerConfigGroup* mixerGroup = _groups->value<Mixer::MixerConfigGroup*>(mixerGroupIdx);
        for (int mixerChannelIdx = 0; !typeFact && mixerChannelIdx < mixerGroup->channels()->count(); ++mixerChannelIdx) {
            Mixer::MixerChannel* mixerChannel = mixerGroup->channels()->value<Mixer::MixerChannel*>(mixerChannelIdx);

            if (mixerChannel->actuatorFunction() != function) {
                continue;
            }

            typeFact = mixerChannel->getFact(Function::Type);
        }
    }
    if (typeFact) {
        // Now check if we have multiple functions configured with the same type.
        // If so, add the function label to disambiguate
        for (int mixerGroupIdx = 0; mixerGroupIdx < _groups->count(); ++mixerGroupIdx) {
            Mixer::MixerConfigGroup* mixerGroup = _groups->value<Mixer::MixerConfigGroup*>(mixerGroupIdx);
            for (int mixerChannelIdx = 0; mixerChannelIdx < mixerGroup->channels()->count(); ++mixerChannelIdx) {
                Mixer::MixerChannel* mixerChannel = mixerGroup->channels()->value<Mixer::MixerChannel*>(mixerChannelIdx);

                if (mixerChannel->actuatorFunction() == function) {
                    continue;
                }
                Fact* typeFactOther = mixerChannel->getFact(Function::Type);
                if (typeFactOther && typeFactOther->rawValue() == typeFact->rawValue()) {
                    return typeFact->enumOrValueString() + " (" + _functions.value(function).label +")";
                }
            }
        }
        return typeFact->enumOrValueString();
    }

    const auto iter = _functionsSpecificLabel.find(function);
    if (iter == _functionsSpecificLabel.end()) {
        return _functions.value(function).label;
    }
    return *iter;
}

QSet<int> Mixers::getFunctions(bool requiredOnly) const
{
    QSet<int> ret;
    for (int mixerGroupIdx = 0; mixerGroupIdx < _groups->count(); ++mixerGroupIdx) {
        Mixer::MixerConfigGroup* mixerGroup = _groups->value<Mixer::MixerConfigGroup*>(mixerGroupIdx);
        if (!requiredOnly || mixerGroup->group().required) {
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

QString Mixers::title() const
{
    if (_selectedMixer == -1) {
        return "";
    }
    return _mixerOptions[_selectedMixer].title;
}
QString Mixers::helpUrl() const
{
    if (_selectedMixer == -1) {
        return "";
    }
    return _mixerOptions[_selectedMixer].helpUrl;
}

Fact* Mixers::getFact(const QString& paramName)
{
    if (!_parameterManager->parameterExists(ParameterManager::defaultComponentId, paramName)) {
        qCDebug(ActuatorsConfigLog) << "Mixers: Param does not exist:" << paramName;
        return nullptr;
    }
    Fact* fact = _parameterManager->getParameter(ParameterManager::defaultComponentId, paramName);
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
