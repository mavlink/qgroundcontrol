#pragma once

/// @file QGClibarchive.h
/// @brief Private implementation details for QGCCompression
/// @note This is an internal header - use QGCCompression.h for public API

#include "QGCCompressionTypes.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <archive.h>

Q_DECLARE_LOGGING_CATEGORY(QGClibarchiveLog)

namespace QGClibarchive {

// ============================================================================
// Types (re-export from QGCCompression for internal use)
// ============================================================================

using ProgressCallback = QGCCompression::ProgressCallback;
using ArchiveEntry = QGCCompression::ArchiveEntry;
using ArchiveStats = QGCCompression::ArchiveStats;
using EntryFilter = QGCCompression::EntryFilter;

using QGCCompression::kDefaultFilePermissions;

// ============================================================================
// Archive Reader Mode
// ============================================================================

/// Mode for ArchiveReader format support
enum class ReaderMode {
    AllFormats,  ///< Support all archive formats (ZIP, TAR, 7z, etc.)
    RawFormat    ///< Raw format for single-file decompression (.gz, .xz, etc.)
};

// ============================================================================
// RAII Archive Reader
// ============================================================================

/// RAII wrapper for libarchive reader with automatic cleanup
class ArchiveReader {
public:
    ArchiveReader() = default;
    ~ArchiveReader();

    ArchiveReader(const ArchiveReader&) = delete;
    ArchiveReader& operator=(const ArchiveReader&) = delete;

    /// Open archive for reading (supports Qt resources and regular files)
    /// @param path File path or Qt resource path (:/...)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @return true on success
    bool open(const QString &path, ReaderMode mode = ReaderMode::AllFormats);

    /// Open archive for reading from a QIODevice (streaming)
    /// @param device QIODevice to read from (must be open and readable)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @return true on success
    /// @note The device must remain valid and open until the reader is closed
    bool open(QIODevice *device, ReaderMode mode = ReaderMode::AllFormats);

    /// Get the underlying archive handle
    struct archive* handle() const { return _archive; }

    /// Release ownership of archive handle (caller must free)
    struct archive* release() { auto *a = _archive; _archive = nullptr; return a; }

    /// Check if archive is open
    bool isOpen() const { return _archive != nullptr; }

    /// Get the total data size (for progress reporting)
    /// @return Size in bytes: resource data size, file size, or device size (0 if unknown)
    qint64 dataSize() const;

    /// Get the detected archive format name (after reading first header)
    /// @return Format name like "ZIP 2.0 (deflation)", "POSIX ustar", "RAW", or empty if not detected
    QString formatName() const;

    /// Get the detected compression filter name
    /// @return Filter name like "gzip", "xz", "zstd", "none", or empty if not detected
    QString filterName() const;

private:
    struct archive *_archive = nullptr;
    QString _filePath;  // Stored path for size queries
    QByteArray _resourceData;  // Holds data for Qt resources
    QIODevice *_device = nullptr;  // Device for streaming reads
};

// ============================================================================
// QIODevice Streaming Callbacks
// ============================================================================

/// Read callback for QIODevice streaming
/// @param clientData QIODevice pointer
/// @param buffer Output pointer to buffer
/// @return Bytes read, or ARCHIVE_FATAL on error
la_ssize_t deviceReadCallback(struct archive *a, void *clientData, const void **buffer);

/// Skip callback for QIODevice streaming
/// @param clientData QIODevice pointer
/// @param request Bytes to skip
/// @return Bytes actually skipped
la_int64_t deviceSkipCallback(struct archive *a, void *clientData, la_int64_t request);

/// Close callback for QIODevice streaming (does not close device)
/// @param clientData QIODevice pointer
/// @return ARCHIVE_OK
int deviceCloseCallback(struct archive *a, void *clientData);

/// Seek callback for random-access QIODevice (improves ZIP performance)
/// @param clientData QIODevice pointer
/// @param offset Seek offset
/// @param whence SEEK_SET, SEEK_CUR, or SEEK_END
/// @return New position, or ARCHIVE_FATAL on error
la_int64_t deviceSeekCallback(struct archive *a, void *clientData, la_int64_t offset, int whence);

// ============================================================================
// Utility Functions
// ============================================================================

/// Convert libarchive entry to ArchiveEntry struct
/// @param entry libarchive entry pointer (from archive_read_next_header)
/// @return Populated ArchiveEntry with name, size, modified time, isDirectory, permissions
ArchiveEntry toArchiveEntry(struct archive_entry *entry);

// ============================================================================
// Low-level Operations
// ============================================================================

/// Open an archive for reading, handling both Qt resources and regular files
/// Qt resources are loaded into resourceData; regular files are streamed
/// @param a Initialized archive reader (call archive_read_support_* first)
/// @param filePath Path to file (Qt resource :/ or filesystem path)
/// @param resourceData Output buffer for Qt resource data (must stay alive until archive is closed)
/// @return true on success
bool openArchiveForReading(struct archive *a, const QString &filePath, QByteArray &resourceData);

/// Extract any supported archive format using libarchive's auto-detection
/// Supports: ZIP, 7z, TAR (all variants), cpio, ar, xar, etc.
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param outputDirectoryPath Path where files will be extracted
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @return true on success, false on failure or size limit exceeded
bool extractAnyArchive(const QString &archivePath, const QString &outputDirectoryPath,
                       ProgressCallback progress = nullptr,
                       qint64 maxBytes = 0);

/// Extract archive atomically using a temporary staging directory
/// Provides all-or-nothing semantics: if extraction fails, no partial files remain.
/// Uses QTemporaryDir to stage files, then moves them to the final destination.
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param outputDirectoryPath Path where files will be extracted
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @return true on success, false on failure (no partial files left on failure)
/// @note Requires ~2x disk space during extraction for staging
/// @note Cross-filesystem moves may be slower (requires copy + delete)
bool extractArchiveAtomic(const QString &archivePath, const QString &outputDirectoryPath,
                          ProgressCallback progress = nullptr,
                          qint64 maxBytes = 0);

/// Extract a single file from any archive by name
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of file inside archive
/// @param outputPath Output file path
/// @return true on success, false if not found or extraction failed
bool extractSingleFile(const QString &archivePath, const QString &fileName,
                       const QString &outputPath);

/// Extract a single file from any archive directly to memory
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of file inside archive
/// @return File contents, or empty QByteArray if not found or extraction failed
QByteArray extractFileToMemory(const QString &archivePath, const QString &fileName);

/// Extract multiple files from any archive by name in one pass
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileNames Names of files inside archive
/// @param outputDirectoryPath Output directory
/// @return true if all files found and extracted
bool extractMultipleFiles(const QString &archivePath, const QStringList &fileNames,
                          const QString &outputDirectoryPath);

/// Extract files matching glob patterns from archive (uses libarchive archive_match)
/// Supports wildcards: * (any chars), ? (single char), [...] (char class)
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param patterns Glob patterns to match (e.g., "*.json", "dir/*", "file.txt")
/// @param outputDirectoryPath Output directory
/// @param extractedFiles Optional output list of files that were extracted
/// @return true if at least one file matched and extracted, false on error or no matches
bool extractByPattern(const QString &archivePath, const QStringList &patterns,
                      const QString &outputDirectoryPath,
                      QStringList *extractedFiles = nullptr);

/// Validate archive integrity by reading all entries
/// @param archivePath Path to the archive file
/// @return true if archive is valid
bool validateArchive(const QString &archivePath);

/// Check if a file exists in an archive without extracting
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of file to check for
/// @return true if file exists in archive
bool fileExistsInArchive(const QString &archivePath, const QString &fileName);

/// List all entries in an archive
/// @param archivePath Path to the archive file (or Qt resource path)
/// @return List of entry names, empty on failure
QStringList listArchiveEntries(const QString &archivePath);

/// List all entries in an archive with detailed metadata
/// @param archivePath Path to the archive file (or Qt resource path)
/// @return List of ArchiveEntry structs, empty on failure
QList<ArchiveEntry> listArchiveEntriesDetailed(const QString &archivePath);

/// Get summary statistics for an archive
/// @param archivePath Path to the archive file (or Qt resource path)
/// @return ArchiveStats struct (all zeros on failure)
ArchiveStats getArchiveStats(const QString &archivePath);

/// Extract archive with per-entry filtering
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param outputDirectoryPath Path where files will be extracted
/// @param filter Callback to decide which entries to extract
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @return true on success (even if all entries filtered out), false on failure
bool extractWithFilter(const QString &archivePath, const QString &outputDirectoryPath,
                       EntryFilter filter,
                       ProgressCallback progress = nullptr,
                       qint64 maxBytes = 0);

// ============================================================================
// Single-File Decompression
// ============================================================================

/// Decompress a single compressed file (.gz, .xz, .zst, etc.) to disk
/// @param inputPath Path to compressed file (or Qt resource path)
/// @param outputPath Path for decompressed output
/// @param progress Optional progress callback
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @return true on success, false on failure or size limit exceeded
bool decompressSingleFile(const QString &inputPath, const QString &outputPath,
                          ProgressCallback progress = nullptr,
                          qint64 maxBytes = 0);

/// Decompress data in memory
/// @param data Compressed input data
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @return Decompressed data, or empty on failure or size limit exceeded
QByteArray decompressDataFromMemory(const QByteArray &data, qint64 maxBytes = 0);

// ============================================================================
// QIODevice-based Operations (streaming)
// ============================================================================

/// Extract archive from QIODevice to directory
/// @param device QIODevice to read from (must be open and readable)
/// @param outputDirectoryPath Path where files will be extracted
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @return true on success, false on failure or size limit exceeded
bool extractFromDevice(QIODevice *device, const QString &outputDirectoryPath,
                       ProgressCallback progress = nullptr,
                       qint64 maxBytes = 0);

/// Extract a single file from archive device directly to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param fileName Name of file inside archive
/// @return File contents, or empty QByteArray if not found
QByteArray extractFileDataFromDevice(QIODevice *device, const QString &fileName);

/// Decompress single-file format from QIODevice to disk
/// @param device QIODevice to read from (must be open and readable)
/// @param outputPath Output file path
/// @param progress Optional progress callback
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @return true on success, false on failure or size limit exceeded
bool decompressFromDevice(QIODevice *device, const QString &outputPath,
                          ProgressCallback progress = nullptr,
                          qint64 maxBytes = 0);

/// Decompress single-file format from QIODevice to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @return Decompressed data, or empty QByteArray on failure or size limit exceeded
QByteArray decompressDataFromDevice(QIODevice *device, qint64 maxBytes = 0);

// ============================================================================
// Format Detection (Thread-Local State)
// ============================================================================

/// Get the detected format name from the last archive operation (thread-local)
/// @return Format name like "ZIP 2.0 (deflation)", "POSIX ustar", "RAW", or empty
QString lastDetectedFormatName();

/// Get the detected filter name from the last archive operation (thread-local)
/// @return Filter name like "gzip", "xz", "zstd", "none", or empty
QString lastDetectedFilterName();

} // namespace QGClibarchive
