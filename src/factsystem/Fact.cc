#include "Fact.h"
#include <limits>
#include <QVariant>
#include <QMetaProperty>

typedef struct {
    Fact::Provider_t    provider;
    const char*         prefix;
} ProviderToPrefix_t;

static const ProviderToPrefix_t s_rgProviderToPrefix[] = {
    { Fact::parameterProvider,  "P." },
    { Fact::telemetryProvider,  "T." },
    { Fact::ruleProvider,       "R." },
    { Fact::settingProvider,    "S." },
};

class s_TranslationValidator {
public:
    s_TranslationValidator(void)
    {
        // We need to validate that our translation arrays are in the correct order. This makes sure that
        // we can index directly into them using the enum values and get the right information back.
        
        Q_ASSERT(s_rgProviderToPrefix[Fact::parameterProvider].provider == Fact::parameterProvider);
        Q_ASSERT(s_rgProviderToPrefix[Fact::telemetryProvider].provider == Fact::telemetryProvider);
        Q_ASSERT(s_rgProviderToPrefix[Fact::ruleProvider].provider == Fact::ruleProvider);
        Q_ASSERT(s_rgProviderToPrefix[Fact::settingProvider].provider == Fact::settingProvider);
    }
};

static s_TranslationValidator validate;

const char* Fact::_bitmaskLabel = "bitmask";

Fact::Fact(void) :
    _provider(parameterProvider),
    _value(std::numeric_limits<float>::quiet_NaN()),
    _type(valueTypeFloat),
    _cArray(0),
    _modified(false),
    _rangeMin(std::numeric_limits<float>::quiet_NaN()),
    _rangeMax(std::numeric_limits<float>::quiet_NaN()),
    _increment(std::numeric_limits<float>::quiet_NaN()),
    _defaultValue(std::numeric_limits<float>::quiet_NaN()),
    _readOnly(true)
{
    static bool registered = false;
    if (!registered) {
        qRegisterMetaType< QList<ValueLabel_t> >("QList<ValueLabel_t>");
        registered = true;
    }
}

Fact::Fact(const Fact& other) :
    _provider(other._provider),
    _id(other._id),
    _value(other._value),
    _type(other._type),
    _cArray(other._cArray),
    _modified(other._modified),
    _group(other._group),
    _name(other._name),
    _units(other._units),
    _description(other._description),
    _rangeMin(other._rangeMin),
    _rangeMax(other._rangeMax),
    _increment(other._increment),
    _defaultValue(other._defaultValue),
    _readOnly(other._readOnly),
    _valueLabels(other._valueLabels)
{
}

Fact& Fact::operator=(const Fact& other)
{
    _provider = other._provider;
    _id = other._id;
    _value = other._value;
    _type = other._type;
    _cArray = other._cArray;
    _modified = other._modified;
    _group = other._group;
    _name = other._name;
    _units = other._units;
    _description = other._description;
    _rangeMin = other._rangeMin;
    _rangeMax = other._rangeMax;
    _increment = other._increment;
    _defaultValue = other._defaultValue;
    _readOnly = other._readOnly;
    _valueLabels = other._valueLabels;
    
    return *this;
}

/// @return Returns the list of available fact providers. When writing code which needs to handle multiple providers
/// you should use the provider list instead of referencing individual providers. This allows you to write code
/// which will continue to work when providers are added or removed in future versions.
Fact::ProviderList_t Fact::getProviderList(void)
{
    ProviderList_t list;
    
    for (size_t i=0; i<sizeof(s_rgProviderToPrefix)/sizeof(s_rgProviderToPrefix[0]); i++) {
        list += s_rgProviderToPrefix[i].provider;
    }
    return list;
}

/// @return Returns the prefix associated with the specified provider.
const char* Fact::getProviderPrefix(Provider_t provider)
{
    return s_rgProviderToPrefix[provider].prefix;
}

bool Fact::parseProviderFactIdReference(const QString& providerFactIdStr, Provider_t& provider, QString& factId)
{
    for (size_t i=0; i<sizeof(s_rgProviderToPrefix)/sizeof(s_rgProviderToPrefix[0]); i++) {
        if (providerFactIdStr.startsWith(s_rgProviderToPrefix[i].prefix, Qt::CaseInsensitive)) {
            provider = s_rgProviderToPrefix[i].provider;
            factId = providerFactIdStr.right(providerFactIdStr.length() - strlen(s_rgProviderToPrefix[i].prefix));
            return true;
        }
    }
    
    return false;
}

bool Fact::valueIsBitmask(void) const
{
    return _units == _bitmaskLabel;
}

const Fact::ValueLabel_t& Fact::valueLabelAt(int index)
{
    Q_ASSERT(index >= 0 && index <= _valueLabels.count());
    
    return _valueLabels[index];
}

bool Fact::isValidProperty(const QString& property)
{
    return staticMetaObject.indexOfProperty(property.toAscii().constData()) != -1;
}

QString Fact::valuePropertyName(void)
{
    for (int i=0; i<staticMetaObject.propertyCount(); i++) {
        QMetaProperty metaProperty = staticMetaObject.property(i);
        if (metaProperty.isUser()) {
            return metaProperty.name();
        }
    }
    Q_ASSERT(false);
    
    return QString();
}

bool Fact::isValueProperty(const QString& property)
{
    int index = staticMetaObject.indexOfProperty(property.toAscii().constData());
    Q_ASSERT(index != -1);
    QMetaProperty metaProperty = staticMetaObject.property(index);
    return metaProperty.isUser();
}

bool Fact::lookupValueLabel(float value, QString& label)
{
    QListIterator<ValueLabel_t> i(_valueLabels);
    while (i.hasNext()) {
        const ValueLabel_t& valueLabel = i.next();
        if (valueLabel.value == value) {
            label = valueLabel.label;
            return true;
        }
    }
    
    return false;
}
