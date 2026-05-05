#include "FtpListingParser.h"

#include <QtCore/QTimeZone>
#include <limits>

#include "MAVLinkFTP.h"

void FtpListingParser::reset(const QString& root, uint firstLogId)
{
    _root = root;
    _nextLogId = firstLogId;
    _acceptedLogPaths.clear();
    _acceptedDirectories.clear();
}

FtpListingParser::ParseResult FtpListingParser::parse(const QStringList& entries, const QString& subdir,
                                                      bool collectDirectories)
{
    ParseResult result;
    const QDate directoryDate = subdir.isEmpty() ? QDate() : QDate::fromString(subdir, QStringLiteral("yyyy-MM-dd"));

    for (const QString& record : entries) {
        const MavlinkFTP::DirectoryEntryParseResult parsedEntry = MavlinkFTP::parseDirectoryEntry(record);
        const MavlinkFTP::DirectoryEntry& entry = parsedEntry.entry;

        if (collectDirectories && (entry.type == MavlinkFTP::DirectoryEntryType::Directory)) {
            const QString& directoryName = entry.name;
            if (!parsedEntry.valid()) {
                result.unsafePaths.append(directoryName);
                continue;
            }
            if (!isSafePathComponent(directoryName)) {
                result.unsafePaths.append(directoryName);
                continue;
            }
            if (_acceptedDirectories.contains(directoryName)) {
                ++result.duplicateDirectories;
                continue;
            }
            if (_acceptedDirectories.size() >= kMaxLogDirectories) {
                result.directoryLimitReached = true;
                continue;
            }
            _acceptedDirectories.insert(directoryName);
            result.directories.append(directoryName);
            continue;
        }

        if (entry.type != MavlinkFTP::DirectoryEntryType::File) {
            continue;
        }

        const QString& fileName = entry.name;
        if (!_isSupportedLogName(fileName)) {
            continue;
        }
        const bool metadataIsUsable =
            parsedEntry.valid() ||
            (parsedEntry.error == MavlinkFTP::DirectoryEntryParseError::InvalidModificationTime) ||
            (parsedEntry.error == MavlinkFTP::DirectoryEntryParseError::ExtraFields);
        if (!metadataIsUsable || !entry.size.has_value() || (*entry.size > (std::numeric_limits<uint>::max)())) {
            ++result.malformedSupportedLogs;
            continue;
        }
        if (!isSafePathComponent(fileName)) {
            result.unsafePaths.append(fileName);
            continue;
        }

        const uint fileSize = static_cast<uint>(*entry.size);

        const QString ftpPath = subdir.isEmpty()
                                    ? (_root + QStringLiteral("/") + fileName)
                                    : (_root + QStringLiteral("/") + subdir + QStringLiteral("/") + fileName);
        if (_acceptedLogPaths.contains(ftpPath)) {
            ++result.duplicateLogs;
            continue;
        }
        if (_nextLogId >= kMaxLogEntries) {
            result.logLimitReached = true;
            continue;
        }

        QDateTime dateTime;
        if (entry.modificationTime.has_value() && (*entry.modificationTime > 0)) {
            dateTime = QDateTime::fromSecsSinceEpoch(*entry.modificationTime, QTimeZone::UTC);
        }

        if (!dateTime.isValid() && directoryDate.isValid()) {
            const QString baseName = fileName.left(fileName.lastIndexOf(QLatin1Char('.')));
            const QTime fileTime = QTime::fromString(baseName, QStringLiteral("HH_mm_ss"));
            dateTime = QDateTime(directoryDate, fileTime.isValid() ? fileTime : QTime(), QTimeZone::UTC);
        }

        _acceptedLogPaths.insert(ftpPath);
        result.logs.append(LogDescriptor{_nextLogId++, dateTime, fileSize, ftpPath});
    }

    return result;
}

bool FtpListingParser::isSafePathComponent(const QString& component)
{
    if (component.isEmpty() || component == QStringLiteral(".") || component == QStringLiteral("..") ||
        component.contains(QLatin1Char('/')) || component.contains(QLatin1Char('\\'))) {
        return false;
    }

    for (qsizetype index = 0; index < component.size(); ++index) {
        if (component.at(index).unicode() < 0x20U) {
            return false;
        }
    }

    return true;
}

bool FtpListingParser::_isSupportedLogName(const QString& fileName)
{
    return fileName.endsWith(QStringLiteral(".ulg"), Qt::CaseInsensitive) ||
           fileName.endsWith(QStringLiteral(".bin"), Qt::CaseInsensitive);
}
