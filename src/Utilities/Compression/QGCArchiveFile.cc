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
    _errorString.clear();

    if (mode != ReadOnly) {
        setError(QStringLiteral("QGCArchiveFile only supports ReadOnly mode"));
        return false;
    }

    if (_entryName.isEmpty()) {
        setError(QStringLiteral("Entry name cannot be empty"));
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
    _entryFound = false;
    const char* skippedKind = nullptr;

    const QGClibarchive::EntryWalk walk =
        QGClibarchive::forEachEntry(_reader->handle(), [this, &skippedKind](struct archive_entry* entry) {
            // Capture format info on first header
            if (_formatName.isEmpty()) {
                captureFormatInfo();
            }

            if (QString::fromUtf8(archive_entry_pathname(entry)) != _entryName) {
                return true;
            }

            // Read as a file, a directory, a link or a special file would look like one that is empty
            skippedKind = QGClibarchive::skippedEntryKind(entry);
            if (skippedKind != nullptr) {
                return false;
            }

            _entryFound = true;
            _entrySize = archive_entry_size(entry);

            // Get modification time if available
            if (archive_entry_mtime_is_set(entry)) {
                _entryModified = QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry));
            }

            qCDebug(QGCArchiveFileLog) << "Found entry:" << _entryName << "size:" << _entrySize
                                       << "modified:" << _entryModified;
            return false;
        });

    if (_entryFound) {
        return true;
    }

    if (skippedKind != nullptr) {
        setError(QStringLiteral("Entry is a %1, which is not read: %2").arg(QLatin1String(skippedKind), _entryName));
    } else if (walk == QGClibarchive::EntryWalk::Failed) {
        setError(QString::fromUtf8(archive_error_string(_reader->handle())));
    } else {
        setError(QStringLiteral("Entry not found in archive: ") + _entryName);
    }
    return false;
}
