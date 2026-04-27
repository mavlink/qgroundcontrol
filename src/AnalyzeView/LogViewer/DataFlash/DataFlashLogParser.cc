#include "DataFlashLogParser.h"

#include "DataFlashUtility.h"
#include "QGCLoggingCategory.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFile>
#include <QtCore/QFutureWatcher>
#include <QtCore/QHash>
#include <QtCore/QSet>

#include <algorithm>
#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(DataFlashLogParserLog, "AnalyzeView.DataFlashLogParser")

namespace {
struct ParseResult {
    bool ok = false;
    QString errorMessage;
    QStringList availableSignals;
    QStringList plottableSignals;
    QVariantList parameters;
    QVariantList events;
    QVariantList modeSegments;
    QHash<QString, QVector<QPointF>> signalSamples;
    double minTimestamp = -1.0;
    double maxTimestamp = -1.0;
    int sampleCount = 0;
    QString detectedVehicleType;
};

QString _vehicleTypeFromMessageText(const QString &messageText)
{
    const QString text = messageText.toLower();
    if (text.contains(QStringLiteral("arducopter"))) {
        return QStringLiteral("ArduCopter");
    }
    if (text.contains(QStringLiteral("arduplane"))) {
        return QStringLiteral("ArduPlane");
    }
    if (text.contains(QStringLiteral("ardurover"))) {
        return QStringLiteral("ArduRover");
    }
    if (text.contains(QStringLiteral("ardusub"))) {
        return QStringLiteral("ArduSub");
    }

    return QString();
}

QString _ardupilotModeName(const QString &vehicleType, int modeNumber)
{
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
    static const QHash<int, QString> copterModes = {
        {0, QStringLiteral("Stabilize")},
        {1, QStringLiteral("Acro")},
        {2, QStringLiteral("AltHold")},
        {3, QStringLiteral("Auto")},
        {4, QStringLiteral("Guided")},
        {5, QStringLiteral("Loiter")},
        {6, QStringLiteral("RTL")},
        {7, QStringLiteral("Circle")},
        {9, QStringLiteral("Land")},
        {11, QStringLiteral("Drift")},
        {13, QStringLiteral("Sport")},
        {14, QStringLiteral("Flip")},
        {15, QStringLiteral("AutoTune")},
        {16, QStringLiteral("PosHold")},
        {17, QStringLiteral("Brake")},
        {18, QStringLiteral("Throw")},
        {19, QStringLiteral("Avoid_ADSB")},
        {20, QStringLiteral("Guided_NoGPS")},
        {21, QStringLiteral("Smart_RTL")},
        {22, QStringLiteral("FlowHold")},
        {23, QStringLiteral("Follow")},
        {24, QStringLiteral("ZigZag")},
        {25, QStringLiteral("SystemID")},
        {26, QStringLiteral("Heli_Autorotate")},
        {27, QStringLiteral("Auto RTL")},
        {28, QStringLiteral("Turtle")},
    };

    if (vehicleType == QStringLiteral("ArduPlane")) {
        return planeModes.value(modeNumber, QStringLiteral("Mode %1").arg(modeNumber));
    }
    if (vehicleType == QStringLiteral("ArduCopter")) {
        return copterModes.value(modeNumber, QStringLiteral("Mode %1").arg(modeNumber));
    }

    // Numeric ArduPilot mode values are vehicle-specific. Without reliable
    // vehicle-type detection, preserve numeric representation.
    return QStringLiteral("Mode %1").arg(modeNumber);
}

double _extractTimestampSeconds(const QMap<QString, QVariant> &values)
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

void _appendEvent(QVariantList &events, double timestampSecs, const QString &type, const QString &description)
{
    if ((timestampSecs < 0.0) || description.isEmpty()) {
        return;
    }

    QVariantMap eventRow;
    eventRow[QStringLiteral("time")] = timestampSecs;
    eventRow[QStringLiteral("type")] = type;
    eventRow[QStringLiteral("description")] = description;
    events.append(eventRow);
}

ParseResult _parseDataFlashFile(const QString &filePath)
{
    ParseResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = DataFlashLogParser::tr("Failed to open DataFlash file");
        return result;
    }

    const qint64 fileSize = file.size();
    if (fileSize <= 0) {
        result.errorMessage = DataFlashLogParser::tr("DataFlash file is empty");
        return result;
    }
    if (fileSize > std::numeric_limits<qsizetype>::max()) {
        result.errorMessage = DataFlashLogParser::tr("DataFlash file is too large to parse");
        return result;
    }

    uchar *const mappedData = file.map(0, fileSize);
    if (mappedData == nullptr) {
        result.errorMessage = DataFlashLogParser::tr("Failed to memory-map DataFlash file");
        return result;
    }

    struct ScopedFileUnmap {
        QFile &file;
        uchar *data = nullptr;
        ~ScopedFileUnmap()
        {
            if (data != nullptr) {
                file.unmap(data);
            }
        }
    } scopedFileUnmap{file, mappedData};

    const QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char *>(mappedData), static_cast<qsizetype>(fileSize));

    QMap<uint8_t, DataFlashUtility::MessageFormat> formats;
    if (!DataFlashUtility::parseFmtMessages(bytes.constData(), bytes.size(), formats)) {
        result.errorMessage = DataFlashLogParser::tr("No valid FMT messages were found");
        return result;
    }

    QSet<QString> signalSet;
    QSet<QString> plottableSet;
    double minTimestampSecs = -1.0;
    double maxTimestampSecs = -1.0;
    bool hasOpenModeSegment = false;
    double modeSegmentStartSecs = -1.0;
    QString currentModeName;

    DataFlashUtility::iterateMessages(bytes.constData(), bytes.size(), formats, [&result, &signalSet, &plottableSet, &minTimestampSecs, &maxTimestampSecs, &hasOpenModeSegment, &modeSegmentStartSecs, &currentModeName](uint8_t, const char *payload, int, const DataFlashUtility::MessageFormat &fmt) {
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
                result.parameters.append(row);
            }
        } else if (fmt.name == QStringLiteral("MSG")) {
            const QString text = values.value(QStringLiteral("Message")).toString();
            const QString detected = _vehicleTypeFromMessageText(text);
            if (result.detectedVehicleType.isEmpty() && !detected.isEmpty()) {
                result.detectedVehicleType = detected;
            }
        } else if (fmt.name == QStringLiteral("MODE")) {
            QString modeName = values.value(QStringLiteral("Mode")).toString();
            bool isNumericMode = false;
            const int modeNumber = modeName.toInt(&isNumericMode);
            if (isNumericMode) {
                modeName = _ardupilotModeName(result.detectedVehicleType, modeNumber);
            } else if (modeName.isEmpty()) {
                modeName = QStringLiteral("Unknown");
            }
            _appendEvent(result.events, timestampSecs, QStringLiteral("mode"), DataFlashLogParser::tr("Mode: %1").arg(modeName));
            if (timestampSecs >= 0.0) {
                if (hasOpenModeSegment && (timestampSecs > modeSegmentStartSecs)) {
                    QVariantMap segment;
                    segment[QStringLiteral("mode")] = currentModeName;
                    segment[QStringLiteral("start")] = modeSegmentStartSecs;
                    segment[QStringLiteral("end")] = timestampSecs;
                    result.modeSegments.append(segment);
                }
                hasOpenModeSegment = true;
                modeSegmentStartSecs = timestampSecs;
                currentModeName = modeName;
            }
        } else if (fmt.name == QStringLiteral("ERR")) {
            const QString subsystem = values.value(QStringLiteral("Subsys")).toString();
            const QString ecode = values.value(QStringLiteral("ECode")).toString();
            _appendEvent(result.events, timestampSecs, QStringLiteral("error"), DataFlashLogParser::tr("Error: Subsys=%1 ECode=%2").arg(subsystem, ecode));
        } else if (fmt.name == QStringLiteral("EV")) {
            const int eventId = values.value(QStringLiteral("Id")).toInt();
            QString description = DataFlashLogParser::tr("Event %1").arg(eventId);
            if (eventId == 10) {
                description = DataFlashLogParser::tr("Armed");
            } else if (eventId == 11) {
                description = DataFlashLogParser::tr("Disarmed");
            }
            _appendEvent(result.events, timestampSecs, QStringLiteral("event"), description);
        }

        result.sampleCount++;
        if (timestampSecs >= 0.0) {
            for (auto it = values.cbegin(); it != values.cend(); ++it) {
                const QString signalName = QStringLiteral("%1.%2").arg(fmt.name, it.key());
                const int typeId = it.value().metaType().id();
                const bool numeric = (typeId == QMetaType::Int) || (typeId == QMetaType::UInt) ||
                                     (typeId == QMetaType::LongLong) || (typeId == QMetaType::ULongLong) ||
                                     (typeId == QMetaType::Float) || (typeId == QMetaType::Double);
                if (numeric) {
                    result.signalSamples[signalName].append(QPointF(timestampSecs, it.value().toDouble()));
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
        result.modeSegments.append(segment);
    }

    result.availableSignals = signalSet.values();
    std::sort(result.availableSignals.begin(), result.availableSignals.end());
    result.plottableSignals = plottableSet.values();
    std::sort(result.plottableSignals.begin(), result.plottableSignals.end());
    result.minTimestamp = minTimestampSecs;
    result.maxTimestamp = maxTimestampSecs;
    result.ok = true;
    return result;
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
    ++_parseRequestId; // Invalidate any in-flight async parse results.
    clear();
    const ParseResult result = _parseDataFlashFile(filePath);
    if (!result.ok) {
        _setParseError(result.errorMessage);
        return false;
    }

    _availableSignals = result.availableSignals;
    _plottableSignals = result.plottableSignals;
    _parameters = result.parameters;
    _events = result.events;
    _modeSegments = result.modeSegments;
    _signalSamples = result.signalSamples;
    _sampleCount = result.sampleCount;
    _detectedVehicleType = result.detectedVehicleType;
    emit availableSignalsChanged();
    emit plottableSignalsChanged();
    emit parametersChanged();
    emit eventsChanged();
    emit modeSegmentsChanged();
    emit detectedVehicleTypeChanged();
    if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
        _minTimestamp = result.minTimestamp;
        _maxTimestamp = result.maxTimestamp;
        emit timeRangeChanged();
    }
    emit sampleCountChanged();

    _parsed = true;
    emit parsedChanged();
    qCDebug(DataFlashLogParserLog) << "Parsed signals" << _availableSignals.count() << "parameters" << _parameters.count() << "events" << _events.count();
    return true;
}

void DataFlashLogParser::parseFileAsync(const QString &filePath)
{
    const quint64 requestId = ++_parseRequestId;
    clear();

    auto *watcher = new QFutureWatcher<ParseResult>(this);
    (void) connect(watcher, &QFutureWatcher<ParseResult>::finished, this, [this, watcher, filePath, requestId]() {
        const ParseResult result = watcher->result();
        watcher->deleteLater();

        if (requestId != _parseRequestId) {
            // A newer parse request has superseded this result.
            return;
        }

        if (!result.ok) {
            _setParseError(result.errorMessage);
            emit parseFileFinished(filePath, false, result.errorMessage);
            return;
        }

        _availableSignals = result.availableSignals;
        _plottableSignals = result.plottableSignals;
        _parameters = result.parameters;
        _events = result.events;
        _modeSegments = result.modeSegments;
        _signalSamples = result.signalSamples;
        _sampleCount = result.sampleCount;
        _detectedVehicleType = result.detectedVehicleType;
        emit availableSignalsChanged();
        emit plottableSignalsChanged();
        emit parametersChanged();
        emit eventsChanged();
        emit modeSegmentsChanged();
        emit detectedVehicleTypeChanged();
        if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
            _minTimestamp = result.minTimestamp;
            _maxTimestamp = result.maxTimestamp;
            emit timeRangeChanged();
        }
        emit sampleCountChanged();

        _parsed = true;
        emit parsedChanged();
        emit parseFileFinished(filePath, true, QString());
    });

    watcher->setFuture(QtConcurrent::run([filePath]() {
        return _parseDataFlashFile(filePath);
    }));
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

    if (!_detectedVehicleType.isEmpty()) {
        _detectedVehicleType.clear();
        emit detectedVehicleTypeChanged();
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
    QVariantList output;
    const auto signalIt = _signalSamples.constFind(signalName);
    if (signalIt == _signalSamples.cend()) {
        return output;
    }

    const QVector<QPointF> &points = signalIt.value();
    output.reserve(points.size());
    for (const QPointF &point : points) {
        QVariantMap row;
        row[QStringLiteral("x")] = point.x();
        row[QStringLiteral("y")] = point.y();
        output.append(row);
    }

    return output;
}

double DataFlashLogParser::signalValueAt(const QString &signalName, double timestampSeconds) const
{
    const auto signalIt = _signalSamples.constFind(signalName);
    if ((signalIt == _signalSamples.cend()) || signalIt->isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const QVector<QPointF> &points = signalIt.value();
    const auto lower = std::lower_bound(points.cbegin(), points.cend(), timestampSeconds,
        [](const QPointF &point, double timestamp) {
            return point.x() < timestamp;
        });

    if (lower == points.cbegin()) {
        return lower->y();
    }

    if (lower == points.cend()) {
        return points.constLast().y();
    }

    const auto previous = std::prev(lower);
    const double lowerDistance = std::fabs(lower->x() - timestampSeconds);
    const double previousDistance = std::fabs(previous->x() - timestampSeconds);

    return (previousDistance <= lowerDistance)
        ? previous->y()
        : lower->y();
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

void DataFlashLogParser::_setParseError(const QString &error)
{
    if (_parseError != error) {
        _parseError = error;
        emit parseErrorChanged();
    }
    qCWarning(DataFlashLogParserLog) << error;
}
