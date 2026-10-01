#include "LogViewerController.h"

#include "QGCLoggingCategory.h"

#include <algorithm>
#include <cmath>

QGC_LOGGING_CATEGORY(LogViewerControllerLog, "AnalyzeView.LogViewerController")

namespace {

// Compares digit runs by numeric value so "control[2]" sorts before "control[10]"
bool _naturalLess(const QString& a, const QString& b)
{
    const auto isDigit = [](QChar c) { return (c >= QLatin1Char('0')) && (c <= QLatin1Char('9')); };

    qsizetype i = 0;
    qsizetype j = 0;
    while ((i < a.size()) && (j < b.size())) {
        if (isDigit(a[i]) && isDigit(b[j])) {
            const qsizetype aStart = i;
            const qsizetype bStart = j;
            while ((i < a.size()) && isDigit(a[i])) {
                i++;
            }
            while ((j < b.size()) && isDigit(b[j])) {
                j++;
            }
            const QStringView aNumber = QStringView(a).sliced(aStart, i - aStart);
            const QStringView bNumber = QStringView(b).sliced(bStart, j - bStart);
            if (aNumber.size() != bNumber.size()) {
                return aNumber.size() < bNumber.size();
            }
            const int numberCompare = aNumber.compare(bNumber);
            if (numberCompare != 0) {
                return numberCompare < 0;
            }
            continue;
        }
        if (a[i] != b[j]) {
            return a[i] < b[j];
        }
        i++;
        j++;
    }

    return (a.size() - i) < (b.size() - j);
}

}  // namespace

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
    _plottableFields.clear();
    _fieldRows.clear();
    _selectedFields.clear();
    _expandedGroups.clear();
    emit plottableFieldsChanged();
    emit fieldRowsChanged();
    emit selectedFieldsChanged();
    _setLog(SourceType::None, QString());
}

void LogViewerController::openTLog(const QString &path)
{
    _setLog(SourceType::TLog, path);
}

void LogViewerController::openBinLog(const QString &path)
{
    _setLog(SourceType::Bin, path);
}

void LogViewerController::openULogFile(const QString &path)
{
    _setLog(SourceType::ULog, path);
}

void LogViewerController::setPlottableFields(const QStringList &fieldNames)
{
    _plottableFields = fieldNames;
    std::sort(_plottableFields.begin(), _plottableFields.end(), _naturalLess);
    emit plottableFieldsChanged();
    _selectedFields.clear();
    emit selectedFieldsChanged();
    _rebuildFieldRows();
}

void LogViewerController::clearSelection()
{
    if (_selectedFields.isEmpty()) {
        return;
    }

    _selectedFields.clear();
    emit selectedFieldsChanged();
}

void LogViewerController::toggleGroupExpanded(const QString &groupName)
{
    if (_expandedGroups.contains(groupName)) {
        _expandedGroups.remove(groupName);
    } else {
        _expandedGroups.insert(groupName);
    }

    _rebuildFieldRows();
}

bool LogViewerController::isGroupExpanded(const QString &groupName) const
{
    return _expandedGroups.contains(groupName);
}

void LogViewerController::setFieldSelected(const QString &fieldName, bool selected)
{
    const bool currentlySelected = _selectedFields.contains(fieldName);
    if (currentlySelected == selected) {
        return;
    }

    if (selected) {
        _selectedFields.append(fieldName);
    } else {
        _selectedFields.removeAll(fieldName);
    }

    emit selectedFieldsChanged();
}

bool LogViewerController::isFieldSelected(const QString &fieldName) const
{
    return _selectedFields.contains(fieldName);
}

QString LogViewerController::fieldColor(const QString &fieldName) const
{
    return _assignColorForKey(fieldName);
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
    if (eventType == QStringLiteral("warning")) {
        return _assignColorForKey(QStringLiteral("event-warning"));
    }

    return _assignColorForKey(QStringLiteral("event-other"));
}

void LogViewerController::_setLog(SourceType sourceType, const QString &path)
{
    if (_sourceType != sourceType) {
        _sourceType = sourceType;
        emit sourceTypeChanged();
    }

    if (_currentLogPath != path) {
        _currentLogPath = path;
        emit currentLogPathChanged();
    }

    qCDebug(LogViewerControllerLog) << "sourceType" << static_cast<int>(_sourceType) << "path" << _currentLogPath;
}

void LogViewerController::_rebuildFieldRows()
{
    QHash<QString, QStringList> groupedMap;
    QStringList groups;

    for (const QString &field : _plottableFields) {
        const int splitIndex = field.indexOf('.');
        const QString groupName = (splitIndex > 0) ? field.left(splitIndex) : tr("Other");
        const QString shortName = (splitIndex > 0) ? field.mid(splitIndex + 1) : field;
        if (!groupedMap.contains(groupName)) {
            groups.append(groupName);
        }
        groupedMap[groupName].append(shortName);
    }

    std::sort(groups.begin(), groups.end(), _naturalLess);

    QVariantList rows;
    for (const QString &groupName : groups) {
        QVariantMap groupRow;
        groupRow[QStringLiteral("rowType")] = QStringLiteral("group");
        groupRow[QStringLiteral("group")] = groupName;
        rows.append(groupRow);

        if (!_expandedGroups.contains(groupName)) {
            continue;
        }

        QStringList fieldNames = groupedMap.value(groupName);
        std::sort(fieldNames.begin(), fieldNames.end(), _naturalLess);
        for (const QString &shortName : fieldNames) {
            QVariantMap fieldRow;
            fieldRow[QStringLiteral("rowType")] = QStringLiteral("field");
            fieldRow[QStringLiteral("group")] = groupName;
            fieldRow[QStringLiteral("shortName")] = shortName;
            fieldRow[QStringLiteral("fullName")] = QStringLiteral("%1.%2").arg(groupName, shortName);
            rows.append(fieldRow);
        }
    }

    _fieldRows = rows;
    emit fieldRowsChanged();
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
