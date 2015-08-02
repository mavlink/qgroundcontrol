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

Fact::Fact(void) :
    _componentId(-1),
    _value(0),
    _type(FactMetaData::valueTypeInt32),
    _metaData(NULL)
{    
    FactMetaData* metaData = new FactMetaData(_type, this);
    setMetaData(metaData);
}

Fact::Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent) :
    QObject(parent),
    _name(name),
    _componentId(componentId),
    _value(0),
    _type(type),
    _metaData(NULL)
{

}

void Fact::forceSetValue(const QVariant& value)
{
    if (_metaData) {
        QVariant    typedValue;
        QString     errorString;
        
        if (_metaData->convertAndValidate(value, true /* convertOnly */, typedValue, errorString)) {
            _value.setValue(typedValue);
            emit valueChanged(_value);
            emit _containerValueChanged(_value);
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::setValue(const QVariant& value)
{
    if (_metaData) {
        QVariant    typedValue;
        QString     errorString;
        
        if (_metaData->convertAndValidate(value, true /* convertOnly */, typedValue, errorString)) {
            if (typedValue != _value) {
                _value.setValue(typedValue);
                emit valueChanged(_value);
                emit _containerValueChanged(_value);
            }
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::_containerSetValue(const QVariant& value)
{
    _value = value;
    emit valueChanged(_value);
    emit vehicleUpdated(_value);
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
    if (_metaData) {
        if (!_metaData->defaultValueAvailable()) {
            qDebug() << "Access to unavailable default value";
        }
        return _metaData->defaultValue();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

FactMetaData::ValueType_t Fact::type(void)
{
    return _type;
}

QString Fact::shortDescription(void)
{
    if (_metaData) {
        return _metaData->shortDescription();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QString Fact::longDescription(void)
{
    if (_metaData) {
        return _metaData->longDescription();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QString Fact::units(void)
{
    if (_metaData) {
        return _metaData->units();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QVariant Fact::min(void)
{
    if (_metaData) {
        return _metaData->min();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QVariant Fact::max(void)
{
    if (_metaData) {
        return _metaData->max();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

bool Fact::minIsDefaultForType(void)
{
    if (_metaData) {
        return _metaData->minIsDefaultForType();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

bool Fact::maxIsDefaultForType(void)
{
    if (_metaData) {
        return _metaData->maxIsDefaultForType();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

QString Fact::group(void)
{
    if (_metaData) {
        return _metaData->group();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

void Fact::setMetaData(FactMetaData* metaData)
{
    _metaData = metaData;
}

bool Fact::valueEqualsDefault(void)
{
    if (_metaData) {
        if (_metaData->defaultValueAvailable()) {
            return _metaData->defaultValue() == value();
        } else {
            return false;
        }
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

bool Fact::defaultValueAvailable(void)
{
    if (_metaData) {
        return _metaData->defaultValueAvailable();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

QString Fact::validate(const QString& value, bool convertOnly)
{
    if (_metaData) {
        
        QVariant    typedValue;
        QString     errorString;
        
        _metaData->convertAndValidate(value, convertOnly, typedValue, errorString);
        
        return errorString;
    } else {
        qWarning() << "Meta data pointer missing";
        return QString("Internal error: Meta data pointer missing");
    }
}
