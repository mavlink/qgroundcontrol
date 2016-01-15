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
#include <cmath>

const FactMetaData::BuiltInTranslation_s FactMetaData::_rgBuildInTranslations[] = {
    { "centi-degrees",  "degrees",  FactMetaData::_centiDegreesToDegrees,   FactMetaData::_degreesToCentiDegrees },
    { "radians",        "degrees",  FactMetaData::_radiansToDegrees,        FactMetaData::_degreesToRadians },
};

FactMetaData::FactMetaData(QObject* parent)
    : QObject(parent)
    , _type(valueTypeInt32)
    , _decimalPlaces(defaultDecimalPlaces)
    , _rawDefaultValue(0)
    , _defaultValueAvailable(false)
    , _group("*Default Group")
    , _rawMax(_maxForType())
    , _maxIsDefaultForType(true)
    , _rawMin(_minForType())
    , _minIsDefaultForType(true)
    , _rawTranslator(_defaultTranslator)
    , _cookedTranslator(_defaultTranslator)
    , _rebootRequired(false)
{

}

FactMetaData::FactMetaData(ValueType_t type, QObject* parent)
    : QObject(parent)
    , _type(type)
    , _decimalPlaces(defaultDecimalPlaces)
    , _rawDefaultValue(0)
    , _defaultValueAvailable(false)
    , _group("*Default Group")
    , _rawMax(_maxForType())
    , _maxIsDefaultForType(true)
    , _rawMin(_minForType())
    , _minIsDefaultForType(true)
    , _rawTranslator(_defaultTranslator)
    , _cookedTranslator(_defaultTranslator)
    , _rebootRequired(false)
{

}

FactMetaData::FactMetaData(const FactMetaData& other, QObject* parent)
    : QObject(parent)
{
    *this = other;
}

const FactMetaData& FactMetaData::operator=(const FactMetaData& other)
{
    _decimalPlaces          = other._decimalPlaces;
    _rawDefaultValue        = other._rawDefaultValue;
    _defaultValueAvailable  = other._defaultValueAvailable;
    _bitmaskStrings         = other._bitmaskStrings;
    _bitmaskValues          = other._bitmaskValues;
    _enumStrings            = other._enumStrings;
    _enumValues             = other._enumValues;
    _group                  = other._group;
    _longDescription        = other._longDescription;
    _rawMax                 = other._rawMax;
    _maxIsDefaultForType    = other._maxIsDefaultForType;
    _rawMin                 = other._rawMin;
    _minIsDefaultForType    = other._minIsDefaultForType;
    _name                   = other._name;
    _shortDescription       = other._shortDescription;
    _type                   = other._type;
    _rawUnits               = other._rawUnits;
    _cookedUnits            = other._cookedUnits;
    _rawTranslator          = other._rawTranslator;
    _cookedTranslator       = other._cookedTranslator;
    _rebootRequired         = other._rebootRequired;

    return *this;
}

QVariant FactMetaData::rawDefaultValue(void) const
{
    if (_defaultValueAvailable) {
        return _rawDefaultValue;
    } else {
        qWarning() << "Attempt to access unavailable default value";
        return QVariant(0);
    }
}

void FactMetaData::setRawDefaultValue(const QVariant& rawDefaultValue)
{
    if (_rawMin <= rawDefaultValue && rawDefaultValue <= _rawMax) {
        _rawDefaultValue = rawDefaultValue;
        _defaultValueAvailable = true;
    } else {
        qWarning() << "Attempt to set default value which is outside min/max range";
    }
}

void FactMetaData::setRawMin(const QVariant& rawMin)
{
    if (rawMin > _minForType()) {
        _rawMin = rawMin;
        _minIsDefaultForType = false;
    } else {
        qWarning() << "Attempt to set min below allowable value for fact: " << name()
                   << ", value attempted: " << rawMin
                   << ", type: " << type() << ", min for type: " << _minForType();
        _rawMin = _minForType();
    }
}

void FactMetaData::setRawMax(const QVariant& rawMax)
{
    if (rawMax > _maxForType()) {
        qWarning() << "Attempt to set max above allowable value";
        _rawMax = _maxForType();
    } else {
        _rawMax = rawMax;
        _maxIsDefaultForType = false;
    }
}

QVariant FactMetaData::_minForType(void) const
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

QVariant FactMetaData::_maxForType(void) const
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

bool FactMetaData::convertAndValidateRaw(const QVariant& rawValue, bool convertOnly, QVariant& typedValue, QString& errorString)
{
    bool convertOk = false;
    
    errorString.clear();
    
    switch (type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue = QVariant(rawValue.toInt(&convertOk));
            if (!convertOnly && convertOk) {
                if (rawMin() > typedValue || typedValue > rawMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toInt()).arg(cookedMax().toInt());
                }
            }
            break;
            
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue = QVariant(rawValue.toUInt(&convertOk));
            if (!convertOnly && convertOk) {
                if (rawMin() > typedValue || typedValue > rawMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toUInt()).arg(cookedMax().toUInt());
                }
            }
            break;
            
        case FactMetaData::valueTypeFloat:
            typedValue = QVariant(rawValue.toFloat(&convertOk));
            if (!convertOnly && convertOk) {
                if (rawMin() > typedValue || typedValue > rawMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toFloat()).arg(cookedMax().toFloat());
                }
            }
            break;
            
        case FactMetaData::valueTypeDouble:
            typedValue = QVariant(rawValue.toDouble(&convertOk));
            if (!convertOnly && convertOk) {
                if (rawMin() > typedValue || typedValue > rawMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toDouble()).arg(cookedMax().toDouble());
                }
            }
            break;
    }
    
    if (!convertOk) {
        errorString += "Invalid number";
    }
    
    return convertOk && errorString.isEmpty();
}

bool FactMetaData::convertAndValidateCooked(const QVariant& cookedValue, bool convertOnly, QVariant& typedValue, QString& errorString)
{
    bool convertOk = false;

    errorString.clear();

    switch (type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue = QVariant(cookedValue.toInt(&convertOk));
            if (!convertOnly && convertOk) {
                if (cookedMin() > typedValue || typedValue > cookedMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toInt()).arg(cookedMax().toInt());
                }
            }
            break;

        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue = QVariant(cookedValue.toUInt(&convertOk));
            if (!convertOnly && convertOk) {
                if (cookedMin() > typedValue || typedValue > cookedMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toUInt()).arg(cookedMax().toUInt());
                }
            }
            break;

        case FactMetaData::valueTypeFloat:
            typedValue = QVariant(cookedValue.toFloat(&convertOk));
            if (!convertOnly && convertOk) {
                if (cookedMin() > typedValue || typedValue > cookedMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toFloat()).arg(cookedMax().toFloat());
                }
            }
            break;

        case FactMetaData::valueTypeDouble:
            typedValue = QVariant(cookedValue.toDouble(&convertOk));
            if (!convertOnly && convertOk) {
                if (cookedMin() > typedValue || typedValue > cookedMax()) {
                    errorString = QString("Value must be within %1 and %2").arg(cookedMin().toDouble()).arg(cookedMax().toDouble());
                }
            }
            break;
    }

    if (!convertOk) {
        errorString += "Invalid number";
    }

    return convertOk && errorString.isEmpty();
}

void FactMetaData::setBitmaskInfo(const QStringList& strings, const QVariantList& values)
{
    if (strings.count() != values.count()) {
        qWarning() << "Count mismatch strings:values" << strings.count() << values.count();
        return;
    }

    _bitmaskStrings = strings;
    _bitmaskValues = values;
    _setBuiltInTranslator();
}

void FactMetaData::addBitmaskInfo(const QString& name, const QVariant& value)
{
    _bitmaskStrings << name;
    _bitmaskValues << value;
}

void FactMetaData::setEnumInfo(const QStringList& strings, const QVariantList& values)
{
    if (strings.count() != values.count()) {
        qWarning() << "Count mismatch strings:values" << strings.count() << values.count();
        return;
    }

    _enumStrings = strings;
    _enumValues = values;
    _setBuiltInTranslator();
}

void FactMetaData::addEnumInfo(const QString& name, const QVariant& value)
{
    _enumStrings << name;
    _enumValues << value;
}

void FactMetaData::setTranslators(Translator rawTranslator, Translator cookedTranslator)
{
    _rawTranslator = rawTranslator;
    _cookedTranslator = cookedTranslator;
}

void FactMetaData::_setBuiltInTranslator(void)
{
    if (_enumStrings.count()) {
        // No translation if enum
        setTranslators(_defaultTranslator, _defaultTranslator);
        _cookedUnits = _rawUnits;
    } else {
        for (size_t i=0; i<sizeof(_rgBuildInTranslations)/sizeof(_rgBuildInTranslations[0]); i++) {
            const BuiltInTranslation_s* pBuiltInTranslation = &_rgBuildInTranslations[i];

            if (pBuiltInTranslation->rawUnits == _rawUnits.toLower()) {
                _cookedUnits = pBuiltInTranslation->cookedUnits;
                setTranslators(pBuiltInTranslation->rawTranslator, pBuiltInTranslation->cookedTranslator);
            }
        }
    }
}

QVariant FactMetaData::_degreesToRadians(const QVariant& degrees)
{
    return QVariant(degrees.toDouble() * (M_PI / 180.0));
}

QVariant FactMetaData::_radiansToDegrees(const QVariant& radians)
{
    return QVariant(radians.toDouble() * (180 / M_PI));
}

QVariant FactMetaData::_centiDegreesToDegrees(const QVariant& centiDegrees)
{
    return QVariant(centiDegrees.toFloat() / 100.0f);
}

QVariant FactMetaData::_degreesToCentiDegrees(const QVariant& degrees)
{
    return QVariant((unsigned int)(degrees.toFloat() * 100.0f));
}

void FactMetaData::setRawUnits(const QString& rawUnits)
{
    _rawUnits = rawUnits;
    _cookedUnits = rawUnits;

    _setBuiltInTranslator();
}
