#include "DataFlashLogParser.h"

#include "DataFlashUtility.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QSet>

#include <algorithm>
#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(DataFlashLogParserLog, "AnalyzeView.DataFlashLogParser")

namespace {
QString _ardupilotModeName(int modeNumber)
{
    // ArduPlane flight mode mapping provided by user/project issue context.
    static const QHash<int, QString> planeModes = {
        {0, QStringLiteral("Manual")},
        {1, QStringLiteral("CIRCLE")},
        {2, QStringLiteral("STABILIZE")},
        {3, QStringLiteral("TRAINING")},
        {4, QStringLiteral("ACRO")},
        {5, QStringLiteral("FBWA")},
        {6, QStringLiteral("FBWB")},
        {7, QStringLiteral("CRUISE")},
        {8, QStringLiteral("AUTOTUNE")},
        {10, QStringLiteral("Auto")},
        {11, QStringLiteral("RTL")},
        {12, QStringLiteral("Loiter")},
        {13, QStringLiteral("TAKEOFF")},
        {14, QStringLiteral("AVOID_ADSB")},
        {15, QStringLiteral("Guided")},
        {17, QStringLiteral("QSTABILIZE")},
        {18, QStringLiteral("QHOVER")},
        {19, QStringLiteral("QLOITER")},
        {20, QStringLiteral("QLAND")},
        {21, QStringLiteral("QRTL")},
        {22, QStringLiteral("QAUTOTUNE")},
        {23, QStringLiteral("QACRO")},
        {24, QStringLiteral("THERMAL")},
        {25, QStringLiteral("Loiter to QLand")},
        {26, QStringLiteral("AUTOLAND")},
    };

    return planeModes.value(modeNumber);
}
} // namespace

DataFlashLogParser::DataFlashLogParser(QObject *parent)
    : QObject(parent)
{
    qCDebug(DataFlashLogParserLog) << this;
}

DataFlashLogParser::~DataFlashLogParser()
{
    qCDebug(DataFlashLogParserLog) << this;
}

bool DataFlashLogParser::parseFile(const QString &filePath)
{
    clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        _setParseError(tr("Failed to open DataFlash file"));
        return false;
    }

    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty()) {
        _setParseError(tr("DataFlash file is empty"));
        return false;
    }

    QMap<uint8_t, DataFlashUtility::MessageFormat> formats;
    if (!DataFlashUtility::parseFmtMessages(bytes.constData(), bytes.size(), formats)) {
        _setParseError(tr("No valid FMT messages were found"));
        return false;
    }

    QSet<QString> signalSet;
    QSet<QString> plottableSet;
    double minTimestampSecs = -1.0;
    double maxTimestampSecs = -1.0;
    bool hasOpenModeSegment = false;
    double modeSegmentStartSecs = -1.0;
    QString currentModeName;
    DataFlashUtility::iterateMessages(bytes.constData(), bytes.size(), formats, [this, &signalSet, &plottableSet, &minTimestampSecs, &maxTimestampSecs, &hasOpenModeSegment, &modeSegmentStartSecs, &currentModeName](uint8_t, const char *payload, int, const DataFlashUtility::MessageFormat &fmt) {
        const QMap<QString, QVariant> values = DataFlashUtility::parseMessage(payload, fmt);
        const double timestampSecs = _extractTimestampSeconds(values);
        if (timestampSecs >= 0.0) {
            if (minTimestampSecs < 0.0 || timestampSecs < minTimestampSecs) {
                minTimestampSecs = timestampSecs;
            }
            maxTimestampSecs = std::max(maxTimestampSecs, timestampSecs);
        }

        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            signalSet.insert(QStringLiteral("%1.%2").arg(fmt.name, it.key()));
        }

        if (fmt.name == QStringLiteral("PARM")) {
            const QString paramName = values.value(QStringLiteral("Name")).toString();
            const QVariant paramValue = values.contains(QStringLiteral("Value")) ? values.value(QStringLiteral("Value")) : values.value(QStringLiteral("Val"));
            if (!paramName.isEmpty()) {
                QVariantMap row;
                row[QStringLiteral("name")] = paramName;
                row[QStringLiteral("value")] = paramValue;
                _parameters.append(row);
            }
        } else if (fmt.name == QStringLiteral("MODE")) {
            QString modeName = values.value(QStringLiteral("Mode")).toString();
            bool isNumericMode = false;
            const int modeNumber = modeName.toInt(&isNumericMode);
            if (isNumericMode) {
                const QString mappedName = _ardupilotModeName(modeNumber);
                modeName = mappedName.isEmpty() ? QStringLiteral("Mode %1").arg(modeNumber)
                                                : QStringLiteral("%1 (%2)").arg(mappedName).arg(modeNumber);
            } else if (modeName.isEmpty()) {
                modeName = QStringLiteral("Unknown");
            }
            _appendEvent(timestampSecs, QStringLiteral("mode"), tr("Mode: %1").arg(modeName));
            if (timestampSecs >= 0.0) {
                if (hasOpenModeSegment && (timestampSecs > modeSegmentStartSecs)) {
                    QVariantMap segment;
                    segment[QStringLiteral("mode")] = currentModeName;
                    segment[QStringLiteral("start")] = modeSegmentStartSecs;
                    segment[QStringLiteral("end")] = timestampSecs;
                    _modeSegments.append(segment);
                }
                hasOpenModeSegment = true;
                modeSegmentStartSecs = timestampSecs;
                currentModeName = modeName;
            }
        } else if (fmt.name == QStringLiteral("ERR")) {
            const QString subsystem = values.value(QStringLiteral("Subsys")).toString();
            const QString ecode = values.value(QStringLiteral("ECode")).toString();
            _appendEvent(timestampSecs, QStringLiteral("error"), QStringLiteral("Error: Subsys=%1 ECode=%2").arg(subsystem, ecode));
        } else if (fmt.name == QStringLiteral("EV")) {
            const int eventId = values.value(QStringLiteral("Id")).toInt();
            QString description = QStringLiteral("Event %1").arg(eventId);
            if (eventId == 10) {
                description = tr("Armed");
            } else if (eventId == 11) {
                description = tr("Disarmed");
            }
            _appendEvent(timestampSecs, QStringLiteral("event"), description);
        }

        _sampleCount++;
        if (timestampSecs >= 0.0) {
            for (auto it = values.cbegin(); it != values.cend(); ++it) {
                const QString signalName = QStringLiteral("%1.%2").arg(fmt.name, it.key());
                const int typeId = it.value().metaType().id();
                const bool numeric = (typeId == QMetaType::Int) || (typeId == QMetaType::UInt) ||
                                     (typeId == QMetaType::LongLong) || (typeId == QMetaType::ULongLong) ||
                                     (typeId == QMetaType::Float) || (typeId == QMetaType::Double);
                if (numeric) {
                    QVariantMap point;
                    point[QStringLiteral("x")] = timestampSecs;
                    point[QStringLiteral("y")] = it.value().toDouble();
                    _signalSamples[signalName].append(point);
                    plottableSet.insert(signalName);
                }
            }
        }

        return true;
    });

    if (hasOpenModeSegment && (maxTimestampSecs >= modeSegmentStartSecs)) {
        QVariantMap segment;
        segment[QStringLiteral("mode")] = currentModeName;
        segment[QStringLiteral("start")] = modeSegmentStartSecs;
        segment[QStringLiteral("end")] = maxTimestampSecs;
        _modeSegments.append(segment);
    }

    _availableSignals = signalSet.values();
    std::sort(_availableSignals.begin(), _availableSignals.end());
    _plottableSignals = plottableSet.values();
    std::sort(_plottableSignals.begin(), _plottableSignals.end());
    emit availableSignalsChanged();
    emit plottableSignalsChanged();
    emit parametersChanged();
    emit eventsChanged();
    emit modeSegmentsChanged();
    if (_minTimestamp != minTimestampSecs || _maxTimestamp != maxTimestampSecs) {
        _minTimestamp = minTimestampSecs;
        _maxTimestamp = maxTimestampSecs;
        emit timeRangeChanged();
    }
    emit sampleCountChanged();

    _parsed = true;
    emit parsedChanged();
    qCDebug(DataFlashLogParserLog) << "Parsed signals" << _availableSignals.count() << "parameters" << _parameters.count() << "events" << _events.count();
    return true;
}

void DataFlashLogParser::clear()
{
    const bool oldParsed = _parsed;
    _parsed = false;
    if (oldParsed) {
        emit parsedChanged();
    }

    if (!_parseError.isEmpty()) {
        _parseError.clear();
        emit parseErrorChanged();
    }

    if (!_availableSignals.isEmpty()) {
        _availableSignals.clear();
        emit availableSignalsChanged();
    }

    if (!_parameters.isEmpty()) {
        _parameters.clear();
        emit parametersChanged();
    }

    if (!_events.isEmpty()) {
        _events.clear();
        emit eventsChanged();
    }

    if (!_modeSegments.isEmpty()) {
        _modeSegments.clear();
        emit modeSegmentsChanged();
    }

    if (!_plottableSignals.isEmpty()) {
        _plottableSignals.clear();
        emit plottableSignalsChanged();
    }

    _signalSamples.clear();
    if (_minTimestamp != -1.0 || _maxTimestamp != -1.0) {
        _minTimestamp = -1.0;
        _maxTimestamp = -1.0;
        emit timeRangeChanged();
    }

    if (_sampleCount != 0) {
        _sampleCount = 0;
        emit sampleCountChanged();
    }
}

QVariantList DataFlashLogParser::signalSamples(const QString &signalName) const
{
    return _signalSamples.value(signalName);
}

double DataFlashLogParser::signalValueAt(const QString &signalName, double timestampSeconds) const
{
    const QVariantList points = _signalSamples.value(signalName);
    if (points.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    int nearestIndex = 0;
    double nearestDistance = std::fabs(points.first().toMap().value(QStringLiteral("x")).toDouble() - timestampSeconds);

    for (int i = 1; i < points.count(); ++i) {
        const double distance = std::fabs(points[i].toMap().value(QStringLiteral("x")).toDouble() - timestampSeconds);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = i;
        }
    }

    return points[nearestIndex].toMap().value(QStringLiteral("y")).toDouble();
}

QString DataFlashLogParser::modeAt(double timestampSeconds) const
{
    for (const QVariant &variant : _modeSegments) {
        const QVariantMap segment = variant.toMap();
        const double start = segment.value(QStringLiteral("start")).toDouble();
        const double end = segment.value(QStringLiteral("end")).toDouble();
        if (timestampSeconds >= start && timestampSeconds <= end) {
            return segment.value(QStringLiteral("mode")).toString();
        }
    }

    return QString();
}

QVariantList DataFlashLogParser::eventsNear(double timestampSeconds, double thresholdSeconds) const
{
    QVariantList matches;
    const double threshold = std::max(0.0, thresholdSeconds);

    for (const QVariant &variant : _events) {
        const QVariantMap eventItem = variant.toMap();
        const double eventTime = eventItem.value(QStringLiteral("time")).toDouble();
        if (std::fabs(eventTime - timestampSeconds) <= threshold) {
            matches.append(eventItem);
        }
    }

    return matches;
}

double DataFlashLogParser::_extractTimestampSeconds(const QMap<QString, QVariant> &values)
{
    if (values.contains(QStringLiteral("TimeUS"))) {
        return values.value(QStringLiteral("TimeUS")).toDouble() / 1000000.0;
    }
    if (values.contains(QStringLiteral("TimeMS"))) {
        return values.value(QStringLiteral("TimeMS")).toDouble() / 1000.0;
    }
    if (values.contains(QStringLiteral("Time"))) {
        return values.value(QStringLiteral("Time")).toDouble() / 1000.0;
    }

    return -1.0;
}

void DataFlashLogParser::_appendEvent(double timestampSecs, const QString &type, const QString &description)
{
    if (description.isEmpty()) {
        return;
    }

    QVariantMap eventRow;
    eventRow[QStringLiteral("time")] = timestampSecs;
    eventRow[QStringLiteral("type")] = type;
    eventRow[QStringLiteral("description")] = description;
    _events.append(eventRow);
}

void DataFlashLogParser::_setParseError(const QString &error)
{
    if (_parseError != error) {
        _parseError = error;
        emit parseErrorChanged();
    }
    qCWarning(DataFlashLogParserLog) << error;
}
