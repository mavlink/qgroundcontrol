#ifndef FACT_H
#define FACT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>
#include "mavlink.h"

class Fact : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QVariant value READ value WRITE setValue USER true)
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(Provider_t provider READ provider WRITE setProvider)
    Q_PROPERTY(ValueType_t type READ type WRITE setType)
    Q_PROPERTY(bool modified READ isModified WRITE setModified)
    Q_PROPERTY(QString group READ group WRITE setGroup)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString units READ units WRITE setUnits)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(double rangeMin READ rangeMin WRITE setRangeMin)
    Q_PROPERTY(double rangeMax READ rangeMax WRITE setRangeMax)
    Q_PROPERTY(bool hasRange READ hasRange STORED false)
    Q_PROPERTY(double increment READ increment WRITE setIncrement)
    Q_PROPERTY(bool hasIncrement READ hasIncrement STORED false)
    Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue)
    Q_PROPERTY(bool hasDefaultValue READ hasDefaultValue STORED false)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool valueIsBitmask READ valueIsBitmask STORED false)
    Q_PROPERTY(int valueLabelsCount READ valueLabelsCount STORED false)
    Q_PROPERTY(QList<ValueLabel_t> valueLabels READ valueLabels WRITE setValueLabels)
    Q_PROPERTY(bool isArray READ isArray STORED false)
    Q_PROPERTY(int arrayLength READ arrayLength WRITE setArrayLength)

    Q_ENUMS(Provider_t)
    Q_ENUMS(ValueType_t)
    
public:
    /// These are the set of providers where facts can originate from.
    typedef enum {
        parameterProvider = 0,
        telemetryProvider,
        ruleProvider,
        settingProvider,
    } Provider_t;
    
    typedef QList<Provider_t> ProviderList_t;
    
    typedef enum {
        valueTypeChar = 0,
        valueTypeUint8,
        valueTypeInt8,
        valueTypeUint16,
        valueTypeInt16,
        valueTypeUint32,
        valueTypeInt32,
        valueTypeUint64,
        valueTypeInt64,
        valueTypeFloat,
        valueTypeDouble
    } ValueType_t;
    
    typedef struct {
        int     value;
        QString label;
    } ValueLabel_t;
    
    Fact(void);
    Fact(const Fact& fact);
    Fact& operator=(const Fact& other);
    
    // Property system methods
    QVariant    value(void) const               { return _value; }
    void        setValue(const QVariant& value) { _value = value; }
    
    const   QString& id(void) const     { return _id; }
    void    setId(const QString& id)    { _id = id; }
    
    Provider_t  provider(void) const                { return _provider; }
    void        setProvider(Provider_t provider)    { _provider = provider; }
    
    ValueType_t type(void) const                { return _type; }
    void        setType(ValueType_t type)    { _type = type; }
    
    bool isModified(void) const { return _modified; }
    void setModified(bool modified) { _modified = modified; }
    
    const   QString& group(void) const      { return _group; }
    void    setGroup(const QString& group)  { _group = group; }
    
    const   QString& name(void) const       { return _name; }
    void    setName(const QString& name)    { _name = name; }
    
    const   QString& units(void) const      { return _units; }
    void    setUnits(const QString& units)  { _units = units; }
    
    const   QString& description(void) const            { return _description; }
    void    setDescription(const QString& description)  { _description = description; }
    
    double  rangeMin(void) const            { return _rangeMin; }
    void    setRangeMin(double rangeMin)    { _rangeMin = rangeMin; }
    double  rangeMax(void) const            { return _rangeMax; }
    void    setRangeMax(double rangeMax)    { _rangeMax = rangeMax; }
    bool    hasRange(void)                  { return !isnan(_rangeMin) && !isnan(_rangeMax); }
    
    double  increment(void) const           { return _increment; }
    void    setIncrement(double increment)  { _increment = increment; }
    bool    hasIncrement(void)              { return !isnan(_increment); }
    
    double  defaultValue(void) const                { return _defaultValue; }
    void    setDefaultValue(double defaultValue)    { _defaultValue = defaultValue; }
    bool    hasDefaultValue(void)                   { return !isnan(_defaultValue); }
    
    bool    isReadOnly(void) const     { return _readOnly; }
    void    setReadOnly(bool readOnly) { _readOnly = readOnly; }
    
    bool valueIsBitmask(void) const;
    
    int valueLabelsCount(void) const { return _valueLabels.count(); }
    Q_INVOKABLE const ValueLabel_t& valueLabelAt(int index);
    bool lookupValueLabel(float value, QString& label);
    void addValueLabel(ValueLabel_t& valueLabel) { _valueLabels += valueLabel; }
    
    bool    isArray(void) { return _cArray != 0; }
    int     arrayLength(void) { return _cArray; }
    void    setArrayLength(int cArray) { _cArray = cArray; }

    const QList<ValueLabel_t>& valueLabels(void) { return _valueLabels; }
    void setValueLabels(const QList<ValueLabel_t>& valueLabels) { _valueLabels = valueLabels; }
    
    static ProviderList_t getProviderList(void);
    static const char* getProviderPrefix(Provider_t provider);
    static bool parseProviderFactIdReference(const QString& providerFactIdStr, Provider_t& provider, QString& factId);
    
    static bool isValidProperty(const QString& property);
    static bool isValueProperty(const QString& property);
    static QString valuePropertyName(void);
    
private:
    Provider_t          _provider;      /// provider where this fact came from
    QString             _id;            /// fact id
    QVariant            _value;         /// current value for fact
    ValueType_t         _type;          /// fact type
    int                 _cArray;         /// number of elements in array, 0 not an array
    bool                _modified;      /// true if value was changed and not yet sent back to provider
    QString             _group;
    QString             _name;          /// human readable name
    QString             _units;
    QString             _description;
    double              _rangeMin;      /// NaN for no range
    double              _rangeMax;      /// NaN for no range
    double              _increment;     /// NaN for no increment
    double              _defaultValue;  /// NaN for no default
    bool                _readOnly;
    QList<ValueLabel_t> _valueLabels;
    
    static const char*  _bitmaskLabel;  /// _units == _bitmaskLabel means _valueLabels contains bitmask bits
};

#endif