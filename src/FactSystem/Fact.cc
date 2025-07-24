/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Fact.h"
#include "FactValueSliderListModel.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(FactLog, "qgc.factsystem.fact")

Fact::Fact(QObject *parent)
    : QObject(parent)
{
    // qCDebug(FactLog) << Q_FUNC_INFO << this;

    FactMetaData *const metaData = new FactMetaData(_type, this);
    setMetaData(metaData);

    _init();
}

Fact::Fact(int componentId, const QString &name, FactMetaData::ValueType_t type, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _componentId(componentId)
    , _type(type)
{
    // qCDebug(FactLog) << Q_FUNC_INFO << this;

    FactMetaData *const metaData = new FactMetaData(_type, this);
    setMetaData(metaData);

    _init();
}

Fact::Fact(const QString& settingsGroup, FactMetaData *metaData, QObject *parent)
    : QObject(parent)
    , _name(metaData->name())
    , _componentId(0)
    , _type(metaData->type())
{
    // qCDebug(FactLog) << Q_FUNC_INFO << this;

    QGCCorePlugin::instance()->adjustSettingMetaData(settingsGroup, *metaData);
    setMetaData(metaData, true /* setDefaultFromMetaData */);

    _init();
}

Fact::Fact(const Fact &other, QObject *parent)
    : QObject(parent)
{
    // qCDebug(FactLog) << Q_FUNC_INFO << this;

    *this = other;

    _init();
}

Fact::~Fact()
{
    // qCDebug(FactLog) << Q_FUNC_INFO << this;
}

void Fact::_init()
{
    (void) connect(this, &Fact::containerRawValueChanged, this, &Fact::_checkForRebootMessaging);
}

const Fact &Fact::operator=(const Fact& other)
{
    _name = other._name;
    _componentId = other._componentId;
    _rawValue = other._rawValue;
    _type = other._type;
    _sendValueChangedSignals = other._sendValueChangedSignals;
    _deferredValueChangeSignal = other._deferredValueChangeSignal;
    _valueSliderModel = nullptr;
    if (_metaData && other._metaData) {
        *_metaData = *other._metaData;
    } else {
        _metaData = nullptr;
    }

    return *this;
}

void Fact::forceSetRawValue(const QVariant &value)
{
    if (_metaData) {
        QVariant typedValue;
        QString errorString;

        if (_metaData->convertAndValidateRaw(value, true /* convertOnly */, typedValue, errorString)) {
            _rawValue.setValue(typedValue);
            _sendValueChangedSignal(cookedValue());
            //-- Must be in this order
            emit containerRawValueChanged(rawValue());
            emit rawValueChanged(_rawValue);
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
}

void Fact::setRawValue(const QVariant &value)
{
    if (_metaData) {
        QVariant typedValue;
        QString errorString;

        if (_metaData->convertAndValidateRaw(value, true /* convertOnly */, typedValue, errorString)) {
            if (typedValue != _rawValue) {
                _rawValue.setValue(typedValue);
                _sendValueChangedSignal(cookedValue());
                //-- Must be in this order
                emit containerRawValueChanged(rawValue());
                emit rawValueChanged(_rawValue);
            }
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
}

void Fact::setCookedValue(const QVariant& value)
{
    if (_metaData) {
        setRawValue(_metaData->cookedTranslator()(value));
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
}

int Fact::valueIndex(const QString &value) const
{
    if (_metaData) {
        return _metaData->enumStrings().indexOf(value);
    }
    qCWarning(FactLog) << kMissingMetadata << name();
    return -1;
}

void Fact::setEnumStringValue(const QString &value)
{
    const int index = valueIndex(value);
    if (index != -1) {
        setCookedValue(_metaData->enumValues()[index]);
    }
}

void Fact::setEnumIndex(int index)
{
    if (_metaData) {
        setCookedValue(_metaData->enumValues()[index]);
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
}

void Fact::containerSetRawValue(const QVariant &value)
{
    if (_rawValue != value) {
        _rawValue = value;
        _sendValueChangedSignal(cookedValue());
        emit rawValueChanged(_rawValue);
    }

    // This always need to be signalled in order to support forceSetRawValue usage and waiting for vehicleUpdated signal
    emit vehicleUpdated(_rawValue);
}

QVariant Fact::cookedValue() const
{
    if (_metaData) {
        return _metaData->rawTranslator()(_rawValue);
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return _rawValue;
    }
}

QString Fact::enumStringValue()
{
    if (_metaData) {
        const int enumIndex = this->enumIndex();
        if ((enumIndex >= 0) && (enumIndex < _metaData->enumStrings().count())) {
            return _metaData->enumStrings()[enumIndex];
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }

    return QString();
}

int Fact::enumIndex()
{
    if (_metaData) {
        //-- Only enums have an index
        if (!_metaData->enumValues().isEmpty()) {
            int index = 0;
            for (const QVariant &enumValue: _metaData->enumValues()) {
                if (enumValue == rawValue()) {
                    return index;
                }
                //-- Float comparisons don't always work
                if ((type() == FactMetaData::valueTypeFloat) || (type() == FactMetaData::valueTypeDouble)) {
                    const double diff = fabs(enumValue.toDouble() - rawValue().toDouble());
                    static constexpr double accuracy = 1.0 / 1000000.0;
                    if (diff < accuracy) {
                        return index;
                    }
                }
                index++;
            }
            // Current value is not in list, add it manually
            _metaData->addEnumInfo(tr("Unknown: %1").arg(rawValue().toString()), rawValue());
            emit enumsChanged();
            return index;
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
    return -1;
}

QStringList Fact::enumStrings() const
{
    if (_metaData) {
        return _metaData->enumStrings();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QStringList();
    }
}

QVariantList Fact::enumValues() const
{
    if (_metaData) {
        return _metaData->enumValues();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariantList();
    }
}

void Fact::setEnumInfo(const QStringList &strings, const QVariantList &values)
{
    if (_metaData) {
        _metaData->setEnumInfo(strings, values);
        emit enumsChanged();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
}

QStringList Fact::bitmaskStrings() const
{
    if (_metaData) {
        return _metaData->bitmaskStrings();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QStringList();
    }
}

QVariantList Fact::bitmaskValues() const
{
    if (_metaData) {
        return _metaData->bitmaskValues();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariantList();
    }
}

QStringList Fact::selectedBitmaskStrings() const
{
    if (_metaData) {
        const auto values = _metaData->bitmaskValues();
        const auto strings = _metaData->bitmaskStrings();
        if (values.size() != strings.size()) {
            qCWarning(FactLog) << "Size of bitmask value and string is different."  << name();
            return {};
        }

        QStringList selected;
        for (qsizetype i = 0; i < values.size(); i++) {
            if (rawValue().toInt() & values[i].toInt()) {
                selected += strings[i];
            }
        }

        if (selected.isEmpty()) {
            selected += "Not value selected";
        }

        return selected;
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return {};
    }
}

QString Fact::_variantToString(const QVariant &variant, int decimalPlaces) const
{
    QString valueString;

    switch (type()) {
    case FactMetaData::valueTypeFloat:
    {
        const float fValue = variant.toFloat();
        if (qIsNaN(fValue)) {
            valueString = QStringLiteral("--.--");
        } else {
            valueString = QStringLiteral("%1").arg(fValue, 0, 'f', decimalPlaces);
        }
    }
        break;
    case FactMetaData::valueTypeDouble:
    {
        const double dValue = variant.toDouble();
        if (qIsNaN(dValue)) {
            valueString = QStringLiteral("--.--");
        } else {
            valueString = QStringLiteral("%1").arg(dValue, 0, 'f', decimalPlaces);
        }
        break;
    }
    case FactMetaData::valueTypeBool:
        valueString = variant.toBool() ? tr("true") : tr("false");
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    {
        const double dValue = variant.toDouble();
        if (qIsNaN(dValue)) {
            valueString = QStringLiteral("--:--:--");
        } else {
            QTime time(0, 0, 0, 0);
            time = time.addSecs(dValue);
            valueString = time.toString(QStringLiteral("hh:mm:ss"));
        }
        break;
    }
    default:
        valueString = variant.toString();
        break;
    }

    return valueString;
}

QString Fact::rawValueStringFullPrecision() const
{
    return _variantToString(rawValue(), 18);
}

QString Fact::rawValueString() const
{
    return _variantToString(rawValue(), decimalPlaces());
}

QString Fact::cookedValueString() const
{
    return _variantToString(cookedValue(), decimalPlaces());
}

QVariant Fact::rawDefaultValue() const
{
    if (_metaData) {
        if (!_metaData->defaultValueAvailable()) {
            qCDebug(FactLog) << "Access to unavailable default value";
        }
        return _metaData->rawDefaultValue();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariant(0);
    }
}

QVariant Fact::cookedDefaultValue() const
{
    if (_metaData) {
        if (!_metaData->defaultValueAvailable()) {
            qCDebug(FactLog) << "Access to unavailable default value";
        }
        return _metaData->cookedDefaultValue();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariant(0);
    }
}

QString Fact::cookedDefaultValueString() const
{
    return _variantToString(cookedDefaultValue(), decimalPlaces());
}

QString Fact::shortDescription() const
{
    if (_metaData) {
        return _metaData->shortDescription();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::longDescription() const
{
    if (_metaData) {
        return _metaData->longDescription();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::rawUnits() const
{
    if (_metaData) {
        return _metaData->rawUnits();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::cookedUnits() const
{
    if (_metaData) {
        return _metaData->cookedUnits();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QString();
    }
}

QVariant Fact::rawMin() const
{
    if (_metaData) {
        return _metaData->rawMin();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariant(0);
    }
}

QVariant Fact::cookedMin() const
{
    if (_metaData) {
        return _metaData->cookedMin();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariant(0);
    }
}

QString Fact::cookedMinString() const
{
    return _variantToString(cookedMin(), decimalPlaces());
}

QVariant Fact::rawMax() const
{
    if (_metaData) {
        return _metaData->rawMax();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariant(0);
    }
}

QVariant Fact::cookedMax() const
{
    if (_metaData) {
        return _metaData->cookedMax();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QVariant(0);
    }
}

QString Fact::cookedMaxString() const
{
    return _variantToString(cookedMax(), decimalPlaces());
}

bool Fact::minIsDefaultForType() const
{
    if (_metaData) {
        return _metaData->minIsDefaultForType();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

bool Fact::maxIsDefaultForType() const
{
    if (_metaData) {
        return _metaData->maxIsDefaultForType();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

int Fact::decimalPlaces() const
{
    if (_metaData) {
        return _metaData->decimalPlaces();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return FactMetaData::kDefaultDecimalPlaces;
    }
}

QString Fact::category() const
{
    if (_metaData) {
        return _metaData->category();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QString();
    }
}

QString Fact::group() const
{
    if (_metaData) {
        return _metaData->group();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QString();
    }
}

void Fact::setMetaData(FactMetaData *metaData, bool setDefaultFromMetaData)
{
    _metaData = metaData;
    if (setDefaultFromMetaData && metaData->defaultValueAvailable()) {
        setRawValue(rawDefaultValue());
    }
    emit valueChanged(cookedValue());
}

bool Fact::valueEqualsDefault() const
{
    if (_metaData) {
        if (_metaData->defaultValueAvailable()) {
            return _metaData->rawDefaultValue() == rawValue();
        } else {
            return false;
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

bool Fact::defaultValueAvailable() const
{
    if (_metaData) {
        return _metaData->defaultValueAvailable();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

QString Fact::validate(const QString &cookedValue, bool convertOnly)
{
    if (_metaData) {
        QVariant typedValue;
        QString errorString;

        _metaData->convertAndValidateCooked(cookedValue, convertOnly, typedValue, errorString);

        return errorString;
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return QStringLiteral("Internal error: Meta data pointer missing");
    }
}

QVariant Fact::clamp(const QString &cookedValue)
{
    if (_metaData) {
        QVariant typedValue;
        if (_metaData->clampValue(cookedValue, typedValue)) {
            return typedValue;
        } else {
            //-- If conversion failed, return current value
            return rawValue();
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
    return QVariant();
}

bool Fact::vehicleRebootRequired() const
{
    if (_metaData) {
        return _metaData->vehicleRebootRequired();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

bool Fact::qgcRebootRequired() const
{
    if (_metaData) {
        return _metaData->qgcRebootRequired();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

void Fact::setSendValueChangedSignals(bool sendValueChangedSignals)
{
    if (sendValueChangedSignals != _sendValueChangedSignals) {
        _sendValueChangedSignals = sendValueChangedSignals;
        emit sendValueChangedSignalsChanged(_sendValueChangedSignals);
    }
}

void Fact::_sendValueChangedSignal(const QVariant &value)
{
    if (_sendValueChangedSignals) {
        emit valueChanged(value);
        _deferredValueChangeSignal = false;
    } else {
        _deferredValueChangeSignal = true;
    }
}

void Fact::sendDeferredValueChangedSignal()
{
    if (_deferredValueChangeSignal) {
        _deferredValueChangeSignal = false;
        emit valueChanged(cookedValue());
    }
}

QString Fact::enumOrValueString()
{
    if (_metaData) {
        if (_metaData->enumStrings().count()) {
            return enumStringValue();
        } else {
            return cookedValueString();
        }
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
    return QString();
}

double Fact::rawIncrement() const
{
    if (_metaData) {
        return _metaData->rawIncrement();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Fact::cookedIncrement() const
{
    if (_metaData) {
        return _metaData->cookedIncrement();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

bool Fact::hasControl() const
{
    if (_metaData) {
        return _metaData->hasControl();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

bool Fact::readOnly() const
{
    if (_metaData) {
        return _metaData->readOnly();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

bool Fact::writeOnly() const
{
    if (_metaData) {
        return _metaData->writeOnly();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

bool Fact::volatileValue() const
{
    if (_metaData) {
        return _metaData->volatileValue();
    } else {
        qCWarning(FactLog) << kMissingMetadata << name();
        return false;
    }
}

FactValueSliderListModel *Fact::valueSliderModel()
{
    if (!_valueSliderModel) {
        _valueSliderModel = new FactValueSliderListModel(*this);
    }

    return _valueSliderModel;
}

void Fact::_checkForRebootMessaging()
{
    if (qgcApp()) {
        if (!qgcApp()->runningUnitTests()) {
            if (vehicleRebootRequired()) {
                qgcApp()->showRebootAppMessage(tr("Reboot vehicle for changes to take effect."));
            } else if (qgcRebootRequired()) {
                qgcApp()->showRebootAppMessage(tr("Restart application for changes to take effect."));
            }
        }
    }
}
