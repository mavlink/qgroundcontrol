#include "QGCArchiveFile.h"

#include <archive.h>
#include <archive_entry.h>

#include "QGCLoggingCategory.h"
#include "QGClibarchive.h"

QGC_LOGGING_CATEGORY(QGCArchiveFileLog, "Utilities.QGCArchiveFile")

// ============================================================================
// Constructors
// ============================================================================

QGCArchiveFile::QGCArchiveFile(const QString &archivePath, const QString &entryName, QObject *parent)
    : QGCArchiveDeviceBase(archivePath, parent)
    , _entryName(entryName)
{
}

QGCArchiveFile::QGCArchiveFile(QIODevice *source, const QString &entryName, QObject *parent)
    : QGCArchiveDeviceBase(source, parent)
    , _entryName(entryName)
{
}

// ============================================================================
// QIODevice Interface
// ============================================================================

bool QGCArchiveFile::open(OpenMode mode)
{
    if (mode != ReadOnly) {
        _errorString = QStringLiteral("QGCArchiveFile only supports ReadOnly mode");
        return false;
    }

    if (_entryName.isEmpty()) {
        _errorString = QStringLiteral("Entry name cannot be empty");
        return false;
    }

    // Handle file path constructor
    if (!_filePath.isEmpty()) {
        if (!initSourceFromPath()) {
            return false;
        }
    }

    if (!initArchive()) {
        return false;
    }

    if (!prepareForReading()) {
        return false;
    }

    return QIODevice::open(mode);
}

qint64 QGCArchiveFile::size() const
{
    return _entrySize >= 0 ? _entrySize : QIODevice::size();
}

// ============================================================================
// Protected Implementation
// ============================================================================

bool QGCArchiveFile::initArchive()
{
    _archive = archive_read_new();
    if (!_archive) {
        _errorString = QStringLiteral("Failed to create archive reader");
        return false;
    }

    // All formats for archives
    configureArchiveFormats(true);

    return openArchive();
}

bool QGCArchiveFile::prepareForReading()
{
    return seekToEntry();
}

void QGCArchiveFile::resetState()
{
    QGCArchiveDeviceBase::resetState();
    _entryFound = false;
    _entrySize = -1;
    _entryModified = QDateTime();
}

// ============================================================================
// Private Methods
// ============================================================================

bool QGCArchiveFile::seekToEntry()
{
    struct archive_entry *entry = nullptr;
    _entryFound = false;

    QGClibarchive::NextEntry next;
    while ((next = QGClibarchive::readNextEntry(_archive, &entry)) == QGClibarchive::NextEntry::Found) {
        // Capture format info on first header
        if (_formatName.isEmpty()) {
            captureFormatInfo();
        }

        const QString currentEntry = QString::fromUtf8(archive_entry_pathname(entry));

        if (currentEntry == _entryName) {
            _entryFound = true;
            _entrySize = archive_entry_size(entry);

            // Get modification time if available
            if (archive_entry_mtime_is_set(entry)) {
                _entryModified = QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry));
            }

            qCDebug(QGCArchiveFileLog) << "Found entry:" << _entryName
                                        << "size:" << _entrySize
                                        << "modified:" << _entryModified;
            return true;
        }
    }

    if (next == QGClibarchive::NextEntry::Error) {
        _errorString = QString::fromUtf8(archive_error_string(_archive));
    } else {
        _errorString = QStringLiteral("Entry not found in archive: ") + _entryName;
    }
    return false;
}
