#include "LogFormatter.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "LogEntry.h"

namespace LogFormatter {

// ---------------------------------------------------------------------------
// Single-entry formatting
// ---------------------------------------------------------------------------

static QString escapeCsv(const QString& field)
{
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace('"', QStringLiteral("\"\""));
        return '"' + escaped + '"';
    }
    return field;
}

QString formatCsvRow(const LogEntry& entry)
{
    return escapeCsv(entry.timestamp.toString(Qt::ISODateWithMs)) + ',' + entry.levelLabel() + ',' +
           escapeCsv(entry.category) + ',' + escapeCsv(entry.message) + ',' +
           escapeCsv(entry.file) + ',' + (entry.file.isEmpty() ? QString() : QString::number(entry.line));
}

QString csvHeader()
{
    return QStringLiteral("timestamp,level,category,message,file,line");
}

// ---------------------------------------------------------------------------
// Batch formatting
// ---------------------------------------------------------------------------

QByteArray format(const QList<LogEntry>& entries, int fmt)
{
    switch (fmt) {
        case JSON:
            return formatAsJson(entries);
        case CSV:
            return formatAsCsv(entries);
        case JSONLines:
            return formatAsJsonLines(entries);
        default:
            return formatAsText(entries);
    }
}

QByteArray formatAsText(const QList<LogEntry>& entries)
{
    QByteArray result;
    result.reserve(entries.size() * 150);
    for (const auto& e : entries) {
        result.append(e.formatted.toUtf8());
        if (!e.file.isEmpty()) {
            if (e.line > 0) {
                result.append(QStringLiteral(" (%1:%2)").arg(e.file).arg(e.line).toUtf8());
            } else {
                result.append(QStringLiteral(" (%1)").arg(e.file).toUtf8());
            }
        }
        result.append('\n');
    }
    return result;
}

QJsonObject entryToJson(const LogEntry& e, JsonSchema schema)
{
    if (schema == RemoteCompactSchema) {
        return {
            {"t", e.timestamp.toMSecsSinceEpoch()},
            {"l", static_cast<int>(e.level)},
            {"c", e.category},
            {"m", e.message},
        };
    }
    return {
        {"timestamp", e.timestamp.toString(Qt::ISODateWithMs)},
        {"level", e.levelLabel()},
        {"category", e.category},
        {"message", e.message},
    };
}

QByteArray formatAsJson(const QList<LogEntry>& entries)
{
    QJsonArray array;
    for (const auto& e : entries) {
        array.append(entryToJson(e));
    }
    return QJsonDocument(array).toJson(QJsonDocument::Indented);
}

QByteArray formatAsCsv(const QList<LogEntry>& entries)
{
    QByteArray result;
    result.reserve(entries.size() * 100);
    result.append(csvHeader().toUtf8());
    result.append('\n');
    for (const auto& e : entries) {
        result.append(formatCsvRow(e).toUtf8());
        result.append('\n');
    }
    return result;
}

QByteArray formatAsJsonLines(const QList<LogEntry>& entries)
{
    QByteArray result;
    result.reserve(entries.size() * 150);
    for (const auto& e : entries) {
        result.append(QJsonDocument(entryToJson(e)).toJson(QJsonDocument::Compact));
        result.append('\n');
    }
    return result;
}

}  // namespace LogFormatter
