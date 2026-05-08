#include "LogFileParser.h"

#include "LogViewerDataFlashParser.h"
#include "LogParseResultPrivate.h"
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
