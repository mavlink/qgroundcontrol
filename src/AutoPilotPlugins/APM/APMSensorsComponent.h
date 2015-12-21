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

#ifndef APMSensorsComponent_H
#define APMSensorsComponent_H

#include "APMComponent.h"

class APMSensorsComponent : public APMComponent
{
    Q_OBJECT
    
public:
    APMSensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);

    bool compassSetupNeeded(void) const;
    bool accelSetupNeeded(void) const;

    // Virtuals from APMComponent
    QStringList setupCompleteChangedTriggerList(void) const Q_DECL_FINAL;
    
    // Virtuals from VehicleComponent
    QString name(void) const Q_DECL_FINAL;
    QString description(void) const Q_DECL_FINAL;
    QString iconResource(void) const Q_DECL_FINAL;
    bool requiresSetup(void) const Q_DECL_FINAL;
    bool setupComplete(void) const Q_DECL_FINAL;
    QUrl setupSource(void) const Q_DECL_FINAL;
    QUrl summaryQmlSource(void) const Q_DECL_FINAL;
    QString prerequisiteSetup(void) const Q_DECL_FINAL;
    
private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
