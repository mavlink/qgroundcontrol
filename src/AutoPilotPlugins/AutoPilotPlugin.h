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

#ifndef AUTOPILOTPLUGIN_H
#define AUTOPILOTPLUGIN_H

#include <QObject>
#include <QList>
#include <QString>
#include <QQmlContext>

#include "UASInterface.h"
#include "VehicleComponent.h"
#include "FactSystem.h"

/// This is the base class for AutoPilot plugins
///
/// The AutoPilotPlugin class is an abstract base class which represent the methods and objects
/// which are specific to a certain AutoPilot. This is the only place where AutoPilot specific
/// code should reside in QGroundControl. The remainder of the QGroundControl source is
/// generic to a common mavlink implementation.

class AutoPilotPlugin : public QObject
{
    Q_OBJECT

public:
    /// @brief Returns the list of VehicleComponent objects associated with the AutoPilot.
    virtual QList<VehicleComponent*> getVehicleComponents(void) const = 0;
    
    /// Returns the parameter facts for the specified UAS.
    ///
    /// Access to parameter properties is done through QObject::property or the full
    /// QMetaObject methods. The property name is the parameter name. You should not
    /// request parameter facts until the plugin reports that it is ready.
    virtual QObject* parameterFacts(void) const = 0;
    
    /// Adds the FactSystem properties to the Qml context. You should not call
    /// this method until the plugin reports that it is ready.
    virtual void addFactsToQmlContext(QQmlContext* context) const = 0;
    
    /// Returns true if the plugin is ready for use
    virtual bool pluginIsReady(void) const = 0;
    
    /// FIXME: Kind of hacky
    static void clearStaticData(void);
    
signals:
    /// Signalled when plugin is ready for use
    void pluginReady(void);
    
protected:
    // All access to AutoPilotPugin objects is through getInstanceForAutoPilotPlugin
    AutoPilotPlugin(QObject* parent = NULL) : QObject(parent) { }
};

#endif
