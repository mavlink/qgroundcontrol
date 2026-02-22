#include "PX4LogTopic.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PX4LogTopicLog, "AnalyzeView.PX4LogTopic")

PX4LogTopic::PX4LogTopic(const std::string &name,
                         std::shared_ptr<ulog_cpp::Subscription> subscription,
                         QObject *parent)
    : QObject(parent)
    , _name(QString::fromStdString(name))
    , _subscription(std::move(subscription))
{
    for (const std::string &fieldName : _subscription->fieldNames()) {
        // Skip padding fields (ULog convention: fields starting with '_padding')
        if (fieldName.rfind("_padding", 0) == 0) {
            continue;
        }

        const auto &field = _subscription->field(fieldName);
        const auto basicType = field->type().type;

        // Only expose numeric fields (skip nested types and char arrays)
        if (basicType == ulog_cpp::Field::BasicType::NESTED ||
            basicType == ulog_cpp::Field::BasicType::CHAR) {
            continue;
        }

        // For array fields, expose each element as "fieldName[i]"
        if (field->arrayLength() > 1) {
            for (int i = 0; i < field->arrayLength(); ++i) {
                _fieldNames.append(QString::fromStdString(fieldName) + QStringLiteral("[") + QString::number(i) + QStringLiteral("]"));
            }
        } else {
            _fieldNames.append(QString::fromStdString(fieldName));
        }
    }

    qCDebug(PX4LogTopicLog) << "Topic:" << _name
                            << "fields:" << _fieldNames.count()
                            << "samples:" << sampleCount();
}

PX4LogTopic::~PX4LogTopic()
{
}

int PX4LogTopic::sampleCount() const
{
    return static_cast<int>(_subscription->size());
}

QList<qreal> PX4LogTopic::timestamps() const
{
    QList<qreal> result;
    result.reserve(sampleCount());

    for (const auto &sample : *_subscription) {
        try {
            result.append(static_cast<qreal>(sample.at("timestamp").as<uint64_t>()));
        } catch (const ulog_cpp::AccessException &e) {
            qCWarning(PX4LogTopicLog) << _name << "failed to read timestamp:" << e.what();
            result.append(0.0);
        }
    }

    return result;
}

static qreal extractNumericValue(const ulog_cpp::TypedDataView &sample,
                                 const std::string &rawFieldName,
                                 int arrayIndex)
{
    try {
        if (arrayIndex >= 0) {
            return sample.at(rawFieldName)[arrayIndex].as<double>();
        }
        return sample.at(rawFieldName).as<double>();
    } catch (const ulog_cpp::AccessException &) {
        return std::numeric_limits<qreal>::quiet_NaN();
    }
}

/// Parse "fieldName[2]" into ("fieldName", 2). Returns ("fieldName", -1) if no index.
static std::pair<std::string, int> parseFieldSpec(const QString &fieldName)
{
    const int bracketPos = fieldName.indexOf(QLatin1Char('['));
    if (bracketPos < 0) {
        return {fieldName.toStdString(), -1};
    }

    const QString baseName = fieldName.left(bracketPos);
    const QString indexStr = fieldName.mid(bracketPos + 1, fieldName.length() - bracketPos - 2);
    bool ok = false;
    const int index = indexStr.toInt(&ok);
    return {baseName.toStdString(), ok ? index : -1};
}

QList<qreal> PX4LogTopic::fieldValues(const QString &fieldName) const
{
    const auto [rawName, arrayIndex] = parseFieldSpec(fieldName);

    QList<qreal> result;
    result.reserve(sampleCount());

    for (const auto &sample : *_subscription) {
        result.append(extractNumericValue(sample, rawName, arrayIndex));
    }

    return result;
}

QList<QPointF> PX4LogTopic::fieldSeries(const QString &fieldName) const
{
    const auto [rawName, arrayIndex] = parseFieldSpec(fieldName);

    QList<QPointF> result;
    result.reserve(sampleCount());

    for (const auto &sample : *_subscription) {
        try {
            const qreal timestamp = static_cast<qreal>(sample.at("timestamp").as<uint64_t>());
            const qreal value = extractNumericValue(sample, rawName, arrayIndex);
            result.append(QPointF(timestamp, value));
        } catch (const ulog_cpp::AccessException &e) {
            qCWarning(PX4LogTopicLog) << _name << "failed to read sample:" << e.what();
        }
    }

    return result;
}
