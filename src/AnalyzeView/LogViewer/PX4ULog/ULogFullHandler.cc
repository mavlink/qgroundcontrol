#include "ULogFullHandler.h"

#include "LogParseResultPrivate.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QStringList>

#include <algorithm>
#include <stdexcept>

#include <ulog_cpp/subscription.hpp>

QGC_LOGGING_CATEGORY(ULogFullHandlerLog, "AnalyzeView.ULogFullHandler")

namespace {

QString _px4NavStateName(int state)
{
    switch (state) {
    // Values from PX4 VehicleStatus.msg NAVIGATION_STATE_* constants
    case 0:  return QStringLiteral("Manual");
    case 1:  return QStringLiteral("Altitude");
    case 2:  return QStringLiteral("Position");
    case 3:  return QStringLiteral("Mission");
    case 4:  return QStringLiteral("Hold");
    case 5:  return QStringLiteral("Return");
    case 6:  return QStringLiteral("Position Slow");
    case 8:  return QStringLiteral("Altitude Cruise");
    case 10: return QStringLiteral("Acro");
    case 12: return QStringLiteral("Descend");
    case 13: return QStringLiteral("Termination");
    case 14: return QStringLiteral("Offboard");
    case 15: return QStringLiteral("Stabilized");
    case 17: return QStringLiteral("Takeoff");
    case 18: return QStringLiteral("Land");
    case 19: return QStringLiteral("Follow Target");
    case 20: return QStringLiteral("Precision Land");
    case 21: return QStringLiteral("Orbit");
    case 22: return QStringLiteral("VTOL Takeoff");
    default: return QStringLiteral("Mode %1").arg(state);
    }
}

bool _isNumericScalarField(const ulog_cpp::Field &field)
{
    if (field.arrayLength() >= 0) {
        return false; // arrays excluded from plottable fields
    }
    using BT = ulog_cpp::Field::BasicType;
    switch (field.type().type) {
    case BT::INT8:
    case BT::UINT8:
    case BT::INT16:
    case BT::UINT16:
    case BT::INT32:
    case BT::UINT32:
    case BT::INT64:
    case BT::UINT64:
    case BT::FLOAT:
    case BT::DOUBLE:
    case BT::BOOL:
        return true;
    default:
        return false;
    }
}

} // namespace

ULogFullHandler::ULogFullHandler(LogParseResult &result)
    : _result(result)
{
}

void ULogFullHandler::error(const std::string &msg, bool is_recoverable)
{
    const QString errorMessage = QString::fromStdString(msg);
    if (!is_recoverable) {
        _hadFatalError = true;
        if (_result.errorMessage.isEmpty()) {
            _result.errorMessage = errorMessage;
        }
    }
    qCWarning(ULogFullHandlerLog) << "ULog parse error:" << errorMessage;
}

void ULogFullHandler::messageFormat(const ulog_cpp::MessageFormat &message_format)
{
    _formats[message_format.name()] = std::make_shared<ulog_cpp::MessageFormat>(message_format);
}

void ULogFullHandler::addLoggedMessage(const ulog_cpp::AddLoggedMessage &add_logged_message)
{
    const auto it = _formats.find(add_logged_message.messageName());
    if (it != _formats.cend()) {
        _subscriptions[add_logged_message.msgId()] = {
            it->second,
            add_logged_message.multiId(),
            add_logged_message.messageName()
        };
    }
}

void ULogFullHandler::headerComplete()
{
    _headerComplete = true;
    for (auto &[name, fmt] : _formats) {
        fmt->resolveDefinition(_formats);
    }
}

void ULogFullHandler::data(const ulog_cpp::Data &data)
{
    if (!_headerComplete) {
        return;
    }

    const auto it = _subscriptions.find(data.msgId());
    if (it == _subscriptions.cend()) {
        return;
    }

    const SubscriptionInfo &sub = it->second;
    if (!sub.format) {
        return;
    }

    try {
        const ulog_cpp::TypedDataView view(data, *sub.format);

        // Extract timestamp (ULog convention: field named "timestamp", unit µs)
        double timestampSecs = -1.0;
        if (sub.format->fieldMap().count("timestamp") > 0) {
            const uint64_t tsUs = view.at("timestamp").as<uint64_t>();
            timestampSecs = static_cast<double>(tsUs) / 1e6;
            _lastTimestampSecs = timestampSecs;
        }

        // Field name: "topic_name.field" or "topic_name[N].field" for multi-instance
        const QString prefix = (sub.multiId > 0)
            ? QStringLiteral("%1[%2].").arg(QString::fromStdString(sub.topicName)).arg(sub.multiId)
            : QString::fromStdString(sub.topicName) + QLatin1Char('.');

        for (const auto &field : sub.format->fields()) {
            // Skip padding fields and the timestamp itself
            if (field->name().rfind("_padding", 0) == 0) {
                continue;
            }
            if (field->name() == "timestamp") {
                continue;
            }
            if (!field->definitionResolved()) {
                continue;
            }

            const QString fieldName = prefix + QString::fromStdString(field->name());
            _fieldSet.insert(fieldName);

            if (!_isNumericScalarField(*field) || timestampSecs < 0.0) {
                continue;
            }

            const double value = view.at(field).as<double>();
            _result.fieldSamples[fieldName].append(QPointF(timestampSecs, value));
            _plottableFieldSet.insert(fieldName);
        }

        _result.sampleCount++;

        if (timestampSecs >= 0.0) {
            if (_result.minTimestamp < 0.0 || timestampSecs < _result.minTimestamp) {
                _result.minTimestamp = timestampSecs;
            }
            _result.maxTimestamp = std::max(_result.maxTimestamp, timestampSecs);
        }
    } catch (const std::exception &e) {
        qCWarning(ULogFullHandlerLog) << "Failed to decode data message:" << e.what();
    }
}

void ULogFullHandler::logging(const ulog_cpp::Logging &logging)
{
    const double timestampSecs = static_cast<double>(logging.timestamp()) / 1e6;
    const QString text = QString::fromStdString(logging.message());

    if (text.isEmpty()) {
        return;
    }

    QVariantMap msgRow;
    msgRow[QStringLiteral("time")] = timestampSecs;
    msgRow[QStringLiteral("text")] = text;
    _result.messages.append(msgRow);

    using Level = ulog_cpp::Logging::Level;
    if (logging.logLevel() <= Level::Warning) {
        const QString eventType = (logging.logLevel() <= Level::Error)
            ? QStringLiteral("error")
            : QStringLiteral("warning");

        if (timestampSecs >= 0.0) {
            QVariantMap eventRow;
            eventRow[QStringLiteral("time")] = timestampSecs;
            eventRow[QStringLiteral("type")] = eventType;
            eventRow[QStringLiteral("description")] = text;
            _result.events.append(eventRow);
        }
    }
}

void ULogFullHandler::parameter(const ulog_cpp::Parameter &param)
{
    const QString name = QString::fromStdString(param.field().name());
    if (name.isEmpty()) {
        return;
    }

    QVariant value;
    bool isFloat = false;
    try {
        // ULog parameters are restricted to int32_t and float per spec.
        // as<double>() safely static_casts either type.
        value = param.value().as<double>();
        isFloat = (param.field().type().type == ulog_cpp::Field::BasicType::FLOAT);
    } catch (const std::exception &) {
        value = QVariant();
    }

    QVariantMap row;
    row[QStringLiteral("name")]    = name;
    row[QStringLiteral("value")]   = value;
    row[QStringLiteral("isFloat")] = isFloat;
    // hasDefault / defaultValue / isDefault are filled in by finalize() once all
    // ParameterDefault messages have been collected.
    _result.parameters.append(row);
}

void ULogFullHandler::parameterDefault(const ulog_cpp::ParameterDefault &param_default)
{
    const QString name = QString::fromStdString(param_default.field().name());
    if (name.isEmpty()) {
        return;
    }
    try {
        const double defaultVal = param_default.value().as<double>();
        // Only store system defaults (bit 0 set). Configuration defaults (bit 1) are
        // user-visible overrides and not what we want to compare against.
        using DT = ulog_cpp::ulog_parameter_default_type_t;
        if ((static_cast<uint8_t>(param_default.defaultType()) &
             static_cast<uint8_t>(DT::system)) != 0) {
            _paramDefaults[name] = defaultVal;
        }
    } catch (const std::exception &) {
        // Ignore unreadable defaults
    }
}

void ULogFullHandler::dropout(const ulog_cpp::Dropout &dropout)
{
    if (_lastTimestampSecs < 0.0) {
        return;
    }

    const double start = _lastTimestampSecs;
    const double end = start + static_cast<double>(dropout.durationMs()) / 1000.0;
    QVariantMap row;
    row[QStringLiteral("start")] = start;
    row[QStringLiteral("end")] = end;
    _result.dropouts.append(row);
}

void ULogFullHandler::finalize()
{
    // Detect vehicle type from vehicle_status.vehicle_type
    // PX4 vehicle_type enum: 0=Unknown, 1=Rotary Wing, 2=Fixed Wing, 3=Rover, 4=Airship
    const auto vehicleTypeIt = _result.fieldSamples.constFind(QStringLiteral("vehicle_status.vehicle_type"));
    if (vehicleTypeIt != _result.fieldSamples.cend() && !vehicleTypeIt->isEmpty()) {
        const int vtype = static_cast<int>(vehicleTypeIt->first().y());
        switch (vtype) {
        case 1: _result.detectedVehicleType = QStringLiteral("Multirotor/Helicopter"); break;
        case 2: _result.detectedVehicleType = QStringLiteral("Fixed Wing");            break;
        case 3: _result.detectedVehicleType = QStringLiteral("Rover");                 break;
        case 4: _result.detectedVehicleType = QStringLiteral("Airship");               break;
        default: break; // 0 = Unknown, leave empty so UI shows "Unknown"
        }
    }

    // Derive mode segments from vehicle_status.nav_state samples.
    // nav_state is a uint8_t mapped to the PX4 navigation_state enum.
    const auto navStateIt = _result.fieldSamples.constFind(QStringLiteral("vehicle_status.nav_state"));
    if (navStateIt != _result.fieldSamples.cend()) {
        const QVector<QPointF> &samples = navStateIt.value();
        int lastNavState = -1;
        double segmentStart = -1.0;
        QString segmentMode;

        for (const QPointF &pt : samples) {
            const int navState = static_cast<int>(pt.y());
            if (navState != lastNavState) {
                // Close the previous segment
                if (lastNavState >= 0 && segmentStart >= 0.0) {
                    QVariantMap seg;
                    seg[QStringLiteral("mode")] = segmentMode;
                    seg[QStringLiteral("start")] = segmentStart;
                    seg[QStringLiteral("end")] = pt.x();
                    _result.modeSegments.append(seg);
                }
                lastNavState = navState;
                segmentStart = pt.x();
                segmentMode = _px4NavStateName(navState);
            }
        }

        // Close the final open segment
        if (lastNavState >= 0 && segmentStart >= 0.0 && _result.maxTimestamp >= segmentStart) {
            QVariantMap seg;
            seg[QStringLiteral("mode")] = segmentMode;
            seg[QStringLiteral("start")] = segmentStart;
            seg[QStringLiteral("end")] = _result.maxTimestamp;
            _result.modeSegments.append(seg);
        }
    }

    // Sort field lists for consistent display
    _result.availableFields = _fieldSet.values();
    std::sort(_result.availableFields.begin(), _result.availableFields.end());
    _result.plottableFields = _plottableFieldSet.values();
    std::sort(_result.plottableFields.begin(), _result.plottableFields.end());

    // Annotate parameter rows with default value info now that all ParameterDefault
    // messages have been collected.
    for (int i = 0; i < _result.parameters.size(); i++) {
        QVariantMap row = _result.parameters[i].toMap();
        const QString name = row.value(QStringLiteral("name")).toString();
        const auto it = _paramDefaults.constFind(name);
        if (it != _paramDefaults.constEnd()) {
            const double defaultVal = it.value();
            const QVariant valueVariant = row.value(QStringLiteral("value"));
            row[QStringLiteral("hasDefault")]   = true;
            row[QStringLiteral("defaultValue")] = defaultVal;
            // Treat parameters whose value couldn't be read (invalid QVariant from
            // the catch block in parameter()) as non-default so they are never
            // hidden by the "Changed only" filter.
            bool isDefault = false;
            if (valueVariant.isValid() && valueVariant.canConvert<double>()) {
                const double currentVal = valueVariant.toDouble();
                // qFuzzyCompare is undefined when either value is 0.0, so use exact
                // equality for the zero case and relative comparison otherwise.
                isDefault = (currentVal == defaultVal)
                    || (!qFuzzyIsNull(defaultVal) && qFuzzyCompare(currentVal, defaultVal));
            }
            row[QStringLiteral("isDefault")]    = isDefault;
        } else {
            row[QStringLiteral("hasDefault")]   = false;
            row[QStringLiteral("defaultValue")] = QVariant();
            row[QStringLiteral("isDefault")]    = false;
        }
        _result.parameters[i] = row;
    }

    _result.sourceType = LogParseResult::SourceType::PX4ULog;
    _result.ok = true;
}
