#include "LogFileParser.h"

#include "LogViewerDataFlashParser.h"
#include "LogParseResultPrivate.h"
#include "LogViewerParamMetaData.h"
#include "QGCLoggingCategory.h"
#include "LogViewerULogParser.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFileInfo>
#include <QtCore/QFutureWatcher>

#include <algorithm>
#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(LogFileParserLog, "AnalyzeView.LogFileParser")

namespace {

LogParseResult _parseFile(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();

    if (suffix == QStringLiteral("bin") || suffix == QStringLiteral("log")) {
        return DataFlashParser::parseFile(filePath);
    }

    if (suffix == QStringLiteral("ulg")) {
        return ULogParser::parseFile(filePath);
    }

    const QString fileTypeDescription = suffix.isEmpty()
        ? LogFileParser::tr("no extension")
        : QStringLiteral(".%1").arg(suffix);

    LogParseResult result;
    result.errorMessage = LogFileParser::tr(
        "Unsupported file type (%1) for file '%2'. Expected .bin, .log, or .ulg.")
        .arg(fileTypeDescription, filePath);
    return result;
}

} // namespace

// ============================================================================
// LogFileParser
// ============================================================================

LogFileParser::LogFileParser(QObject *parent)
    : QObject(parent)
{
    qCDebug(LogFileParserLog) << this;
}

LogFileParser::~LogFileParser()
{
    qCDebug(LogFileParserLog) << this;
}

bool LogFileParser::parseFile(const QString &filePath)
{
    ++_parseRequestId;
    clear();
    const LogParseResult result = _parseFile(filePath);
    if (!result.ok) {
        _setParseError(result.errorMessage);
        return false;
    }
    _applyResult(result);
    qCDebug(LogFileParserLog) << "Parsed fields" << _availableFields.count()
                              << "parameters" << _parameters.count()
                              << "events" << _events.count();
    return true;
}

void LogFileParser::parseFileAsync(const QString &filePath)
{
    const quint64 requestId = ++_parseRequestId;
    clear();

    auto *watcher = new QFutureWatcher<LogParseResult>(this);
    (void) connect(watcher, &QFutureWatcher<LogParseResult>::finished, this,
        [this, watcher, filePath, requestId]() {
            const LogParseResult result = watcher->result();
            watcher->deleteLater();

            if (requestId != _parseRequestId) {
                return;
            }

            if (!result.ok) {
                _setParseError(result.errorMessage);
                emit parseFileFinished(filePath, false, result.errorMessage);
                return;
            }

            _applyResult(result);
            emit parseFileFinished(filePath, true, QString());
        });

    watcher->setFuture(QtConcurrent::run([filePath]() {
        return _parseFile(filePath);
    }));
}

void LogFileParser::_applyResult(const LogParseResult &result)
{
    _availableFields = result.availableFields;
    _plottableFields = result.plottableFields;
    _parameters = result.parameters;

    // Enrich parameter rows with FactMetaData (decimal places, units,
    // short description, enum strings/values) from the bundled metadata JSON.
    if (!_parameters.isEmpty()) {
        if (result.sourceType == LogParseResult::SourceType::PX4ULog) {
            LogViewerParamMetaData::enrichForPX4(_parameters);
#ifndef QGC_NO_ARDUPILOT_DIALECT
        } else if (result.sourceType == LogParseResult::SourceType::APMDataFlash) {
            LogViewerParamMetaData::enrichForAPM(
                _parameters,
                result.detectedVehicleType,
                result.firmwareMajorVersion,
                result.firmwareMinorVersion);
#endif
        }
    }
    _events = result.events;
    _messages = result.messages;
    _modeSegments = result.modeSegments;
    _dropouts = result.dropouts;
    _fieldSamples = result.fieldSamples;
    _sampleCount = result.sampleCount;
    _detectedVehicleType = result.detectedVehicleType;
    emit availableFieldsChanged();
    emit plottableFieldsChanged();
    emit parametersChanged();
    emit eventsChanged();
    emit messagesChanged();
    emit modeSegmentsChanged();
    emit dropoutsChanged();
    emit detectedVehicleTypeChanged();
    if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
        _minTimestamp = result.minTimestamp;
        _maxTimestamp = result.maxTimestamp;
        emit timeRangeChanged();
    }
    emit sampleCountChanged();

    _parsed = true;
    emit parsedChanged();
}

void LogFileParser::clear()
{
    const bool oldParsed = _parsed;
    _parsed = false;
    if (oldParsed) { emit parsedChanged(); }

    if (!_parseError.isEmpty()) { _parseError.clear(); emit parseErrorChanged(); }
    if (!_availableFields.isEmpty()) { _availableFields.clear(); emit availableFieldsChanged(); }
    if (!_parameters.isEmpty()) { _parameters.clear(); emit parametersChanged(); }
    if (!_events.isEmpty()) { _events.clear(); emit eventsChanged(); }
    if (!_messages.isEmpty()) { _messages.clear(); emit messagesChanged(); }
    if (!_modeSegments.isEmpty()) { _modeSegments.clear(); emit modeSegmentsChanged(); }
    if (!_dropouts.isEmpty()) { _dropouts.clear(); emit dropoutsChanged(); }
    if (!_detectedVehicleType.isEmpty()) { _detectedVehicleType.clear(); emit detectedVehicleTypeChanged(); }
    if (!_plottableFields.isEmpty()) { _plottableFields.clear(); emit plottableFieldsChanged(); }

    _fieldSamples.clear();
    _gpsLatField.clear();
    _gpsLonField.clear();
    _gpsAltField.clear();
    if (_minTimestamp != -1.0 || _maxTimestamp != -1.0) {
        _minTimestamp = -1.0;
        _maxTimestamp = -1.0;
        emit timeRangeChanged();
    }
    if (_sampleCount != 0) { _sampleCount = 0; emit sampleCountChanged(); }
}

QVariantList LogFileParser::fieldSamples(const QString &fieldName) const
{
    QVariantList output;
    const auto it = _fieldSamples.constFind(fieldName);
    if (it == _fieldSamples.cend()) { return output; }
    const QVector<QPointF> &points = it.value();
    output.reserve(points.size());
    for (const QPointF &p : points) { output.append(p); }
    return output;
}

QVariantMap LogFileParser::fieldMinMax(const QString &fieldName) const
{
    const auto it = _fieldSamples.constFind(fieldName);
    if (it == _fieldSamples.cend() || it->isEmpty()) { return {}; }
    const QVector<QPointF> &points = it.value();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    for (const QPointF &p : points) {
        if (p.y() < minY) minY = p.y();
        if (p.y() > maxY) maxY = p.y();
    }
    return QVariantMap{{QStringLiteral("min"), minY}, {QStringLiteral("max"), maxY}};
}

QVariantList LogFileParser::fieldSamplesFiltered(const QString &fieldName, double minX, double maxX, int pixelWidth) const
{
    QVariantList output;
    const auto it = _fieldSamples.constFind(fieldName);
    if (it == _fieldSamples.cend() || pixelWidth <= 0 || maxX <= minX) { return output; }

    const QVector<QPointF> &points = it.value();

    // Find the slice within [minX, maxX]
    const auto sliceBegin = std::lower_bound(points.cbegin(), points.cend(), minX,
        [](const QPointF &p, double t) { return p.x() < t; });
    const auto sliceEnd = std::upper_bound(sliceBegin, points.cend(), maxX,
        [](double t, const QPointF &p) { return t < p.x(); });

    const qsizetype sliceCount = std::distance(sliceBegin, sliceEnd);
    if (sliceCount == 0) { return output; }

    // If already sparse enough, return slice as-is
    if (sliceCount <= 4 * pixelWidth) {
        output.reserve(sliceCount);
        for (auto jt = sliceBegin; jt != sliceEnd; ++jt) { output.append(*jt); }
        return output;
    }

    // Screen-space min/max bucketing: one bucket per pixel column.
    // For each column track first, min-y, max-y, last indices; flush in time order.
    output.reserve(4 * pixelWidth);
    const double range = maxX - minX;

    auto columnOf = [&](double x) -> int {
        return static_cast<int>((x - minX) / range * pixelWidth);
    };

    // Per-column state
    int    curCol   = -1;
    qsizetype firstIdx = -1;
    qsizetype minIdx   = -1;
    qsizetype maxIdx   = -1;
    qsizetype lastIdx  = -1;

    auto flush = [&]() {
        if (firstIdx < 0) return;
        // Collect the up-to-4 representative indices in time order, deduplicated
        qsizetype indices[4] = { firstIdx, minIdx, maxIdx, lastIdx };
        std::sort(indices, indices + 4);
        qsizetype prev = -1;
        for (qsizetype idx : indices) {
            if (idx != prev) {
                output.append(*(sliceBegin + idx));
                prev = idx;
            }
        }
    };

    for (qsizetype i = 0; i < sliceCount; ++i) {
        const QPointF &p = *(sliceBegin + i);
        const int col = std::clamp(columnOf(p.x()), 0, pixelWidth - 1);
        if (col != curCol) {
            flush();
            curCol   = col;
            firstIdx = i;
            minIdx   = i;
            maxIdx   = i;
        } else {
            if (p.y() < (sliceBegin + minIdx)->y()) minIdx = i;
            if (p.y() > (sliceBegin + maxIdx)->y()) maxIdx = i;
        }
        lastIdx = i;
    }
    flush();

    return output;
}

double LogFileParser::fieldValueAt(const QString &fieldName, double timestampSeconds) const
{
    const auto it = _fieldSamples.constFind(fieldName);
    if (it == _fieldSamples.cend() || it->isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const QVector<QPointF> &points = it.value();
    const auto lower = std::lower_bound(points.cbegin(), points.cend(), timestampSeconds,
        [](const QPointF &p, double t) { return p.x() < t; });

    if (lower == points.cbegin()) { return lower->y(); }
    if (lower == points.cend()) { return points.constLast().y(); }

    const auto prev = std::prev(lower);
    return (std::fabs(prev->x() - timestampSeconds) <= std::fabs(lower->x() - timestampSeconds))
        ? prev->y() : lower->y();
}

QString LogFileParser::modeAt(double timestampSeconds) const
{
    for (const QVariant &v : _modeSegments) {
        const QVariantMap seg = v.toMap();
        const double start = seg.value(QStringLiteral("start")).toDouble();
        const double end   = seg.value(QStringLiteral("end")).toDouble();
        if (timestampSeconds >= start && timestampSeconds <= end) {
            return seg.value(QStringLiteral("mode")).toString();
        }
    }
    return QString();
}

QVariantList LogFileParser::eventsNear(double timestampSeconds, double thresholdSeconds) const
{
    QVariantList matches;
    const double threshold = std::max(0.0, thresholdSeconds);
    const auto lower = std::lower_bound(_events.cbegin(), _events.cend(),
        timestampSeconds - threshold,
        [](const QVariant &v, double t) {
            return v.toMap().value(QStringLiteral("time")).toDouble() < t;
        });
    for (auto it = lower; it != _events.cend(); ++it) {
        const QVariantMap ev = it->toMap();
        if (ev.value(QStringLiteral("time")).toDouble() > timestampSeconds + threshold) { break; }
        matches.append(ev);
    }
    return matches;
}

void LogFileParser::_setParseError(const QString &error)
{
    if (_parseError != error) {
        _parseError = error;
        emit parseErrorChanged();
    }
}

QString LogFileParser::gpsAltitudeFieldName() const
{
    return _gpsAltField;
}

QVariantMap LogFileParser::gpsCoordAt(double timestampSeconds) const
{
    if (_gpsLatField.isEmpty() || _gpsLonField.isEmpty()) {
        return {};
    }

    const auto latIt = _fieldSamples.constFind(_gpsLatField);
    const auto lonIt = _fieldSamples.constFind(_gpsLonField);
    if (latIt == _fieldSamples.cend() || lonIt == _fieldSamples.cend()) {
        return {};
    }

    const QVector<QPointF> &latPts = latIt.value();
    const QVector<QPointF> &lonPts = lonIt.value();
    if (latPts.isEmpty() || lonPts.isEmpty()) {
        return {};
    }

    // Binary search for the sample with timestamp closest to timestampSeconds.
    int lo = 0;
    int hi = latPts.size() - 1;
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (latPts[mid].x() < timestampSeconds) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    // lo is the first index >= timestampSeconds; compare with lo-1.
    if (lo > 0) {
        const double dPrev = timestampSeconds - latPts[lo - 1].x();
        const double dCurr = latPts[lo].x() - timestampSeconds;
        if (dPrev < dCurr) {
            --lo;
        }
    }

    const int lonIdx = std::min(lo, static_cast<int>(lonPts.size()) - 1);
    QVariantMap coord;
    coord[QStringLiteral("latitude")]  = latPts[lo].y();
    coord[QStringLiteral("longitude")] = lonPts[lonIdx].y();
    return coord;
}

QVariantList LogFileParser::gpsPath() const
{
    // Candidate field-name pairs tried in priority order.
    // All values are stored in degrees (the parsers handle any raw-unit conversion).
    // statusField: if non-null, that field's value must be >= statusMinValue to accept the sample.
    // This is used for APM GPS messages where Status < 3 means no valid 3D fix.
    struct CandidatePair {
        const char *latField;
        const char *lonField;
        const char *altField;
        const char *statusField;
        double      statusMinValue;
    };

    static const CandidatePair candidates[] = {
        // PX4 ULog — vehicle_global_position (EKF-fused position, double degrees)
        { "vehicle_global_position.lat",           "vehicle_global_position.lon",           "vehicle_global_position.alt",            nullptr,       0 },
        { "vehicle_global_position.latitude_deg",  "vehicle_global_position.longitude_deg", "vehicle_global_position.alt",            nullptr,       0 },
        // PX4 ULog — vehicle_gps_position / sensor_gps (newer firmware uses latitude_deg)
        { "vehicle_gps_position.latitude_deg",     "vehicle_gps_position.longitude_deg",    "vehicle_gps_position.altitude_msl_m",    nullptr,       0 },
        { "vehicle_gps_position[0].latitude_deg",  "vehicle_gps_position[0].longitude_deg", "vehicle_gps_position[0].altitude_msl_m", nullptr,       0 },
        { "sensor_gps.latitude_deg",               "sensor_gps.longitude_deg",              "sensor_gps.altitude_msl_m",              nullptr,       0 },
        { "sensor_gps[0].latitude_deg",            "sensor_gps[0].longitude_deg",           "sensor_gps[0].altitude_msl_m",           nullptr,       0 },
        // APM DataFlash — GPS message (Status >= 3 = 3D fix; 'L' type already divided by 1e7)
        { "GPS.Lat",                               "GPS.Lng",                               "GPS.Alt",                                "GPS.Status",  3 },
        { "GPS2.Lat",                              "GPS2.Lng",                              "GPS2.Alt",                               "GPS2.Status", 3 },
        // APM DataFlash — POS message (EKF-fused; no status field, 'L' type already divided by 1e7)
        { "POS.Lat",                               "POS.Lng",                               "POS.Alt",                                nullptr,       0 },
    };

    for (const auto &c : candidates) {
        const auto latIt = _fieldSamples.constFind(QLatin1String(c.latField));
        const auto lonIt = _fieldSamples.constFind(QLatin1String(c.lonField));
        if (latIt == _fieldSamples.cend() || lonIt == _fieldSamples.cend()) {
            continue;
        }

        const QVector<QPointF> &latPts = latIt.value();
        const QVector<QPointF> &lonPts = lonIt.value();
        if (latPts.isEmpty() || lonPts.isEmpty()) {
            continue;
        }

        // Resolve optional status field (same message, same sample count as lat/lon).
        const QVector<QPointF> *statusPts = nullptr;
        if (c.statusField) {
            const auto statusIt = _fieldSamples.constFind(QLatin1String(c.statusField));
            if (statusIt != _fieldSamples.cend() && !statusIt.value().isEmpty()) {
                statusPts = &statusIt.value();
            }
        }

        qCDebug(LogFileParserLog) << "gpsPath: found candidate" << c.latField
            << "samples:" << latPts.size()
            << "first lat:" << latPts.first().y()
            << "first lon:" << lonPts.first().y();

        QVariantList path;
        const int n = std::min(latPts.size(), lonPts.size());
        path.reserve(n);

        for (int i = 0; i < n; i++) {
            // Skip samples that don't have a valid GPS fix.
            if (statusPts && i < statusPts->size() && (*statusPts)[i].y() < c.statusMinValue) {
                continue;
            }

            const double lat = latPts[i].y();
            const double lon = lonPts[i].y();

            if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0
                    || (qFuzzyIsNull(lat) && qFuzzyIsNull(lon))) {
                continue;
            }
            QVariantMap coord;
            coord[QStringLiteral("latitude")]  = lat;
            coord[QStringLiteral("longitude")] = lon;
            path.append(coord);
        }

        qCDebug(LogFileParserLog) << "gpsPath: valid points after filter:" << path.size();

        if (!path.isEmpty()) {
            _gpsLatField = QLatin1String(c.latField);
            _gpsLonField = QLatin1String(c.lonField);
            // Only cache the alt field if it actually exists and has samples;
            // otherwise the altitude chart would be shown with no data.
            const QLatin1String altField(c.altField);
            const auto altIt = _fieldSamples.constFind(altField);
            _gpsAltField = (altIt != _fieldSamples.cend() && !altIt.value().isEmpty()) ? altField : QLatin1String{};
            return path;
        }

        qCDebug(LogFileParserLog) << "gpsPath: all" << n << "points filtered out for candidate" << c.latField;
    }

    qCDebug(LogFileParserLog) << "gpsPath: no GPS data found; available fields containing 'lat' or 'lon':";
    for (auto it = _fieldSamples.cbegin(); it != _fieldSamples.cend(); ++it) {
        const QString &fn = it.key();
        if (fn.contains(QLatin1String("lat"), Qt::CaseInsensitive) || fn.contains(QLatin1String("lon"), Qt::CaseInsensitive)) {
            qCDebug(LogFileParserLog) << " " << fn << "samples:" << it.value().size()
                << (it.value().isEmpty() ? 0.0 : it.value().first().y());
        }
    }
    return {};
}
