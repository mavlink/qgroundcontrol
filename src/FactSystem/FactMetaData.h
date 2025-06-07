/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

Q_DECLARE_LOGGING_CATEGORY(FactMetaDataLog)

/// Holds the meta data associated with a Fact. This is kept in a separate object from the Fact itself
/// since you may have multiple instances of the same Fact. But there is only ever one FactMetaData
/// instance or each Fact.
class FactMetaData : public QObject
{
    Q_OBJECT

public:
    enum ValueType_t {
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
    };
    Q_ENUM(ValueType_t)

    typedef QVariant (*Translator)(const QVariant &from);

    // Custom function to validate a cooked value.
    //  @return Error string for failed validation explanation to user. Empty string indicates no error.
    typedef QString (*CustomCookedValidator)(const QVariant &cookedValue);

    typedef QMap<QString /* param Name */, FactMetaData*> NameToMetaDataMap_t;

    explicit FactMetaData(QObject *parent = nullptr);
    explicit FactMetaData(ValueType_t type, QObject *parent = nullptr);
    explicit FactMetaData(ValueType_t type, const QString &name, QObject *parent = nullptr);
    explicit FactMetaData(const FactMetaData &other, QObject *parent = nullptr);
    ~FactMetaData();

    typedef QMap<QString, QString> DefineMap_t;

    static QMap<QString, FactMetaData*> createMapFromJsonFile(const QString &jsonFilename, QObject *metaDataParent);
    static QMap<QString, FactMetaData*> createMapFromJsonArray(const QJsonArray &jsonArray, const DefineMap_t &defineMap, QObject *metaDataParent);

    static FactMetaData *createFromJsonObject(const QJsonObject &json, const QMap<QString, QString> &defineMap, QObject *metaDataParent);

    const FactMetaData &operator=(const FactMetaData &other);

    /// Converts from meters to the user specified horizontal distance unit
    static QVariant metersToAppSettingsHorizontalDistanceUnits(const QVariant &meters);

    /// Converts from user specified horizontal distance unit to meters
    static QVariant appSettingsHorizontalDistanceUnitsToMeters(const QVariant &distance);

    /// Returns the string for horizontal distance units which has configued by user
    static QString appSettingsHorizontalDistanceUnitsString();

    /// Converts from meters to the user specified vertical distance unit
    static QVariant metersToAppSettingsVerticalDistanceUnits(const QVariant &meters);

    /// Converts from user specified vertical distance unit to meters
    static QVariant appSettingsVerticalDistanceUnitsToMeters(const QVariant &distance);

    /// Returns the string for vertical distance units which has configued by user
    static QString appSettingsVerticalDistanceUnitsString();

    /// Converts from grams to the user specified weight unit
    static QVariant gramsToAppSettingsWeightUnits(const QVariant &grams);

    /// Converts from user specified weight unit to grams
    static QVariant appSettingsWeightUnitsToGrams(const QVariant &weight);

    /// Returns the string for weight units which has configued by user
    static QString appSettingsWeightUnitsString();

    /// Converts from meters to the user specified distance unit
    static QVariant squareMetersToAppSettingsAreaUnits(const QVariant &squareMeters);

    /// Converts from user specified distance unit to meters
    static QVariant appSettingsAreaUnitsToSquareMeters(const QVariant &area);

    /// Returns the string for distance units which has configued by user
    static QString appSettingsAreaUnitsString();

    /// Converts from meters/second to the user specified speed unit
    static QVariant metersSecondToAppSettingsSpeedUnits(const QVariant &metersSecond);

    /// Converts from user specified speed unit to meters/second
    static QVariant appSettingsSpeedUnitsToMetersSecond(const QVariant &speed);

    /// Returns the string for speed units which has configued by user
    static QString appSettingsSpeedUnitsString();

    static const QString defaultCategory() { return QString(kDefaultCategory); }
    static const QString defaultGroup() { return QString(kDefaultGroup); }

    // Splits a comma separated list of strings into a QStringList. Taking into account the possibility that
    // the commas may have been translated to other characters such as chinese commas.
    static QStringList splitTranslatedList(const QString &translatedList);

    int decimalPlaces() const;
    QVariant rawDefaultValue() const;
    QVariant cookedDefaultValue() const { return _rawTranslator(rawDefaultValue()); }
    bool defaultValueAvailable() const { return _defaultValueAvailable; }
    QStringList bitmaskStrings() const { return _bitmaskStrings; }
    QVariantList bitmaskValues() const { return _bitmaskValues; }
    QStringList enumStrings() const { return _enumStrings; }
    QVariantList enumValues() const { return _enumValues; }
    QString category() const { return _category; }
    QString group() const { return _group; }
    QString longDescription() const { return _longDescription;}
    QVariant rawMax() const { return _rawMax; }
    QVariant cookedMax() const;
    bool maxIsDefaultForType() const { return (_rawMax == _maxForType()); }
    QVariant rawMin() const { return _rawMin; }
    QVariant cookedMin() const;
    bool minIsDefaultForType() const { return (_rawMin == _minForType()); }
    QString name() const { return _name; }
    QString shortDescription() const { return _shortDescription; }
    ValueType_t type() const { return _type; }
    QString rawUnits() const { return _rawUnits; }
    QString cookedUnits() const { return _cookedUnits; }
    bool vehicleRebootRequired() const { return _vehicleRebootRequired; }
    bool qgcRebootRequired() const { return _qgcRebootRequired; }
    bool hasControl() const { return _hasControl; }
    bool readOnly() const { return _readOnly; }
    bool writeOnly() const { return _writeOnly; }
    bool volatileValue() const { return _volatile; }

    /// Amount to increment value when used in controls such as spin button or slider with detents.
    /// NaN for no increment available.
    double rawIncrement() const { return _rawIncrement; }
    double cookedIncrement() const;

    Translator rawTranslator() const { return _rawTranslator; }
    Translator cookedTranslator() const { return _cookedTranslator; }

    /// Used to add new values to the bitmask lists after the meta data has been loaded
    void addBitmaskInfo(const QString &name, const QVariant &value);

    /// Used to add new values to the enum lists after the meta data has been loaded
    void addEnumInfo(const QString &name, const QVariant &value);

    /// Used to remove values from the enum lists after the meta data has been loaded
    void removeEnumInfo(const QVariant &value);

    void setDecimalPlaces(int decimalPlaces) { _decimalPlaces = decimalPlaces; }
    void setRawDefaultValue(const QVariant &rawDefaultValue);
    void setBitmaskInfo(const QStringList &strings, const QVariantList &values);
    void setEnumInfo(const QStringList &strings, const QVariantList &values);
    void setCategory(const QString &category) { _category = category; }
    void setGroup(const QString &group) { _group = group; }
    void setLongDescription(const QString &longDescription) { _longDescription = longDescription;}
    void setRawMax(const QVariant &rawMax);
    void setRawMin(const QVariant &rawMin);
    void setName(const QString &name) { _name = name; }
    void setShortDescription(const QString &shortDescription) { _shortDescription = shortDescription; }
    void setRawUnits(const QString &rawUnits);
    void setVehicleRebootRequired(bool rebootRequired) { _vehicleRebootRequired = rebootRequired; }
    void setQGCRebootRequired(bool rebootRequired) { _qgcRebootRequired = rebootRequired; }
    void setRawIncrement(double increment) { _rawIncrement = increment; }
    void setHasControl(bool bValue) { _hasControl = bValue; }
    void setReadOnly(bool bValue) { _readOnly = bValue; }
    void setWriteOnly(bool bValue) { _writeOnly = bValue; }
    void setVolatileValue(bool bValue);

    void setTranslators(Translator rawTranslator, Translator cookedTranslator);

    /// Set the translators to the standard built in versions
    void setBuiltInTranslator();

    /// Converts the specified raw value, validating against meta data
    ///     @param rawValue: Value to convert, can be string
    ///     @param convertOnly: true: convert to correct type only, do not validate against meta data
    ///     @param typeValue: Converted value, correctly typed
    ///     @param errorString: Error string if convert fails, values are cooked values since user visible
    /// @returns false: Convert failed, errorString set
    bool convertAndValidateRaw(const QVariant &rawValue, bool convertOnly, QVariant &typedValue, QString &errorString) const;

    /// Same as convertAndValidateRaw except for cookedValue input
    bool convertAndValidateCooked(const QVariant &cookedValue, bool convertOnly, QVariant &typedValue, QString &errorString) const;

    /// Converts the specified cooked value and clamps it (max/min)
    ///     @param cookedValue: Value to convert, can be string
    ///     @param typeValue: Converted value, correctly typed and clamped
    /// @returns false: Convertion failed
    bool clampValue(const QVariant &cookedValue, QVariant &typedValue) const;

    /// Sets a custom cooked validator function for this metadata. The custom validator will be called
    /// prior to the standard validator when convertAndValidateCooked is called.
    void setCustomCookedValidator(CustomCookedValidator customValidator) { _customCookedValidator = customValidator; }

    static constexpr int kDefaultDecimalPlaces = 3;  ///< Default value for decimal places if not specified/known
    static constexpr int kUnknownDecimalPlaces = -1; ///< Number of decimal places to specify is not known

    static ValueType_t stringToType(const QString &typeString, bool &unknownType);
    static QString typeToString(ValueType_t type);
    static size_t typeToSize(ValueType_t type);

    static QVariant minForType(ValueType_t type);
    static QVariant maxForType(ValueType_t type);

    static constexpr const char *kDefaultCategory = QT_TRANSLATE_NOOP("FactMetaData", "Other");
    static constexpr const char *kDefaultGroup = QT_TRANSLATE_NOOP("FactMetaData", "Misc");
    static constexpr const char *qgcFileType = "FactMetaData";

private:
    QVariant _minForType() const { return minForType(_type); };
    QVariant _maxForType() const { return maxForType(_type); };
    /// Set translators according to app settings
    void _setAppSettingsTranslators();

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
    bool isInCookedLimit(const QVariant &variantValue) const {
        return ((cookedMin().value<T>() <= variantValue.value<T>()) && (variantValue.value<T>() <= cookedMax().value<T>()));
    }

    template<class T>
    bool isInRawLimit(const QVariant &variantValue) const {
        return ((rawMin().value<T>() <= variantValue.value<T>()) && (variantValue.value<T>() <= rawMax().value<T>()));
    }

    bool isInRawMinLimit(const QVariant &variantValue) const;
    bool isInRawMaxLimit(const QVariant &variantValue) const;

    static bool _parseEnum(const QString& name, const QJsonObject &jsonObject, const DefineMap_t &defineMap, QStringList &rgDescriptions, QStringList &rgValues, QString &errorString);
    static bool _parseValuesArray(const QJsonObject &jsonObject, QStringList &rgDescriptions, QList<double> &rgValues, QString &errorString);
    static bool _parseBitmaskArray(const QJsonObject &jsonObject, QStringList &rgDescriptions, QList<int> &rgValues, QString &errorString);

    // Built in translators
    static QVariant _defaultTranslator(const QVariant &from) { return from; }
    static QVariant _degreesToRadians(const QVariant &degrees);
    static QVariant _radiansToDegrees(const QVariant &radians);
    static QVariant _centiDegreesToDegrees(const QVariant &centiDegrees);
    static QVariant _degreesToCentiDegrees(const QVariant &degrees);
    static QVariant _userGimbalDegreesToMavlinkGimbalDegrees(const QVariant &userGimbalDegrees);
    static QVariant _mavlinkGimbalDegreesToUserGimbalDegrees(const QVariant &mavlinkGimbalDegrees);
    static QVariant _metersToFeet(const QVariant &meters);
    static QVariant _feetToMeters(const QVariant &feet);
    static QVariant _squareMetersToSquareKilometers(const QVariant &squareMeters);
    static QVariant _squareKilometersToSquareMeters(const QVariant &squareKilometers);
    static QVariant _squareMetersToHectares(const QVariant &squareMeters);
    static QVariant _hectaresToSquareMeters(const QVariant &hectares);
    static QVariant _squareMetersToSquareFeet(const QVariant &squareMeters);
    static QVariant _squareFeetToSquareMeters(const QVariant &squareFeet);
    static QVariant _squareMetersToAcres(const QVariant &squareMeters);
    static QVariant _acresToSquareMeters(const QVariant &acres);
    static QVariant _squareMetersToSquareMiles(const QVariant &squareMeters);
    static QVariant _squareMilesToSquareMeters(const QVariant &squareMiles);
    static QVariant _metersPerSecondToMilesPerHour(const QVariant &metersPerSecond);
    static QVariant _milesPerHourToMetersPerSecond(const QVariant &milesPerHour);
    static QVariant _metersPerSecondToKilometersPerHour(const QVariant &metersPerSecond);
    static QVariant _kilometersPerHourToMetersPerSecond(const QVariant &kilometersPerHour);
    static QVariant _metersPerSecondToKnots(const QVariant &metersPerSecond);
    static QVariant _knotsToMetersPerSecond(const QVariant &knots);
    static QVariant _percentToNorm(const QVariant &percent);
    static QVariant _normToPercent(const QVariant &normalized);
    static QVariant _centimetersToInches(const QVariant &centimeters);
    static QVariant _inchesToCentimeters(const QVariant &inches);
    static QVariant _celsiusToFarenheit(const QVariant &celsius);
    static QVariant _farenheitToCelsius(const QVariant &farenheit);
    static QVariant _kilogramsToGrams(const QVariant &kg);
    static QVariant _ouncesToGrams(const QVariant &oz);
    static QVariant _poundsToGrams(const QVariant &lbs);
    static QVariant _gramsToKilograms(const QVariant &g);
    static QVariant _gramsToOunces(const QVariant &g);
    static QVariant _gramsToPunds(const QVariant &g);

    enum UnitTypes {
        UnitHorizontalDistance = 0,
        UnitVerticalDistance,
        UnitArea,
        UnitSpeed,
        UnitTemperature,
        UnitWeight
    };

    struct AppSettingsTranslation_s {
        QString rawUnits;
        const char *cookedUnits = nullptr;
        UnitTypes unitType = UnitHorizontalDistance;
        uint32_t unitOption = 0;
        Translator rawTranslator;
        Translator cookedTranslator;
    };

    static const AppSettingsTranslation_s *_findAppSettingsUnitsTranslation(const QString &rawUnits, UnitTypes type);

    static void _loadJsonDefines(const QJsonObject &jsonDefinesObject, QMap<QString, QString> &defineMap);

    ValueType_t _type = valueTypeInt32; // must be first for correct constructor init
    int _decimalPlaces = kUnknownDecimalPlaces;
    QVariant _rawDefaultValue = 0;
    bool _defaultValueAvailable = false;
    QStringList _bitmaskStrings;
    QVariantList _bitmaskValues;
    QStringList _enumStrings;
    QVariantList _enumValues;
    QString _category = kDefaultCategory;
    QString _group = kDefaultGroup;
    QString _longDescription;
    QVariant _rawMax = _maxForType();
    QVariant _rawMin = _minForType();
    QString _name;
    QString _shortDescription;
    QString _rawUnits;
    QString _cookedUnits;
    Translator _rawTranslator = _defaultTranslator;
    Translator _cookedTranslator = _defaultTranslator;
    bool _vehicleRebootRequired = false;
    bool _qgcRebootRequired = false;
    double _rawIncrement = std::numeric_limits<double>::quiet_NaN();
    bool _hasControl = true;
    bool _readOnly = false;
    bool _writeOnly = false;
    bool _volatile = false;
    CustomCookedValidator _customCookedValidator = nullptr;

    // Exact conversion constants
    static constexpr struct UnitConsts_s {
        static constexpr const qreal secondsPerHour = 3600.0;
        static constexpr const qreal knotsToKPH = 1.852;
        static constexpr const qreal milesToMeters = 1609.344;
        static constexpr const qreal feetToMeters = 0.3048;
        static constexpr const qreal inchesToCentimeters = 2.54;
        static constexpr const qreal ouncesToGrams = 28.3495;
        static constexpr const qreal poundsToGrams = 453.592;
        static constexpr const qreal acresToSquareMeters = 4046.86;
        static constexpr const qreal squareMetersToAcres = 0.000247105;
        static constexpr const qreal feetToSquareMeters = 0.0929;
        static constexpr const qreal squareMetersToSquareFeet = 10.7639;
        static constexpr const qreal squareMetersToSquareMiles = 3.86102e-7;
        static constexpr const qreal squareMilesToSquareMeters = 2589988.11;
    } constants{};

    struct BuiltInTranslation_s {
        QString rawUnits;
        const char *cookedUnits;
        Translator rawTranslator;
        Translator cookedTranslator;
    };

    static const BuiltInTranslation_s _rgBuiltInTranslations[];
    static const AppSettingsTranslation_s _rgAppSettingsTranslations[];

    static constexpr const char *_jsonMetaDataDefinesName = "QGC.MetaData.Defines";
    static constexpr const char *_jsonMetaDataFactsName = "QGC.MetaData.Facts";
    static constexpr const char *_enumStringsJsonKey = "enumStrings";
    static constexpr const char *_enumValuesJsonKey = "enumValues";

    // This is the newer json format for enums and bitmasks. They are used by the new COMPONENT_METADATA parameter metadata for example.
    static constexpr const char *_enumValuesArrayJsonKey = "values";
    static constexpr const char *_enumBitmaskArrayJsonKey = "bitmask";
    static constexpr const char *_enumValuesArrayValueJsonKey = "value";
    static constexpr const char *_enumValuesArrayDescriptionJsonKey = "description";
    static constexpr const char *_enumBitmaskArrayIndexJsonKey = "index";
    static constexpr const char *_enumBitmaskArrayDescriptionJsonKey = "description";

    static constexpr const char *_rgKnownTypeStrings[] = {
        "Uint8",
        "Int8",
        "Uint16",
        "Int16",
        "Uint32",
        "Int32",
        "Uint64",
        "Int64",
        "Float",
        "Double",
        "String",
        "Bool",
        "ElapsedSeconds",
        "Custom",
    };

    static constexpr const ValueType_t _rgKnownValueTypes[] = {
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
        valueTypeElapsedTimeInSeconds,
        valueTypeCustom,
    };

    static constexpr const char *_decimalPlacesJsonKey = "decimalPlaces";
    static constexpr const char *_nameJsonKey = "name";
    static constexpr const char *_typeJsonKey = "type";
    static constexpr const char *_shortDescriptionJsonKey = "shortDesc";
    static constexpr const char *_longDescriptionJsonKey = "longDesc";
    static constexpr const char *_unitsJsonKey = "units";
    static constexpr const char *_defaultValueJsonKey = "default";
    static constexpr const char *_mobileDefaultValueJsonKey = "mobileDefault";
    static constexpr const char *_minJsonKey = "min";
    static constexpr const char *_maxJsonKey = "max";
    static constexpr const char *_incrementJsonKey = "increment";
    static constexpr const char *_hasControlJsonKey = "control";
    static constexpr const char *_qgcRebootRequiredJsonKey = "qgcRebootRequired";
    static constexpr const char *_rebootRequiredJsonKey = "rebootRequired";
    static constexpr const char *_categoryJsonKey = "category";
    static constexpr const char *_groupJsonKey = "group";
    static constexpr const char *_volatileJsonKey = "volatile";
};
