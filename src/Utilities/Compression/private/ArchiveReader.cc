#include "ArchiveReader.h"

#include <algorithm>

#include "Formats.h"
#include "GzipCheck.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LibArchiveLog, "Utilities.LibArchive")

namespace LibArchive {

// ============================================================================
// Failures
// ============================================================================

QString libarchiveError(struct archive* a)
{
    const char* reason = archive_error_string(a);
    return ((reason != nullptr) && (*reason != '\0')) ? QString::fromUtf8(reason)
                                                      : QStringLiteral("Truncated or unreadable input");
}

Result failure(Error error, const QString& detail)
{
    qCDebug(LibArchiveLog).noquote() << "Failed:" << error << detail;
    return {.error = error, .detail = detail};
}

// ============================================================================
// libarchive callbacks for reading a file
// ============================================================================

namespace {

la_ssize_t fileReadCallback(struct archive*, void* clientData, const void** buffer)
{
    auto* source = static_cast<FileSource*>(clientData);
    const qint64 bytesRead = source->file->read(source->block.data(), source->block.size());
    if (bytesRead < 0) {
        source->readFailed = true;
        return ARCHIVE_FATAL;
    }
    if (source->gzip != nullptr) {
        source->gzip->feed(source->block.constData(), bytesRead);
    }

    *buffer = source->block.constData();
    return static_cast<la_ssize_t>(bytesRead);
}

la_int64_t fileSkipCallback(struct archive*, void* clientData, la_int64_t request)
{
    const auto* source = static_cast<FileSource*>(clientData);
    QFile* file = source->file;

    // The bytes of a gzip member are all checked: libarchive reads them if they are not skipped
    if ((source->gzip != nullptr) && source->gzip->inMember()) {
        return 0;
    }

    // A file can be positioned beyond its end, so skip no more than what is left. libarchive treats a short
    // skip as an archive that is cut inside the skipped data.
    const qint64 currentPos = file->pos();
    const qint64 skipped = qBound(qint64(0), file->size() - currentPos, static_cast<qint64>(request));
    // If the seek fails, libarchive reads and discards the bytes instead
    return file->seek(currentPos + skipped) ? skipped : 0;
}

la_int64_t fileSeekCallback(struct archive*, void* clientData, la_int64_t offset, int whence)
{
    QFile* file = static_cast<FileSource*>(clientData)->file;

    qint64 newPos;
    switch (whence) {
        case SEEK_SET:
            newPos = offset;
            break;
        case SEEK_CUR:
            newPos = file->pos() + offset;
            break;
        case SEEK_END:
            newPos = file->size() + offset;
            break;
        default:
            return ARCHIVE_FATAL;
    }

    if (!file->seek(newPos)) {
        return ARCHIVE_FATAL;
    }

    return newPos;
}

}  // namespace

// ============================================================================
// ArchiveReader
// ============================================================================

ArchiveReader::ArchiveReader() = default;

ArchiveReader::~ArchiveReader() = default;

Result ArchiveReader::verifyInput()
{
    // Only if the input itself is gzip: the filter that reads the input is the last but one, before the input
    const int filterCount = _archive ? archive_filter_count(_archive.get()) : 0;
    if ((filterCount < 2) || (archive_filter_code(_archive.get(), filterCount - 2) != ARCHIVE_FILTER_GZIP)) {
        return {};
    }

    bool sound = true;
    if (_file.isOpen() && _gzip) {
        // libarchive can stop before the end of its input, as at the end marker of a TAR archive. What is left of
        // the member is read for its checksum, but no more than padding takes: a member that goes on beyond that
        // is not checked, so that a small archive cannot hide a long time of inflating behind its end
        constexpr qint64 kMaxUnreadSize = qint64{16} * 1024 * 1024;
        const qint64 inflatedBefore = _gzip->inflatedSize();
        bool checked = true;
        while (checked && _gzip->inMember()) {
            const qint64 bytesRead = _file.read(_source.block.data(), _source.block.size());
            if (bytesRead <= 0) {
                break;
            }
            _gzip->feed(_source.block.constData(), bytesRead);
            checked = (_gzip->inflatedSize() - inflatedBefore) <= kMaxUnreadSize;
        }
        if (!checked) {
            qCDebug(LibArchiveLog) << "The gzip data after the end of the archive is not checked";
        }
        sound = !checked || _gzip->isSound();
    } else if (!_memoryData.isEmpty()) {
        GzipCheck check;
        check.feed(_memoryData.constData(), _memoryData.size());
        sound = check.isSound();
    }

    if (sound) {
        return {};
    }
    return failure(Error::InvalidArchive, QStringLiteral("The gzip data does not match its checksum"));
}

qint64 ArchiveReader::dataSize() const
{
    if (!_memoryData.isEmpty()) {
        return _memoryData.size();
    }
    return _file.isOpen() ? std::max<qint64>(0, _file.size()) : 0;
}

void ArchiveReader::captureFilter()
{
    // Filter 0 is the one the format reads from: the decompression, or "none" for the input itself
    const char* filter = archive_filter_name(_archive.get(), 0);
    _filterName = filter ? QString::fromUtf8(filter) : QStringLiteral("none");

    qCDebug(LibArchiveLog) << "Detected format:" << archive_format_name(_archive.get()) << "filter:" << _filterName;
}

Result ArchiveReader::_create(Input input, const QByteArray& signature)
{
    if (input == Input::Stream) {
        if (const QString reason = undecodableStreamReason(signature); !reason.isEmpty()) {
            return failure(Error::UnsupportedFormat, reason);
        }
        _streamStart = signature;
    }

    _archive.reset(archive_read_new());
    if (!_archive) {
        return failure(Error::InvalidArchive, QStringLiteral("Failed to create archive reader"));
    }

    configureReader(_archive.get(), input);
    return {};
}

Result ArchiveReader::_refuseNestedGzip()
{
    // Filter 0 is the one the format reads from, and the last one the input itself. GzipCheck sees the bytes of the
    // input, which only the last but one decodes: a gzip stream that another filter gives could be checked only by
    // decoding that filter a second time. Members one after another are one filter
    const int filterCount = archive_filter_count(_archive.get());
    for (int filter = 0; filter < (filterCount - 2); ++filter) {
        if (archive_filter_code(_archive.get(), filter) == ARCHIVE_FILTER_GZIP) {
            Result refused = failure(Error::UnsupportedFormat,
                                     QStringLiteral("A gzip stream inside another stream is not read: its checksum "
                                                    "cannot be checked"));
            refused.nestedGzip = true;
            return refused;
        }
    }
    return {};
}

Result ArchiveReader::open(const QString& path, Input input)
{
    // QFile handles Qt paths on every platform, and reads a Qt resource like a file
    _file.setFileName(path);
    if (!_file.open(QIODevice::ReadOnly)) {
        return failure(Error::InvalidArchive,
                       QStringLiteral("Failed to open file: %1 %2").arg(path, _file.errorString()));
    }

    const QByteArray signature = (input == Input::Stream) ? _file.peek(kStreamSignatureSize) : QByteArray();
    if (Result created = _create(input, signature); !created) {
        return created;
    }
    _gzip = std::make_unique<GzipCheck>();
    _source.file = &_file;
    _source.gzip = _gzip.get();
    _source.block = QByteArray(static_cast<qsizetype>(QGCFileHelper::optimalBufferSize(path)), Qt::Uninitialized);

    // The seek callback lets libarchive read a ZIP archive from its central directory. Must be set before
    // opening. A file that cannot seek, such as a pipe, gets neither it nor the skip callback: libarchive then
    // reads the input in order. No open or close callback: the file is already open, and the reader closes it
    const bool canSeek = !_file.isSequential();
    if (canSeek) {
        archive_read_set_seek_callback(_archive.get(), fileSeekCallback);
    }
    if (archive_read_open2(_archive.get(), &_source, nullptr, fileReadCallback, canSeek ? fileSkipCallback : nullptr,
                           nullptr) != ARCHIVE_OK) {
        Result opened =
            failure(Error::InvalidArchive,
                    QStringLiteral("Failed to open file: %1 %2").arg(path, libarchiveError(_archive.get())));
        opened.unrecognized = !_source.readFailed;
        return opened;
    }

    return _refuseNestedGzip();
}

Result ArchiveReader::open(const QByteArray& data, Input input)
{
    if (Result created = _create(input, data); !created) {
        return created;
    }
    // The implicitly shared copy keeps the bytes that libarchive reads alive as long as the reader
    _memoryData = data;

    if (archive_read_open_memory(_archive.get(), _memoryData.constData(), static_cast<size_t>(_memoryData.size())) !=
        ARCHIVE_OK) {
        Result opened = failure(Error::InvalidArchive,
                                QStringLiteral("Failed to open compressed data: ") + libarchiveError(_archive.get()));
        opened.unrecognized = true;
        return opened;
    }

    return _refuseNestedGzip();
}

}  // namespace LibArchive
