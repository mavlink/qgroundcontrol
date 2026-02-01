#include "LogFormatter.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

QString LogFormatter::formatText(const QGCLogEntry& entry)
{
    return entry.toString();
}

QString LogFormatter::formatJson(const QGCLogEntry& entry)
{
    QJsonObject json;
    json["timestamp"] = entry.timestamp.toString(Qt::ISODateWithMs);
    json["level"] = QGCLogEntry::levelLabel(entry.level);
    json["category"] = entry.category;
    json["message"] = entry.message;

    if (!entry.function.isEmpty()) {
        if (!entry.file.isEmpty()) {
            json["file"] = entry.file;
        }
        json["function"] = entry.function;
        json["line"] = entry.line;
    }

    return QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

QString LogFormatter::formatCsvRow(const QGCLogEntry& entry)
{
    QStringList fields;
    fields << escapeCsvField(entry.timestamp.toString(Qt::ISODateWithMs));
    fields << escapeCsvField(QGCLogEntry::levelLabel(entry.level));
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

QString LogFormatter::format(const QList<QGCLogEntry>& entries, Format format)
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
    return formatAsText(entries);
}

QString LogFormatter::formatAsText(const QList<QGCLogEntry>& entries)
{
    QString result;
    result.reserve(entries.size() * 200);

    for (const QGCLogEntry& entry : entries) {
        result += formatText(entry);
        result += '\n';
    }

    return result;
}

QString LogFormatter::formatAsJson(const QList<QGCLogEntry>& entries)
{
    QJsonArray array;

    for (const QGCLogEntry& entry : entries) {
        QJsonObject json;
        json["timestamp"] = entry.timestamp.toString(Qt::ISODateWithMs);
        json["level"] = QGCLogEntry::levelLabel(entry.level);
        json["category"] = entry.category;
        json["message"] = entry.message;

        if (!entry.function.isEmpty()) {
            if (!entry.file.isEmpty()) {
                json["file"] = entry.file;
            }
            json["function"] = entry.function;
            json["line"] = entry.line;
        }

        array.append(json);
    }

    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Indented));
}

QString LogFormatter::formatAsCsv(const QList<QGCLogEntry>& entries)
{
    QString result;
    result.reserve(entries.size() * 200 + 100);

    result += csvHeader();
    result += '\n';

    for (const QGCLogEntry& entry : entries) {
        result += formatCsvRow(entry);
        result += '\n';
    }

    return result;
}

QString LogFormatter::formatAsJsonLines(const QList<QGCLogEntry>& entries)
{
    QString result;
    result.reserve(entries.size() * 200);

    for (const QGCLogEntry& entry : entries) {
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
    return QStringLiteral("txt");
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
    return QStringLiteral("text/plain");
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
