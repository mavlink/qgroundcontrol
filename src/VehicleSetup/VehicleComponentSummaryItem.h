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

#ifndef VehicleComponentSummaryItem_H
#define VehicleComponentSummaryItem_H

#include <QObject>
#include <QQmlContext>
#include <QQuickItem>

#include "UASInterface.h"

/// @file
///     @brief Vehicle Component class. A vehicle component is an object which
///             abstracts the physical portion of a vehicle into a set of
///             configurable values and user interface.
///     @author Don Gagne <don@thegagnes.com>

class VehicleComponentSummaryItem : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString state READ state CONSTANT)
    
public:
    VehicleComponentSummaryItem(const QString& name, const QString& state, QObject* parent = NULL);
    
    QString name(void) const { return _name; }
    QString state(void) const { return _state; }
    
protected:
    QString _name;
    QString _state;
};

#endif
