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

/// @brief A Fact is used to hold a single value within the system.
///
/// Along with the value property is a set of meta data which further describes the Fact. This information is
/// exposed through QObject Properties such that you can bind to it from QML as well as use it within C++ code.
/// Since the meta data is common to all instances of the same Fact, it is acually stored once in a seperate object.
class Fact : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(QVariant defaultValue READ defaultValue CONSTANT)
    Q_PROPERTY(FactMetaData::ValueType_t type READ type CONSTANT)
    Q_PROPERTY(QString shortDescription READ shortDescription CONSTANT)
    Q_PROPERTY(QString longDescription READ longDescription CONSTANT)
    Q_PROPERTY(QString units READ units CONSTANT)
    Q_PROPERTY(QVariant min READ min CONSTANT)
    Q_PROPERTY(QVariant max READ max CONSTANT)
    
    Q_ENUMS(FactMetaData::ValueType_t)
    
public:
    Fact(QString name = "", QObject* parent = NULL);
    
    // Property system methods
    
    /// Read accessor or name property
    QString name(void) const { return _name; }
    
    /// Read accessor for value property
    QVariant value(void) const { return _value; }
    
    /// Write accessor for value property
    void setValue(const QVariant& value);
    
    /// Read accesor for defaultValue property
    QVariant defaultValue(void) { return _metaData->defaultValue; }
    
    /// Read accesor for type property
    FactMetaData::ValueType_t type(void) { return _metaData->type; }
    
    /// Read accesor for shortDescription property
    QString shortDescription(void) { return _metaData->shortDescription; }
    
    /// Read accesor for longDescription property
    QString longDescription(void) { return _metaData->longDescription; }
    
    /// Read accesor for units property
    QString units(void) { return _metaData->units; }
    
    /// Read accesor for min property
    QVariant min(void) { return _metaData->min; }

    /// Read accesor for max property
    QVariant max(void) { return _metaData->max; }
    
    /// Sets the meta data associated with the Fact.
    void setMetaData(FactMetaData* metaData) { _metaData = metaData; }
    
    void _containerSetValue(const QVariant& value);
    
signals:
    /// QObject Property System signal for value property changes
    ///
    /// This signal is only meant for use by the QT property system. It should not be connected to by client code.
    void valueChanged(QVariant value);
    
    /// Signalled when property has been changed by a call to the property write accessor
    ///
    /// This signal is meant for use by Fact container implementations.
    void _containerValueChanged(QVariant& value);
    
private:
    QString         _name;      ///< Fact name
    QVariant        _value;     ///< Fact value
    FactMetaData*   _metaData;  ///< FactMetaData object for Fact
};

#endif