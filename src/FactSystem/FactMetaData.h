/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>

/// Holds the meta data associated with a Fact.
///
/// Holds the meta data associated with a Fact. This is kept in a separate object from the Fact itself
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
        valueTypeUint64,
        valueTypeInt64,
        valueTypeFloat,
        valueTypeDouble,
        valueTypeString,
        valueTypeBool,
        valueTypeElapsedTimeInSeconds,  // Internally stored as double, valueString displays as HH:MM:SS
        valueTypeCustom,                // Internally stored as a QByteArray
    } ValueType_t;

    typedef QVariant (*Translator)(const QVariant& from);

    // Custom function to validate a cooked value.
    //  @return Error string for failed validation explanation to user. Empty string indicates no error.
    typedef QString (*CustomCookedValidator)(const QVariant& cookedValue);

    typedef QMap<QString /* param Name */, FactMetaData*> NameToMetaDataMap_t;

    FactMetaData(QObject* parent = nullptr);
    FactMetaData(ValueType_t type, QObject* parent = nullptr);
    FactMetaData(ValueType_t type, const QString name, QObject* parent = nullptr);
    FactMetaData(const FactMetaData& other, QObject* parent = nullptr);

    typedef QMap<QString, QString> DefineMap_t;

    static QMap<QString, FactMetaData*> createMapFromJsonFile(const QString& jsonFilename, QObject* metaDataParent);
    static QMap<QString, FactMetaData*> createMapFromJsonArray(const QJsonArray jsonArray, DefineMap_t& defineMap, QObject* metaDataParent);

    static FactMetaData* createFromJsonObject(const QJsonObject& json, QMap<QString, QString>& defineMap, QObject* metaDataParent);

    const FactMetaData& operator=(const FactMetaData& other);

    /// Converts from meters to the user specified horizontal distance unit
    static QVariant metersToAppSettingsHorizontalDistanceUnits(const QVariant& meters);

    /// Converts from user specified horizontal distance unit to meters
    static QVariant appSettingsHorizontalDistanceUnitsToMeters(const QVariant& distance);

    /// Returns the string for horizontal distance units which has configued by user
    static QString appSettingsHorizontalDistanceUnitsString(void);

    /// Converts from meters to the user specified vertical distance unit
    static QVariant metersToAppSettingsVerticalDistanceUnits(const QVariant& meters);

    /// Converts from user specified vertical distance unit to meters
    static QVariant appSettingsVerticalDistanceUnitsToMeters(const QVariant& distance);

    /// Returns the string for vertical distance units which has configued by user
    static QString appSettingsVerticalDistanceUnitsString(void);

    /// Converts from grams to the user specified weight unit
    static QVariant gramsToAppSettingsWeightUnits(const QVariant& grams);

    /// Converts from user specified weight unit to grams
    static QVariant appSettingsWeightUnitsToGrams(const QVariant& weight);

    /// Returns the string for weight units which has configued by user
    static QString appSettingsWeightUnitsString(void);

    /// Converts from meters to the user specified distance unit
    static QVariant squareMetersToAppSettingsAreaUnits(const QVariant& squareMeters);

    /// Converts from user specified distance unit to meters
    static QVariant appSettingsAreaUnitsToSquareMeters(const QVariant& area);

    /// Returns the string for distance units which has configued by user
    static QString appSettingsAreaUnitsString(void);

    /// Returns the string for speed units which has configued by user
    static QString appSettingsSpeedUnitsString();

    static const QString defaultCategory    ();
    static const QString defaultGroup       ();

    int             decimalPlaces           (void) const;
    QVariant        rawDefaultValue         (void) const;
    QVariant        cookedDefaultValue      (void) const { return _rawTranslator(rawDefaultValue()); }
    bool            defaultValueAvailable   (void) const { return _defaultValueAvailable; }
    QStringList     bitmaskStrings          (void) const { return _bitmaskStrings; }
    QVariantList    bitmaskValues           (void) const { return _bitmaskValues; }
    QStringList     enumStrings             (void) const { return _enumStrings; }
    QVariantList    enumValues              (void) const { return _enumValues; }
    QString         category                (void) const { return _category; }
    QString         group                   (void) const { return _group; }
    QString         longDescription         (void) const { return _longDescription;}
    QVariant        rawMax                  (void) const { return _rawMax; }
    QVariant        cookedMax               (void) const;
    bool            maxIsDefaultForType     (void) const { return _maxIsDefaultForType; }
    QVariant        rawMin                  (void) const { return _rawMin; }
    QVariant        cookedMin               (void) const;
    bool            minIsDefaultForType     (void) const { return _minIsDefaultForType; }
    QString         name                    (void) const { return _name; }
    QString         shortDescription        (void) const { return _shortDescription; }
    ValueType_t     type                    (void) const { return _type; }
    QString         rawUnits                (void) const { return _rawUnits; }
    QString         cookedUnits             (void) const { return _cookedUnits; }
    bool            vehicleRebootRequired   (void) const { return _vehicleRebootRequired; }
    bool            qgcRebootRequired       (void) const { return _qgcRebootRequired; }
    bool            hasControl              (void) const { return _hasControl; }
    bool            readOnly                (void) const { return _readOnly; }
    bool            writeOnly               (void) const { return _writeOnly; }
    bool            volatileValue           (void) const { return _volatile; }

    /// Amount to increment value when used in controls such as spin button or slider with detents.
    /// NaN for no increment available.
    double          rawIncrement            (void) const { return _rawIncrement; }
    double          cookedIncrement         (void) const;

    Translator      rawTranslator           (void) const { return _rawTranslator; }
    Translator      cookedTranslator        (void) const { return _cookedTranslator; }

    /// Used to add new values to the bitmask lists after the meta data has been loaded
    void addBitmaskInfo(const QString& name, const QVariant& value);

    /// Used to add new values to the enum lists after the meta data has been loaded
    void addEnumInfo(const QString& name, const QVariant& value);

    /// Used to remove values from the enum lists after the meta data has been loaded
    void removeEnumInfo(const QVariant& value);

    void setDecimalPlaces           (int decimalPlaces)                 { _decimalPlaces = decimalPlaces; }
    void setRawDefaultValue         (const QVariant& rawDefaultValue);
    void setBitmaskInfo             (const QStringList& strings, const QVariantList& values);
    void setEnumInfo                (const QStringList& strings, const QVariantList& values);
    void setCategory                (const QString& category)           { _category = category; }
    void setGroup                   (const QString& group)              { _group = group; }
    void setLongDescription         (const QString& longDescription)    { _longDescription = longDescription;}
    void setRawMax                  (const QVariant& rawMax);
    void setRawMin                  (const QVariant& rawMin);
    void setName                    (const QString& name)               { _name = name; }
    void setShortDescription        (const QString& shortDescription)   { _shortDescription = shortDescription; }
    void setRawUnits                (const QString& rawUnits);
    void setVehicleRebootRequired   (bool rebootRequired)               { _vehicleRebootRequired = rebootRequired; }
    void setQGCRebootRequired       (bool rebootRequired)               { _qgcRebootRequired = rebootRequired; }
    void setRawIncrement            (double increment)                  { _rawIncrement = increment; }
    void setHasControl              (bool bValue)                       { _hasControl = bValue; }
    void setReadOnly                (bool bValue)                       { _readOnly = bValue; }
    void setWriteOnly               (bool bValue)                       { _writeOnly = bValue; }
    void setVolatileValue           (bool bValue);

    void setTranslators(Translator rawTranslator, Translator cookedTranslator);

    /// Set the translators to the standard built in versions
    void setBuiltInTranslator(void);

    /// Converts the specified raw value, validating against meta data
    ///     @param rawValue: Value to convert, can be string
    ///     @param convertOnly: true: convert to correct type only, do not validate against meta data
    ///     @param typeValue: Converted value, correctly typed
    ///     @param errorString: Error string if convert fails, values are cooked values since user visible
    /// @returns false: Convert failed, errorString set
    bool convertAndValidateRaw(const QVariant& rawValue, bool convertOnly, QVariant& typedValue, QString& errorString);

    /// Same as convertAndValidateRaw except for cookedValue input
    bool convertAndValidateCooked(const QVariant& cookedValue, bool convertOnly, QVariant& typedValue, QString& errorString);

    /// Converts the specified cooked value and clamps it (max/min)
    ///     @param cookedValue: Value to convert, can be string
    ///     @param typeValue: Converted value, correctly typed and clamped
    /// @returns false: Convertion failed
    bool clampValue(const QVariant& cookedValue, QVariant& typedValue);

    /// Sets a custom cooked validator function for this metadata. The custom validator will be called
    /// prior to the standard validator when convertAndValidateCooked is called.
    void setCustomCookedValidator(CustomCookedValidator customValidator) { _customCookedValidator = customValidator; }

    static const int kDefaultDecimalPlaces = 3;  ///< Default value for decimal places if not specified/known
    static const int kUnknownDecimalPlaces = -1; ///< Number of decimal places to specify is not known
    static const char* kDefaultCategory;
    static const char* kDefaultGroup;

    static ValueType_t stringToType(const QString& typeString, bool& unknownType);
    static QString typeToString(ValueType_t type);
    static size_t typeToSize(ValueType_t type);

    static const char* qgcFileType;

private:
    QVariant _minForType                (void) const;
    QVariant _maxForType                (void) const;
    void    _setAppSettingsTranslators  (void);

    /// Clamp a value to be within cookedMin and cookedMax
    template<class T>
    void clamp(QVariant& variantValue) const {
        if (cookedMin().value<T>() > variantValue.value<T>()) {
            variantValue = cookedMin();
        } else if(variantValue.value<T>() > cookedMax().value<T>()) {
            variantValue = cookedMax();
        }
    }

    template<class T>
    bool isInCookedLimit(const QVariant& variantValue) const {
        return cookedMin().value<T>() <= variantValue.value<T>() && variantValue.value<T>() <= cookedMax().value<T>();
    }

    template<class T>
    bool isInRawLimit(const QVariant& variantValue) const {
        return rawMin().value<T>() <= variantValue.value<T>() && variantValue.value<T>() <= rawMax().value<T>();
    }

    bool isInRawMinLimit(const QVariant& variantValue) const;
    bool isInRawMaxLimit(const QVariant& variantValue) const;

    static bool _parseEnum          (const QJsonObject& jsonObject, DefineMap_t defineMap, QStringList& rgDescriptions, QStringList& rgValues, QString& errorString);
    static bool _parseValuesArray   (const QJsonObject& jsonObject, QStringList& rgDescriptions, QList<double>& rgValues, QString& errorString);
    static bool _parseBitmaskArray  (const QJsonObject& jsonObject, QStringList& rgDescriptions, QList<int>& rgValues, QString& errorString);

    // Built in translators
    static QVariant _defaultTranslator(const QVariant& from) { return from; }
    static QVariant _degreesToRadians(const QVariant& degrees);
    static QVariant _radiansToDegrees(const QVariant& radians);
    static QVariant _centiDegreesToDegrees(const QVariant& centiDegrees);
    static QVariant _degreesToCentiDegrees(const QVariant& degrees);
    static QVariant _userGimbalDegreesToMavlinkGimbalDegrees(const QVariant& userGimbalDegrees);
    static QVariant _mavlinkGimbalDegreesToUserGimbalDegrees(const QVariant& mavlinkGimbalDegrees);
    static QVariant _metersToFeet(const QVariant& meters);
    static QVariant _feetToMeters(const QVariant& feet);
    static QVariant _squareMetersToSquareKilometers(const QVariant& squareMeters);
    static QVariant _squareKilometersToSquareMeters(const QVariant& squareKilometers);
    static QVariant _squareMetersToHectares(const QVariant& squareMeters);
    static QVariant _hectaresToSquareMeters(const QVariant& hectares);
    static QVariant _squareMetersToSquareFeet(const QVariant& squareMeters);
    static QVariant _squareFeetToSquareMeters(const QVariant& squareFeet);
    static QVariant _squareMetersToAcres(const QVariant& squareMeters);
    static QVariant _acresToSquareMeters(const QVariant& acres);
    static QVariant _squareMetersToSquareMiles(const QVariant& squareMeters);
    static QVariant _squareMilesToSquareMeters(const QVariant& squareMiles);
    static QVariant _metersPerSecondToMilesPerHour(const QVariant& metersPerSecond);
    static QVariant _milesPerHourToMetersPerSecond(const QVariant& milesPerHour);
    static QVariant _metersPerSecondToKilometersPerHour(const QVariant& metersPerSecond);
    static QVariant _kilometersPerHourToMetersPerSecond(const QVariant& kilometersPerHour);
    static QVariant _metersPerSecondToKnots(const QVariant& metersPerSecond);
    static QVariant _knotsToMetersPerSecond(const QVariant& knots);
    static QVariant _percentToNorm(const QVariant& percent);
    static QVariant _normToPercent(const QVariant& normalized);
    static QVariant _centimetersToInches(const QVariant& centimeters);
    static QVariant _inchesToCentimeters(const QVariant& inches);
    static QVariant _celsiusToFarenheit(const QVariant& celsius);
    static QVariant _farenheitToCelsius(const QVariant& farenheit);
    static QVariant _kilogramsToGrams(const QVariant& kg);
    static QVariant _ouncesToGrams(const QVariant& oz);
    static QVariant _poundsToGrams(const QVariant& lbs);
    static QVariant _gramsToKilograms(const QVariant& g);
    static QVariant _gramsToOunces(const QVariant& g);
    static QVariant _gramsToPunds(const QVariant& g);


    enum UnitTypes {
        UnitHorizontalDistance = 0,
        UnitVerticalDistance,
        UnitArea,
        UnitSpeed,
        UnitTemperature,
        UnitWeight
    };

    struct AppSettingsTranslation_s {
        QString       rawUnits;
        const char*   cookedUnits;
        UnitTypes     unitType;
        uint32_t      unitOption;
        Translator    rawTranslator;
        Translator    cookedTranslator;
    };

    static const AppSettingsTranslation_s* _findAppSettingsUnitsTranslation(const QString& rawUnits, UnitTypes type);

    static void _loadJsonDefines(const QJsonObject& jsonDefinesObject, QMap<QString, QString>& defineMap);

    ValueType_t     _type;                  // must be first for correct constructor init
    int             _decimalPlaces;
    QVariant        _rawDefaultValue;
    bool            _defaultValueAvailable;
    QStringList     _bitmaskStrings;
    QVariantList    _bitmaskValues;
    QStringList     _enumStrings;
    QVariantList    _enumValues;
    QString         _category;
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
    bool            _vehicleRebootRequired;
    bool            _qgcRebootRequired;
    double          _rawIncrement;
    bool            _hasControl;
    bool            _readOnly;
    bool            _writeOnly;
    bool            _volatile;
    CustomCookedValidator _customCookedValidator = nullptr;

    // Exact conversion constants
    static const struct UnitConsts_s {
        static const qreal secondsPerHour;
        static const qreal knotsToKPH;
        static const qreal milesToMeters;
        static const qreal feetToMeters;
        static const qreal inchesToCentimeters;
        static const qreal ouncesToGrams;
        static const qreal poundsToGrams;
    } constants;

    struct BuiltInTranslation_s {
        QString rawUnits;
        const char* cookedUnits;
        Translator  rawTranslator;
        Translator  cookedTranslator;

    };

    static const BuiltInTranslation_s _rgBuiltInTranslations[];

    static const AppSettingsTranslation_s _rgAppSettingsTranslations[];

    static const char*          _rgKnownTypeStrings[];
    static const ValueType_t    _rgKnownValueTypes[];

    static const char* _nameJsonKey;
    static const char* _decimalPlacesJsonKey;
    static const char* _typeJsonKey;
    static const char* _shortDescriptionJsonKey;
    static const char* _longDescriptionJsonKey;
    static const char* _unitsJsonKey;
    static const char* _defaultValueJsonKey;
    static const char* _mobileDefaultValueJsonKey;
    static const char* _minJsonKey;
    static const char* _maxJsonKey;
    static const char* _incrementJsonKey;
    static const char* _hasControlJsonKey;
    static const char* _qgcRebootRequiredJsonKey;
    static const char* _rebootRequiredJsonKey;
    static const char* _categoryJsonKey;
    static const char* _groupJsonKey;
    static const char* _volatileJsonKey;
    static const char* _enumStringsJsonKey;
    static const char* _enumValuesJsonKey;
    static const char* _enumValuesArrayJsonKey;
    static const char* _enumBitmaskArrayJsonKey;
    static const char* _enumValuesArrayValueJsonKey;
    static const char* _enumValuesArrayDescriptionJsonKey;
    static const char* _enumBitmaskArrayIndexJsonKey;
    static const char* _enumBitmaskArrayDescriptionJsonKey;

    static const char* _jsonMetaDataDefinesName;
    static const char* _jsonMetaDataFactsName;
};
