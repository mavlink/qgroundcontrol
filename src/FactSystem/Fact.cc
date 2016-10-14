/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "Fact.h"
#include "QGCMAVLink.h"

#include <QtQml>

Fact::Fact(QObject* parent)
    : QObject(parent)
    , _componentId(-1)
    , _rawValue(0)
    , _type(FactMetaData::valueTypeInt32)
    , _metaData(NULL)
    , _sendValueChangedSignals(true)
    , _deferredValueChangeSignal(false)
{    
    FactMetaData* metaData = new FactMetaData(_type, this);
    setMetaData(metaData);
}

Fact::Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent)
    : QObject(parent)
    , _name(name)
    , _componentId(componentId)
    , _rawValue(0)
    , _type(type)
    , _metaData(NULL)
    , _sendValueChangedSignals(true)
    , _deferredValueChangeSignal(false)
{
    FactMetaData* metaData = new FactMetaData(_type, this);
    setMetaData(metaData);
}

Fact::Fact(const Fact& other, QObject* parent)
    : QObject(parent)
{
    *this = other;
}

const Fact& Fact::operator=(const Fact& other)
{
    _name                       = other._name;
    _componentId                = other._componentId;
    _rawValue                   = other._rawValue;
    _type                       = other._type;
    _sendValueChangedSignals    = other._sendValueChangedSignals;
    _deferredValueChangeSignal  = other._deferredValueChangeSignal;

    if (_metaData && other._metaData) {
        *_metaData = *other._metaData;
    } else {
        _metaData = NULL;
    }
    
    return *this;
}

void Fact::forceSetRawValue(const QVariant& value)
{
    if (_metaData) {
        QVariant    typedValue;
        QString     errorString;
        
        if (_metaData->convertAndValidateRaw(value, true /* convertOnly */, typedValue, errorString)) {
            _rawValue.setValue(typedValue);
            _sendValueChangedSignal(cookedValue());
            emit _containerRawValueChanged(rawValue());
            emit rawValueChanged(_rawValue);
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::setRawValue(const QVariant& value)
{
    if (_metaData) {
        QVariant    typedValue;
        QString     errorString;
        
        if (_metaData->convertAndValidateRaw(value, true /* convertOnly */, typedValue, errorString)) {
            if (typedValue != _rawValue) {
                _rawValue.setValue(typedValue);
                _sendValueChangedSignal(cookedValue());
                emit _containerRawValueChanged(rawValue());
                emit rawValueChanged(_rawValue);
            }
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::setCookedValue(const QVariant& value)
{
    if (_metaData) {
        setRawValue(_metaData->cookedTranslator()(value));
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::setEnumStringValue(const QString& value)
{
    if (_metaData) {
        int index = _metaData->enumStrings().indexOf(value);
        if (index != -1) {
            setCookedValue(_metaData->enumValues()[index]);
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::setEnumIndex(int index)
{
    if (_metaData) {
        setCookedValue(_metaData->enumValues()[index]);
    } else {
        qWarning() << "Meta data pointer missing";
    }
}

void Fact::_containerSetRawValue(const QVariant& value)
{
    _rawValue = value;
    _sendValueChangedSignal(cookedValue());
    emit vehicleUpdated(_rawValue);
    emit rawValueChanged(_rawValue);
}

QString Fact::name(void) const
{
    return _name;
}

int Fact::componentId(void) const
{
    return _componentId;
}

QVariant Fact::cookedValue(void) const
{
    if (_metaData) {
        return _metaData->rawTranslator()(_rawValue);
    } else {
        qWarning() << "Meta data pointer missing";
        return _rawValue;
    }
}

QString Fact::enumStringValue(void)
{
    if (_metaData) {
        int enumIndex = this->enumIndex();
        if (enumIndex >= 0 && enumIndex < _metaData->enumStrings().count()) {
            return _metaData->enumStrings()[enumIndex];
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }

    return QString();
}

int Fact::enumIndex(void)
{
    if (_metaData) {
        int index = 0;

        foreach (QVariant enumValue, _metaData->enumValues()) {
            if (enumValue == rawValue()) {
                return index;
            }
            index ++;
        }

        // Current value is not in list, add it manually
        _metaData->addEnumInfo(QString("Unknown: %1").arg(rawValue().toString()), rawValue());
        emit enumStringsChanged();
        emit enumValuesChanged();
        return index;
    } else {
        qWarning() << "Meta data pointer missing";
    }

    return -1;
}

QStringList Fact::enumStrings(void) const
{
    if (_metaData) {
        return _metaData->enumStrings();
    } else {
        qWarning() << "Meta data pointer missing";
        return QStringList();
    }
}

QVariantList Fact::enumValues(void) const
{
    if (_metaData) {
        return _metaData->enumValues();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariantList();
    }
}

QStringList Fact::bitmaskStrings(void) const
{
    if (_metaData) {
        return _metaData->bitmaskStrings();
    } else {
        qWarning() << "Meta data pointer missing";
        return QStringList();
    }
}

QVariantList Fact::bitmaskValues(void) const
{
    if (_metaData) {
        return _metaData->bitmaskValues();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariantList();
    }
}

QString Fact::_variantToString(const QVariant& variant, int decimalPlaces) const
{
    QString valueString;

    switch (type()) {
    case FactMetaData::valueTypeFloat:
    {
        float fValue = variant.toFloat();
        if (qIsNaN(fValue)) {
            valueString = QStringLiteral("--.--");
        } else {
            valueString = QString("%1").arg(fValue, 0, 'f', decimalPlaces);
        }
    }
        break;
    case FactMetaData::valueTypeDouble:
    {
        double dValue = variant.toDouble();
        if (qIsNaN(dValue)) {
            valueString = QStringLiteral("--.--");
        } else {
            valueString = QString("%1").arg(dValue, 0, 'f', decimalPlaces);
        }
    }
        break;
    default:
        valueString = variant.toString();
        break;
    }

    return valueString;
}

QString Fact::rawValueStringFullPrecision(void) const
{
    return _variantToString(rawValue(), 18);
}


QString Fact::rawValueString(void) const
{
    return _variantToString(rawValue(), decimalPlaces());
}

QString Fact::cookedValueString(void) const
{
    return _variantToString(cookedValue(), decimalPlaces());
}

QVariant Fact::rawDefaultValue(void) const
{
    if (_metaData) {
        if (!_metaData->defaultValueAvailable()) {
            qDebug() << "Access to unavailable default value";
        }
        return _metaData->rawDefaultValue();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QVariant Fact::cookedDefaultValue(void) const
{
    if (_metaData) {
        if (!_metaData->defaultValueAvailable()) {
            qDebug() << "Access to unavailable default value";
        }
        return _metaData->cookedDefaultValue();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QString Fact::cookedDefaultValueString(void) const
{
    return _variantToString(cookedDefaultValue(), decimalPlaces());
}

FactMetaData::ValueType_t Fact::type(void) const
{
    return _type;
}

QString Fact::shortDescription(void) const
{
    if (_metaData) {
        return _metaData->shortDescription();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QString Fact::longDescription(void) const
{
    if (_metaData) {
        return _metaData->longDescription();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QString Fact::rawUnits(void) const
{
    if (_metaData) {
        return _metaData->rawUnits();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QString Fact::cookedUnits(void) const
{
    if (_metaData) {
        return _metaData->cookedUnits();
    } else {
        qWarning() << "Meta data pointer missing";
        return QString();
    }
}

QVariant Fact::rawMin(void) const
{
    if (_metaData) {
        return _metaData->rawMin();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QVariant Fact::cookedMin(void) const
{
    if (_metaData) {
        return _metaData->cookedMin();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QString Fact::cookedMinString(void) const
{
    return _variantToString(cookedMin(), decimalPlaces());
}

QVariant Fact::rawMax(void) const
{
    if (_metaData) {
        return _metaData->rawMax();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QVariant Fact::cookedMax(void) const
{
    if (_metaData) {
        return _metaData->cookedMax();
    } else {
        qWarning() << "Meta data pointer missing";
        return QVariant(0);
    }
}

QString Fact::cookedMaxString(void) const
{
    return _variantToString(cookedMax(), decimalPlaces());
}

bool Fact::minIsDefaultForType(void) const
{
    if (_metaData) {
        return _metaData->minIsDefaultForType();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

bool Fact::maxIsDefaultForType(void) const
{
    if (_metaData) {
        return _metaData->maxIsDefaultForType();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

int Fact::decimalPlaces(void) const
{
    if (_metaData) {
        return _metaData->decimalPlaces();
    } else {
        qWarning() << "Meta data pointer missing";
        return FactMetaData::defaultDecimalPlaces;
    }
}

QString Fact::group(void) const
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
    emit valueChanged(cookedValue());
}

bool Fact::valueEqualsDefault(void) const
{
    if (_metaData) {
        if (_metaData->defaultValueAvailable()) {
            return _metaData->rawDefaultValue() == rawValue();
        } else {
            return false;
        }
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

bool Fact::defaultValueAvailable(void) const
{
    if (_metaData) {
        return _metaData->defaultValueAvailable();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

QString Fact::validate(const QString& cookedValue, bool convertOnly)
{
    if (_metaData) {
        QVariant    typedValue;
        QString     errorString;
        
        _metaData->convertAndValidateCooked(cookedValue, convertOnly, typedValue, errorString);
        
        return errorString;
    } else {
        qWarning() << "Meta data pointer missing";
        return QString("Internal error: Meta data pointer missing");
    }
}

bool Fact::rebootRequired(void) const
{
    if (_metaData) {
        return _metaData->rebootRequired();
    } else {
        qWarning() << "Meta data pointer missing";
        return false;
    }
}

void Fact::setSendValueChangedSignals (bool sendValueChangedSignals)
{
    if (sendValueChangedSignals != _sendValueChangedSignals) {
        _sendValueChangedSignals = sendValueChangedSignals;
        emit sendValueChangedSignalsChanged(_sendValueChangedSignals);
    }
}

void Fact::_sendValueChangedSignal(QVariant value)
{
    if (_sendValueChangedSignals) {
        emit valueChanged(value);
        _deferredValueChangeSignal = false;
    } else {
        _deferredValueChangeSignal = true;
    }
}

void Fact::sendDeferredValueChangedSignal(void)
{
    if (_deferredValueChangeSignal) {
        _deferredValueChangeSignal = false;
        emit valueChanged(cookedValue());
    }
}

QString Fact::enumOrValueString(void)
{
    if (_metaData) {
        if (_metaData->enumStrings().count()) {
            return enumStringValue();
        } else {
            return cookedValueString();
        }
    } else {
        qWarning() << "Meta data pointer missing";
    }
    return QString();
}

double Fact::increment(void) const
{
    if (_metaData) {
        return _metaData->increment();
    } else {
        qWarning() << "Meta data pointer missing";
    }
    return std::numeric_limits<double>::quiet_NaN();
}
