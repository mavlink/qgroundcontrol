#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

/// Parses and validates MAVLink FTP directory records for one onboard-log scan.
class FtpListingParser final
{
    Q_DISABLE_COPY_MOVE(FtpListingParser)

public:
    struct LogDescriptor
    {
        uint id = 0;
        QDateTime time;
        uint size = 0;
        QString ftpPath;
    };

    struct ParseResult
    {
        QList<LogDescriptor> logs;
        QStringList directories;
        QStringList unsafePaths;
        uint malformedSupportedLogs = 0;
        uint duplicateLogs = 0;
        uint duplicateDirectories = 0;
        bool logLimitReached = false;
        bool directoryLimitReached = false;

        bool partial() const
        {
            return !unsafePaths.isEmpty() || (malformedSupportedLogs > 0) || logLimitReached || directoryLimitReached;
        }
    };

    FtpListingParser() = default;

    void reset(const QString& root, uint firstLogId = 0);
    ParseResult parse(const QStringList& entries, const QString& subdir, bool collectDirectories);

    uint nextLogId() const { return _nextLogId; }

    static bool isSafePathComponent(const QString& component);

    static constexpr uint kMaxLogEntries = 10000;
    static constexpr qsizetype kMaxLogDirectories = 1000;

private:
    static bool _isSupportedLogName(const QString& fileName);

    QString _root;
    uint _nextLogId = 0;
    QSet<QString> _acceptedLogPaths;
    QSet<QString> _acceptedDirectories;
};
