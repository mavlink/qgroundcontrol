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
///     @brief Object which exposes a FactMetaData
///
///     @author Don Gagne <don@thegagnes.com>

#include "FactMetaData.h"

#include <QDebug>

#include <limits>

FactMetaData::FactMetaData(ValueType_t type, QObject* parent) :
    QObject(parent),
    _group("*Default Group"),
    _type(type),
    _defaultValue(0),
    _defaultValueAvailable(false),
    _min(_minForType()),
    _max(_maxForType()),
    _minIsDefaultForType(true),
    _maxIsDefaultForType(true)
{

}

QVariant FactMetaData::defaultValue(void)
{
    if (_defaultValueAvailable) {
        return _defaultValue;
    } else {
        qWarning() << "Attempt to access unavailable default value";
        return QVariant(0);
    }
}

void FactMetaData::setDefaultValue(const QVariant& defaultValue)
{
    if (_min <= defaultValue && defaultValue <= _max) {
        _defaultValue = defaultValue;
        _defaultValueAvailable = true;
    } else {
        qWarning() << "Attempt to set default value which is outside min/max range";
    }
}

void FactMetaData::setMin(const QVariant& min)
{
    if (min > _minForType()) {
        _min = min;
        _minIsDefaultForType = false;
    } else {
        qWarning() << "Attempt to set min below allowable value";
        _min = _minForType();
    }
}

void FactMetaData::setMax(const QVariant& max)
{
    if (max > _maxForType()) {
        qWarning() << "Attempt to set max above allowable value";
        _max = _maxForType();
    } else {
        _max = max;
        _maxIsDefaultForType = false;
    }
}

QVariant FactMetaData::_minForType(void)
{
    switch (_type) {
        case valueTypeUint8:
            return QVariant(std::numeric_limits<unsigned char>::min());
        case valueTypeInt8:
            return QVariant(std::numeric_limits<signed char>::min());
        case valueTypeUint16:
            return QVariant(std::numeric_limits<unsigned short int>::min());
        case valueTypeInt16:
            return QVariant(std::numeric_limits<short int>::min());
        case valueTypeUint32:
            return QVariant(std::numeric_limits<unsigned int>::min());
        case valueTypeInt32:
            return QVariant(std::numeric_limits<int>::min());
        case valueTypeFloat:
            return QVariant(-std::numeric_limits<float>::max());
        case valueTypeDouble:
            return QVariant(-std::numeric_limits<double>::max());
    }
    
    // Make windows compiler happy, even switch is full cased
    return QVariant();
}

QVariant FactMetaData::_maxForType(void)
{
    switch (_type) {
        case valueTypeUint8:
            return QVariant(std::numeric_limits<unsigned char>::max());
        case valueTypeInt8:
            return QVariant(std::numeric_limits<signed char>::max());
        case valueTypeUint16:
            return QVariant(std::numeric_limits<unsigned short int>::max());
        case valueTypeInt16:
            return QVariant(std::numeric_limits<short int>::max());
        case valueTypeUint32:
            return QVariant(std::numeric_limits<unsigned int>::max());
        case valueTypeInt32:
            return QVariant(std::numeric_limits<int>::max());
        case valueTypeFloat:
            return QVariant(std::numeric_limits<float>::max());
        case valueTypeDouble:
            return QVariant(std::numeric_limits<double>::max());
    }
    
    // Make windows compiler happy, even switch is full cased
    return QVariant();
}

bool FactMetaData::convertAndValidate(const QVariant& value, bool convertOnly, QVariant& typedValue, QString& errorString)
{
    bool convertOk;
    
    errorString.clear();
    
    switch (type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue = QVariant(value.toInt(&convertOk));
            if (!convertOnly && convertOk) {
                if (min() > typedValue || typedValue > max()) {
                    errorString = QString("Value must be within %1 and %2").arg(min().toInt()).arg(max().toInt());
                }
            }
            break;
            
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue = QVariant(value.toUInt(&convertOk));
            if (!convertOnly && convertOk) {
                if (min() > typedValue || typedValue > max()) {
                    errorString = QString("Value must be within %1 and %2").arg(min().toUInt()).arg(max().toUInt());
                }
            }
            break;
            
        case FactMetaData::valueTypeFloat:
            typedValue = QVariant(value.toFloat(&convertOk));
            if (!convertOnly && convertOk) {
                if (min() > typedValue || typedValue > max()) {
                    errorString = QString("Value must be within %1 and %2").arg(min().toFloat()).arg(max().toFloat());
                }
            }
            break;
            
        case FactMetaData::valueTypeDouble:
            typedValue = QVariant(value.toDouble(&convertOk));
            if (!convertOnly && convertOk) {
                if (min() > typedValue || typedValue > max()) {
                    errorString = QString("Value must be within %1 and %2").arg(min().toDouble()).arg(max().toDouble());
                }
            }
            break;
    }
    
    if (!convertOk) {
        errorString = "Invalid number";
    }
    
    return convertOk && errorString.isEmpty();
}
