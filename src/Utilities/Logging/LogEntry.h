#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QMetaType>
#include <QtCore/QString>

/// Represents a single log entry with structured data.
struct LogEntry
{
    enum Level {
        Debug,
        Info,
        Warning,
        Critical,
        Fatal
    };

    QDateTime timestamp;
    Level level = Debug;
    QString category;
    QString message;
    QString file;
    QString function;
    int line = 0;

    /// Formats the entry as a single-line string for display/storage.
    QString toString() const;

    /// Parses a QtMsgType to Level.
    static Level fromQtMsgType(QtMsgType type);

    /// Returns a short label for the level (e.g., "W" for Warning).
    static QString levelLabel(Level level);
};

Q_DECLARE_METATYPE(LogEntry)
