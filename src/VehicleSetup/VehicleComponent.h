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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef VEHICLECOMPONENT_H
#define VEHICLECOMPONENT_H

#include <QObject>
#include <QQmlContext>
#include <QQuickItem>

#include "Vehicle.h"

class AutoPilotPlugin;

/// A vehicle component is an object which abstracts the physical portion of a vehicle into a set of
/// configurable values and user interface.

class VehicleComponent : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString  name                    READ name                   CONSTANT)
    Q_PROPERTY(QString  description             READ description            CONSTANT)
    Q_PROPERTY(bool     requiresSetup           READ requiresSetup          CONSTANT)
    Q_PROPERTY(bool     setupComplete           READ setupComplete          STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(QString  iconResource            READ iconResource           CONSTANT)
    Q_PROPERTY(QUrl     setupSource             READ setupSource            CONSTANT)
    Q_PROPERTY(QUrl     summaryQmlSource        READ summaryQmlSource       CONSTANT)
    Q_PROPERTY(QString  prerequisiteSetup       READ prerequisiteSetup      CONSTANT)
    Q_PROPERTY(bool     allowSetupWhileArmed    READ allowSetupWhileArmed   CONSTANT)
    
public:
    VehicleComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    ~VehicleComponent();
    
    virtual QString name(void) const = 0;
    virtual QString description(void) const = 0;
    virtual QString iconResource(void) const = 0;
    virtual bool requiresSetup(void) const = 0;
    virtual bool setupComplete(void) const = 0;
    virtual QUrl setupSource(void) const = 0;
    virtual QUrl summaryQmlSource(void) const = 0;
    virtual QString prerequisiteSetup(void) const = 0;

    // @return true: Setup panel can be shown while vehicle is armed
    virtual bool allowSetupWhileArmed(void) const;
    
    virtual void addSummaryQmlComponent(QQmlContext* context, QQuickItem* parent);
    
    /// @brief Returns an list of parameter names for which a change should cause the setupCompleteChanged
    ///         signal to be emitted. Last element is signalled by NULL.
    virtual QStringList setupCompleteChangedTriggerList(void) const = 0;

    /// Should be called after the component is created (but not in constructor) to setup the
    /// signals which are used to track parameter changes which affect setupComplete state.
    virtual void setupTriggerSignals(void);

signals:
    void setupCompleteChanged(bool setupComplete);

protected slots:
    void _triggerUpdated(QVariant value);

protected:
    Vehicle*            _vehicle;
    AutoPilotPlugin*    _autopilot;
};

#endif
