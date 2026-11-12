#pragma once

/// @file QGCCompressionTypes.h
/// @brief Shared type definitions for compression/archive operations

#include <QtCore/QDateTime>
#include <QtCore/QString>

#include <functional>

namespace QGCCompression {

// ============================================================================
// Outcome of an Operation
// ============================================================================

/// Error codes for decompression operations
enum class Error
{
    None,               ///< No error (success)
    FileNotFound,       ///< Input file does not exist
    PermissionDenied,   ///< Cannot read input or write output
    InvalidArchive,     ///< Archive or compressed stream cannot be read or is damaged
    UnsupportedFormat,  ///< Format not recognized or not supported, or an archive entry that is not extracted
    SizeLimitExceeded,  ///< Decompressed size exceeded limit
    Cancelled,          ///< Operation cancelled by progress callback
    FileNotInArchive,   ///< Requested file not found in archive
    IoError,            ///< Output cannot be written, or a device to read from is not open
    InternalError       ///< Unexpected internal error
};

/// How an operation ended, which is what it returns. It converts to true if the operation succeeded.
/// What the operation read comes back through a reference parameter. That is empty after a failure, and also
/// after the success of reading a compressed file or an entry without data, or an archive without entries.
struct [[nodiscard]] Outcome
{
    Error error = Error::None;  ///< Why the operation failed
    QString message = {};       ///< What could not be done and why, empty on success
    /// Format libarchive detected, like "ZIP 2.0 (deflation)", "POSIX ustar" or "RAW". Empty, as filterName is,
    /// if the operation ended before it had read the first header of its input
    QString formatName = {};
    QString filterName = {};  ///< Compression it detected, like "gzip", "xz" or "zstd", or "none"

    explicit operator bool() const { return error == Error::None; }
};

// ============================================================================
// Progress Callback
// ============================================================================

/// Progress callback for long-running operations
/// @param bytesProcessed Bytes of the archive or compressed file read so far
/// @param totalBytes Size of the archive or compressed file (0 if unknown)
/// @return true to continue, false to cancel operation, which then fails with Error::Cancelled
using ProgressCallback = std::function<bool(qint64 bytesProcessed, qint64 totalBytes)>;

// ============================================================================
// Archive Entry Metadata
// ============================================================================

/// Default Unix permissions for extracted files (rw-r--r--)
constexpr quint32 kDefaultFilePermissions = 0644;

/// Metadata for a single entry in an archive
struct ArchiveEntry {
    QString name;                    ///< Path/name within archive
    qint64 size = 0;                 ///< Uncompressed size in bytes
    QDateTime modified;              ///< Last modification time
    bool isDirectory = false;        ///< True if entry is a directory
    quint32 permissions = kDefaultFilePermissions;  ///< Unix-style permissions
};

/// Summary statistics for an archive
struct ArchiveStats {
    int totalEntries = 0;            ///< Total number of entries (files + directories)
    int fileCount = 0;               ///< Number of files
    int directoryCount = 0;          ///< Number of directories
    qint64 totalUncompressedSize = 0;///< Sum of all file sizes (uncompressed)
    qint64 largestFileSize = 0;      ///< Size of largest file
    QString largestFileName;         ///< Name of largest file
};

/// Count an entry in the statistics of its archive
inline void addTo(ArchiveStats& stats, const ArchiveEntry& entry)
{
    stats.totalEntries++;
    if (entry.isDirectory) {
        stats.directoryCount++;
        return;
    }
    stats.fileCount++;
    stats.totalUncompressedSize += entry.size;
    if (entry.size > stats.largestFileSize) {
        stats.largestFileSize = entry.size;
        stats.largestFileName = entry.name;
    }
}

/// Entry filter callback for selective extraction
/// Called for each entry before extraction; return true to extract, false to skip
/// @param entry Metadata for the archive entry
/// @return true to extract this entry, false to skip it
using EntryFilter = std::function<bool(const ArchiveEntry &entry)>;

} // namespace QGCCompression
