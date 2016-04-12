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

#ifndef FactMetaData_H
#define FactMetaData_H

#include <QObject>
#include <QString>
#include <QVariant>


/// Holds the meta data associated with a Fact.
///
/// Holds the meta data associated with a Fact. This is kept in a seperate object from the Fact itself
/// since you may have multiple instances of the same Fact. But there is only ever one FactMetaData
/// instance or each Fact.
class FactMetaData : public QObject
{
    Q_OBJECT
    
public:
    typedef enum {
        valueTypeUint8,
        valueTypeInt8,
        valueTypeUint16,
        valueTypeInt16,
        valueTypeUint32,
        valueTypeInt32,
        valueTypeFloat,
        valueTypeDouble
    } ValueType_t;

    typedef QVariant (*Translator)(const QVariant& from);
    
    FactMetaData(QObject* parent = NULL);
    FactMetaData(ValueType_t type, QObject* parent = NULL);
    FactMetaData(const FactMetaData& other, QObject* parent = NULL);

    const FactMetaData& operator=(const FactMetaData& other);

    /// Converts from meters to the user specified distance unit
    static QVariant metersToAppSettingsDistanceUnits(const QVariant& meters);

    /// Converts from user specified distance unit to meters
    static QVariant appSettingsDistanceUnitsToMeters(const QVariant& distance);

    /// Returns the string for distance units which has configued by user
    static QString appSettingsDistanceUnitsString(void);

    int             decimalPlaces           (void) const;
    QVariant        rawDefaultValue         (void) const;
    QVariant        cookedDefaultValue      (void) const { return _rawTranslator(rawDefaultValue()); }
    bool            defaultValueAvailable   (void) const { return _defaultValueAvailable; }
    QStringList     bitmaskStrings          (void) const { return _bitmaskStrings; }
    QVariantList    bitmaskValues           (void) const { return _bitmaskValues; }
    QStringList     enumStrings             (void) const { return _enumStrings; }
    QVariantList    enumValues              (void) const { return _enumValues; }
    QString         group                   (void) const { return _group; }
    QString         longDescription         (void) const { return _longDescription;}
    QVariant        rawMax                  (void) const { return _rawMax; }
    QVariant        cookedMax               (void) const { return _rawTranslator(_rawMax); }
    bool            maxIsDefaultForType     (void) const { return _maxIsDefaultForType; }
    QVariant        rawMin                  (void) const { return _rawMin; }
    QVariant        cookedMin               (void) const { return _rawTranslator(_rawMin); }
    bool            minIsDefaultForType     (void) const { return _minIsDefaultForType; }
    QString         name                    (void) const { return _name; }
    QString         shortDescription        (void) const { return _shortDescription; }
    ValueType_t     type                    (void) const { return _type; }
    QString         rawUnits                (void) const { return _rawUnits; }
    QString         cookedUnits             (void) const { return _cookedUnits; }
    bool            rebootRequired          (void) const { return _rebootRequired; }

    /// Amount to increment value when used in controls such as spin button or slider with detents.
    /// NaN for no increment available.
    double          increment               (void) const { return _increment; }

    Translator      rawTranslator           (void) const { return _rawTranslator; }
    Translator      cookedTranslator        (void) const { return _cookedTranslator; }

    /// Used to add new values to the bitmask lists after the meta data has been loaded
    void addBitmaskInfo(const QString& name, const QVariant& value);

    /// Used to add new values to the enum lists after the meta data has been loaded
    void addEnumInfo(const QString& name, const QVariant& value);

    void setDecimalPlaces   (int decimalPlaces)                 { _decimalPlaces = decimalPlaces; }
    void setRawDefaultValue (const QVariant& rawDefaultValue);
    void setBitmaskInfo     (const QStringList& strings, const QVariantList& values);
    void setEnumInfo        (const QStringList& strings, const QVariantList& values);
    void setGroup           (const QString& group)              { _group = group; }
    void setLongDescription (const QString& longDescription)    { _longDescription = longDescription;}
    void setRawMax          (const QVariant& rawMax);
    void setRawMin          (const QVariant& rawMin);
    void setName            (const QString& name)               { _name = name; }
    void setShortDescription(const QString& shortDescription)   { _shortDescription = shortDescription; }
    void setRawUnits        (const QString& rawUnits);
    void setRebootRequired  (bool rebootRequired)               { _rebootRequired = rebootRequired; }
    void setIncrement       (double increment)                  { _increment = increment; }

    void setTranslators(Translator rawTranslator, Translator cookedTranslator);

    /// Set the translators to the standard built in versions
    void setBuiltInTranslator(void);

    /// Set translators according to app settings
    void setAppSettingsTranslators(void);

    /// Converts the specified raw value, validating against meta data
    ///     @param rawValue Value to convert, can be string
    ///     @param convertOnly true: convert to correct type only, do not validate against meta data
    ///     @param typeValue Converted value, correctly typed
    ///     @param errorString Error string if convert fails, values are cooked values since user visible
    /// @returns false: Convert failed, errorString set
    bool convertAndValidateRaw(const QVariant& rawValue, bool convertOnly, QVariant& typedValue, QString& errorString);

    /// Same as convertAndValidateRaw except for cookedValue input
    bool convertAndValidateCooked(const QVariant& cookedValue, bool convertOnly, QVariant& typedValue, QString& errorString);

    static const int defaultDecimalPlaces = 3;  ///< Default value for decimal places if not specified/known
    static const int unknownDecimalPlaces = -1; ///< Number of decimal places to specify is not known

    static ValueType_t stringToType(const QString& typeString, bool& unknownType);
    static size_t typeToSize(ValueType_t type);

private:
    QVariant _minForType(void) const;
    QVariant _maxForType(void) const;

    // Built in translators
    static QVariant _defaultTranslator(const QVariant& from) { return from; }
    static QVariant _degreesToRadians(const QVariant& degrees);
    static QVariant _radiansToDegrees(const QVariant& radians);
    static QVariant _centiDegreesToDegrees(const QVariant& centiDegrees);
    static QVariant _degreesToCentiDegrees(const QVariant& degrees);
    static QVariant _metersToFeet(const QVariant& meters);
    static QVariant _feetToMeters(const QVariant& feet);
    static QVariant _metersPerSecondToMilesPerHour(const QVariant& metersPerSecond);
    static QVariant _milesPerHourToMetersPerSecond(const QVariant& milesPerHour);
    static QVariant _metersPerSecondToKilometersPerHour(const QVariant& metersPerSecond);
    static QVariant _kilometersPerHourToMetersPerSecond(const QVariant& kilometersPerHour);
    static QVariant _metersPerSecondToKnots(const QVariant& metersPerSecond);
    static QVariant _knotsToMetersPerSecond(const QVariant& knots);

    struct AppSettingsTranslation_s {
        const char* rawUnits;
        const char* cookedUnits;
        bool        speed;
        uint32_t    speedOrDistanceUnits;
        Translator  rawTranslator;
        Translator  cookedTranslator;

    };

    static const AppSettingsTranslation_s* _findAppSettingsDistanceUnitsTranslation(const QString& rawUnits);

    ValueType_t     _type;                  // must be first for correct constructor init
    int             _decimalPlaces;
    QVariant        _rawDefaultValue;
    bool            _defaultValueAvailable;
    QStringList     _bitmaskStrings;
    QVariantList    _bitmaskValues;
    QStringList     _enumStrings;
    QVariantList    _enumValues;
    QString         _group;
    QString         _longDescription;
    QVariant        _rawMax;
    bool            _maxIsDefaultForType;
    QVariant        _rawMin;
    bool            _minIsDefaultForType;
    QString         _name;
    QString         _shortDescription;
    QString         _rawUnits;
    QString         _cookedUnits;
    Translator      _rawTranslator;
    Translator      _cookedTranslator;
    bool            _rebootRequired;
    double          _increment;

    struct BuiltInTranslation_s {
        const char* rawUnits;
        const char* cookedUnits;
        Translator  rawTranslator;
        Translator  cookedTranslator;

    };
    static const BuiltInTranslation_s _rgBuiltInTranslations[];

    static const AppSettingsTranslation_s _rgAppSettingsTranslations[];
};

#endif
