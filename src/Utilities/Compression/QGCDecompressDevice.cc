#include "QGCDecompressDevice.h"

#include <archive.h>
#include <archive_entry.h>

#include "QGCLoggingCategory.h"
#include "QGClibarchive.h"

QGC_LOGGING_CATEGORY(QGCDecompressDeviceLog, "Utilities.QGCDecompressDevice")

// ============================================================================
// Constructors
// ============================================================================

QGCDecompressDevice::QGCDecompressDevice(QIODevice *source, QObject *parent)
    : QGCArchiveDeviceBase(source, parent)
{
}

QGCDecompressDevice::QGCDecompressDevice(const QString &filePath, QObject *parent)
    : QGCArchiveDeviceBase(filePath, parent)
{
}

// ============================================================================
// Protected Implementation
// ============================================================================

bool QGCDecompressDevice::initArchive()
{
    // Raw format for single-file decompression
    configureArchiveFormats(false);

    return openArchive();
}

bool QGCDecompressDevice::prepareForReading()
{
    // Read the first (and only) entry header for raw format
    struct archive_entry *entry = nullptr;
    const int header = archive_read_next_header(_reader->handle(), &entry);
    if (QGClibarchive::isEmptyStream(_reader->handle(), header)) {
        // A stream of an empty file has no entry to read from
        _eof = true;
    } else if (header != ARCHIVE_OK) {
        setError(QGClibarchive::streamHeaderError(_reader->handle(), header));
        _reader.reset();
        return false;
    }

    _headerRead = true;
    captureFormatInfo();

    qCDebug(QGCDecompressDeviceLog) << "Opened compressed stream, format:" << _formatName
                                     << "filter:" << _filterName;

    return true;
}

void QGCDecompressDevice::resetState()
{
    QGCArchiveDeviceBase::resetState();
    _headerRead = false;
}
