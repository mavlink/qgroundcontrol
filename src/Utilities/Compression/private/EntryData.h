#pragma once

/// @file EntryData.h
/// @brief Copies the data of an entry from a reader to memory or to a file, within the limits of an operation
/// @note Private to QGCCompression

#include <functional>

#include <QtCore/QDateTime>
#include <QtCore/QString>

#include "ArchiveReader.h"

namespace LibArchive {

/// Size limit and progress reporting of one operation, shared by all the entries whose data it copies
struct DataTransfer
{
    qint64 maxBytes = 0;  ///< Bytes the output of the operation may take (0 = unlimited)
    /// Whether maxBytes is the room on the disk rather than the limit of the caller
    bool limitedByDisk = false;
    ProgressCallback progress = {};  ///< Optional
    qint64 totalSize = 0;            ///< Size of the input for the progress callback (0 if unknown)
    /// Start of the detail for data that is truncated or corrupt
    const char* damageReport = "Read error:";
    qint64 bytesWritten = 0;  ///< Bytes the output takes so far

    /// Whether @a bytes more of output stay within the limit
    /// @return Success, SizeLimitExceeded or DiskSpaceExceeded
    Result fits(qint64 bytes) const;

    /// Add @a bytes to bytesWritten, unless they exceed the limit
    /// @return As fits()
    Result count(qint64 bytes);
};

/// Receives the next block of the data of an entry
/// @return Success, or why it cannot take the block
using DataSink = std::function<Result(const char* data, qint64 size)>;

/// Copy the data of the current entry to a sink
/// This is the only loop over entry data: it enforces the size limit, reports progress, lets the caller cancel,
/// and succeeds only on data that ends cleanly. The blocks must follow one another, as they do in any entry but a
/// sparse one, which an extraction skips.
/// @param reader Archive reader (must have entry header already read)
/// @param transfer Limit and progress of the operation. bytesWritten is updated
/// @param sink Called with each block, in order
/// @return Success, what the sink returned, SizeLimitExceeded, DiskSpaceExceeded or Cancelled, or
///         InvalidArchive for data that is truncated, corrupt or out of order, whose detail starts with
///         transfer.damageReport
Result copyEntryData(ArchiveReader& reader, DataTransfer& transfer, const DataSink& sink);

/// Copy the data of a single stream, the one entry of its reader, to a sink, then check what libarchive does not
/// As copyEntryData(). The data that a filter decompressed must not start with a stream that this build cannot
/// decode, which libarchive passes on as data (UnsupportedFormat, as ArchiveReader::open() refuses such input), and
/// the input must match its checksums (ArchiveReader::verifyInput())
/// @return As copyEntryData(), UnsupportedFormat, or InvalidArchive for input that does not match its checksums
Result copyStreamData(ArchiveReader& reader, DataTransfer& transfer, const DataSink& sink);

/// Failure for a file or a directory that could not be created at @a path: PermissionDenied if writing there is not
/// permitted, IoError otherwise
Result writeFailure(const QString& path, const QString& detail);

/// Write the data of the current entry to a file
/// Every file an operation writes is written here. On failure no partial file is left.
/// @param reader Archive reader (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @param transfer Limit and progress of the operation the entry is part of
/// @param modified Modification time of the file (invalid = the time it is written)
/// @param wholeStream Whether the entry is all of a single stream (decompressFile()), whose data is then copied and
///        checked by copyStreamData(). Such a file is written atomically with QSaveFile: nothing appears at
///        @a outputPath until it is complete, and a file that was there is unchanged if it fails. An entry of an
///        archive is written at its path, without a temporary file and its sync to disk: a failed extraction removes
///        all it wrote anyway
/// @return As copyEntryData(), or copyStreamData() for a whole stream
Result copyEntryToFile(ArchiveReader& reader, const QString& outputPath, DataTransfer& transfer,
                       const QDateTime& modified = {}, bool wholeStream = false);

/// Limit and progress of an operation that writes into a directory: the limit of the caller, or the room on the
/// disk if that is smaller
/// @param maxBytes Limit of the caller (0 = unlimited)
DataTransfer transferInto(const QString& directoryPath, qint64 maxBytes, const ProgressCallback& progress,
                          qint64 totalSize, const char* damageReport);

}  // namespace LibArchive
