/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

#include "AutoPilotPlugin.h"

class Vehicle;
class QQuickItem;
class QQmlContext;

Q_DECLARE_LOGGING_CATEGORY(VehicleComponentLog)

/// A vehicle component is an object which abstracts the physical portion of a vehicle into a set of
/// configurable values and user interface.
class VehicleComponent : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  name                                                READ name                   CONSTANT)
    Q_PROPERTY(QString  description                                         READ description            CONSTANT)
    Q_PROPERTY(bool     requiresSetup                                       READ requiresSetup          CONSTANT)
    Q_PROPERTY(bool     setupComplete                                       READ setupComplete          STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(QString  iconResource                                        READ iconResource           CONSTANT)
    Q_PROPERTY(QUrl     setupSource                                         READ setupSource            NOTIFY setupSourceChanged)
    Q_PROPERTY(QUrl     summaryQmlSource                                    READ summaryQmlSource       CONSTANT)
    Q_PROPERTY(bool     allowSetupWhileArmed                                READ allowSetupWhileArmed   CONSTANT)
    Q_PROPERTY(bool     allowSetupWhileFlying                               READ allowSetupWhileFlying  CONSTANT)
    Q_PROPERTY(AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent READ KnownVehicleComponent  CONSTANT)

public:
    explicit VehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent, QObject *parent = nullptr);
    virtual ~VehicleComponent();

    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QString iconResource() const = 0;
    virtual bool requiresSetup() const = 0;
    virtual bool setupComplete() const = 0;
    virtual QUrl setupSource() const = 0;
    virtual QUrl summaryQmlSource() const = 0;

    // @return true: Setup panel can be shown while vehicle is armed
    virtual bool allowSetupWhileArmed() const { return false; }

    // @return true: Setup panel can be shown while vehicle is flying (and armed)
    virtual bool allowSetupWhileFlying() const { return false; }

    virtual void addSummaryQmlComponent(QQmlContext* context, QQuickItem* parent);

    /// Returns an list of parameter names for which a change should cause the setupCompleteChanged
    /// signal to be emitted.
    virtual QStringList setupCompleteChangedTriggerList() const = 0;

    /// Should be called after the component is created (but not in constructor) to setup the
    /// signals which are used to track parameter changes which affect setupComplete state.
    virtual void setupTriggerSignals();

    AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent() const { return _KnownVehicleComponent; }

signals:
    void setupCompleteChanged();
    void setupSourceChanged();

protected slots:
    void _triggerUpdated(QVariant value) { emit setupCompleteChanged(); }

protected:
    Vehicle *_vehicle = nullptr;
    AutoPilotPlugin *_autopilot = nullptr;
    AutoPilotPlugin::KnownVehicleComponent _KnownVehicleComponent;
};
