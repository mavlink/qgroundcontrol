#include "QGCDecompressDevice.h"
#include "QGCLoggingCategory.h"

#include <archive.h>
#include <archive_entry.h>

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
// QIODevice Interface
// ============================================================================

bool QGCDecompressDevice::open(OpenMode mode)
{
    if (mode != ReadOnly) {
        _errorString = QStringLiteral("QGCDecompressDevice only supports ReadOnly mode");
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

// ============================================================================
// Protected Implementation
// ============================================================================

bool QGCDecompressDevice::initArchive()
{
    _archive = archive_read_new();
    if (!_archive) {
        _errorString = QStringLiteral("Failed to create archive reader");
        return false;
    }

    // Raw format for single-file decompression
    configureArchiveFormats(false);

    return openArchive();
}

bool QGCDecompressDevice::prepareForReading()
{
    // Read the first (and only) entry header for raw format
    struct archive_entry *entry = nullptr;
    if (archive_read_next_header(_archive, &entry) != ARCHIVE_OK) {
        _errorString = QString::fromUtf8(archive_error_string(_archive));
        archive_read_free(_archive);
        _archive = nullptr;
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
