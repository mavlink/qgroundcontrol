#pragma once

#include "QGCLogEntry.h"

#include <QtCore/QList>
#include <QtCore/QString>

/// Formats log entries for output.
/// Supports multiple output formats: plain text, JSON, CSV.
class LogFormatter
{
public:
    enum Format {
        PlainText,   ///< Human-readable text (default)
        JSON,        ///< JSON array of objects
        CSV,         ///< Comma-separated values with header
        JSONLines    ///< One JSON object per line (JSONL/NDJSON)
    };

    /// Formats a single entry as plain text.
    static QString formatText(const QGCLogEntry& entry);

    /// Formats a single entry as JSON object string.
    static QString formatJson(const QGCLogEntry& entry);

    /// Formats a single entry as CSV row (no header).
    static QString formatCsvRow(const QGCLogEntry& entry);

    /// Returns CSV header row.
    static QString csvHeader();

    /// Formats multiple entries in the specified format.
    static QString format(const QList<QGCLogEntry>& entries, Format format);

    /// Formats multiple entries as plain text (one per line).
    static QString formatAsText(const QList<QGCLogEntry>& entries);

    /// Formats multiple entries as JSON array.
    static QString formatAsJson(const QList<QGCLogEntry>& entries);

    /// Formats multiple entries as CSV with header.
    static QString formatAsCsv(const QList<QGCLogEntry>& entries);

    /// Formats multiple entries as JSON Lines (one JSON object per line).
    static QString formatAsJsonLines(const QList<QGCLogEntry>& entries);

    /// Returns file extension for format.
    static QString fileExtension(Format format);

    /// Returns MIME type for format.
    static QString mimeType(Format format);

private:
    static QString escapeCsvField(const QString& str);
};
