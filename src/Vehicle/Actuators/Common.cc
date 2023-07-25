/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Common.h"

#include <QDebug>

QGC_LOGGING_CATEGORY(ActuatorsConfigLog, "ActuatorsConfigLog")


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

Condition::Condition(const QString &condition, ParameterManager* parameterManager)
{
    QRegularExpression re("^([0-9A-Za-z_-]+)([\\!=<>]+)(-?\\d+)$");
    QRegularExpressionMatch match = re.match(condition);
    if (condition == "true") {
        _operation = Operation::AlwaysTrue;
    } else if (condition == "false") {
        _operation = Operation::AlwaysFalse;
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
            qCWarning(ActuatorsConfigLog) << "Unknown condition operation: " << operation;
        }
        _value = match.captured(3).toInt();

        qCDebug(ActuatorsConfigLog) << "Condition: Param:" << _parameter << "op:" << operation << "value:" << _value;

        if (parameterManager->parameterExists(FactSystem::defaultComponentId, _parameter)) {
            Fact* param = parameterManager->getParameter(FactSystem::defaultComponentId, _parameter);
            if (param->type() == FactMetaData::ValueType_t::valueTypeBool ||
                    param->type() == FactMetaData::ValueType_t::valueTypeInt32) {
                _fact = param;
            } else {
                qCDebug(ActuatorsConfigLog) << "Condition: Unsupported param type:" << (int)param->type();
            }
        } else {
            qCDebug(ActuatorsConfigLog) << "Condition: Param does not exist:" << _parameter;
        }
    }

}

bool Condition::evaluate() const
{
    if (_operation == Operation::AlwaysFalse) {
        return false;
    }

    if (_operation == Operation::AlwaysTrue || _parameter.isEmpty()) {
        return true;
    }

    if (!_fact) {
        return false;
    }

    int32_t paramValue = _fact->rawValue().toInt();
    switch (_operation) {
        case Operation::AlwaysTrue: return true;
        case Operation::AlwaysFalse: return false;
        case Operation::GreaterThan: return paramValue > _value;
        case Operation::GreaterEqual: return paramValue >= _value;
        case Operation::Equal: return paramValue == _value;
        case Operation::NotEqual: return paramValue != _value;
        case Operation::LessThan: return paramValue < _value;
        case Operation::LessEqual: return paramValue <= _value;
    }
    return false;
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

