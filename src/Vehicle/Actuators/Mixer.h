/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>

#include "Common.h"

#include <QmlObjectListModel.h>

namespace Mixer {

enum class Function {
    Unspecified=0,
    PositionX,
    PositionY,
    PositionZ,
    SpinDirection, ///< CCW = true or 1
    AxisX,
    AxisY,
    AxisZ,
    Type,
};

struct MixerParameter {
    Parameter param;
    Function function{Function::Unspecified};
    QString identifier; ///< optional identifier for rules
    QList<float> values{}; ///< fixed values if not configurable (param.name == "")
};

/**
 * Actuator type configuration (directly corresponds to the json entries)
 */
struct ActuatorType
{
    struct Values {
        float min{};
        float max{};
        float defaultVal{};
        bool reversible{};
    };

    int functionMin{};
    int functionMax{};
    Values values{};
    QList<Parameter> perItemParams{};
    int labelIndexOffset{};
};

using ActuatorTypes = QMap<QString, ActuatorType>; ///< key is the group name, where 'DEFAULT' is the default group

/**
 * Mixer configuration option (directly corresponds to the json entries)
 */
struct MixerOption
{
    struct ActuatorGroup {
        QString groupLabel{};
        QString count{}; ///< param to configure the amount. If empty, fixedCount is non-zero
        int fixedCount{};
        QString actuatorType{};
        bool required{}; ///< if true, actuator has to be configured for a valid setup
        QList<Parameter> parameters{};
        QList<MixerParameter> perItemParameters{};
        QStringList itemLabelPrefix{}; ///< size = either fixedCount or 1 or 0 items
    };
    QString option{};
    QString type{}; ///< Mixer type, e.g. multirotor
    QString title{};
    QString helpUrl{};
    QList<ActuatorGroup> actuators{};
};

using MixerOptions = QList<MixerOption>;

struct Rule {
    struct RuleItem {
        bool hasMin{false};
        bool hasMax{false};
        bool hasDefault{false};

        float min{};
        float max{};
        float defaultVal{};
        bool hidden{false};
        bool disabled{false};
    };
    QString selectIdentifier;
    QList<QString> applyIdentifiers;
    QMap<int, QList<RuleItem>> items;
};

using Rules = QList<Rule>;

/**
 * Config parameters that apply to a group of actuators
 */
class ConfigParameter : public QObject
{
    Q_OBJECT
public:
    ConfigParameter(QObject* parent, Fact* fact, const QString& label, bool advanced)
        : QObject(parent), _fact(fact), _label(label), _advanced(advanced) {}

    Q_PROPERTY(QString label          READ label       CONSTANT)
    Q_PROPERTY(Fact* fact             READ fact        CONSTANT)
    Q_PROPERTY(bool advanced          READ advanced    CONSTANT)

    const QString& label() const { return _label; }
    Fact* fact() { return _fact; }
    bool advanced() const { return _advanced; }

private:
    Fact* _fact{nullptr};
    const QString _label;
    const bool _advanced;
};

class ChannelConfigInstance;

/**
 * Config parameters that apply to individual channels
 */
class ChannelConfig : public QObject
{
    Q_OBJECT
public:

    ChannelConfig(QObject* parent, const MixerParameter& parameter, bool isActuatorTypeConfig=false)
        : QObject(parent), _parameter(parameter), _isActuatorTypeConfig(isActuatorTypeConfig) {}

    Q_PROPERTY(QString label      READ label        CONSTANT)
    Q_PROPERTY(bool visible       READ visible      NOTIFY visibleChanged)
    Q_PROPERTY(bool advanced      READ advanced     CONSTANT)

    const QString& label() const { return _parameter.param.label; }
    Function function() const { return _parameter.function; }
    bool advanced() const { return _parameter.param.advanced; }
    bool isActuatorTypeConfig() const { return _isActuatorTypeConfig; }

    bool visible() const { return _visible; }

    const QList<float>& fixedValues() const { return _parameter.values; }

    Parameter::DisplayOption displayOption() const { return _parameter.param.displayOption; }
    int indexOffset() const { return _parameter.param.indexOffset; }

    const QString& identifier() const { return _parameter.identifier; }

    const MixerParameter& config() const { return _parameter; }

    virtual ChannelConfigInstance* instantiate(int paramIndex, int actuatorTypeIndex,
        ParameterManager* parameterManager, std::function<void(Function, Fact*)> factAddedCb);

signals:
    void visibleChanged();
protected:
    void channelInstanceCreated(ChannelConfigInstance* instance);

private slots:
    void instanceVisibleChanged();

private:
    const MixerParameter _parameter;
    const bool _isActuatorTypeConfig; ///< actuator type config instead of mixer channel config
    bool _visible{true}; ///< this is false if none of the instances is visible
    QList<ChannelConfigInstance*> _instances;
};

class ChannelConfigVirtualAxis : public ChannelConfig
{
    Q_OBJECT
public:
    ChannelConfigVirtualAxis(QObject* parent, const MixerParameter& parameter)
        : ChannelConfig(parent, parameter) {}

    ChannelConfigInstance* instantiate(int paramIndex, int actuatorTypeIndex,
        ParameterManager* parameterManager, std::function<void(Function, Fact*)> factAddedCb) override;
private:
};

/**
 * Per-channel instance for a ChannelConfig
 */
class ChannelConfigInstance : public QObject
{
    Q_OBJECT
public:
    ChannelConfigInstance(QObject* parent, Fact* fact, ChannelConfig& config)
        : QObject(parent), _fact(fact), _config(config) {}

    Q_PROPERTY(ChannelConfig* config      READ channelConfig    CONSTANT)
    Q_PROPERTY(Fact* fact                 READ fact             CONSTANT)
    Q_PROPERTY(bool visible               READ visible          NOTIFY visibleChanged)
    Q_PROPERTY(bool enabled               READ enabled          NOTIFY enabledChanged)

    ChannelConfig* channelConfig() const { return &_config; }

    Fact* fact() { return _fact; }

    bool visible() const { return _visibleRule && _visibleAxis; }
    bool enabled() const { return _enabledRule; }

    bool visibleRule() const { return _visibleRule; }
    bool enabledRule() const { return _enabledRule; }

    void setVisibleRule(bool visible) { _visibleRule = visible; emit visibleChanged(); }
    void setEnabledRule(bool enabled) { _enabledRule = enabled; emit enabledChanged(); }

    void setVisibleAxis(bool visible) { _visibleAxis = visible; emit visibleChanged(); }

    int ruleApplyIdentifierIdx() const { return _ruleApplyIdentifierIdx; }
    void setRuleApplyIdentifierIdx(int idx) { _ruleApplyIdentifierIdx = idx; }

    virtual void allInstancesInitialized(QmlObjectListModel* configInstances) {}

signals:
    void visibleChanged();
    void enabledChanged();

protected:
    Fact* _fact{nullptr};

private:
    ChannelConfig& _config;
    int _ruleApplyIdentifierIdx{-1};

    bool _visibleRule{true};
    bool _enabledRule{true};

    bool _visibleAxis{true};
};

class ChannelConfigInstanceVirtualAxis : public ChannelConfigInstance
{
    Q_OBJECT
public:
    enum class Direction {
        Custom = 0,
        Upwards = 1,
        Downwards = 2,
        Forwards = 3,
        Backwards = 4,
        Leftwards = 5,
        Rightwards = 6,
    };
    ChannelConfigInstanceVirtualAxis(QObject* parent, ChannelConfig& config)
        : ChannelConfigInstance(parent, nullptr, config) {}

    void allInstancesInitialized(QmlObjectListModel* configInstances) override;

private slots:
    void setFactFromAxes(bool keepVisible = false);
    void setAxesFromFact();
    void axisVisibleChanged();
    void axisEnableChanged();

private:
    ChannelConfigInstance* _axes[3]{};
    bool _ignoreChange{false};
};

class MixerChannel : public QObject
{
    Q_OBJECT
public:
    MixerChannel(QObject* parent, const QString& label, int actuatorFunction,
            int paramIndex, int actuatorTypeIndex, QmlObjectListModel& channelConfigs, ParameterManager* parameterManager,
            const Rule* rule, std::function<void(Function, Fact*)> factAddedCb);

    Q_PROPERTY(QString label                            READ label               CONSTANT)
    Q_PROPERTY(QmlObjectListModel* configInstances      READ configInstances     NOTIFY configInstancesChanged)

    const QString& label() const { return _label; }
    int actuatorFunction() const { return _actuatorFunction; }

    QmlObjectListModel* configInstances() { return _configInstances; }

    bool getGeometry(const ActuatorTypes& actuatorTypes, const MixerOption::ActuatorGroup& group,
            ActuatorGeometry& geometry) const;

    Fact* getFact(Function function) const;

public slots:
    void applyRule(bool noConstraints = false);

signals:
    void configInstancesChanged();

private:

    const QString _label;
    const int _actuatorFunction;
    const int _paramIndex;
    const int _actuatorTypeIndex;
    const Rule* _rule;

    QmlObjectListModel* _configInstances = new QmlObjectListModel(this); ///< list of ChannelConfigInstance*
    int _ruleSelectIdentifierIdx{-1};
    int _currentSelectIdentifierValue{};
    bool _applyingRule{false};
};

class MixerConfigGroup : public QObject
{
    Q_OBJECT
public:
    MixerConfigGroup(QObject* parent, const MixerOption::ActuatorGroup& group)
        : QObject(parent), _group(group) {}

    Q_PROPERTY(QString label                        READ label              CONSTANT)
    Q_PROPERTY(QmlObjectListModel* channels         READ channels           NOTIFY channelsChanged)
    Q_PROPERTY(QmlObjectListModel* channelConfigs   READ channelConfigs     NOTIFY channelConfigsChanged)
    Q_PROPERTY(ConfigParameter* countParam          READ countParam         CONSTANT)
    Q_PROPERTY(QmlObjectListModel* configParams     READ configParams       CONSTANT)

    const QString& label() const { return _group.groupLabel; }

    // per-channel-params
    QmlObjectListModel* channelConfigs() { return _channelConfigs; }
    void addChannelConfig(ChannelConfig* channelConfig);

    QmlObjectListModel* channels() { return _channels; }
    void addChannel(MixerChannel* channel);

    ConfigParameter* countParam() const { return _countParam; }
    void addConfigParam(ConfigParameter* param);
    QmlObjectListModel* configParams() { return _params; }

    void setCountParam(ConfigParameter* param) { delete _countParam; _countParam = param; }

    const MixerOption::ActuatorGroup& group() const { return _group; }

signals:
    void channelsChanged();
    void channelConfigsChanged();

private:
    const MixerOption::ActuatorGroup& _group;
    QmlObjectListModel* _channels = new QmlObjectListModel(this); ///< list of MixerChannel*
    QmlObjectListModel* _channelConfigs = new QmlObjectListModel(this); ///< list of ChannelConfig*

    ConfigParameter* _countParam{nullptr};
    QmlObjectListModel* _params  = new QmlObjectListModel(this); ///< list of ConfigParameter*
};

class Mixers : public QObject
{
    Q_OBJECT
public:
    Mixers(ParameterManager* parameterManager)
        : _parameterManager(parameterManager) {}

    struct OutputFunction {
        QString label;
        Condition noteCondition;
        QString note;
        bool excludeFromActuatorTesting;
        QString actuatorType{};

        operator QString() const { return label; }
    };

    void reset(const ActuatorTypes& actuatorTypes, const MixerOptions& mixerOptions,
            const QMap<int, OutputFunction>& functions, const Rules& rules);

    Q_PROPERTY(QmlObjectListModel* groups         READ groups        NOTIFY groupsChanged)
    Q_PROPERTY(QString title                      READ title         NOTIFY groupsChanged)
    Q_PROPERTY(QString helpUrl                    READ helpUrl       NOTIFY groupsChanged)

    QmlObjectListModel* groups() { return _groups; }

    QString title() const;
    QString helpUrl() const;

    const ActuatorTypes& actuatorTypes() const { return _actuatorTypes; }

    /**
     * Get the mixer-specific label for a certain output function and the current configuration,
     * combined with the generic label.
     * E.g. 'Front Left Tilt (Servo 1)' for the 'Servo 1' function.
     * @param function actuator output function
     * @return empty string if there's no such function
     */
    QString getSpecificLabelForFunction(int function) const;

    /**
     * Get the set of all (required) actuator functions
     */
    QSet<int> getFunctions(bool requiredOnly) const;

    QString configuredType() const;

    const QMap<int, OutputFunction>& functions() const { return _functions; }

public slots:

    /**
     * Call this on param update(s)
     */
    void update();

signals:
    void groupsChanged();
    void paramChanged();
    void geometryParamChanged();

private:

    Fact* getFact(const QString& paramName);
    void subscribeFact(Fact* fact, bool geometry=false);
    void unsubscribeFacts();

    QSet<Fact*> _subscribedFacts{};
    QSet<Fact*> _subscribedFactsGeometry{};

    ActuatorTypes _actuatorTypes;
    MixerOptions _mixerOptions;
    QMap<int, OutputFunction> _functions;  ///< all possible output functions

    QList<Condition> _mixerConditions;
    int _selectedMixer{-1};
    QmlObjectListModel* _groups = new QmlObjectListModel(this); ///< list of MixerConfigGroup*

    QMap<int, QString> _functionsSpecificLabel; ///< function with specific label, e.g. 'Front Left Motor (Motor 1)'
    ParameterManager* _parameterManager{nullptr};

    Rules _rules;
};

} // namespace Mixer
