#include "ActuatorOutputs.h"
#include "MAVLinkLib.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QHash>
#include <QtCore/QSet>

QGC_LOGGING_CATEGORY(ActuatorOutputsLog, "Vehicle.Actuators.ActuatorOutputs")

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
        if (parameterManager && parameterManager->parameterExists(ParameterManager::defaultComponentId, param)) {
            fact = parameterManager->getParameter(ParameterManager::defaultComponentId, param);
            if (channelConfig->displayOption() == Parameter::DisplayOption::Bitset) {
                fact = new FactBitset(channelConfig, fact, paramIndex + channelConfig->indexOffset());
            } else if (channelConfig->displayOption() == Parameter::DisplayOption::BoolTrueIfPositive) {
                fact = new FactFloatAsBool(channelConfig, fact);
            }
            factAddedCb(fact);
        } else {
            qCDebug(ActuatorOutputsLog) << "ActuatorOutputChannel: Param does not exist:" << param;
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

ActuatorChannelCell::ActuatorChannelCell(QObject *parent, ChannelConfigInstance *instance)
    : QObject(parent)
    , _instance(instance)
{
    if (_instance && _instance->channelConfig()) {
        connect(_instance->channelConfig(), &ChannelConfig::visibleChanged, this, &ActuatorChannelCell::visibleChanged);
    }
}

Fact* ActuatorChannelCell::fact()
{
    return _instance ? _instance->fact() : nullptr;
}

bool ActuatorChannelCell::visible() const
{
    return _instance && _instance->channelConfig() && _instance->channelConfig()->visible();
}

bool ActuatorChannelCell::advanced() const
{
    return _instance && _instance->channelConfig() && _instance->channelConfig()->advanced();
}

ActuatorChannelRow::ActuatorChannelRow(QObject *parent, ActuatorOutputChannel *channel, ConfigParameter *primaryParam,
        bool firstInGroup, int timerIndex, int gridRow, int headerGridRow, bool showTimerHeader,
        const QStringList &tableKeys)
    : QObject(parent)
    , _channel(channel)
    , _primaryParam(primaryParam)
    , _firstInGroup(firstInGroup)
    , _timerIndex(timerIndex)
    , _gridRow(gridRow)
    , _headerGridRow(headerGridRow)
    , _showTimerHeader(showTimerHeader)
{
    QHash<QString, ChannelConfigInstance*> byParam;
    if (_channel) {
        QmlObjectListModel *instances = _channel->configInstances();
        for (int i = 0; i < instances->count(); i++) {
            auto instance = qobject_cast<ChannelConfigInstance*>(instances->get(i));
            if (instance && instance->channelConfig()) {
                byParam.insert(instance->channelConfig()->parameter(), instance);
            }
        }
    }
    for (const QString &key : tableKeys) {
        _configInstances->append(new ActuatorChannelCell(this, byParam.value(key, nullptr)));
    }
}

void ActuatorOutput::rebuildChannelRows()
{
    _channelRows->clearAndDeleteContents();
    _tableChannelConfigs->clear();

    // Union by parameter name so subgroups with extra per-channel params still get a header.
    QStringList tableKeys;
    QSet<QString> seenKeys;
    for (int sgIdx = 0; sgIdx < _subgroups->count(); sgIdx++) {
        ActuatorOutputSubgroup *subgroup = qobject_cast<ActuatorOutputSubgroup*>(_subgroups->get(sgIdx));
        if (!subgroup) {
            continue;
        }
        for (int cfgIdx = 0; cfgIdx < subgroup->channelConfigs()->count(); cfgIdx++) {
            ChannelConfig *config = qobject_cast<ChannelConfig*>(subgroup->channelConfigs()->get(cfgIdx));
            if (!config) {
                continue;
            }
            const QString key = config->parameter();
            if (seenKeys.contains(key)) {
                continue;
            }
            seenKeys.insert(key);
            tableKeys.append(key);
            _tableChannelConfigs->append(config);
        }
    }

    const bool shared = hasSharedTimerGroups();
    int timerIndex = 0;
    int gridRow = 1;
    for (int sgIdx = 0; sgIdx < _subgroups->count(); sgIdx++) {
        ActuatorOutputSubgroup *subgroup = qobject_cast<ActuatorOutputSubgroup*>(_subgroups->get(sgIdx));
        if (!subgroup || subgroup->channels()->count() == 0) {
            continue;
        }
        ++timerIndex;
        const int headerGridRow = shared ? gridRow++ : 0;
        for (int chIdx = 0; chIdx < subgroup->channels()->count(); chIdx++) {
            ActuatorOutputChannel *channel = qobject_cast<ActuatorOutputChannel*>(subgroup->channels()->get(chIdx));
            if (!channel) {
                continue;
            }
            _channelRows->append(new ActuatorChannelRow(this, channel, subgroup->primaryParam(),
                    chIdx == 0, timerIndex, gridRow, headerGridRow, shared && chIdx == 0, tableKeys));
            ++gridRow;
        }
    }
    _tableRowCount = gridRow;
    emit subgroupsChanged();
}

bool ActuatorOutput::hasPrimaryProtocol() const
{
    for (int sgIdx = 0; sgIdx < _subgroups->count(); sgIdx++) {
        ActuatorOutputSubgroup *subgroup = qobject_cast<ActuatorOutputSubgroup*>(_subgroups->get(sgIdx));
        if (subgroup && subgroup->primaryParam() && subgroup->primaryParam()->fact()) {
            return true;
        }
    }
    return false;
}

bool ActuatorOutput::hasSharedTimerGroups() const
{
    for (int sgIdx = 0; sgIdx < _subgroups->count(); sgIdx++) {
        ActuatorOutputSubgroup *subgroup = qobject_cast<ActuatorOutputSubgroup*>(_subgroups->get(sgIdx));
        if (subgroup && subgroup->channels()->count() > 1) {
            return true;
        }
    }
    return false;
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

bool ActuatorOutput::hasExistingOutputFunctionParams() const {
    bool hasExistingOutputFunction = false;
    forEachOutputFunction([&hasExistingOutputFunction](
            ActuatorOutputSubgroup *, ChannelConfigInstance *,
            [[maybe_unused]] Fact *fact) { hasExistingOutputFunction = true; });
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
