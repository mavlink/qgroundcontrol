#include "EntryData.h"

#include <algorithm>
#include <cerrno>
#include <limits>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

#include "Formats.h"
#include "QGCFileHelper.h"

namespace LibArchive {

namespace {

/// Whether creating a file or a directory at @a path failed for lack of permission
/// Called after the creation failed, to tell this reason from the others: the file exists and is not writable,
/// or the nearest existing directory above it is not writable.
bool isWriteDenied(const QString& path)
{
    QFileInfo existing(path);
    if (existing.exists()) {
        return !existing.isWritable();
    }
    do {
        const QString parentPath = existing.absolutePath();
        if (parentPath == existing.absoluteFilePath()) {
            return false;
        }
        existing.setFile(parentPath);
    } while (!existing.exists());
    return existing.isDir() && !existing.isWritable();
}

/// What the system reported about an output file that could not be written
/// @param systemError errno after a write, which was 0 before it (0 = not known)
QString outputError(const QFileDevice& outFile, const char* what, const QString& outputPath, int systemError = 0)
{
    // Qt has no reason for a write that stores a part of the data, as one does when the disk fills up: it reports no
    // error, or one with the text of a device that has none. The system call that Qt then made for the rest of the
    // data has left the reason in errno
    QString reason = outFile.errorString();
    if ((outFile.error() == QFileDevice::NoError) || (reason == QFile().errorString())) {
        reason =
            (systemError != 0) ? qt_error_string(systemError) : QStringLiteral("not all of the data could be written");
    }
    return QStringLiteral("%1: %2 %3").arg(QLatin1String(what), outputPath, reason);
}

/// Bytes that may be written into a directory before its disk counts as full: the free space, less enough to
/// keep a tenth of what is written free
/// @return 0 if the free space is unknown, which sets no limit
qint64 roomIn(const QString& directoryPath)
{
    const qint64 available = QGCFileHelper::availableDiskSpace(directoryPath);
    if (available < 0) {
        return 0;
    }
    return std::max<qint64>(1, available / 11 * 10);
}

/// Refuses a stream that this build cannot decode inside a stream that it decoded, such as .lzma inside gzip, and
/// one whose header is truncated or damaged
/// The reader checks only the start of its input. libarchive passes an inner stream that no registered filter
/// recognizes on as data, so the start of the decompressed data gets the same checks.
class DecompressedStreamCheck
{
public:
    explicit DecompressedStreamCheck(const ArchiveReader& reader)
        : _pending(archive_filter_count(reader.handle()) > 1)
    {}

    /// Take the next block of the decompressed data. The check is made once there is enough of it
    Result take(const char* data, qint64 size)
    {
        if (!_pending) {
            return {};
        }
        _start.append(data, static_cast<qsizetype>(std::min<qint64>(size, kStreamSignatureSize - _start.size())));
        return (_start.size() < kStreamSignatureSize) ? Result{} : finish();
    }

    /// Check the data taken so far, if not yet done: at the end of data shorter than the check needs
    Result finish()
    {
        if (!_pending) {
            return {};
        }
        _pending = false;
        const QString reason = undecodableStreamReason(_start);
        const QString damagedFormat = streamSignatureFormat(_start);
        _start.clear();
        if (!reason.isEmpty()) {
            return failure(Error::UnsupportedFormat, reason);
        }
        return damagedFormat.isEmpty()
                   ? Result{}
                   : failure(Error::InvalidArchive, damagedStreamHeader(damagedFormat, "decompressed data"));
    }

private:
    bool _pending;
    QByteArray _start;
};

}  // namespace

Result DataTransfer::fits(qint64 bytes) const
{
    if ((maxBytes <= 0) || (bytes <= (maxBytes - bytesWritten))) {
        return {};
    }
    qCDebug(LibArchiveLog) << "The output has room for" << maxBytes << "bytes";
    return failure(limitedByDisk ? Error::DiskSpaceExceeded : Error::SizeLimitExceeded);
}

Result DataTransfer::count(qint64 bytes)
{
    Result within = fits(bytes);
    if (within) {
        // Saturate instead of overflowing. Only possible without a limit, where the count is only logged
        bytesWritten += std::min(bytes, std::numeric_limits<qint64>::max() - bytesWritten);
    }
    return within;
}

Result copyEntryData(ArchiveReader& reader, DataTransfer& transfer, const DataSink& sink)
{
    struct archive* const a = reader.handle();
    qint64 copied = 0;
    const void* buff = nullptr;
    size_t size = 0;
    la_int64_t offset = 0;
    int r;

    while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
        if (offset != copied) {
            return failure(Error::InvalidArchive,
                           QLatin1String(transfer.damageReport) +
                               QStringLiteral(" a block of data that does not follow the one before it"));
        }

        const qint64 blockSize = static_cast<qint64>(size);
        if (Result counted = transfer.count(blockSize); !counted) {
            return counted;
        }
        copied += blockSize;

        if (Result taken = sink(static_cast<const char*>(buff), blockSize); !taken) {
            return taken;
        }

        // Progress is the position in the input, because only the size of the input is known in advance
        if (transfer.progress && !transfer.progress(archive_filter_bytes(a, -1), transfer.totalSize)) {
            return failure(Error::Cancelled);
        }
    }

    if (r == ARCHIVE_EOF) {
        return {};
    }
    return failure(Error::InvalidArchive, QLatin1String(transfer.damageReport) + QLatin1Char(' ') + libarchiveError(a));
}

Result copyStreamData(ArchiveReader& reader, DataTransfer& transfer, const DataSink& sink)
{
    DecompressedStreamCheck check(reader);
    Result copied = copyEntryData(reader, transfer, [&](const char* data, qint64 size) {
        Result taken = check.take(data, size);
        return taken ? sink(data, size) : taken;
    });
    if (copied) {
        copied = check.finish();
    }
    if (copied) {
        copied = reader.verifyInput();
    }
    return copied;
}

QString damagedStreamHeader(const QString& format, const char* what)
{
    return QStringLiteral("The %1 starts with the signature of a %2 stream, but its header is truncated or damaged")
        .arg(QLatin1String(what), format);
}

Result writeFailure(const QString& path, const QString& detail)
{
    return failure(isWriteDenied(path) ? Error::PermissionDenied : Error::IoError, detail);
}

namespace {

/// Write the data of the current entry into an open file, and set its modification time
/// @return As copyEntryToFile()
Result writeEntryData(QFileDevice& outFile, ArchiveReader& reader, DataTransfer& transfer, const QDateTime& modified,
                      bool wholeStream, const QString& outputPath)
{
    const DataSink write = [&](const char* data, qint64 size) {
        errno = 0;
        if (outFile.write(data, size) != size) {
            return failure(Error::IoError, outputError(outFile, "Failed to write output", outputPath, errno));
        }
        return Result{};
    };
    Result copied = wholeStream ? copyStreamData(reader, transfer, write) : copyEntryData(reader, transfer, write);
    if (!copied) {
        return copied;
    }

    errno = 0;
    if (!outFile.flush()) {
        return failure(Error::IoError, outputError(outFile, "Failed to write output", outputPath, errno));
    }
    // After the last write, which would update the time again. Failing to set the time is not an error
    if (modified.isValid()) {
        (void) outFile.setFileTime(modified, QFileDevice::FileModificationTime);
    }
    return {};
}

}  // namespace

Result copyEntryToFile(ArchiveReader& reader, const QString& outputPath, DataTransfer& transfer,
                       const QDateTime& modified, bool wholeStream)
{
    if (wholeStream) {
        QSaveFile outFile(outputPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            return writeFailure(outputPath, outputError(outFile, "Failed to open output file", outputPath));
        }
        if (Result written = writeEntryData(outFile, reader, transfer, modified, true, outputPath); !written) {
            outFile.cancelWriting();
            return written;
        }
        if (!outFile.commit()) {
            return failure(Error::IoError, outputError(outFile, "Failed to commit output file", outputPath));
        }
        return {};
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        return writeFailure(outputPath, outputError(outFile, "Failed to open output file", outputPath));
    }
    const Result written = writeEntryData(outFile, reader, transfer, modified, false, outputPath);
    if (!written) {
        outFile.close();
        (void) outFile.remove();
    }
    return written;
}

DataTransfer transferInto(const QString& directoryPath, qint64 maxBytes, const ProgressCallback& progress,
                          qint64 totalSize, const char* damageReport)
{
    const qint64 room = roomIn(directoryPath);
    const bool limitedByDisk = (room > 0) && ((maxBytes <= 0) || (room < maxBytes));
    return {.maxBytes = limitedByDisk ? room : maxBytes,
            .limitedByDisk = limitedByDisk,
            .progress = progress,
            .totalSize = totalSize,
            .damageReport = damageReport};
}

}  // namespace LibArchive
