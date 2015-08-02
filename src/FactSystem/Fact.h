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
class Fact : public QObject
{
    Q_OBJECT
    
public:
    Fact(void);
    Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent = NULL);
    
    Q_PROPERTY(int componentId READ componentId CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(QVariant valueString READ valueString NOTIFY valueChanged)
    Q_PROPERTY(QString units READ units CONSTANT)
    Q_PROPERTY(QVariant defaultValue READ defaultValue CONSTANT)
    Q_PROPERTY(bool defaultValueAvailable READ defaultValueAvailable CONSTANT)
    Q_PROPERTY(bool valueEqualsDefault READ valueEqualsDefault NOTIFY valueChanged)
    Q_PROPERTY(FactMetaData::ValueType_t type READ type CONSTANT)
    Q_PROPERTY(QString shortDescription READ shortDescription CONSTANT)
    Q_PROPERTY(QString longDescription READ longDescription CONSTANT)
    Q_PROPERTY(QVariant min READ min CONSTANT)
    Q_PROPERTY(bool minIsDefaultForType READ minIsDefaultForType CONSTANT)
    Q_PROPERTY(QVariant max READ max CONSTANT)
    Q_PROPERTY(bool maxIsDefaultForType READ maxIsDefaultForType CONSTANT)
    Q_PROPERTY(QString group READ group CONSTANT)
    
    /// Convert and validate value
    ///     @param convertOnly true: validate type conversion only, false: validate against meta data as well
    Q_INVOKABLE QString validate(const QString& value, bool convertOnly);
    
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
    bool minIsDefaultForType(void);
    QVariant max(void);    
    bool maxIsDefaultForType(void);
    QString group(void);
    
    /// Sets and sends new value to vehicle even if value is the same
    void forceSetValue(const QVariant& value);
    
    /// Sets the meta data associated with the Fact.
    void setMetaData(FactMetaData* metaData);
    
    void _containerSetValue(const QVariant& value);
    
signals:
    /// QObject Property System signal for value property changes
    ///
    /// This signal is only meant for use by the QT property system. It should not be connected to by client code.
    void valueChanged(QVariant value);
    
    /// Signalled when the param write ack comes back from the vehicle
    void vehicleUpdated(QVariant value);
    
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