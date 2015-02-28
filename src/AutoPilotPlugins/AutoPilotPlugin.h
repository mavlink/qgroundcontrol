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
    AutoPilotPlugin(UASInterface* uas, QObject* parent);
    
    Q_PROPERTY(QVariantMap parameters READ parameters CONSTANT)
    Q_PROPERTY(QVariantList components READ components CONSTANT)
    Q_PROPERTY(QUrl setupBackgroundImage READ setupBackgroundImage CONSTANT)
    
    /// Re-request the full set of parameters from the autopilot
    Q_INVOKABLE void refreshAllParameters(void);
    
    /// Request a refresh on the specific parameter
    Q_INVOKABLE void refreshParameter(const QString& param);
    
    // Request a refresh on all parameters that begin with the specified prefix
    Q_INVOKABLE void refreshParametersPrefix(const QString& paramPrefix);

    // Property accessors
    virtual const QVariantList& components(void) = 0;
    virtual const QVariantMap& parameters(void) = 0;    
    virtual QUrl setupBackgroundImage(void) = 0;
    
    /// Returns true if the plugin is ready for use
    virtual bool pluginIsReady(void) const = 0;
    
    /// FIXME: Kind of hacky
    static void clearStaticData(void);
    
    UASInterface* uas(void) { return _uas; }
    
signals:
    /// Signalled when plugin is ready for use
    void pluginReady(void);
    
protected:
    /// All access to AutoPilotPugin objects is through getInstanceForAutoPilotPlugin
    AutoPilotPlugin(QObject* parent = NULL) : QObject(parent) { }
    
    UASInterface* _uas;
};

#endif
