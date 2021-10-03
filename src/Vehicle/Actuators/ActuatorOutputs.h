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

#include <functional>

#include "Common.h"
#include "ActuatorActions.h"

#include <QmlObjectListModel.h>

namespace ActuatorOutputs {

/**
 * Config parameters that apply to a subgroup of actuators
 */
class ConfigParameter : public QObject
{
    Q_OBJECT
public:
    enum class Function {
        Unspecified = 0,
        Enable,             ///< Parameter to enable/disable the outputs
        Primary,            ///< Primary parameter to configure the group of outputs
    };

    ConfigParameter(QObject* parent, Fact* fact, const QString& label, Function function)
        : QObject(parent), _fact(fact), _label(label), _function(function) {}

    Q_PROPERTY(QString label          READ label       CONSTANT)
    Q_PROPERTY(Fact* fact             READ fact        CONSTANT)

    const QString& label() const { return _label; }
    Fact* fact() { return _fact; }
    Function function() const { return _function; }

private:
    Fact* _fact{nullptr};
    const QString _label;
    const Function _function;
};

/**
 * Config parameters that apply to individual channels
 */
class ChannelConfig : public QObject
{
    Q_OBJECT
public:

    /// Describes the meaning of the parameter
    enum class Function {
        Unspecified = 0,
        OutputFunction,
        Disarmed,
        Minimum,
        Maximum,
        Failsafe
    };

    ChannelConfig(QObject* parent, const Parameter& param, Function function,
            const Condition& visibilityCondition)
        : QObject(parent), _parameter(param), _function(function), _visibilityCondition(visibilityCondition) {}

    Q_PROPERTY(QString label      READ label      CONSTANT)
    Q_PROPERTY(bool advanced      READ advanced   CONSTANT)
    Q_PROPERTY(bool visible       READ visible    NOTIFY visibleChanged)

    const QString& label() const { return _parameter.label; }
    const QString& parameter() const { return _parameter.name; }
    Function function() const { return _function; }
    const Condition& visibilityCondition() const { return _visibilityCondition; }
    bool advanced() const { return _parameter.advanced; }
    bool visible() const { return _visibilityCondition.evaluate(); }

    Parameter::DisplayOption displayOption() const { return _parameter.displayOption; }
    int indexOffset() const { return _parameter.indexOffset; }

    void reevaluate();

signals:
    void visibleChanged();
private:
    const Parameter _parameter;
    const Function _function;
    const Condition _visibilityCondition;
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

    Q_PROPERTY(ChannelConfig* config      READ channelConfig   CONSTANT)
    Q_PROPERTY(Fact* fact                 READ fact            CONSTANT)

    Fact* fact() { return _fact; }

    ChannelConfig* channelConfig() const { return &_config; }

private:

    Fact* _fact{nullptr};
    ChannelConfig& _config;
};

class ActuatorOutputChannel : public QObject
{
    Q_OBJECT
public:
    ActuatorOutputChannel(QObject* parent, const QString& label, int paramIndex, QmlObjectListModel& channelConfigs,
            ParameterManager* parameterManager, std::function<void(Fact*)> factAddedCb);

    Q_PROPERTY(QString label                            READ label               CONSTANT)
    Q_PROPERTY(QmlObjectListModel* configInstances      READ configInstances     NOTIFY configInstancesChanged)

    const QString& label() const { return _label; }

    QmlObjectListModel* configInstances() { return _configInstances; }

signals:
    void configInstancesChanged();

private:
    const QString _label;
    const int _paramIndex{};

    QmlObjectListModel* _configInstances = new QmlObjectListModel(this); ///< list of ChannelConfigInstance*
};

class ActuatorOutputSubgroup : public QObject
{
    Q_OBJECT
public:
    ActuatorOutputSubgroup(QObject* parent, const QString& label)
        : QObject(parent), _label(label) {}

    Q_PROPERTY(QString label                        READ label            CONSTANT)
    Q_PROPERTY(QmlObjectListModel* channels         READ channels         NOTIFY channelsChanged)
    Q_PROPERTY(QmlObjectListModel* channelConfigs   READ channelConfigs   NOTIFY channelConfigsChanged)
    Q_PROPERTY(ConfigParameter* primaryParam        READ primaryParam     CONSTANT)
    Q_PROPERTY(QmlObjectListModel* configParams     READ configParams     CONSTANT)

    const QString& label() const { return _label; }

    // per-channel-params
    QmlObjectListModel* channelConfigs() { return _channelConfigs; }
    void addChannelConfig(ChannelConfig* channelConfig);


    QmlObjectListModel* channels() { return _channels; }
    void addChannel(ActuatorOutputChannel* channel);

    ConfigParameter* primaryParam() const { return _primaryParam; }
    void addConfigParam(ConfigParameter* param);
    QmlObjectListModel* configParams() { return _params; }

    const QList<ActuatorActions::Config>& actions() const { return _actions; }
    void addAction(const ActuatorActions::Config& action) { _actions.append(action); }

signals:
    void channelsChanged();
    void channelConfigsChanged();
private:

    const QString _label;
    QmlObjectListModel* _channels = new QmlObjectListModel(this); ///< list of ActuatorOutputChannel*
    QmlObjectListModel* _channelConfigs = new QmlObjectListModel(this); ///< list of ChannelConfig*

    ConfigParameter* _primaryParam{nullptr};
    QmlObjectListModel* _params  = new QmlObjectListModel(this); ///< list of ConfigParameter*

    QList<ActuatorActions::Config> _actions;
};

class ActuatorOutput : public QObject
{
    Q_OBJECT
public:
    ActuatorOutput(QObject* parent, const QString& label, const Condition& groupVisibilityCondition);

    Q_PROPERTY(QString label                     READ label              CONSTANT)
    Q_PROPERTY(bool groupsVisible                READ groupsVisible      NOTIFY groupsVisibleChanged)
    Q_PROPERTY(QmlObjectListModel* subgroups     READ subgroups          NOTIFY subgroupsChanged)
    Q_PROPERTY(ConfigParameter* enableParam      READ enableParam        CONSTANT)
    Q_PROPERTY(QmlObjectListModel* configParams  READ configParams       CONSTANT)
    Q_PROPERTY(QStringList notes                 READ notes              NOTIFY notesChanged)

    const QString& label() const { return _label; }

    QmlObjectListModel* subgroups() { return _subgroups; }
    bool groupsVisible() const { return _groupVisibilityCondition.evaluate(); }
    ConfigParameter* enableParam() const { return _enableParam; }
    QmlObjectListModel* configParams() { return _params; }

    void addSubgroup(ActuatorOutputSubgroup* subgroup);

    void addConfigParam(ConfigParameter* param);

    void getAllChannelFunctions(QList<Fact*>& allFunctions) const;

    bool hasExistingOutputFunctionParams() const;

    void addNote(const QString& note) { _notes.append(note); emit notesChanged(); }
    void clearNotes() { _notes.clear(); emit notesChanged(); }
    const QStringList& notes() const { return _notes; }

    void forEachOutputFunction(std::function<void(ActuatorOutputSubgroup*, ChannelConfigInstance*, Fact*)> callback) const;

signals:
    void subgroupsChanged();
    void groupsVisibleChanged();
    void notesChanged();

private:
    const QString _label;
    const Condition _groupVisibilityCondition;
    QmlObjectListModel* _subgroups = new QmlObjectListModel(this); ///< list of ActuatorOutputSubgroup*

    ConfigParameter* _enableParam{nullptr};
    QmlObjectListModel* _params  = new QmlObjectListModel(this); ///< list of ConfigParameter*
    QStringList _notes;
};

} // namespace ActuatorOutputs
