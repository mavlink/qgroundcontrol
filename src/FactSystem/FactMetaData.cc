/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Object which exposes a FactMetaData
///
///     @author Don Gagne <don@thegagnes.com>

#include "FactMetaData.h"
#include "QGroundControlQmlGlobal.h"

#include <QDebug>
#include <QtMath>

#include <limits>
#include <cmath>

// Conversion Constants
// Time
const qreal FactMetaData::UnitConsts_s::secondsPerHour = 3600.0;

// Velocity
const qreal FactMetaData::UnitConsts_s::knotsToKPH = 1.852; // exact, hence weird base for knotsToMetersPerSecond

// Length
const qreal FactMetaData::UnitConsts_s::milesToMeters = 1609.344;
const qreal FactMetaData::UnitConsts_s::feetToMeters = 0.3048;

// Built in translations for all Facts
const FactMetaData::BuiltInTranslation_s FactMetaData::_rgBuiltInTranslations[] = {
    { "centi-degrees",  "deg",  FactMetaData::_centiDegreesToDegrees,   FactMetaData::_degreesToCentiDegrees },
    { "radians",        "deg",  FactMetaData::_radiansToDegrees,        FactMetaData::_degreesToRadians },
    { "norm",           "%",    FactMetaData::_normToPercent,           FactMetaData::_percentToNorm },
};

// Translations driven by app settings
const FactMetaData::AppSettingsTranslation_s FactMetaData::_rgAppSettingsTranslations[] = {
    { "m",      "m",        false,  QGroundControlQmlGlobal::DistanceUnitsMeters,           FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "meters", "meters",   false,  QGroundControlQmlGlobal::DistanceUnitsMeters,           FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "m/s",    "m/s",      true,   QGroundControlQmlGlobal::SpeedUnitsMetersPerSecond,     FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "m^2",    "m^2",      false,  QGroundControlQmlGlobal::AreaUnitsSquareMeters,         FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "m",      "ft",       false,  QGroundControlQmlGlobal::DistanceUnitsFeet,             FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "meters", "ft",       false,  QGroundControlQmlGlobal::DistanceUnitsFeet,             FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "m^2",    "km^2",     false,  QGroundControlQmlGlobal::AreaUnitsSquareKilometers,     FactMetaData::_squareMetersToSquareKilometers,      FactMetaData::_squareKilometersToSquareMeters },
    { "m^2",    "ha",       false,  QGroundControlQmlGlobal::AreaUnitsHectares,             FactMetaData::_squareMetersToHectares,              FactMetaData::_hectaresToSquareMeters },
    { "m^2",    "ft^2",     false,  QGroundControlQmlGlobal::AreaUnitsSquareFeet,           FactMetaData::_squareMetersToSquareFeet,            FactMetaData::_squareFeetToSquareMeters },
    { "m^2",    "ac",       false,  QGroundControlQmlGlobal::AreaUnitsAcres,                FactMetaData::_squareMetersToAcres,                 FactMetaData::_acresToSquareMeters },
    { "m^2",    "mi^2",     false,  QGroundControlQmlGlobal::AreaUnitsSquareMiles,          FactMetaData::_squareMetersToSquareMiles,           FactMetaData::_squareMilesToSquareMeters },
    { "m/s",    "ft/s",     true,   QGroundControlQmlGlobal::SpeedUnitsFeetPerSecond,       FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "m/s",    "mph",      true,   QGroundControlQmlGlobal::SpeedUnitsMilesPerHour,        FactMetaData::_metersPerSecondToMilesPerHour,       FactMetaData::_milesPerHourToMetersPerSecond },
    { "m/s",    "km/h",     true,   QGroundControlQmlGlobal::SpeedUnitsKilometersPerHour,   FactMetaData::_metersPerSecondToKilometersPerHour,  FactMetaData::_kilometersPerHourToMetersPerSecond },
    { "m/s",    "kn",       true,   QGroundControlQmlGlobal::SpeedUnitsKnots,               FactMetaData::_metersPerSecondToKnots,              FactMetaData::_knotsToMetersPerSecond },
};

FactMetaData::FactMetaData(QObject* parent)
    : QObject(parent)
    , _type(valueTypeInt32)
    , _decimalPlaces(unknownDecimalPlaces)
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
    , _increment(std::numeric_limits<double>::quiet_NaN())
{

}

FactMetaData::FactMetaData(ValueType_t type, QObject* parent)
    : QObject(parent)
    , _type(type)
    , _decimalPlaces(unknownDecimalPlaces)
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
    , _increment(std::numeric_limits<double>::quiet_NaN())
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
    _increment              = other._increment;

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
    if (rawMin >= _minForType()) {
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
    setBuiltInTranslator();
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
    setBuiltInTranslator();
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

void FactMetaData::setBuiltInTranslator(void)
{
    if (_enumStrings.count()) {
        // No translation if enum
        setTranslators(_defaultTranslator, _defaultTranslator);
        _cookedUnits = _rawUnits;
        return;
    } else {
        for (size_t i=0; i<sizeof(_rgBuiltInTranslations)/sizeof(_rgBuiltInTranslations[0]); i++) {
            const BuiltInTranslation_s* pBuiltInTranslation = &_rgBuiltInTranslations[i];

            if (pBuiltInTranslation->rawUnits == _rawUnits.toLower()) {
                _cookedUnits = pBuiltInTranslation->cookedUnits;
                setTranslators(pBuiltInTranslation->rawTranslator, pBuiltInTranslation->cookedTranslator);
                return;
            }
        }
    }

    // Translator not yet set, try app settings translators
    _setAppSettingsTranslators();
}

QVariant FactMetaData::_degreesToRadians(const QVariant& degrees)
{
    return QVariant(qDegreesToRadians(degrees.toDouble()));
}

QVariant FactMetaData::_radiansToDegrees(const QVariant& radians)
{
    return QVariant(qRadiansToDegrees(radians.toDouble()));
}

QVariant FactMetaData::_centiDegreesToDegrees(const QVariant& centiDegrees)
{
    return QVariant(centiDegrees.toReal() / 100.0);
}

QVariant FactMetaData::_degreesToCentiDegrees(const QVariant& degrees)
{
    return QVariant(qRound(degrees.toReal() * 100.0));
}

QVariant FactMetaData::_metersToFeet(const QVariant& meters)
{
    return QVariant(meters.toDouble() * 1.0/constants.feetToMeters);
}

QVariant FactMetaData::_feetToMeters(const QVariant& feet)
{
    return QVariant(feet.toDouble() * constants.feetToMeters);
}

QVariant FactMetaData::_squareMetersToSquareKilometers(const QVariant& squareMeters)
{
    return QVariant(squareMeters.toDouble() * 0.000001);
}

QVariant FactMetaData::_squareKilometersToSquareMeters(const QVariant& squareKilometers)
{
    return QVariant(squareKilometers.toDouble() * 1000000.0);
}

QVariant FactMetaData::_squareMetersToHectares(const QVariant& squareMeters)
{
    return QVariant(squareMeters.toDouble() * 0.0001);
}

QVariant FactMetaData::_hectaresToSquareMeters(const QVariant& hectares)
{
    return QVariant(hectares.toDouble() * 1000.0);
}

QVariant FactMetaData::_squareMetersToSquareFeet(const QVariant& squareMeters)
{
    return QVariant(squareMeters.toDouble() * 10.7639);
}

QVariant FactMetaData::_squareFeetToSquareMeters(const QVariant& squareFeet)
{
    return QVariant(squareFeet.toDouble() * 0.0929);
}

QVariant FactMetaData::_squareMetersToAcres(const QVariant& squareMeters)
{
    return QVariant(squareMeters.toDouble() * 0.000247105);
}

QVariant FactMetaData::_acresToSquareMeters(const QVariant& acres)
{
    return QVariant(acres.toDouble() * 4046.86);
}

QVariant FactMetaData::_squareMetersToSquareMiles(const QVariant& squareMeters)
{
    return QVariant(squareMeters.toDouble() * 3.86102e-7);
}

QVariant FactMetaData::_squareMilesToSquareMeters(const QVariant& squareMiles)
{
    return QVariant(squareMiles.toDouble() * 258999039.98855);
}

QVariant FactMetaData::_metersPerSecondToMilesPerHour(const QVariant& metersPerSecond)
{
    return QVariant((metersPerSecond.toDouble() * 1.0/constants.milesToMeters) * constants.secondsPerHour);
}

QVariant FactMetaData::_milesPerHourToMetersPerSecond(const QVariant& milesPerHour)
{
    return QVariant((milesPerHour.toDouble() * constants.milesToMeters) / constants.secondsPerHour);
}

QVariant FactMetaData::_metersPerSecondToKilometersPerHour(const QVariant& metersPerSecond)
{
    return QVariant((metersPerSecond.toDouble() / 1000.0) * constants.secondsPerHour);
}

QVariant FactMetaData::_kilometersPerHourToMetersPerSecond(const QVariant& kilometersPerHour)
{
    return QVariant((kilometersPerHour.toDouble() * 1000.0) / constants.secondsPerHour);
}

QVariant FactMetaData::_metersPerSecondToKnots(const QVariant& metersPerSecond)
{
    return QVariant(metersPerSecond.toDouble() * constants.secondsPerHour / (1000.0 * constants.knotsToKPH));
}

QVariant FactMetaData::_knotsToMetersPerSecond(const QVariant& knots)
{
    return QVariant(knots.toDouble() * (1000.0 * constants.knotsToKPH / constants.secondsPerHour));
}

QVariant FactMetaData::_percentToNorm(const QVariant& percent)
{
    return QVariant(percent.toDouble() / 100.0);
}

QVariant FactMetaData::_normToPercent(const QVariant& normalized)
{
    return QVariant(normalized.toDouble() * 100.0);
}

void FactMetaData::setRawUnits(const QString& rawUnits)
{
    _rawUnits = rawUnits;
    _cookedUnits = rawUnits;

    setBuiltInTranslator();
}

FactMetaData::ValueType_t FactMetaData::stringToType(const QString& typeString, bool& unknownType)
{
    QStringList         knownTypeStrings;
    QList<ValueType_t>  knownTypes;

    unknownType = false;

    knownTypeStrings << QStringLiteral("Uint8")
                        << QStringLiteral("Int8")
                        << QStringLiteral("Uint16")
                        << QStringLiteral("Int16")
                        << QStringLiteral("Uint32")
                        << QStringLiteral("Int32")
                        << QStringLiteral("Float")
                        << QStringLiteral("Double");

    knownTypes << valueTypeUint8
                << valueTypeInt8
                << valueTypeUint16
                << valueTypeInt16
                << valueTypeUint32
                << valueTypeInt32
                << valueTypeFloat
                << valueTypeDouble;

    for (int i=0; i<knownTypeStrings.count(); i++) {
        if (knownTypeStrings[i].compare(typeString, Qt::CaseInsensitive) == 0) {
            return knownTypes[i];
        }
    }

    unknownType = true;

    return valueTypeDouble;
}

size_t FactMetaData::typeToSize(ValueType_t type)
{
    switch (type) {
        case valueTypeUint8:
        case valueTypeInt8:
            return 1;

        case valueTypeUint16:
        case valueTypeInt16:
            return 2;

        case valueTypeUint32:
        case valueTypeInt32:
        case valueTypeFloat:
            return 4;

        case valueTypeDouble:
            return 8;

        default:
            qWarning() << "Unsupported fact value type" << type;
            return 4;
    }
}


/// Set translators according to app settings
void FactMetaData::_setAppSettingsTranslators(void)
{
    if (!_enumStrings.count()) {
        for (size_t i=0; i<sizeof(_rgAppSettingsTranslations)/sizeof(_rgAppSettingsTranslations[0]); i++) {
            const AppSettingsTranslation_s* pAppSettingsTranslation = &_rgAppSettingsTranslations[i];

            if (pAppSettingsTranslation->rawUnits == _rawUnits.toLower() &&
                    ((pAppSettingsTranslation->speed && pAppSettingsTranslation->speedOrDistanceUnits == QGroundControlQmlGlobal::speedUnits()->rawValue().toUInt()) ||
                     (!pAppSettingsTranslation->speed && pAppSettingsTranslation->speedOrDistanceUnits == QGroundControlQmlGlobal::distanceUnits()->rawValue().toUInt()))) {
                _cookedUnits = pAppSettingsTranslation->cookedUnits;
                setTranslators(pAppSettingsTranslation->rawTranslator, pAppSettingsTranslation->cookedTranslator);
                return;
            }
        }
    }
}

const FactMetaData::AppSettingsTranslation_s* FactMetaData::_findAppSettingsDistanceUnitsTranslation(const QString& rawUnits)
{
    for (size_t i=0; i<sizeof(_rgAppSettingsTranslations)/sizeof(_rgAppSettingsTranslations[0]); i++) {
        const AppSettingsTranslation_s* pAppSettingsTranslation = &_rgAppSettingsTranslations[i];

        if (pAppSettingsTranslation->rawUnits == rawUnits &&
                 (!pAppSettingsTranslation->speed && pAppSettingsTranslation->speedOrDistanceUnits == QGroundControlQmlGlobal::distanceUnits()->rawValue().toUInt())) {
            return pAppSettingsTranslation;
        }
    }

    return NULL;
}

const FactMetaData::AppSettingsTranslation_s* FactMetaData::_findAppSettingsAreaUnitsTranslation(const QString& rawUnits)
{
    for (size_t i=0; i<sizeof(_rgAppSettingsTranslations)/sizeof(_rgAppSettingsTranslations[0]); i++) {
        const AppSettingsTranslation_s* pAppSettingsTranslation = &_rgAppSettingsTranslations[i];

        if (pAppSettingsTranslation->rawUnits == rawUnits &&
                 (!pAppSettingsTranslation->speed && pAppSettingsTranslation->speedOrDistanceUnits == QGroundControlQmlGlobal::areaUnits()->rawValue().toUInt())
                ) {
            return pAppSettingsTranslation;
        }
    }

    return NULL;
}

QVariant FactMetaData::metersToAppSettingsDistanceUnits(const QVariant& meters)
{
    const AppSettingsTranslation_s* pAppSettingsTranslation = _findAppSettingsDistanceUnitsTranslation("m");
    if (pAppSettingsTranslation) {
        return pAppSettingsTranslation->rawTranslator(meters);
    } else {
        return meters;
    }
}

QVariant FactMetaData::appSettingsDistanceUnitsToMeters(const QVariant& distance)
{
    const AppSettingsTranslation_s* pAppSettingsTranslation = _findAppSettingsDistanceUnitsTranslation("m");
    if (pAppSettingsTranslation) {
        return pAppSettingsTranslation->cookedTranslator(distance);
    } else {
        return distance;
    }
}

QString FactMetaData::appSettingsDistanceUnitsString(void)
{
    const AppSettingsTranslation_s* pAppSettingsTranslation = _findAppSettingsDistanceUnitsTranslation("m");
    if (pAppSettingsTranslation) {
        return pAppSettingsTranslation->cookedUnits;
    } else {
        return QStringLiteral("m");
    }
}

QVariant FactMetaData::squareMetersToAppSettingsAreaUnits(const QVariant& squareMeters)
{
    const AppSettingsTranslation_s* pAppSettingsTranslation = _findAppSettingsAreaUnitsTranslation("m^2");
    if (pAppSettingsTranslation) {
        return pAppSettingsTranslation->rawTranslator(squareMeters);
    } else {
        return squareMeters;
    }
}

QVariant FactMetaData::appSettingsAreaUnitsToSquareMeters(const QVariant& area)
{
    const AppSettingsTranslation_s* pAppSettingsTranslation = _findAppSettingsAreaUnitsTranslation("m^2");
    if (pAppSettingsTranslation) {
        return pAppSettingsTranslation->cookedTranslator(area);
    } else {
        return area;
    }
}

QString FactMetaData::appSettingsAreaUnitsString(void)
{
    const AppSettingsTranslation_s* pAppSettingsTranslation = _findAppSettingsAreaUnitsTranslation("m^2");
    if (pAppSettingsTranslation) {
        return pAppSettingsTranslation->cookedUnits;
    } else {
        return QStringLiteral("m^2");
    }
}

int FactMetaData::decimalPlaces(void) const
{
    int actualDecimalPlaces = defaultDecimalPlaces;
    int incrementDecimalPlaces = unknownDecimalPlaces;

    // First determine decimal places from increment
    double increment = _rawTranslator(this->increment()).toDouble();
    if (!qIsNaN(increment)) {
        double integralPart;

        // Get the fractional part only
        increment = fabs(modf(increment, &integralPart));
        if (increment == 0.0) {
            // No fractional part, so no decimal places
            incrementDecimalPlaces = 0;
        } else {
            incrementDecimalPlaces = -ceil(log10(increment));
        }
    }

    // Correct decimal places is the larger of the two, increment or meta data value
    if (incrementDecimalPlaces != unknownDecimalPlaces && _decimalPlaces == unknownDecimalPlaces) {
        actualDecimalPlaces = incrementDecimalPlaces;
    } else {
        // Adjust decimal places for cooked translation
        int settingsDecimalPlaces = _decimalPlaces == unknownDecimalPlaces ? defaultDecimalPlaces : _decimalPlaces;
        double ctest = _rawTranslator(1.0).toDouble();

        settingsDecimalPlaces += -log10(ctest);

        settingsDecimalPlaces = qMin(25, settingsDecimalPlaces);
        settingsDecimalPlaces = qMax(0, settingsDecimalPlaces);

        actualDecimalPlaces = qMax(settingsDecimalPlaces, incrementDecimalPlaces);
    }

    return actualDecimalPlaces;
}
