#pragma once

#include "LogEntry.h"

#include <QtCore/QDateTime>
#include <QtCore/QString>

namespace LogTestHelpers {

inline LogEntry makeEntry(
    const QString &msg,
    LogEntry::Level level = LogEntry::Info,
    const QString &category = QStringLiteral("test.category"),
    const QDateTime &timestamp = QDateTime(QDate(2024, 1, 15), QTime(10, 30, 0), QTimeZone::UTC))
{
    LogEntry e;
    e.timestamp = timestamp;
    e.level = level;
    e.category = category;
    e.message = msg;
    e.buildFormatted();
    return e;
}

} // namespace LogTestHelpers
