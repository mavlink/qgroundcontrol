/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef ESP8266Component_H
#define ESP8266Component_H

#include "VehicleComponent.h"

class ESP8266Component : public VehicleComponent
{
    Q_OBJECT
public:
    ESP8266Component            (Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const;
    
    // Virtuals from VehicleComponent
    QString name                () const;
    QString description         () const;
    QString iconResource        () const;
    bool    requiresSetup       () const;
    bool    setupComplete       () const;
    QUrl    setupSource         () const;
    QUrl    summaryQmlSource    () const;
    QString prerequisiteSetup   () const;
    
private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
