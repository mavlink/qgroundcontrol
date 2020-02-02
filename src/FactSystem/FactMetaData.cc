/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FactMetaData.h"
#include "SettingsManager.h"
#include "JsonHelper.h"
#include "QGCApplication.h"

#include <QDebug>
#include <QtMath>
#include <QJsonParseError>
#include <QJsonArray>

#include <limits>
#include <cmath>

// Conversion Constants
// Time
const qreal FactMetaData::UnitConsts_s::secondsPerHour = 3600.0;

// Velocity
const qreal FactMetaData::UnitConsts_s::knotsToKPH = 1.852; // exact, hence weird base for knotsToMetersPerSecond

// Length
const qreal FactMetaData::UnitConsts_s::milesToMeters =         1609.344;
const qreal FactMetaData::UnitConsts_s::feetToMeters =          0.3048;
const qreal FactMetaData::UnitConsts_s::inchesToCentimeters =   2.54;

static const char* kDefaultCategory = QT_TRANSLATE_NOOP("FactMetaData", "Other");
static const char* kDefaultGroup    = QT_TRANSLATE_NOOP("FactMetaData", "Misc");

const char* FactMetaData::_jsonMetaDataDefinesName =    "QGC.MetaData.Defines";
const char* FactMetaData::_jsonMetaDataFactsName =      "QGC.MetaData.Facts";

// Built in translations for all Facts
const FactMetaData::BuiltInTranslation_s FactMetaData::_rgBuiltInTranslations[] = {
    { "centi-degrees",  "deg",  FactMetaData::_centiDegreesToDegrees,                   FactMetaData::_degreesToCentiDegrees },
    { "radians",        "deg",  FactMetaData::_radiansToDegrees,                        FactMetaData::_degreesToRadians },
    { "gimbal-degrees", "deg",  FactMetaData::_mavlinkGimbalDegreesToUserGimbalDegrees, FactMetaData::_userGimbalDegreesToMavlinkGimbalDegrees },
    { "norm",           "%",    FactMetaData::_normToPercent,                           FactMetaData::_percentToNorm },
};

// Translations driven by app settings
const FactMetaData::AppSettingsTranslation_s FactMetaData::_rgAppSettingsTranslations[] = {
    { "m",      "m",        FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsMeters,         FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "meter",  "meter",    FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsMeters,         FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "meters", "meters",   FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsMeters,         FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "cm/px",  "cm/px",    FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsMeters,         FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "m/s",    "m/s",      FactMetaData::UnitSpeed,       UnitsSettings::SpeedUnitsMetersPerSecond,   FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "C",      "C",        FactMetaData::UnitTemperature, UnitsSettings::TemperatureUnitsCelsius,     FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "m^2",    "m^2",      FactMetaData::UnitArea,        UnitsSettings::AreaUnitsSquareMeters,       FactMetaData::_defaultTranslator,                   FactMetaData::_defaultTranslator },
    { "m",      "ft",       FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsFeet,           FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "meter",  "ft",       FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsFeet,           FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "meters", "ft",       FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsFeet,           FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "cm/px",  "in/px",    FactMetaData::UnitDistance,    UnitsSettings::DistanceUnitsFeet,           FactMetaData::_centimetersToInches,                 FactMetaData::_inchesToCentimeters },
    { "m^2",    "km^2",     FactMetaData::UnitArea,        UnitsSettings::AreaUnitsSquareKilometers,   FactMetaData::_squareMetersToSquareKilometers,      FactMetaData::_squareKilometersToSquareMeters },
    { "m^2",    "ha",       FactMetaData::UnitArea,        UnitsSettings::AreaUnitsHectares,           FactMetaData::_squareMetersToHectares,              FactMetaData::_hectaresToSquareMeters },
    { "m^2",    "ft^2",     FactMetaData::UnitArea,        UnitsSettings::AreaUnitsSquareFeet,         FactMetaData::_squareMetersToSquareFeet,            FactMetaData::_squareFeetToSquareMeters },
    { "m^2",    "ac",       FactMetaData::UnitArea,        UnitsSettings::AreaUnitsAcres,              FactMetaData::_squareMetersToAcres,                 FactMetaData::_acresToSquareMeters },
    { "m^2",    "mi^2",     FactMetaData::UnitArea,        UnitsSettings::AreaUnitsSquareMiles,        FactMetaData::_squareMetersToSquareMiles,           FactMetaData::_squareMilesToSquareMeters },
    { "m/s",    "ft/s",     FactMetaData::UnitSpeed,       UnitsSettings::SpeedUnitsFeetPerSecond,     FactMetaData::_metersToFeet,                        FactMetaData::_feetToMeters },
    { "m/s",    "mph",      FactMetaData::UnitSpeed,       UnitsSettings::SpeedUnitsMilesPerHour,      FactMetaData::_metersPerSecondToMilesPerHour,       FactMetaData::_milesPerHourToMetersPerSecond },
    { "m/s",    "km/h",     FactMetaData::UnitSpeed,       UnitsSettings::SpeedUnitsKilometersPerHour, FactMetaData::_metersPerSecondToKilometersPerHour,  FactMetaData::_kilometersPerHourToMetersPerSecond },
    { "m/s",    "kn",       FactMetaData::UnitSpeed,       UnitsSettings::SpeedUnitsKnots,             FactMetaData::_metersPerSecondToKnots,              FactMetaData::_knotsToMetersPerSecond },
    { "C",      "F",        FactMetaData::UnitTemperature, UnitsSettings::TemperatureUnitsFarenheit,   FactMetaData::_celsiusToFarenheit,                  FactMetaData::_farenheitToCelsius },
};

const char* FactMetaData::_decimalPlacesJsonKey =       "decimalPlaces";
const char* FactMetaData::_nameJsonKey =                "name";
const char* FactMetaData::_typeJsonKey =                "type";
const char* FactMetaData::_shortDescriptionJsonKey =    "shortDescription";
const char* FactMetaData::_longDescriptionJsonKey =     "longDescription";
const char* FactMetaData::_unitsJsonKey =               "units";
const char* FactMetaData::_defaultValueJsonKey =        "defaultValue";
const char* FactMetaData::_mobileDefaultValueJsonKey =  "mobileDefaultValue";
const char* FactMetaData::_minJsonKey =                 "min";
const char* FactMetaData::_maxJsonKey =                 "max";
const char* FactMetaData::_incrementJsonKey =           "increment";
const char* FactMetaData::_hasControlJsonKey =          "control";
const char* FactMetaData::_qgcRebootRequiredJsonKey =   "qgcRebootRequired";

FactMetaData::FactMetaData(QObject* parent)
    : QObject               (parent)
    , _type                 (valueTypeInt32)
    , _decimalPlaces        (kUnknownDecimalPlaces)
    , _rawDefaultValue      (0)
    , _defaultValueAvailable(false)
    , _rawMax               (_maxForType())
    , _maxIsDefaultForType  (true)
    , _rawMin               (_minForType())
    , _minIsDefaultForType  (true)
    , _rawTranslator        (_defaultTranslator)
    , _cookedTranslator     (_defaultTranslator)
    , _vehicleRebootRequired(false)
    , _qgcRebootRequired    (false)
    , _rawIncrement         (std::numeric_limits<double>::quiet_NaN())
    , _hasControl           (true)
    , _readOnly             (false)
    , _writeOnly            (false)
    , _volatile             (false)
{
    _category   = kDefaultCategory;
    _group      = kDefaultGroup;
}

FactMetaData::FactMetaData(ValueType_t type, QObject* parent)
    : QObject               (parent)
    , _type                 (type)
    , _decimalPlaces        (kUnknownDecimalPlaces)
    , _rawDefaultValue      (0)
    , _defaultValueAvailable(false)
    , _rawMax               (_maxForType())
    , _maxIsDefaultForType  (true)
    , _rawMin               (_minForType())
    , _minIsDefaultForType  (true)
    , _rawTranslator        (_defaultTranslator)
    , _cookedTranslator     (_defaultTranslator)
    , _vehicleRebootRequired(false)
    , _qgcRebootRequired    (false)
    , _rawIncrement         (std::numeric_limits<double>::quiet_NaN())
    , _hasControl           (true)
    , _readOnly             (false)
    , _writeOnly            (false)
    , _volatile             (false)
{
    _category   = kDefaultCategory;
    _group      = kDefaultGroup;
}

FactMetaData::FactMetaData(const FactMetaData& other, QObject* parent)
    : QObject(parent)
{
    *this = other;
}

FactMetaData::FactMetaData(ValueType_t type, const QString name, QObject* parent)
    : QObject               (parent)
    , _type                 (type)
    , _decimalPlaces        (kUnknownDecimalPlaces)
    , _rawDefaultValue      (0)
    , _defaultValueAvailable(false)
    , _rawMax               (_maxForType())
    , _maxIsDefaultForType  (true)
    , _rawMin               (_minForType())
    , _minIsDefaultForType  (true)
    , _name                 (name)
    , _rawTranslator        (_defaultTranslator)
    , _cookedTranslator     (_defaultTranslator)
    , _vehicleRebootRequired(false)
    , _qgcRebootRequired    (false)
    , _rawIncrement         (std::numeric_limits<double>::quiet_NaN())
    , _hasControl           (true)
    , _readOnly             (false)
    , _writeOnly            (false)
    , _volatile             (false)
{
    _category   = kDefaultCategory;
    _group      = kDefaultGroup;
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
    _category               = other._category;
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
    _vehicleRebootRequired  = other._vehicleRebootRequired;
    _qgcRebootRequired      = other._qgcRebootRequired;
    _rawIncrement           = other._rawIncrement;
    _hasControl             = other._hasControl;
    _readOnly               = other._readOnly;
    _writeOnly              = other._writeOnly;
    _volatile               = other._volatile;
    return *this;
}

const QString FactMetaData::defaultCategory()
{
    return QString(kDefaultCategory);
}

const QString FactMetaData::defaultGroup()
{
    return QString(kDefaultGroup);
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
    if (_type == valueTypeString || (_rawMin <= rawDefaultValue && rawDefaultValue <= _rawMax)) {
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
        return QVariant(std::numeric_limits<uint32_t>::min());
    case valueTypeInt32:
        return QVariant(std::numeric_limits<int32_t>::min());
    case valueTypeUint64:
        return QVariant((qulonglong)std::numeric_limits<uint64_t>::min());
    case valueTypeInt64:
        return QVariant((qlonglong)std::numeric_limits<int64_t>::min());
    case valueTypeFloat:
        return QVariant(-std::numeric_limits<float>::max());
    case valueTypeDouble:
        return QVariant(-std::numeric_limits<double>::max());
    case valueTypeString:
        return QVariant();
    case valueTypeBool:
        return QVariant(0);
    case valueTypeElapsedTimeInSeconds:
        return QVariant(0.0);
    case valueTypeCustom:
        return QVariant();
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
        return QVariant(std::numeric_limits<uint32_t>::max());
    case valueTypeInt32:
        return QVariant(std::numeric_limits<int32_t>::max());
    case valueTypeUint64:
        return QVariant((qulonglong)std::numeric_limits<uint64_t>::max());
    case valueTypeInt64:
        return QVariant((qlonglong)std::numeric_limits<int64_t>::max());
    case valueTypeFloat:
        return QVariant(std::numeric_limits<float>::max());
    case valueTypeElapsedTimeInSeconds:
    case valueTypeDouble:
        return QVariant(std::numeric_limits<double>::max());
    case valueTypeString:
        return QVariant();
    case valueTypeBool:
        return QVariant(1);
    case valueTypeCustom:
        return QVariant();
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
            if (typedValue < rawMin() || typedValue > rawMax()) {
                errorString = tr("Value must be within %1 and %2").arg(rawMin().toInt()).arg(rawMax().toInt());
            }
        }
        break;
    case FactMetaData::valueTypeInt64:
        typedValue = QVariant(rawValue.toLongLong(&convertOk));
        if (!convertOnly && convertOk) {
            if (typedValue < rawMin() || typedValue > rawMax()) {
                errorString = tr("Value must be within %1 and %2").arg(rawMin().toInt()).arg(rawMax().toInt());
            }
        }
        break;
    case FactMetaData::valueTypeUint8:
    case FactMetaData::valueTypeUint16:
    case FactMetaData::valueTypeUint32:
        typedValue = QVariant(rawValue.toUInt(&convertOk));
        if (!convertOnly && convertOk) {
            if (typedValue < rawMin() || typedValue > rawMax()) {
                errorString = tr("Value must be within %1 and %2").arg(rawMin().toUInt()).arg(rawMax().toUInt());
            }
        }
        break;
    case FactMetaData::valueTypeUint64:
        typedValue = QVariant(rawValue.toULongLong(&convertOk));
        if (!convertOnly && convertOk) {
            if (typedValue < rawMin() || typedValue > rawMax()) {
                errorString = tr("Value must be within %1 and %2").arg(rawMin().toUInt()).arg(rawMax().toUInt());
            }
        }
        break;
    case FactMetaData::valueTypeFloat:
        typedValue = QVariant(rawValue.toFloat(&convertOk));
        if (!convertOnly && convertOk) {
            if (typedValue < rawMin() || typedValue > rawMax()) {
                errorString = tr("Value must be within %1 and %2").arg(rawMin().toDouble()).arg(rawMax().toDouble());
            }
        }
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    case FactMetaData::valueTypeDouble:
        typedValue = QVariant(rawValue.toDouble(&convertOk));
        if (!convertOnly && convertOk) {
            if (typedValue < rawMin() || typedValue > rawMax()) {
                errorString = tr("Value must be within %1 and %2").arg(rawMin().toDouble()).arg(rawMax().toDouble());
            }
        }
        break;
    case FactMetaData::valueTypeString:
        convertOk = true;
        typedValue = QVariant(rawValue.toString());
        break;
    case FactMetaData::valueTypeBool:
        convertOk = true;
        typedValue = QVariant(rawValue.toBool());
        break;
    case FactMetaData::valueTypeCustom:
        convertOk = true;
        typedValue = QVariant(rawValue.toByteArray());
        break;
    }

    if (!convertOk) {
        errorString += tr("Invalid number");
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
                errorString = tr("Value must be within %1 and %2").arg(cookedMin().toInt()).arg(cookedMax().toInt());
            }
        }
        break;
    case FactMetaData::valueTypeInt64:
        typedValue = QVariant(cookedValue.toLongLong(&convertOk));
        if (!convertOnly && convertOk) {
            if (cookedMin() > typedValue || typedValue > cookedMax()) {
                errorString = tr("Value must be within %1 and %2").arg(cookedMin().toInt()).arg(cookedMax().toInt());
            }
        }
        break;
    case FactMetaData::valueTypeUint8:
    case FactMetaData::valueTypeUint16:
    case FactMetaData::valueTypeUint32:
        typedValue = QVariant(cookedValue.toUInt(&convertOk));
        if (!convertOnly && convertOk) {
            if (cookedMin() > typedValue || typedValue > cookedMax()) {
                errorString = tr("Value must be within %1 and %2").arg(cookedMin().toUInt()).arg(cookedMax().toUInt());
            }
        }
        break;
    case FactMetaData::valueTypeUint64:
        typedValue = QVariant(cookedValue.toULongLong(&convertOk));
        if (!convertOnly && convertOk) {
            if (cookedMin() > typedValue || typedValue > cookedMax()) {
                errorString = tr("Value must be within %1 and %2").arg(cookedMin().toUInt()).arg(cookedMax().toUInt());
            }
        }
        break;
    case FactMetaData::valueTypeFloat:
        typedValue = QVariant(cookedValue.toFloat(&convertOk));
        if (!convertOnly && convertOk) {
            if (cookedMin() > typedValue || typedValue > cookedMax()) {
                errorString = tr("Value must be within %1 and %2").arg(cookedMin().toFloat()).arg(cookedMax().toFloat());
            }
        }
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    case FactMetaData::valueTypeDouble:
        typedValue = QVariant(cookedValue.toDouble(&convertOk));
        if (!convertOnly && convertOk) {
            if (cookedMin() > typedValue || typedValue > cookedMax()) {
                errorString = tr("Value must be within %1 and %2").arg(cookedMin().toDouble()).arg(cookedMax().toDouble());
            }
        }
        break;
    case FactMetaData::valueTypeString:
        convertOk = true;
        typedValue = QVariant(cookedValue.toString());
        break;
    case FactMetaData::valueTypeBool:
        convertOk = true;
        typedValue = QVariant(cookedValue.toBool());
        break;
    case FactMetaData::valueTypeCustom:
        convertOk = true;
        typedValue = QVariant(cookedValue.toByteArray());
        break;
    }

    if (!convertOk) {
        errorString += tr("Invalid number");
    }

    return convertOk && errorString.isEmpty();
}

bool FactMetaData::clampValue(const QVariant& cookedValue, QVariant& typedValue)
{
    bool convertOk = false;
    switch (type()) {
    case FactMetaData::valueTypeInt8:
    case FactMetaData::valueTypeInt16:
    case FactMetaData::valueTypeInt32:
        typedValue = QVariant(cookedValue.toInt(&convertOk));
        if (convertOk) {
            if (cookedMin() > typedValue) {
                typedValue = cookedMin();
            } else if(typedValue > cookedMax()) {
                typedValue = cookedMax();
            }
        }
        break;
    case FactMetaData::valueTypeInt64:
        typedValue = QVariant(cookedValue.toLongLong(&convertOk));
        if (convertOk) {
            if (cookedMin() > typedValue) {
                typedValue = cookedMin();
            } else if(typedValue > cookedMax()) {
                typedValue = cookedMax();
            }
        }
        break;
    case FactMetaData::valueTypeUint8:
    case FactMetaData::valueTypeUint16:
    case FactMetaData::valueTypeUint32:
        typedValue = QVariant(cookedValue.toUInt(&convertOk));
        if (convertOk) {
            if (cookedMin() > typedValue) {
                typedValue = cookedMin();
            } else if(typedValue > cookedMax()) {
                typedValue = cookedMax();
            }
        }
        break;
    case FactMetaData::valueTypeUint64:
        typedValue = QVariant(cookedValue.toULongLong(&convertOk));
        if (convertOk) {
            if (cookedMin() > typedValue) {
                typedValue = cookedMin();
            } else if(typedValue > cookedMax()) {
                typedValue = cookedMax();
            }
        }
        break;
    case FactMetaData::valueTypeFloat:
        typedValue = QVariant(cookedValue.toFloat(&convertOk));
        if (convertOk) {
            if (cookedMin() > typedValue) {
                typedValue = cookedMin();
            } else if(typedValue > cookedMax()) {
                typedValue = cookedMax();
            }
        }
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    case FactMetaData::valueTypeDouble:
        typedValue = QVariant(cookedValue.toDouble(&convertOk));
        if (convertOk) {
            if (cookedMin() > typedValue) {
                typedValue = cookedMin();
            } else if(typedValue > cookedMax()) {
                typedValue = cookedMax();
            }
        }
        break;
    case FactMetaData::valueTypeString:
        convertOk = true;
        typedValue = QVariant(cookedValue.toString());
        break;
    case FactMetaData::valueTypeBool:
        convertOk = true;
        typedValue = QVariant(cookedValue.toBool());
        break;
    case FactMetaData::valueTypeCustom:
        convertOk = true;
        typedValue = QVariant(cookedValue.toByteArray());
        break;
    }
    return convertOk;
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

            if (pBuiltInTranslation->rawUnits.toLower() == _rawUnits.toLower()) {
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

QVariant FactMetaData::_userGimbalDegreesToMavlinkGimbalDegrees(const QVariant& userGimbalDegrees)
{
    // User facing gimbal degree values are from 0 (level) to 90 (straight down)
    // Mavlink gimbal degree values are from 0 (level) to -90 (straight down)
    return userGimbalDegrees.toDouble() * -1.0;
}

QVariant FactMetaData::_mavlinkGimbalDegreesToUserGimbalDegrees(const QVariant& mavlinkGimbalDegrees)
{
    // User facing gimbal degree values are from 0 (level) to 90 (straight down)
    // Mavlink gimbal degree values are from 0 (level) to -90 (straight down)
    return mavlinkGimbalDegrees.toDouble() * -1.0;
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

QVariant FactMetaData::_centimetersToInches(const QVariant& centimeters)
{
    return QVariant(centimeters.toDouble() * 1.0/constants.inchesToCentimeters);
}

QVariant FactMetaData::_inchesToCentimeters(const QVariant& inches)
{
    return QVariant(inches.toDouble() * constants.inchesToCentimeters);
}

QVariant FactMetaData::_celsiusToFarenheit(const QVariant& celsius)
{
    return QVariant(celsius.toDouble() * (9.0 / 5.0) + 32);
}

QVariant FactMetaData::_farenheitToCelsius(const QVariant& farenheit)
{
    return QVariant((farenheit.toDouble() - 32) * (5.0 / 9.0));
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
                     << QStringLiteral("Uint64")
                     << QStringLiteral("Int64")
                     << QStringLiteral("Float")
                     << QStringLiteral("Double")
                     << QStringLiteral("String")
                     << QStringLiteral("Bool")
                     << QStringLiteral("ElapsedSeconds")
                     << QStringLiteral("Custom");

    knownTypes << valueTypeUint8
               << valueTypeInt8
               << valueTypeUint16
               << valueTypeInt16
               << valueTypeUint32
               << valueTypeInt32
               << valueTypeUint64
               << valueTypeInt64
               << valueTypeFloat
               << valueTypeDouble
               << valueTypeString
               << valueTypeBool
               << valueTypeElapsedTimeInSeconds
               << valueTypeCustom;

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

    case valueTypeUint64:
    case valueTypeInt64:
    case valueTypeDouble:
        return 8;

    case valueTypeCustom:
        return MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN;

    default:
        qWarning() << "Unsupported fact value type" << type;
        return 0;
    }
}

/// Set translators according to app settings
void FactMetaData::_setAppSettingsTranslators(void)
{
    // We can only translate between real numbers
    if (!_enumStrings.count() && (type() == valueTypeDouble || type() == valueTypeFloat)) {
        for (size_t i=0; i<sizeof(_rgAppSettingsTranslations)/sizeof(_rgAppSettingsTranslations[0]); i++) {
            const AppSettingsTranslation_s* pAppSettingsTranslation = &_rgAppSettingsTranslations[i];

            if (_rawUnits.toLower() != pAppSettingsTranslation->rawUnits.toLower()) {
                continue;
            }

            UnitsSettings* settings = qgcApp()->toolbox()->settingsManager()->unitsSettings();
            uint settingsUnits = 0;

            switch (pAppSettingsTranslation->unitType) {
            case UnitDistance:
                settingsUnits = settings->distanceUnits()->rawValue().toUInt();
                break;
            case UnitSpeed:
                settingsUnits = settings->speedUnits()->rawValue().toUInt();
                break;
            case UnitArea:
                settingsUnits = settings->areaUnits()->rawValue().toUInt();
                break;
            case UnitTemperature:
                settingsUnits = settings->temperatureUnits()->rawValue().toUInt();
                break;
            default:
                break;
            }

            if (settingsUnits == pAppSettingsTranslation->unitOption) {
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

        if (rawUnits.toLower() != pAppSettingsTranslation->rawUnits.toLower()) {
            continue;
        }

        uint settingsUnits = qgcApp()->toolbox()->settingsManager()->unitsSettings()->distanceUnits()->rawValue().toUInt();

        if (pAppSettingsTranslation->unitType == UnitDistance
                && pAppSettingsTranslation->unitOption == settingsUnits) {
            return pAppSettingsTranslation;
        }
    }
    return nullptr;
}

const FactMetaData::AppSettingsTranslation_s* FactMetaData::_findAppSettingsAreaUnitsTranslation(const QString& rawUnits)
{
    for (size_t i=0; i<sizeof(_rgAppSettingsTranslations)/sizeof(_rgAppSettingsTranslations[0]); i++) {
        const AppSettingsTranslation_s* pAppSettingsTranslation = &_rgAppSettingsTranslations[i];

        if (rawUnits.toLower() != pAppSettingsTranslation->rawUnits.toLower()) {
            continue;
        }

        uint settingsUnits = qgcApp()->toolbox()->settingsManager()->unitsSettings()->areaUnits()->rawValue().toUInt();

        if (pAppSettingsTranslation->unitType == UnitArea
                && pAppSettingsTranslation->unitOption == settingsUnits) {
            return pAppSettingsTranslation;
        }
    }

    return nullptr;
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

double FactMetaData::cookedIncrement(void) const
{
    return _rawTranslator(this->rawIncrement()).toDouble();
}

int FactMetaData::decimalPlaces(void) const
{
    int actualDecimalPlaces = kDefaultDecimalPlaces;
    int incrementDecimalPlaces = kUnknownDecimalPlaces;

    // First determine decimal places from increment
    double increment = _rawTranslator(this->rawIncrement()).toDouble();
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

    if (_decimalPlaces == kUnknownDecimalPlaces) {
        if (incrementDecimalPlaces != kUnknownDecimalPlaces) {
            actualDecimalPlaces = incrementDecimalPlaces;
        } else {
            // Adjust decimal places for cooked translation
            int settingsDecimalPlaces = _decimalPlaces == kUnknownDecimalPlaces ? kDefaultDecimalPlaces : _decimalPlaces;
            double ctest = _rawTranslator(1.0).toDouble();

            settingsDecimalPlaces += -log10(ctest);

            settingsDecimalPlaces = qMin(25, settingsDecimalPlaces);
            settingsDecimalPlaces = qMax(0, settingsDecimalPlaces);
        }
    } else {
        actualDecimalPlaces = _decimalPlaces;
    }

    return actualDecimalPlaces;
}

FactMetaData* FactMetaData::createFromJsonObject(const QJsonObject& json, QMap<QString, QString>& defineMap, QObject* metaDataParent)
{
    QString         errorString;

    // Make sure we have the required keys
    QStringList requiredKeys;
    requiredKeys << _nameJsonKey << _typeJsonKey;
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
        qWarning() << errorString;
        return new FactMetaData(valueTypeUint32, metaDataParent);
    }

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { _nameJsonKey,                 QJsonValue::String, true },
        { _typeJsonKey,                 QJsonValue::String, true },
        { _shortDescriptionJsonKey,     QJsonValue::String, false },
        { _longDescriptionJsonKey,      QJsonValue::String, false },
        { _unitsJsonKey,                QJsonValue::String, false },
        { _decimalPlacesJsonKey,        QJsonValue::Double, false },
        { _minJsonKey,                  QJsonValue::Double, false },
        { _maxJsonKey,                  QJsonValue::Double, false },
        { _hasControlJsonKey,           QJsonValue::Bool,   false },
        { _qgcRebootRequiredJsonKey,    QJsonValue::Bool,   false },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
        qWarning() << errorString;
        return new FactMetaData(valueTypeUint32, metaDataParent);
    }

    bool unknownType;
    FactMetaData::ValueType_t type = FactMetaData::stringToType(json[_typeJsonKey].toString(), unknownType);
    if (unknownType) {
        qWarning() << "Unknown type" << json[_typeJsonKey].toString();
        return new FactMetaData(valueTypeUint32, metaDataParent);
    }

    FactMetaData*   metaData = new FactMetaData(type, metaDataParent);

    metaData->_name = json[_nameJsonKey].toString();

    QStringList enumValues, enumStrings;
    if (JsonHelper::parseEnum(json, defineMap, enumStrings, enumValues, errorString, metaData->name())) {
        for (int i=0; i<enumValues.count(); i++) {
            QVariant    enumVariant;
            QString     errorString;

            if (metaData->convertAndValidateRaw(enumValues[i], false /* validate */, enumVariant, errorString)) {
                metaData->addEnumInfo(enumStrings[i], enumVariant);
            } else {
                qWarning() << "Invalid enum value, name:" << metaData->name()
                           << " type:" << metaData->type()
                           << " value:" << enumValues[i]
                           << " error:" << errorString;
            }
        }
    } else {
        qWarning() << errorString;
    }

    metaData->setDecimalPlaces(json[_decimalPlacesJsonKey].toInt(0));
    metaData->setShortDescription(json[_shortDescriptionJsonKey].toString());
    metaData->setLongDescription(json[_longDescriptionJsonKey].toString());

    if (json.contains(_unitsJsonKey)) {
        metaData->setRawUnits(json[_unitsJsonKey].toString());
    }

    QString defaultValueJsonKey;
#ifdef __mobile__
    if (json.contains(_mobileDefaultValueJsonKey)) {
        defaultValueJsonKey = _mobileDefaultValueJsonKey;
    }
#endif
    if (defaultValueJsonKey.isEmpty() && json.contains(_defaultValueJsonKey)) {
        defaultValueJsonKey = _defaultValueJsonKey;
    }
    if (!defaultValueJsonKey.isEmpty()) {
        QVariant typedValue;
        QString errorString;
        QVariant initialValue = json[defaultValueJsonKey].toVariant();
        if (metaData->convertAndValidateRaw(initialValue, true /* convertOnly */, typedValue, errorString)) {
            metaData->setRawDefaultValue(typedValue);
        } else {
            qWarning() << "Invalid default value, name:" << metaData->name()
                       << " type:" << metaData->type()
                       << " value:" << initialValue
                       << " error:" << errorString;
        }
    }

    if (json.contains(_incrementJsonKey)) {
        QVariant typedValue;
        QString errorString;
        QVariant initialValue = json[_incrementJsonKey].toVariant();
        if (metaData->convertAndValidateRaw(initialValue, true /* convertOnly */, typedValue, errorString)) {
            metaData->setRawIncrement(typedValue.toDouble());
        } else {
            qWarning() << "Invalid increment value, name:" << metaData->name()
                       << " type:" << metaData->type()
                       << " value:" << initialValue
                       << " error:" << errorString;
        }
    }

    if (json.contains(_minJsonKey)) {
        QVariant typedValue;
        QString errorString;
        QVariant initialValue = json[_minJsonKey].toVariant();
        if (metaData->convertAndValidateRaw(initialValue, true /* convertOnly */, typedValue, errorString)) {
            metaData->setRawMin(typedValue);
        } else {
            qWarning() << "Invalid min value, name:" << metaData->name()
                       << " type:" << metaData->type()
                       << " value:" << initialValue
                       << " error:" << errorString;
        }
    }

    if (json.contains(_maxJsonKey)) {
        QVariant typedValue;
        QString errorString;
        QVariant initialValue = json[_maxJsonKey].toVariant();
        if (metaData->convertAndValidateRaw(initialValue, true /* convertOnly */, typedValue, errorString)) {
            metaData->setRawMax(typedValue);
        } else {
            qWarning() << "Invalid max value, name:" << metaData->name()
                       << " type:" << metaData->type()
                       << " value:" << initialValue
                       << " error:" << errorString;
        }
    }

    if (json.contains(_hasControlJsonKey)) {
        metaData->setHasControl(json[_hasControlJsonKey].toBool());
    } else {
        metaData->setHasControl(true);
    }

    if (json.contains(_qgcRebootRequiredJsonKey)) {
        metaData->setQGCRebootRequired(json[_qgcRebootRequiredJsonKey].toBool());
    } else {
        metaData->setQGCRebootRequired(false);
    }

    return metaData;
}

void FactMetaData::_loadJsonDefines(const QJsonObject& jsonDefinesObject, QMap<QString, QString>& defineMap)
{
    for (const QString& defineName: jsonDefinesObject.keys()) {
        QString mapKey = _jsonMetaDataDefinesName + QString(".") + defineName;
        defineMap[mapKey] = jsonDefinesObject[defineName].toString();
    }
}

QMap<QString, FactMetaData*> FactMetaData::createMapFromJsonFile(const QString& jsonFilename, QObject* metaDataParent)
{
    QMap<QString, FactMetaData*> metaDataMap;

    QFile jsonFile(jsonFilename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file" << jsonFilename << jsonFile.errorString();
        return metaDataMap;
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to parse json document filename:error:offset" << jsonFilename << jsonParseError.errorString() << jsonParseError.offset;
        return metaDataMap;
    }

    QJsonArray factArray;
    QMap<QString /* define name */, QString /* define value */> defineMap;

    if (doc.isObject()) {
        // Check for Defines/Facts format
        QString errorString;
        QList<JsonHelper::KeyValidateInfo> keyInfoList = {
            { FactMetaData::_jsonMetaDataDefinesName,   QJsonValue::Object, true },
            { FactMetaData::_jsonMetaDataFactsName,     QJsonValue::Array, true },
        };
        if (!JsonHelper::validateKeys(doc.object(), keyInfoList, errorString)) {
            qWarning() << "Json document incorrect format:" << errorString;
            return metaDataMap;
        }

        _loadJsonDefines(doc.object()[FactMetaData::_jsonMetaDataDefinesName].toObject(), defineMap);
        factArray = doc.object()[FactMetaData::_jsonMetaDataFactsName].toArray();
    } else if (doc.isArray()) {
        factArray = doc.array();
    } else {
        qWarning() << "Json document is neither array nor object";
        return metaDataMap;
    }

    return createMapFromJsonArray(factArray, defineMap, metaDataParent);
}

QMap<QString, FactMetaData*> FactMetaData::createMapFromJsonArray(const QJsonArray jsonArray, QMap<QString, QString>& defineMap, QObject* metaDataParent)
{
    QMap<QString, FactMetaData*> metaDataMap;
    for (int i=0; i<jsonArray.count(); i++) {
        QJsonValue jsonValue = jsonArray.at(i);
        if (!jsonValue.isObject()) {
            qWarning() << QStringLiteral("JsonValue at index %1 not an object").arg(i);
            continue;
        }
        QJsonObject jsonObject = jsonValue.toObject();
        FactMetaData* metaData = createFromJsonObject(jsonObject, defineMap, metaDataParent);
        if (metaDataMap.contains(metaData->name())) {
            qWarning() << QStringLiteral("Duplicate fact name:") << metaData->name();
            delete metaData;
        } else {
            metaDataMap[metaData->name()] = metaData;
        }
    }
    return metaDataMap;
}

QVariant FactMetaData::cookedMax(void) const
{
    // We have to be careful with cooked min/max. Running the raw values through the translator could flip min and max.
    return qMax(_rawTranslator(_rawMax), _rawTranslator(_rawMin));
}

QVariant FactMetaData::cookedMin(void) const
{
    // We have to be careful with cooked min/max. Running the raw values through the translator could flip min and max.
    return qMin(_rawTranslator(_rawMax), _rawTranslator(_rawMin));
}

void FactMetaData::setVolatileValue(bool bValue)
{
    _volatile = bValue;
    if (_volatile) {
        _readOnly = true;
    }
}
