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

#ifndef FACTBINDER_H
#define FACTBINDER_H

#include "Fact.h"
#include "AutoPilotPlugin.h"

#include <QObject>
#include <QString>

/// This object is used to instantiate a connection to a Fact from within Qml.
class FactBinder : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(QVariant valueString READ valueString NOTIFY valueChanged)
    Q_PROPERTY(QString units READ units CONSTANT)
    
public:
    FactBinder(void);
    
    QString name(void) const;
    void setName(const QString& name);
    
    QVariant value(void) const;
    void setValue(const QVariant& value);
    
    QString valueString(void) const;
    
    /// Read accesor for units property
    QString units(void) const;
    
signals:
    void nameChanged(void);
    void valueChanged(void);
    
private:
    AutoPilotPlugin*    _autopilotPlugin;
    Fact*               _fact;
};

#endif