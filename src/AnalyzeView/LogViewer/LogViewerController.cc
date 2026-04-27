#include "LogViewerController.h"

#include "QGCLoggingCategory.h"

#include <algorithm>
#include <cmath>

QGC_LOGGING_CATEGORY(LogViewerControllerLog, "AnalyzeView.LogViewerController")

LogViewerController::LogViewerController(QObject *parent)
    : QObject(parent)
{
    qCDebug(LogViewerControllerLog) << this;
}

LogViewerController::~LogViewerController()
{
    qCDebug(LogViewerControllerLog) << this;
}

void LogViewerController::clear()
{
    _plottableSignals.clear();
    _signalRows.clear();
    _selectedSignals.clear();
    _expandedGroups.clear();
    emit signalRowsChanged();
    emit selectedSignalsChanged();
    _setLog(SourceType::None, QString(), tr("No log loaded"));
}

void LogViewerController::openTLog(const QString &path)
{
    _setLog(SourceType::TLog, path, tr("Telemetry log loaded"));
}

void LogViewerController::openBinLog(const QString &path)
{
    _setLog(SourceType::Bin, path, tr("DataFlash log loaded"));
}

void LogViewerController::setPlottableSignals(const QStringList &signalNames)
{
    _plottableSignals = signalNames;
    std::sort(_plottableSignals.begin(), _plottableSignals.end());
    _selectedSignals.clear();
    emit selectedSignalsChanged();
    _rebuildSignalRows();
}

void LogViewerController::clearSelection()
{
    if (_selectedSignals.isEmpty()) {
        return;
    }

    _selectedSignals.clear();
    emit selectedSignalsChanged();
}

void LogViewerController::toggleGroupExpanded(const QString &groupName)
{
    if (_expandedGroups.contains(groupName)) {
        _expandedGroups.remove(groupName);
    } else {
        _expandedGroups.insert(groupName);
    }

    _rebuildSignalRows();
}

bool LogViewerController::isGroupExpanded(const QString &groupName) const
{
    return _expandedGroups.contains(groupName);
}

void LogViewerController::toggleSignal(const QString &signalName)
{
    if (_selectedSignals.contains(signalName)) {
        _selectedSignals.removeAll(signalName);
    } else {
        _selectedSignals.append(signalName);
    }

    emit selectedSignalsChanged();
}

bool LogViewerController::isSignalSelected(const QString &signalName) const
{
    return _selectedSignals.contains(signalName);
}

QString LogViewerController::signalColor(const QString &signalName) const
{
    return _assignColorForKey(signalName);
}

QString LogViewerController::eventColor(const QString &eventType) const
{
    if (eventType == QStringLiteral("mode")) {
        return _assignColorForKey(QStringLiteral("event-mode"));
    }
    if (eventType == QStringLiteral("error")) {
        return _assignColorForKey(QStringLiteral("event-error"));
    }
    if (eventType == QStringLiteral("event")) {
        return _assignColorForKey(QStringLiteral("event-generic"));
    }

    return _assignColorForKey(QStringLiteral("event-other"));
}

QString LogViewerController::modeColor(const QString &modeName) const
{
    return _assignColorForKey(QStringLiteral("mode-%1").arg(modeName));
}

QStringList LogViewerController::modeLegendEntries(const QVariantList &modeSegments) const
{
    QStringList modes;
    for (const QVariant &variant : modeSegments) {
        const QVariantMap segment = variant.toMap();
        const QString mode = segment.value(QStringLiteral("mode")).toString();
        if (!mode.isEmpty() && !modes.contains(mode)) {
            modes.append(mode);
        }
    }

    return modes;
}

void LogViewerController::_setLog(SourceType sourceType, const QString &path, const QString &statusText)
{
    if (_sourceType != sourceType) {
        _sourceType = sourceType;
        emit sourceTypeChanged();
    }

    if (_currentLogPath != path) {
        _currentLogPath = path;
        emit currentLogPathChanged();
    }

    if (_statusText != statusText) {
        _statusText = statusText;
        emit statusTextChanged();
    }

    qCDebug(LogViewerControllerLog) << "sourceType" << static_cast<int>(_sourceType) << "path" << _currentLogPath;
}

void LogViewerController::_rebuildSignalRows()
{
    QHash<QString, QStringList> groupedMap;
    QStringList groups;

    for (const QString &signal : _plottableSignals) {
        const int splitIndex = signal.indexOf('.');
        const QString groupName = (splitIndex > 0) ? signal.left(splitIndex) : QStringLiteral("Other");
        const QString shortName = (splitIndex > 0) ? signal.mid(splitIndex + 1) : signal;
        groupedMap[groupName].append(shortName);
        if (!groups.contains(groupName)) {
            groups.append(groupName);
        }
    }

    std::sort(groups.begin(), groups.end());

    QVariantList rows;
    for (const QString &groupName : groups) {
        QVariantMap groupRow;
        groupRow[QStringLiteral("rowType")] = QStringLiteral("group");
        groupRow[QStringLiteral("group")] = groupName;
        rows.append(groupRow);

        if (!_expandedGroups.contains(groupName)) {
            continue;
        }

        QStringList signalNames = groupedMap.value(groupName);
        std::sort(signalNames.begin(), signalNames.end());
        for (const QString &shortName : signalNames) {
            QVariantMap signalRow;
            signalRow[QStringLiteral("rowType")] = QStringLiteral("signal");
            signalRow[QStringLiteral("group")] = groupName;
            signalRow[QStringLiteral("shortName")] = shortName;
            signalRow[QStringLiteral("fullName")] = QStringLiteral("%1.%2").arg(groupName, shortName);
            rows.append(signalRow);
        }
    }

    _signalRows = rows;
    emit signalRowsChanged();
}

QString LogViewerController::_assignColorForKey(const QString &key) const
{
    static const QStringList palette = {
        QStringLiteral("#3776D6"),
        QStringLiteral("#D9534F"),
        QStringLiteral("#3FA96B"),
        QStringLiteral("#D98E04"),
        QStringLiteral("#7B5CC9"),
        QStringLiteral("#D64E8B"),
        QStringLiteral("#2FA9A2"),
        QStringLiteral("#D96A2D"),
        QStringLiteral("#4A6CD4"),
        QStringLiteral("#6EA827"),
    };

    quint32 hash = 0;
    for (const QChar ch : key) {
        hash = (hash * 31U) + ch.unicode();
    }

    const qsizetype idx = static_cast<qsizetype>(hash % static_cast<quint32>(palette.count()));
    return palette[idx];
}
