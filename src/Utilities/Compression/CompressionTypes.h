#pragma once

/// @file CompressionTypes.h
/// @brief Types shared by Compression.h and CompressionJob

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace Compression {
Q_NAMESPACE

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
    UnsupportedFormat,  ///< Input in a format that the build does not enable: a single stream of legacy lzma,
                        ///< Zstandard, bzip2 or LZ4, or an archive whose name or signature shows that it is 7z or
                        ///< TAR, or a TAR compressed with Zstandard, bzip2 or LZ4
    SizeLimitExceeded,  ///< Decompressed size exceeds the limit
    Cancelled,          ///< Operation cancelled by progress callback
    IoError,            ///< Output cannot be written, or is the input file
    InternalError,      ///< CompressionJob: the operation threw an exception, such as one from an entry filter
    DiskSpaceExceeded   ///< Not enough free space on the disk of the output
};
Q_ENUM_NS(Error)

/// Result of an operation. Converts to true on success
struct [[nodiscard]] Outcome
{
    Error error = Error::None;
    /// What failed and why, translated for the user. Empty on success
    QString message = {};
    /// What libarchive or the system reported about an input that cannot be read or an output that cannot be
    /// written, in English: "Truncated input file", "No space left on device". Empty if the message says it all
    QString detail = {};
    /// Compression filter libarchive detected ("gzip", "xz", "zstd", ...), or "none" for input that is not
    /// compressed. Empty if the operation ended before it read the first header of its input
    QString filterName = {};

    explicit operator bool() const { return error == Error::None; }

    /// The message, followed by the detail in parentheses if there is one: what to show or log for a failure
    QString description() const { return detail.isEmpty() ? message : QStringLiteral("%1 (%2)").arg(message, detail); }
};

// ============================================================================
// Default Limit
// ============================================================================

/// Default limit for a file decompressed to another file (1 GiB)
constexpr qint64 kDefaultMaxDecompressedFileBytes = 1024LL * 1024 * 1024;

// ============================================================================
// Progress Callback
// ============================================================================

/// Progress callback for long-running operations
/// Called after each block of data written and, during an extraction, at each entry, so that an operation can be
/// cancelled even while it writes no data.
/// @param bytesProcessed Bytes of the input (archive or compressed file) read so far
/// @param totalBytes Size of the input (0 if unknown)
/// @return true to continue, false to cancel. The operation then fails with Error::Cancelled
using ProgressCallback = std::function<bool(qint64 bytesProcessed, qint64 totalBytes)>;

// ============================================================================
// Archive Entries
// ============================================================================

/// Archive entry as an EntryFilter sees it
struct ArchiveEntry
{
    QString name;  ///< Path within the archive
    bool isDirectory = false;
};

/// Entry filter callback for selective extraction
/// Called for each entry before it is extracted, directories included. An accepted directory is created. A
/// refused directory is created only if an accepted entry inside it needs it
/// @return true to extract this entry, false to skip it
using EntryFilter = std::function<bool(const ArchiveEntry& entry)>;

// ============================================================================
// Extraction Options
// ============================================================================

/// Options of Compression::extractArchive(), all optional
/// @code
/// Compression::extractArchive(archivePath, outputDirectoryPath, {.maxBytes = limit, .filter = filter});
/// @endcode
struct ExtractOptions
{
    /// Largest total size of the files written (0 = unlimited). extractArchive() explains how it is counted
    qint64 maxBytes = 0;
    /// Which entries to extract (not set = all of them). The size limit counts only the entries it accepts. An
    /// archive whose entries it all refuses extracts successfully
    EntryFilter filter = {};
    /// Called as ProgressCallback documents. Returning false cancels the extraction
    ProgressCallback progress = {};
};

}  // namespace Compression
