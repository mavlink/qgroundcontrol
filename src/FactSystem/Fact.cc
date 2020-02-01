/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Fact.h"
#include "FactValueSliderListModel.h"
#include "QGCMAVLink.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QtQml>
#include <QQmlEngine>

static const char* kMissingMetadata = "Meta data pointer missing";

Fact::Fact(QObject* parent)
    : QObject                   (parent)
    , _componentId              (-1)
    , _rawValue                 (0)
    , _type                     (FactMetaData::valueTypeInt32)
    , _metaData                 (nullptr)
    , _sendValueChangedSignals  (true)
    , _deferredValueChangeSignal(false)
    , _valueSliderModel         (nullptr)
    , _ignoreQGCRebootRequired  (false)
{    
    FactMetaData* metaData = new FactMetaData(_type, this);
    setMetaData(metaData);

    _init();
}

Fact::Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent)
    : QObject                   (parent)
    , _name                     (name)
    , _componentId              (componentId)
    , _rawValue                 (0)
    , _type                     (type)
    , _metaData                 (nullptr)
    , _sendValueChangedSignals  (true)
    , _deferredValueChangeSignal(false)
    , _valueSliderModel         (nullptr)
    , _ignoreQGCRebootRequired  (false)
{
    FactMetaData* metaData = new FactMetaData(_type, this);
    setMetaData(metaData);

    _init();
}

Fact::Fact(const QString& settingsGroup, FactMetaData* metaData, QObject* parent)
    : QObject(parent)
    , _name                     (metaData->name())
    , _componentId              (0)
    , _rawValue                 (0)
    , _type                     (metaData->type())
    , _metaData                 (nullptr)
    , _sendValueChangedSignals  (true)
    , _deferredValueChangeSignal(false)
    , _valueSliderModel         (nullptr)
    , _ignoreQGCRebootRequired  (false)
{
    qgcApp()->toolbox()->corePlugin()->adjustSettingMetaData(settingsGroup, *metaData);
    setMetaData(metaData, true /* setDefaultFromMetaData */);

    _init();
}

Fact::Fact(const Fact& other, QObject* parent)
    : QObject(parent)
{
    *this = other;

    _init();
}

void Fact::_init(void)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    connect(this, &Fact::_containerRawValueChanged, this, &Fact::_checkForRebootMessaging);
}

const Fact& Fact::operator=(const Fact& other)
{
    _name                       = other._name;
    _componentId                = other._componentId;
    _rawValue                   = other._rawValue;
    _type                       = other._type;
    _sendValueChangedSignals    = other._sendValueChangedSignals;
    _deferredValueChangeSignal  = other._deferredValueChangeSignal;
    _valueSliderModel           = nullptr;
    _ignoreQGCRebootRequired    = other._ignoreQGCRebootRequired;
    if (_metaData && other._metaData) {
        *_metaData = *other._metaData;
    } else {
        _metaData = nullptr;
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
            //-- Must be in this order
            emit _containerRawValueChanged(rawValue());
            emit rawValueChanged(_rawValue);
        }
    } else {
        qWarning() << kMissingMetadata << name();
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
                //-- Must be in this order
                emit _containerRawValueChanged(rawValue());
                emit rawValueChanged(_rawValue);
            }
        }
    } else {
        qWarning() << kMissingMetadata << name();
    }
}

void Fact::setCookedValue(const QVariant& value)
{
    if (_metaData) {
        setRawValue(_metaData->cookedTranslator()(value));
    } else {
        qWarning() << kMissingMetadata << name();
    }
}

int Fact::valueIndex(const QString& value)
{
    if (_metaData) {
        return _metaData->enumStrings().indexOf(value);
    }
    qWarning() << kMissingMetadata << name();
    return -1;
}

void Fact::setEnumStringValue(const QString& value)
{
    int index = valueIndex(value);
    if (index != -1) {
        setCookedValue(_metaData->enumValues()[index]);
    }
}

void Fact::setEnumIndex(int index)
{
    if (_metaData) {
        setCookedValue(_metaData->enumValues()[index]);
    } else {
        qWarning() << kMissingMetadata << name();
    }
}

void Fact::_containerSetRawValue(const QVariant& value)
{
    if(_rawValue != value) {
        _rawValue = value;
        _sendValueChangedSignal(cookedValue());
        emit rawValueChanged(_rawValue);
    }

    // This always need to be signalled in order to support forceSetRawValue usage and waiting for vehicleUpdated signal
    emit vehicleUpdated(_rawValue);
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
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
    }

    return QString();
}

int Fact::enumIndex(void)
{
    static const double accuracy = 1.0 / 1000000.0;
    if (_metaData) {
        //-- Only enums have an index
        if(_metaData->enumValues().count()) {
            int index = 0;
            for (QVariant enumValue: _metaData->enumValues()) {
                if (enumValue == rawValue()) {
                    return index;
                }
                //-- Float comparissons don't always work
                if(type() == FactMetaData::valueTypeFloat || type() == FactMetaData::valueTypeDouble) {
                    double diff = fabs(enumValue.toDouble() - rawValue().toDouble());
                    if(diff < accuracy) {
                        return index;
                    }
                }
                index ++;
            }
            // Current value is not in list, add it manually
            _metaData->addEnumInfo(tr("Unknown: %1").arg(rawValue().toString()), rawValue());
            emit enumsChanged();
            return index;
        }
    } else {
        qWarning() << kMissingMetadata << name();
    }
    return -1;
}

QStringList Fact::enumStrings(void) const
{
    if (_metaData) {
        return _metaData->enumStrings();
    } else {
        qWarning() << kMissingMetadata << name();
        return QStringList();
    }
}

QVariantList Fact::enumValues(void) const
{
    if (_metaData) {
        return _metaData->enumValues();
    } else {
        qWarning() << kMissingMetadata << name();
        return QVariantList();
    }
}

void Fact::setEnumInfo(const QStringList& strings, const QVariantList& values)
{
    if (_metaData) {
        _metaData->setEnumInfo(strings, values);
    } else {
        qWarning() << kMissingMetadata << name();
    }
}

QStringList Fact::bitmaskStrings(void) const
{
    if (_metaData) {
        return _metaData->bitmaskStrings();
    } else {
        qWarning() << kMissingMetadata << name();
        return QStringList();
    }
}

QVariantList Fact::bitmaskValues(void) const
{
    if (_metaData) {
        return _metaData->bitmaskValues();
    } else {
        qWarning() << kMissingMetadata << name();
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
    case FactMetaData::valueTypeBool:
        valueString = variant.toBool() ? tr("true") : tr("false");
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    {
        double dValue = variant.toDouble();
        if (qIsNaN(dValue)) {
            valueString = QStringLiteral("--:--:--");
        } else {
            QTime time(0, 0, 0, 0);
            time = time.addSecs(dValue);
            valueString = time.toString(QStringLiteral("hh:mm:ss"));
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
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::longDescription(void) const
{
    if (_metaData) {
        return _metaData->longDescription();
    } else {
        qWarning() << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::rawUnits(void) const
{
    if (_metaData) {
        return _metaData->rawUnits();
    } else {
        qWarning() << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::cookedUnits(void) const
{
    if (_metaData) {
        return _metaData->cookedUnits();
    } else {
        qWarning() << kMissingMetadata << name();
        return QString();
    }
}

QVariant Fact::rawMin(void) const
{
    if (_metaData) {
        return _metaData->rawMin();
    } else {
        qWarning() << kMissingMetadata << name();
        return QVariant(0);
    }
}

QVariant Fact::cookedMin(void) const
{
    if (_metaData) {
        return _metaData->cookedMin();
    } else {
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
        return QVariant(0);
    }
}

QVariant Fact::cookedMax(void) const
{
    if (_metaData) {
        return _metaData->cookedMax();
    } else {
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

bool Fact::maxIsDefaultForType(void) const
{
    if (_metaData) {
        return _metaData->maxIsDefaultForType();
    } else {
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

int Fact::decimalPlaces(void) const
{
    if (_metaData) {
        return _metaData->decimalPlaces();
    } else {
        qWarning() << kMissingMetadata << name();
        return FactMetaData::kDefaultDecimalPlaces;
    }
}

QString Fact::category(void) const
{
    if (_metaData) {
        return _metaData->category();
    } else {
        qWarning() << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::group(void) const
{
    if (_metaData) {
        return _metaData->group();
    } else {
        qWarning() << kMissingMetadata << name();
        return QString();
    }
}

void Fact::setMetaData(FactMetaData* metaData, bool setDefaultFromMetaData)
{
    _metaData = metaData;
    if (setDefaultFromMetaData) {
        setRawValue(rawDefaultValue());
    }
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
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

bool Fact::defaultValueAvailable(void) const
{
    if (_metaData) {
        return _metaData->defaultValueAvailable();
    } else {
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
        return QString("Internal error: Meta data pointer missing");
    }
}

QVariant Fact::clamp(const QString& cookedValue)
{
    if (_metaData) {
        QVariant typedValue;
        if(_metaData->clampValue(cookedValue, typedValue)) {
            return typedValue;
        } else {
            //-- If conversion failed, return current value
            return rawValue();
        }
    } else {
        qWarning() << kMissingMetadata << name();
    }
    return QVariant();
}

bool Fact::vehicleRebootRequired(void) const
{
    if (_metaData) {
        return _metaData->vehicleRebootRequired();
    } else {
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

bool Fact::qgcRebootRequired(void) const
{
    if (_ignoreQGCRebootRequired) {
        return false;
    } else if (_metaData) {
        return _metaData->qgcRebootRequired();
    } else {
        qWarning() << kMissingMetadata << name();
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
        qWarning() << kMissingMetadata << name();
    }
    return QString();
}

double Fact::rawIncrement(void) const
{
    if (_metaData) {
        return _metaData->rawIncrement();
    } else {
        qWarning() << kMissingMetadata << name();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Fact::cookedIncrement(void) const
{
    if (_metaData) {
        return _metaData->cookedIncrement();
    } else {
        qWarning() << kMissingMetadata << name();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

bool Fact::hasControl(void) const
{
    if (_metaData) {
        return _metaData->hasControl();
    } else {
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

bool Fact::readOnly(void) const
{
    if (_metaData) {
        return _metaData->readOnly();
    } else {
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

bool Fact::writeOnly(void) const
{
    if (_metaData) {
        return _metaData->writeOnly();
    } else {
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

bool Fact::volatileValue(void) const
{
    if (_metaData) {
        return _metaData->volatileValue();
    } else {
        qWarning() << kMissingMetadata << name();
        return false;
    }
}

FactValueSliderListModel* Fact::valueSliderModel(void)
{
    if (!_valueSliderModel) {
        _valueSliderModel = new FactValueSliderListModel(*this);
    }
    return _valueSliderModel;
}

void Fact::_checkForRebootMessaging(void)
{
    if(qgcApp()) {
        if (!qgcApp()->runningUnitTests()) {
            if (vehicleRebootRequired()) {
                qgcApp()->showMessage(tr("Change of parameter %1 requires a Vehicle reboot to take effect.").arg(name()));
            } else if (qgcRebootRequired()) {
                qgcApp()->showMessage(tr("Change of '%1' value requires restart of %2 to take effect.").arg(shortDescription()).arg(qgcApp()->applicationName()));
            }
        }
    }
}

void Fact::_setIgnoreQGCRebootRequired(bool ignore)
{
    _ignoreQGCRebootRequired = ignore;
}
