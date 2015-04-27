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

#include "Fact.h"

#include <QtQml>

Fact::Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent) :
    QObject(parent),
    _name(name),
    _componentId(componentId),
    _type(type),
    _metaData(NULL)
{
    _value = 0;
}

void Fact::setValue(const QVariant& value)
{
    QVariant newValue;
    
    switch (type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            newValue = QVariant(value.toInt());
            break;

        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            newValue = QVariant(value.toUInt());
            break;

        case FactMetaData::valueTypeFloat:
            newValue = QVariant(value.toFloat());
            break;

        case FactMetaData::valueTypeDouble:
            newValue = QVariant(value.toDouble());
            break;
    }
    
    if (newValue != _value) {
        _value.setValue(newValue);
        emit valueChanged(_value);
        emit _containerValueChanged(_value);
    }
}

void Fact::_containerSetValue(const QVariant& value)
{
    _value = value;
    emit valueChanged(_value);
}

QString Fact::name(void) const
{
    return _name;
}

int Fact::componentId(void) const
{
    return _componentId;
}

QVariant Fact::value(void) const
{
    return _value;
}

QString Fact::valueString(void) const
{
    return _value.toString();
}

QVariant Fact::defaultValue(void)
{
    Q_ASSERT(_metaData);
    if (!_metaData->defaultValueAvailable()) {
        qDebug() << "Access to unavailable default value";
    }
    return _metaData->defaultValue();
}

FactMetaData::ValueType_t Fact::type(void)
{
    return _type;
}

QString Fact::shortDescription(void)
{
    Q_ASSERT(_metaData);
    return _metaData->shortDescription();
}

QString Fact::longDescription(void)
{
    Q_ASSERT(_metaData);
    return _metaData->longDescription();
}

QString Fact::units(void)
{
    Q_ASSERT(_metaData);
    return _metaData->units();
}

QVariant Fact::min(void)
{
    Q_ASSERT(_metaData);
    return _metaData->min();
}

QVariant Fact::max(void)
{
    Q_ASSERT(_metaData);
    return _metaData->max();
}

QString Fact::group(void)
{
    Q_ASSERT(_metaData);
    return _metaData->group();
}

void Fact::setMetaData(FactMetaData* metaData)
{
    _metaData = metaData;
}

bool Fact::valueEqualsDefault(void)
{
    Q_ASSERT(_metaData);
    if (_metaData->defaultValueAvailable()) {
        return _metaData->defaultValue() == value();
    } else {
        return false;
    }
}

bool Fact::defaultValueAvailable(void)
{
    Q_ASSERT(_metaData);
    return _metaData->defaultValueAvailable();
}