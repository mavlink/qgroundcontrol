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

#ifndef Fact_H
#define Fact_H

#include "FactMetaData.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

/// @brief A Fact is used to hold a single value within the system.
///
/// Along with the value property is a set of meta data which further describes the Fact. This information is
/// exposed through QObject Properties such that you can bind to it from QML as well as use it within C++ code.
/// Since the meta data is common to all instances of the same Fact, it is acually stored once in a seperate object.
class Fact : public QObject
{
    Q_OBJECT
    
public:
    //Fact(int componentId, QString name = "", FactMetaData::ValueType_t type = FactMetaData::valueTypeInt32, QObject* parent = NULL);
    Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent = NULL);
    
    // Property system methods
    
    QString name(void) const;
    int componentId(void) const;
    QVariant value(void) const;
    QString valueString(void) const;
    void setValue(const QVariant& value);
    QVariant defaultValue(void);
	bool defaultValueAvailable(void);
    bool valueEqualsDefault(void);
    FactMetaData::ValueType_t type(void);
    QString shortDescription(void);
    QString longDescription(void);
    QString units(void);
    QVariant min(void);
    QVariant max(void);    
    QString group(void);
    
    /// Sets the meta data associated with the Fact.
    void setMetaData(FactMetaData* metaData);
    
    void _containerSetValue(const QVariant& value);
    
signals:
    /// QObject Property System signal for value property changes
    ///
    /// This signal is only meant for use by the QT property system. It should not be connected to by client code.
    void valueChanged(QVariant value);
    
    /// Signalled when property has been changed by a call to the property write accessor
    ///
    /// This signal is meant for use by Fact container implementations.
    void _containerValueChanged(const QVariant& value);
    
private:
    QString                     _name;
    int                         _componentId;
    QVariant                    _value;
    FactMetaData::ValueType_t   _type;
    FactMetaData*               _metaData;
};

#endif