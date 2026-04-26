#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QString>

#include "LogEntry.h"

namespace LogFormatter {

enum Format
{
    PlainText = 0,
    JSON = 1,
    CSV = 2,
    JSONLines = 3,
};

enum JsonSchema
{
    ExportSchema,
    RemoteCompactSchema,
};

QJsonObject entryToJson(const LogEntry& entry, JsonSchema schema = ExportSchema);

QString formatCsvRow(const LogEntry& entry);
QString csvHeader();

QByteArray format(const QList<LogEntry>& entries, int fmt);
QByteArray formatAsText(const QList<LogEntry>& entries);
QByteArray formatAsJson(const QList<LogEntry>& entries);
QByteArray formatAsCsv(const QList<LogEntry>& entries);
QByteArray formatAsJsonLines(const QList<LogEntry>& entries);

}  // namespace LogFormatter
