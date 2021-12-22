/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Actuators.h"

#include <QString>
#include <QFile>
#include <QtGlobal>
#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>

using namespace ActuatorOutputs;

Actuators::Actuators(QObject* parent, Vehicle* vehicle)
    : QObject(parent), _actuatorTest(vehicle), _mixer(vehicle->parameterManager()),
      _motorAssignment(nullptr, vehicle, _actuatorOutputs), _vehicle(vehicle)
{
    connect(&_mixer, &Mixer::Mixers::paramChanged, this, &Actuators::parametersChanged);
    connect(&_mixer, &Mixer::Mixers::geometryParamChanged, this, &Actuators::updateGeometryImage);
    qRegisterMetaType<Actuators*>("Actuators*");
    connect(&_motorAssignment, &MotorAssignment::activeChanged, this, &Actuators::motorAssignmentActiveChanged);
    connect(&_motorAssignment, &MotorAssignment::messageChanged, this, &Actuators::motorAssignmentMessageChanged);
    connect(&_motorAssignment, &MotorAssignment::onAbort, this, [this]() { highlightActuators(false); });
}

void Actuators::imageClicked(float x, float y)
{
    GeometryImage::VehicleGeometryImageProvider* provider = GeometryImage::VehicleGeometryImageProvider::instance();
    int motorIndex = provider->getHighlightedMotorIndexAtPos(QPointF{ x, y });
    qCDebug(ActuatorsConfigLog) << "Image clicked:" << x << "," << y << "motor index:" << motorIndex;

    if (_motorAssignment.active()) {
        QList<ActuatorGeometry>& actuators = provider->actuators();
        bool found = false;
        for (auto& actuator : actuators) {
            if (actuator.type == ActuatorGeometry::Type::Motor && actuator.index == motorIndex) {
                actuator.renderOptions.highlight = false;
                found = true;
            }
        }
        updateGeometryImage();

        if (found) {
            // call this outside of the loop as it might lead to an actuator refresh
            _motorAssignment.selectMotor(motorIndex);
        }
    }
}

void Actuators::selectActuatorOutput(int index)
{
    if (index >= _actuatorOutputs->count() || index < 0) {
        index = 0;
    }
    _selectedActuatorOutput = index;
    emit selectedActuatorOutputChanged();
}
ActuatorOutput* Actuators::selectedActuatorOutput() const
{
    if (_actuatorOutputs->count() == 0) {
        return nullptr;
    }
    return _actuatorOutputs->value<ActuatorOutputs::ActuatorOutput*>(_selectedActuatorOutput);
}

void Actuators::updateGeometryImage()
{
    GeometryImage::VehicleGeometryImageProvider* provider = GeometryImage::VehicleGeometryImageProvider::instance();

    QList<ActuatorGeometry>& actuators = provider->actuators();
    QList<ActuatorGeometry> previousActuators = actuators;

    // collect the actuators
    actuators.clear();
    for (int mixerGroupIdx = 0; mixerGroupIdx < _mixer.groups()->count(); ++mixerGroupIdx) {
        Mixer::MixerConfigGroup* mixerGroup = _mixer.groups()->value<Mixer::MixerConfigGroup*>(mixerGroupIdx);
        for (int mixerChannelIdx = 0; mixerChannelIdx < mixerGroup->channels()->count(); ++mixerChannelIdx) {
            const Mixer::MixerChannel* mixerChannel = mixerGroup->channels()->value<Mixer::MixerChannel*>(mixerChannelIdx);
            ActuatorGeometry geometry{};
            if (mixerChannel->getGeometry(_mixer.actuatorTypes(), mixerGroup->group(), geometry)) {
                actuators.append(geometry);
                qCDebug(ActuatorsConfigLog) << "Airframe actuator:" << geometry.index << "pos:" << geometry.position;
            }
        }
    }

    // restore render options if actuators did not change
    if (previousActuators.size() == actuators.size()) {
        for (int i = 0; i < actuators.size(); ++i) {
            if (previousActuators[i].type == actuators[i].type && previousActuators[i].index == actuators[i].index) {
                actuators[i].renderOptions = previousActuators[i].renderOptions;
            }
        }
    }

    _imageRefreshFlag = !_imageRefreshFlag;
    emit imageRefreshFlagChanged();
}

bool Actuators::isMultirotor() const
{
    return _mixer.configuredType() == "multirotor";
}

void Actuators::load(const QString &json_file)
{
    QFile file;
    file.setFileName(json_file);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json_data = file.readAll();
    file.close();

    // store the metadata to be loaded later after all params are available
    _jsonMetadata = QJsonDocument::fromJson(json_data.toUtf8());
}

void Actuators::init()
{
    if (_init) {
        return;
    }

    if (!_vehicle->parameterManager()->parametersReady()) {
        qWarning() << "Incorrect calling order, parameters not yet ready";
    }

    if (!parseJson(_jsonMetadata)) {
        return;
    }
    _jsonMetadata = {};

    // Remove groups that have no enable param and none of the function params is available
    for (int groupIdx = 0; groupIdx < _actuatorOutputs->count(); groupIdx++) {
        ActuatorOutput* group = qobject_cast<ActuatorOutput*>(_actuatorOutputs->get(groupIdx));
        if (!group->enableParam() && !group->hasExistingOutputFunctionParams()) {
            qCDebug(ActuatorsConfigLog) << "Removing actuator group w/o function parameters at" << groupIdx;
            _actuatorOutputs->removeAt(groupIdx);
            delete group;
            --groupIdx;
        }
    }

    emit actuatorOutputsChanged();
    parametersChanged();
}

void Actuators::parametersChanged()
{
    qCDebug(ActuatorsConfigLog) << "Param update";

    _mixer.update();

    // gather all enabled functions
    QList<int> allFunctions;
    for (int groupIdx = 0; groupIdx < _actuatorOutputs->count(); groupIdx++) {
        ActuatorOutput* group = qobject_cast<ActuatorOutput*>(_actuatorOutputs->get(groupIdx));
        group->clearNotes();
        QList<Fact*> groupFunctions;
        group->getAllChannelFunctions(groupFunctions);
        for (const auto& groupFunction : groupFunctions) {
            int function = groupFunction->rawValue().toInt();
            if (function != 0) { // disabled
                allFunctions.append(function);
            }

            // update notes for configured functions
            const auto iter = _mixer.functions().find(function);
            if (iter != _mixer.functions().end() && iter->note != "") {
                if (iter->noteCondition.evaluate()) {
                    qCDebug(ActuatorsConfigLog) << "Showing Note:" << iter->note;
                    group->addNote(iter->note);
                }
            }
        }

        // update channel visibility
        for (int subbroupIdx = 0; subbroupIdx < group->subgroups()->count(); subbroupIdx++) {
            ActuatorOutputSubgroup* subgroup = qobject_cast<ActuatorOutputSubgroup*>(group->subgroups()->get(subbroupIdx));
            for (int channelIdx = 0; channelIdx < subgroup->channelConfigs()->count(); channelIdx++) {
                ChannelConfig* channel = qobject_cast<ChannelConfig*>(subgroup->channelConfigs()->get(channelIdx));
                channel->reevaluate();
            }
        }
    }

    std::sort(allFunctions.begin(), allFunctions.end());

    // create list of actuators from configured functions
    QList<ActuatorTesting::Actuator*> actuators;
    QSet<int> uniqueConfiguredFunctions;
    const Mixer::ActuatorTypes &actuatorTypes = _mixer.actuatorTypes();
    for (int function : allFunctions) {
        if (uniqueConfiguredFunctions.find(function) != uniqueConfiguredFunctions.end()) { // only add once
            continue;
        }
        uniqueConfiguredFunctions.insert(function);
        QString label = _mixer.getSpecificLabelForFunction(function);

        // check if we should exclude the function from testing
        bool excludeFromActuatorTesting = false;
        const auto iter = _mixer.functions().find(function);
        if (iter != _mixer.functions().end()) {
            excludeFromActuatorTesting = iter->excludeFromActuatorTesting;
        }

        // find actuator
        if (!excludeFromActuatorTesting) {
            bool found = false;
            for (const auto& actuatorTypeName : actuatorTypes.keys()) {
                const Mixer::ActuatorType& actuatorType = actuatorTypes[actuatorTypeName];
                if (function >= actuatorType.functionMin && function <= actuatorType.functionMax) {
                    bool isMotor = ActuatorGeometry::typeFromStr(actuatorTypeName) == ActuatorGeometry::Type::Motor;
                    actuators.append(
                            new ActuatorTesting::Actuator(&_actuatorTest, label, actuatorType.values.min, actuatorType.values.max,
                                    actuatorType.values.defaultVal, function, isMotor));
                    found = true;
                    break;
                }
            }
            if (!found && actuatorTypes.find("DEFAULT") != actuatorTypes.end()) {
                const Mixer::ActuatorType& actuatorType = actuatorTypes["DEFAULT"];
                actuators.append(
                        new ActuatorTesting::Actuator(&_actuatorTest, label, actuatorType.values.min, actuatorType.values.max,
                                actuatorType.values.defaultVal, function, false));
            }
        }
    }
    _actuatorTest.updateFunctions(actuators);

    // check if there are required functions, but not set on any output
    QSet<int> requiredFunctions = _mixer.requiredFunctions();
    _hasUnsetRequiredFunctions = false;
    for (int requiredFunction : requiredFunctions) {
        if (uniqueConfiguredFunctions.find(requiredFunction) == uniqueConfiguredFunctions.end()) {
            _hasUnsetRequiredFunctions = true;
        }
    }
    emit hasUnsetRequiredFunctionsChanged();

    updateActuatorActions();

    updateGeometryImage();
}

void Actuators::updateActuatorActions()
{
    _actuatorActions->clearAndDeleteContents();
    QSet<int> addedFunctions;
    for (int groupIdx = 0; groupIdx < _actuatorOutputs->count(); groupIdx++) {
        ActuatorOutput* group = qobject_cast<ActuatorOutput*>(_actuatorOutputs->get(groupIdx));

        group->forEachOutputFunction([&](ActuatorOutputSubgroup* subgroup, ChannelConfigInstance*, Fact* fact) {
            int outputFunctionVal = fact->rawValue().toInt();
            if (outputFunctionVal != 0 && !addedFunctions.contains(outputFunctionVal)) {
                auto outputFunctionIter = _mixer.functions().find(outputFunctionVal);
                if (outputFunctionIter != _mixer.functions().end()) {
                    const Mixer::Mixers::OutputFunction& outputFunction = outputFunctionIter.value();
                    for (const auto& action : subgroup->actions()) {
                        if (!action.condition.evaluate()) {
                            continue;
                        }
                        if (!action.actuatorTypes.empty() && action.actuatorTypes.find(outputFunction.actuatorType) == action.actuatorTypes.end()) {
                            continue;
                        }

                        // add the action
                        auto actuatorAction = new ActuatorActions::Action(this, action, outputFunction.label, outputFunctionVal, _vehicle);
                        ActuatorActions::ActionGroup* actionGroup = nullptr;
                        // try to find the group
                        for (int groupIdx = 0; groupIdx < _actuatorActions->count(); groupIdx++) {
                            ActuatorActions::ActionGroup* curActionGroup =
                                    qobject_cast<ActuatorActions::ActionGroup*>(_actuatorActions->get(groupIdx));
                            if (curActionGroup->type() == action.type) {
                                actionGroup = curActionGroup;
                                break;
                            }
                        }

                        if (!actionGroup) {
                            QString groupLabel = action.typeToLabel();
                            actionGroup = new ActuatorActions::ActionGroup(this, groupLabel, action.type);
                            _actuatorActions->append(actionGroup);
                        }
                        actionGroup->addAction(actuatorAction);
                        addedFunctions.insert(outputFunctionVal);
                    }
                }
            }
        });
    }

    emit actuatorActionsChanged();
}

bool Actuators::parseJson(const QJsonDocument &json)
{
    _actuatorOutputs->clearAndDeleteContents();

    QJsonObject obj = json.object();
    QJsonValue outputsJson = obj.value("outputs_v1");
    QJsonValue functionsJson = obj.value("functions_v1");
    QJsonValue mixerJson = obj.value("mixer_v1");
    if (outputsJson.isNull() || functionsJson.isNull() || mixerJson.isNull()) {
        qCWarning(ActuatorsConfigLog) << "Missing json section:" << outputsJson << "\n" << functionsJson << "\n" << mixerJson;
        return false;
    }

    // parse outputs
    QJsonArray outputs = outputsJson.toArray();
    for (const auto &outputJson : outputs) {
        QJsonValue output = outputJson.toObject();
        QString label = output["label"].toString();

        qCDebug(ActuatorsConfigLog) << "Actuator group:" << label;

        Condition groupVisibilityCondition(output["show-subgroups-if"].toString(""), _vehicle->parameterManager());
        subscribeFact(groupVisibilityCondition.fact());

        ActuatorOutput* currentActuatorOutput = new ActuatorOutput(this, label, groupVisibilityCondition);
        _actuatorOutputs->append(currentActuatorOutput);

        auto parseParam = [&currentActuatorOutput, this](const QJsonValue &parameter) {
            Parameter param{};
            param.parse(parameter);
            QString functionStr = parameter["function"].toString("");
            qCDebug(ActuatorsConfigLog) << "param:" << param.name << "label:" << param.label << "function:" << functionStr;
            ConfigParameter::Function function = ConfigParameter::Function::Unspecified;
            if (functionStr == "enable") {
                function = ConfigParameter::Function::Enable;
            } else if (functionStr == "primary") {
                function = ConfigParameter::Function::Primary;
            } else if (functionStr != "") {
                qCWarning(ActuatorsConfigLog) << "Unknown function " << functionStr << "for param" << param.name;
            }
            return new ConfigParameter(currentActuatorOutput, getFact(param.name), param.label, function);
        };

        QJsonArray parameters = output["parameters"].toArray();
        for (const auto& parameterJson : parameters) {
            currentActuatorOutput->addConfigParam(parseParam(parameterJson.toObject()));
        }

        QJsonArray subgroups = output["subgroups"].toArray();
        for (const auto& subgroupJson : subgroups) {
            QJsonValue subgroup = subgroupJson.toObject();
            QString subgroupLabel = subgroup["label"].toString();
            ActuatorOutputSubgroup* actuatorSubgroup = new ActuatorOutputSubgroup(this, subgroupLabel);
            currentActuatorOutput->addSubgroup(actuatorSubgroup);

            QJsonValue supportedActions = subgroup["supported-actions"];
            if (!supportedActions.isNull()) {
                QJsonObject supportedActionsObj = supportedActions.toObject();
                for (const auto& actionName : supportedActionsObj.keys()) {
                    QJsonObject actionObj = supportedActionsObj.value(actionName).toObject();
                    ActuatorActions::Config action{};
                    bool knownAction = true;
                    if (actionName == "beep") {
                        action.type = ActuatorActions::Config::Type::beep;
                    } else if (actionName == "3d-mode-on") {
                        action.type = ActuatorActions::Config::Type::set3DModeOn;
                    } else if (actionName == "3d-mode-off") {
                        action.type = ActuatorActions::Config::Type::set3DModeOff;
                    } else if (actionName == "set-spin-direction1") {
                        action.type = ActuatorActions::Config::Type::setSpinDirection1;
                    } else if (actionName == "set-spin-direction2") {
                        action.type = ActuatorActions::Config::Type::setSpinDirection2;
                    } else {
                        knownAction = false;
                        qCWarning(ActuatorsConfigLog) << "Unknown 'supported-actions':" << actionName;
                    }
                    if (knownAction) {
                        QJsonArray actuatorTypesArr = actionObj["actuator-types"].toArray();
                        for (const auto &type : actuatorTypesArr) {
                            action.actuatorTypes.insert(type.toString());
                        }
                        action.condition = Condition(actionObj["supported-if"].toString(), _vehicle->parameterManager());
                        subscribeFact(action.condition.fact());
                        actuatorSubgroup->addAction(action);
                    }
                }
            }

            QJsonArray parameters = subgroup["parameters"].toArray();
            for (const auto& parameterJson : parameters) {
                actuatorSubgroup->addConfigParam(parseParam(parameterJson.toObject()));
            }

            QJsonArray channelParameters = subgroup["per-channel-parameters"].toArray();
            for (const auto& channelParametersJson : channelParameters) {
                QJsonValue channelParameter = channelParametersJson.toObject();
                Parameter param;
                param.parse(channelParameter);

                ChannelConfig::Function function = ChannelConfig::Function::Unspecified;
                QString functionStr = channelParameter["function"].toString("");
                if (functionStr == "function") {
                    function = ChannelConfig::Function::OutputFunction;
                } else if (functionStr == "disarmed") {
                    function = ChannelConfig::Function::Disarmed;
                } else if (functionStr == "min") {
                    function = ChannelConfig::Function::Minimum;
                } else if (functionStr == "max") {
                    function = ChannelConfig::Function::Maximum;
                } else if (functionStr == "failsafe") {
                    function = ChannelConfig::Function::Failsafe;
                } else if (functionStr != "") {
                    qCWarning(ActuatorsConfigLog) << "Unknown 'function':" << functionStr;
                }

                Condition visibilityCondition(channelParameter["show-if"].toString(""), _vehicle->parameterManager());
                subscribeFact(visibilityCondition.fact());

                qCDebug(ActuatorsConfigLog) << "per-channel-param:" << param.label << "param:" << param.name;
                actuatorSubgroup->addChannelConfig(new ChannelConfig(this, param, function, visibilityCondition));
            }

            QJsonArray channels = subgroup["channels"].toArray();
            for (const auto& channelJson : channels) {
                QJsonValue channel = channelJson.toObject();
                QString channelLabel = channel["label"].toString();
                int paramIndex = channel["param-index"].toInt();
                qCDebug(ActuatorsConfigLog) << "channel label:" << channelLabel << "param-index" << paramIndex;
                actuatorSubgroup->addChannel(
                        new ActuatorOutputChannel(this, channelLabel, paramIndex, *actuatorSubgroup->channelConfigs(),
                                _vehicle->parameterManager(), [this](Fact* fact) { subscribeFact(fact); }));
            }
        }
    }

    _showUi = Condition(obj.value("show-ui-if").toString(""), _vehicle->parameterManager());

    // parse functions
    QMap<int, Mixer::Mixers::OutputFunction> outputFunctions;
    QJsonObject functions = functionsJson.toObject();
    for (const auto& functionKey : functions.keys()) {
        bool ok;
        int key = functionKey.toInt(&ok);
        if (ok) {
            QJsonObject functionObj = functions.value(functionKey).toObject();
            QString label = functionObj["label"].toString();
            if (label != "") {
                QJsonObject noteObj = functionObj["note"].toObject();
                QString note = noteObj["text"].toString();
                QString condition = noteObj["condition"].toString();
                QString noteCondition = functionObj["label"].toString();
                bool exclude = functionObj["exclude-from-actuator-testing"].toBool(false);
                Condition conditionObj{condition, _vehicle->parameterManager()};
                subscribeFact(conditionObj.fact());
                outputFunctions[key] = Mixer::Mixers::OutputFunction{label, conditionObj, note, exclude};
            }
        }
    }
    qCDebug(ActuatorsConfigLog) << "functions:" << outputFunctions;

    Mixer::ActuatorTypes actuatorTypes;
    // parse mixer
    QJsonObject actuatorTypesJson = mixerJson.toObject().value("actuator-types").toObject();
    for (const auto& actuatorTypeName : actuatorTypesJson.keys()) {
        QJsonValue actuatorTypeVal = actuatorTypesJson.value(actuatorTypeName).toObject();
        Mixer::ActuatorType actuatorType{};
        actuatorType.functionMin = actuatorTypeVal["function-min"].toInt();
        actuatorType.functionMax = actuatorTypeVal["function-max"].toInt();
        actuatorType.labelIndexOffset = actuatorTypeVal["label-index-offset"].toInt(0);
        QJsonValue values = actuatorTypeVal["values"].toObject();
        actuatorType.values.min = values["min"].toDouble();
        actuatorType.values.max = values["max"].toDouble();
        actuatorType.values.defaultVal = values["default"].toDouble();
        if (values["default-is-nan"].toBool()) {
            actuatorType.values.defaultVal = NAN;
        }
        actuatorType.values.reversible = values["reversible"].toBool();

        QJsonArray perItemParametersJson = actuatorTypeVal["per-item-parameters"].toArray();
        for (const auto& perItemParameterJson : perItemParametersJson) {
            QJsonValue perItemParameter = perItemParameterJson.toObject();
            Parameter param{};
            param.parse(perItemParameter);
            actuatorType.perItemParams.append(param);
        }
        actuatorTypes[actuatorTypeName] = actuatorType;
    }

    // fill in the actuator types
    auto actuatorTypeIter = actuatorTypes.constBegin();
    while (actuatorTypeIter != actuatorTypes.constEnd()) {
        if (actuatorTypeIter.key() != "DEFAULT") {
            for (int function = actuatorTypeIter.value().functionMin; function <= actuatorTypeIter.value().functionMax; ++function) {
                auto functionIter = outputFunctions.find(function);
                if (functionIter != outputFunctions.end()) {
                    functionIter->actuatorType = actuatorTypeIter.key();
                }
            }
        }
        ++actuatorTypeIter;
    }

    Mixer::MixerOptions mixerOptions{};
    QJsonValue mixerConfigJson = mixerJson.toObject().value("config");
    QJsonArray mixerConfigJsonArr = mixerConfigJson.toArray();
    for (const auto& mixerConfigJson : mixerConfigJsonArr) {
        QJsonValue mixerConfig = mixerConfigJson.toObject();
        Mixer::MixerOption option{};
        option.option = mixerConfig["option"].toString();
        option.type = mixerConfig["type"].toString();
        QJsonArray actuatorsJson = mixerConfig["actuators"].toArray();
        for (const auto& actuatorJson : actuatorsJson) {
            QJsonValue actuatorJsonVal = actuatorJson.toObject();
            Mixer::MixerOption::ActuatorGroup actuator{};
            actuator.groupLabel = actuatorJsonVal["group-label"].toString();
            if (actuatorJsonVal["count"].isString()) {
                actuator.count = actuatorJsonVal["count"].toString();
            } else {
                actuator.fixedCount = actuatorJsonVal["count"].toInt();
            }
            actuator.actuatorType = actuatorJsonVal["actuator-type"].toString();
            actuator.required = actuatorJsonVal["required"].toBool(false);
            QJsonArray parametersJson = actuatorJsonVal["parameters"].toArray();
            for (const auto& parameterJson : parametersJson) {
                QJsonValue parameter = parameterJson.toObject();
                Parameter mixerParameter{};
                mixerParameter.parse(parameter);
                actuator.parameters.append(mixerParameter);
            }

            QJsonArray perItemParametersJson = actuatorJsonVal["per-item-parameters"].toArray();
            for (const auto& parameterJson : perItemParametersJson) {
                QJsonValue parameter = parameterJson.toObject();
                Mixer::MixerParameter mixerParameter{};
                mixerParameter.param.parse(parameter);
                mixerParameter.identifier = parameter["identifier"].toString();
                QString function = parameter["function"].toString();
                if (function == "posx") {
                    mixerParameter.function = Mixer::Function::PositionX;
                } else if (function == "posy") {
                    mixerParameter.function = Mixer::Function::PositionY;
                } else if (function == "posz") {
                    mixerParameter.function = Mixer::Function::PositionZ;
                } else if (function == "spin-dir") {
                    mixerParameter.function = Mixer::Function::SpinDirection;
                } else if (function == "axisx") {
                    mixerParameter.function = Mixer::Function::AxisX;
                } else if (function == "axisy") {
                    mixerParameter.function = Mixer::Function::AxisY;
                } else if (function == "axisz") {
                    mixerParameter.function = Mixer::Function::AxisZ;
                } else if (function == "type") {
                    mixerParameter.function = Mixer::Function::Type;
                } else if (function != "") {
                    qCWarning(ActuatorsConfigLog) << "Unknown param function:" << function;
                }
                // check if not configurable: in that case we expect a list of values
                bool invalid = false;
                if (mixerParameter.param.name == "") {
                    QJsonArray valuesJson = parameter["value"].toArray();
                    for (const auto& valueJson : valuesJson) {
                        mixerParameter.values.append(valueJson.toDouble());
                    }

                    if (actuator.fixedCount != mixerParameter.values.size() && mixerParameter.values.size() != 1) {
                        invalid = true;
                        qCWarning(ActuatorsConfigLog) << "Invalid mixer param config:" << actuator.fixedCount << "," << mixerParameter.values.size();
                    }
                }
                if (!invalid) {
                    actuator.perItemParameters.append(mixerParameter);
                }
            }

            if (actuatorJsonVal["item-label-prefix"].isString()) {
                actuator.itemLabelPrefix.append(actuatorJsonVal["item-label-prefix"].toString());
            } else {
                QJsonArray itemLabelPrefixJson = actuatorJsonVal["item-label-prefix"].toArray();
                for (const auto& itemLabelPrefix : itemLabelPrefixJson) {
                    actuator.itemLabelPrefix.append(itemLabelPrefix.toString());
                }
                if (actuator.fixedCount != actuator.itemLabelPrefix.size() && actuator.itemLabelPrefix.size() > 1) {
                    qCWarning(ActuatorsConfigLog) << "Invalid mixer config (item-label-prefix):" << actuator.fixedCount << ","
                            << actuator.itemLabelPrefix.size();
                }
            }

            option.actuators.append(actuator);
        }
        mixerOptions.append(option);
    }

    QList<Mixer::Rule> rules;
    QJsonValue mixerRulesJson = mixerJson.toObject().value("rules");
    QJsonArray mixerRulesJsonArr = mixerRulesJson.toArray();
    for (const auto& mixerRuleJson : mixerRulesJsonArr) {
        QJsonValue mixerRule = mixerRuleJson.toObject();
        Mixer::Rule rule{};
        rule.selectIdentifier = mixerRule["select-identifier"].toString();

        QJsonArray identifiersJson = mixerRule["apply-identifiers"].toArray();
        for (const auto& identifierJson : identifiersJson) {
            rule.applyIdentifiers.append(identifierJson.toString());
        }

        QJsonObject itemsJson = mixerRule["items"].toObject();
        for (const auto& itemKey : itemsJson.keys()) {
            bool ok;
            int key = itemKey.toInt(&ok);
            if (ok) {
                QJsonArray itemsArr = itemsJson.value(itemKey).toArray();
                QList<Mixer::Rule::RuleItem> items{};
                for (const auto& itemJson : itemsArr) {
                    QJsonObject itemObj = itemJson.toObject();

                    Mixer::Rule::RuleItem item{};
                    if (itemObj.contains("min")) {
                        item.hasMin = true;
                        item.min = itemObj["min"].toDouble();
                    }
                    if (itemObj.contains("max")) {
                        item.hasMax = true;
                        item.max = itemObj["max"].toDouble();
                    }
                    if (itemObj.contains("default")) {
                        item.hasDefault = true;
                        item.defaultVal = itemObj["default"].toDouble();
                    }
                    item.hidden = itemObj["hidden"].toBool(false);
                    item.disabled = itemObj["disabled"].toBool(false);
                    items.append(item);
                }
                if (items.size() == rule.applyIdentifiers.size()) {
                    rule.items[key] = items;
                } else {
                    qCWarning(ActuatorsConfigLog) << "Rules: unexpected num items in " << itemsArr << "expected:" << rule.applyIdentifiers.size();
                }
            }
        }
        rules.append(rule);
    }

    _mixer.reset(actuatorTypes, mixerOptions, outputFunctions, rules);
    _init = true;
    return true;
}

Fact* Actuators::getFact(const QString& paramName)
{
    if (!_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, paramName)) {
        qCDebug(ActuatorsConfigLog) << "Mixer: Param does not exist:" << paramName;
        return nullptr;
    }
    Fact* fact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName);
	subscribeFact(fact);
	return fact;
}

void Actuators::subscribeFact(Fact* fact)
{
    if (fact && !_subscribedFacts.contains(fact)) {
        connect(fact, &Fact::rawValueChanged, this, &Actuators::parametersChanged);
        _subscribedFacts.insert(fact);
    }
}

bool Actuators::showUi() const
{
    return _init && _showUi.evaluate();
}

bool Actuators::initMotorAssignment()
{
    GeometryImage::VehicleGeometryImageProvider* provider = GeometryImage::VehicleGeometryImageProvider::instance();
    int numMotors = 0;
    QList<ActuatorGeometry>& actuators = provider->actuators();
    for (const auto& actuator : actuators) {
        if (actuator.type == ActuatorGeometry::Type::Motor) {
            ++numMotors;
        }
    }

    // get the minimum function for motors
    bool ret = false;
    auto iter = _mixer.actuatorTypes().find("motor");
    if (iter == _mixer.actuatorTypes().end()) {
        qWarning() << "Actuator type 'motor' not found";
    } else {
        ret = _motorAssignment.initAssignment(_selectedActuatorOutput, iter->functionMin, numMotors);
    }
    return ret;
}

void Actuators::highlightActuators(bool highlight)
{
    GeometryImage::VehicleGeometryImageProvider* provider = GeometryImage::VehicleGeometryImageProvider::instance();
    QList<ActuatorGeometry>& actuators = provider->actuators();
    for (auto& actuator : actuators) {
        if (actuator.type == ActuatorGeometry::Type::Motor) {
            actuator.renderOptions.highlight = highlight;
        }
    }
    updateGeometryImage();
}

void Actuators::startMotorAssignment()
{
    highlightActuators(true);
    _motorAssignment.start();
}
void Actuators::abortMotorAssignment()
{
    _motorAssignment.abort();
}
