#include "LogFormatter.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

QString LogFormatter::formatText(const LogEntry& entry)
{
    return entry.toString();
}

QJsonObject LogFormatter::toJsonObject(const LogEntry& entry)
{
    QJsonObject json;
    json["timestamp"] = entry.timestamp.toString(Qt::ISODateWithMs);
    json["level"] = LogEntry::levelLabel(entry.level);
    json["category"] = entry.category;
    json["message"] = entry.message;

    if (!entry.function.isEmpty() || !entry.file.isEmpty() || entry.line != 0) {
        if (!entry.file.isEmpty()) {
            json["file"] = entry.file;
        }
        if (!entry.function.isEmpty()) {
            json["function"] = entry.function;
        }
        if (entry.line != 0) {
            json["line"] = entry.line;
        }
    }

    return json;
}

QString LogFormatter::formatJson(const LogEntry& entry)
{
    return QString::fromUtf8(QJsonDocument(toJsonObject(entry)).toJson(QJsonDocument::Compact));
}

QString LogFormatter::formatCsvRow(const LogEntry& entry)
{
    QStringList fields;
    fields << escapeCsvField(entry.timestamp.toString(Qt::ISODateWithMs));
    fields << escapeCsvField(LogEntry::levelLabel(entry.level));
    fields << escapeCsvField(entry.category);
    fields << escapeCsvField(entry.message);
    fields << escapeCsvField(entry.file);
    fields << escapeCsvField(entry.function);
    fields << QString::number(entry.line);

    return fields.join(',');
}

QString LogFormatter::csvHeader()
{
    return QStringLiteral("timestamp,level,category,message,file,function,line");
}

QString LogFormatter::format(const QList<LogEntry>& entries, Format format)
{
    switch (format) {
    case PlainText:
        return formatAsText(entries);
    case JSON:
        return formatAsJson(entries);
    case CSV:
        return formatAsCsv(entries);
    case JSONLines:
        return formatAsJsonLines(entries);
    }
    Q_UNREACHABLE();
}

QString LogFormatter::formatAsText(const QList<LogEntry>& entries)
{
    QString result;
    result.reserve(entries.size() * 200);

    for (const LogEntry& entry : entries) {
        result += formatText(entry);
        result += '\n';
    }

    return result;
}

QString LogFormatter::formatAsJson(const QList<LogEntry>& entries)
{
    QJsonArray array;

    for (const LogEntry& entry : entries) {
        array.append(toJsonObject(entry));
    }

    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Indented));
}

QString LogFormatter::formatAsCsv(const QList<LogEntry>& entries)
{
    QString result;
    result.reserve(entries.size() * 200 + 100);

    result += csvHeader();
    result += '\n';

    for (const LogEntry& entry : entries) {
        result += formatCsvRow(entry);
        result += '\n';
    }

    return result;
}

QString LogFormatter::formatAsJsonLines(const QList<LogEntry>& entries)
{
    QString result;
    result.reserve(entries.size() * 200);

    for (const LogEntry& entry : entries) {
        result += formatJson(entry);
        result += '\n';
    }

    return result;
}

QString LogFormatter::fileExtension(Format format)
{
    switch (format) {
    case PlainText:
        return QStringLiteral("txt");
    case JSON:
        return QStringLiteral("json");
    case CSV:
        return QStringLiteral("csv");
    case JSONLines:
        return QStringLiteral("jsonl");
    }
    Q_UNREACHABLE();
}

QString LogFormatter::mimeType(Format format)
{
    switch (format) {
    case PlainText:
        return QStringLiteral("text/plain");
    case JSON:
        return QStringLiteral("application/json");
    case CSV:
        return QStringLiteral("text/csv");
    case JSONLines:
        return QStringLiteral("application/x-ndjson");
    }
    Q_UNREACHABLE();
}

QString LogFormatter::escapeCsvField(const QString& str)
{
    if (str.contains(',') || str.contains('"') || str.contains('\n') || str.contains('\r')) {
        QString escaped = str;
        escaped.replace('"', "\"\"");
        return '"' + escaped + '"';
    }
    return str;
}
