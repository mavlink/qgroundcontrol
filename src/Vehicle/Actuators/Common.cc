#include "Common.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ActuatorsConfigLog, "Vehicle.ActuatorsConfig")


void Parameter::parse(const QJsonValue& jsonValue)
{
    label = jsonValue["label"].toString();
    name = jsonValue["name"].toString();
    indexOffset = jsonValue["index-offset"].toInt(0);
    QString displayOptionStr = jsonValue["show-as"].toString();
    if (displayOptionStr == "true-if-positive") {
        displayOption = DisplayOption::BoolTrueIfPositive;
    } else if (displayOptionStr == "bitset") {
        displayOption = DisplayOption::Bitset;
    } else if (displayOptionStr != "") {
        qCDebug(ActuatorsConfigLog) << "Unknown param display option (show-as):" << displayOptionStr;
    }
    advanced = jsonValue["advanced"].toBool(false);
}

FactBitset::FactBitset(QObject* parent, Fact* integerFact, int offset)
    : Fact("", new FactMetaData(FactMetaData::valueTypeBool, "", parent), parent),
      _integerFact(integerFact), _offset(offset)
{
    forceSetRawValue(false); // need to force set an initial bool value so the type is correct
    onIntegerFactChanged();
    connect(this, &Fact::rawValueChanged, this, &FactBitset::onThisFactChanged);
    connect(_integerFact, &Fact::rawValueChanged, this, &FactBitset::onIntegerFactChanged);
}

void FactBitset::onIntegerFactChanged()
{
    if (_ignoreChange) {
        return;
    }
    _ignoreChange = true;
    // sync to this
    setRawValue((_integerFact->rawValue().toInt() & (1 << _offset)) != 0);
    _ignoreChange = false;
}

void FactBitset::onThisFactChanged()
{
    if (_ignoreChange) {
        return;
    }
    _ignoreChange = true;
    // sync to integer fact
    int value = _integerFact->rawValue().toInt();
    if (rawValue().toBool()) {
        _integerFact->forceSetRawValue(value | (1u << _offset));
    } else {
        _integerFact->forceSetRawValue(value & ~(1u << _offset));
    }
    _ignoreChange = false;
}


FactFloatAsBool::FactFloatAsBool(QObject* parent, Fact* floatFact)
    : Fact("", new FactMetaData(FactMetaData::valueTypeBool, "", parent), parent),
      _floatFact(floatFact)
{
    onFloatFactChanged();
    connect(this, &Fact::rawValueChanged, this, &FactFloatAsBool::onThisFactChanged);
    connect(_floatFact, &Fact::rawValueChanged, this, &FactFloatAsBool::onFloatFactChanged);
}

void FactFloatAsBool::onFloatFactChanged()
{
    if (_ignoreChange) {
        return;
    }
    _ignoreChange = true;
    // sync to this
    forceSetRawValue(_floatFact->rawValue().toFloat() > 0.f);
    _ignoreChange = false;
}

void FactFloatAsBool::onThisFactChanged()
{
    if (_ignoreChange) {
        return;
    }
    _ignoreChange = true;
    // sync to float fact
    float value = _floatFact->rawValue().toFloat();
    if (rawValue().toBool()) {
        _floatFact->forceSetRawValue(std::abs(value));
    } else {
        _floatFact->forceSetRawValue(-std::abs(value));
    }
    _ignoreChange = false;
}

Condition::Condition(const QString &condition, ParameterManager* parameterManager, const QString& label)
{
    _label = label;

    const bool debugEnabled = ActuatorsConfigLog().isDebugEnabled();
    QString logPrefix;
    if (debugEnabled) {
        logPrefix = QStringLiteral("Condition");
        if (!label.isEmpty()) {
            logPrefix += QStringLiteral(" [%1]").arg(label);
        }
    }

    QRegularExpression re("^([0-9A-Za-z_-]+)([\\!=<>]+)(-?\\d+)$");
    QRegularExpressionMatch match = re.match(condition);
    if (condition.isEmpty()) {
        _operation = Operation::AlwaysTrue;
        _alwaysTrueReason = QStringLiteral("empty/default");
        if (debugEnabled) {
            qCDebug(ActuatorsConfigLog) << logPrefix + ": <empty> (defaults to true)";
        }
    } else if (condition == "true") {
        _operation = Operation::AlwaysTrue;
        _alwaysTrueReason = QStringLiteral("literal true");
        if (debugEnabled) {
            qCDebug(ActuatorsConfigLog) << logPrefix + ": true";
        }
    } else if (condition == "false") {
        _operation = Operation::AlwaysFalse;
        if (debugEnabled) {
            qCDebug(ActuatorsConfigLog) << logPrefix + ": false";
        }
    } else if (match.hasMatch()) {
        _parameter = match.captured(1);
        QString operation = match.captured(2);
        if (operation == ">") {
            _operation = Operation::GreaterThan;
        } else if (operation == ">=") {
            _operation = Operation::GreaterEqual;
        } else if (operation == "==") {
            _operation = Operation::Equal;
        } else if (operation == "!=") {
            _operation = Operation::NotEqual;
        } else if (operation == "<") {
            _operation = Operation::LessThan;
        } else if (operation == "<=") {
            _operation = Operation::LessEqual;
        } else {
            _operation = Operation::AlwaysTrue;
            _alwaysTrueReason = QStringLiteral("unknown operator '%1'").arg(operation);
            qCWarning(ActuatorsConfigLog) << "Unknown condition operation: " << operation;
        }
        _value = match.captured(3).toInt();

        if (debugEnabled) {
            qCDebug(ActuatorsConfigLog) << logPrefix + QStringLiteral(": Param:%1 op:%2 value:%3").arg(_parameter, operation).arg(_value);
        }

        if (parameterManager->parameterExists(ParameterManager::defaultComponentId, _parameter)) {
            Fact* param = parameterManager->getParameter(ParameterManager::defaultComponentId, _parameter);
            if (param->type() == FactMetaData::ValueType_t::valueTypeBool ||
                    param->type() == FactMetaData::ValueType_t::valueTypeInt32) {
                _fact = param;
            } else if (debugEnabled) {
                qCDebug(ActuatorsConfigLog) << logPrefix + QStringLiteral(": Unsupported param type:%1").arg((int)param->type());
            }
        } else if (debugEnabled) {
            qCDebug(ActuatorsConfigLog) << logPrefix + QStringLiteral(": Param does not exist:%1").arg(_parameter);
        }
    } else {
        _alwaysTrueReason = QStringLiteral("unrecognized '%1'").arg(condition);
        qCWarning(ActuatorsConfigLog) << "Condition"
            << (label.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(label))
            << QStringLiteral(": Unrecognized condition string '%1', defaulting to true").arg(condition);
    }

}

bool Condition::evaluate() const
{
    if (_operation == Operation::AlwaysTrue) {
        if (ActuatorsConfigLog().isDebugEnabled()) {
            QString logPrefix = _label.isEmpty() ? QStringLiteral("Evaluating condition") : QStringLiteral("Evaluating [%1]").arg(_label);
            qCDebug(ActuatorsConfigLog) << logPrefix << "-> true (" << _alwaysTrueReason << ")";
        }
        return true;
    }

    if (_operation == Operation::AlwaysFalse) {
        if (ActuatorsConfigLog().isDebugEnabled()) {
            QString logPrefix = _label.isEmpty() ? QStringLiteral("Evaluating condition") : QStringLiteral("Evaluating [%1]").arg(_label);
            qCDebug(ActuatorsConfigLog) << logPrefix << "-> false (literal false)";
        }
        return false;
    }

    if (!_fact) {
        if (ActuatorsConfigLog().isDebugEnabled()) {
            QString logPrefix = _label.isEmpty() ? QStringLiteral("Evaluating condition") : QStringLiteral("Evaluating [%1]").arg(_label);
            qCDebug(ActuatorsConfigLog) << logPrefix << "-> false (parameter" << _parameter << "unavailable for evaluation)";
        }
        return false;
    }

    int32_t paramValue = _fact->rawValue().toInt();
    bool result = false;
    QString operation;

    switch (_operation) {
        case Operation::GreaterThan:
            result = paramValue > _value;
            operation = ">";
            break;
        case Operation::GreaterEqual:
            result = paramValue >= _value;
            operation = ">=";
            break;
        case Operation::Equal:
            result = paramValue == _value;
            operation = "==";
            break;
        case Operation::NotEqual:
            result = paramValue != _value;
            operation = "!=";
            break;
        case Operation::LessThan:
            result = paramValue < _value;
            operation = "<";
            break;
        case Operation::LessEqual:
            result = paramValue <= _value;
            operation = "<=";
            break;
        default:
            qCWarning(ActuatorsConfigLog) << "Unexpected operation type in condition evaluation";
            break;
    }

    if (ActuatorsConfigLog().isDebugEnabled()) {
        QString logPrefix = _label.isEmpty() ? QStringLiteral("Evaluating condition") : QStringLiteral("Evaluating [%1]").arg(_label);
        qCDebug(ActuatorsConfigLog) << logPrefix << "->" << (result ? "true" : "false")
                                     << "(" << _parameter << "=" << paramValue << operation << _value << ")";
    }
    return result;
}

ActuatorGeometry::Type ActuatorGeometry::typeFromStr(const QString &type)
{
    if (type == "motor") {
        return ActuatorGeometry::Type::Motor;
    } else if (type == "servo") {
        return ActuatorGeometry::Type::Servo;
    }
    return ActuatorGeometry::Type::Other;
}
