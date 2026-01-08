#pragma once

#include "QGCCompressionTypes.h"

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

/// Unified decompression interface for archives and single-file compression
/// Uses libarchive for all operations (ZIP, GZIP, XZ, ZSTD, TAR, etc.)
/// NOTE: This module is decompression-only; QGC does not create compressed files
namespace QGCCompression {

// ============================================================================
// Error Handling
// ============================================================================

/// Error codes for decompression operations
enum class Error {
    None,               ///< No error (success)
    FileNotFound,       ///< Input file does not exist
    PermissionDenied,   ///< Cannot read input or write output
    InvalidArchive,     ///< Archive is corrupt or invalid
    UnsupportedFormat,  ///< Format not recognized or not supported
    SizeLimitExceeded,  ///< Decompressed size exceeded limit
    Cancelled,          ///< Operation cancelled by progress callback
    FileNotInArchive,   ///< Requested file not found in archive
    IoError,            ///< General I/O error (read/write failure)
    InternalError       ///< Unexpected internal error
};

/// Get the error code from the last operation (thread-local)
Error lastError();

/// Get a human-readable error message from the last operation (thread-local)
QString lastErrorString();

/// Get a human-readable name for an error code
QString errorName(Error error);

// ============================================================================
// Format Definitions
// ============================================================================

/// Archive and compression format types (for decompression)
enum class Format {
    Auto,       ///< Auto-detect from file extension or magic bytes
    ZIP,        ///< ZIP archive (multiple files)
    SEVENZ,     ///< 7-Zip archive (.7z)
    GZIP,       ///< GZIP compression (single file, .gz)
    XZ,         ///< XZ/LZMA compression (single file, .xz)
    ZSTD,       ///< Zstandard compression (single file, .zst)
    BZIP2,      ///< BZip2 compression (single file, .bz2)
    LZ4,        ///< LZ4 compression (single file, .lz4)
    TAR,        ///< TAR archive (uncompressed, multiple files)
    TAR_GZ,     ///< TAR + GZIP (.tar.gz, .tgz)
    TAR_XZ,     ///< TAR + XZ (.tar.xz, .txz)
    TAR_ZSTD,   ///< TAR + Zstandard (.tar.zst)
    TAR_BZ2,    ///< TAR + BZip2 (.tar.bz2, .tbz2)
    TAR_LZ4,    ///< TAR + LZ4 (.tar.lz4)
};

// ============================================================================
// Format Detection
// ============================================================================

/// Detect format from file path (extension-based, with content fallback)
/// First tries extension-based detection; if that fails, reads file content
/// and uses magic bytes and Qt MIME detection as fallback
/// @param filePath Path to file
/// @param useContentFallback If true (default), read file content when extension fails
/// @return Detected format, or Format::Auto if detection failed
Format detectFormat(const QString &filePath, bool useContentFallback = true);

/// Detect format from file content only (ignores extension)
/// Reads file and uses magic bytes + Qt MIME detection
/// @param filePath Path to file
/// @return Detected format, or Format::Auto if detection failed
Format detectFormatFromFile(const QString &filePath);

/// Detect format from data (magic bytes + MIME detection)
/// @param data Raw file data (at least first 512 bytes recommended)
/// @return Detected format, or Format::Auto if detection failed
Format detectFormatFromData(const QByteArray &data);

/// Get file extension for a format
QString formatExtension(Format format);

/// Get human-readable name for a format
QString formatName(Format format);

/// Check if format is an archive (contains multiple files)
bool isArchiveFormat(Format format);

/// Check if format is a compression format (single stream)
bool isCompressionFormat(Format format);

/// Check if file path indicates a compressed file (.gz, .xz, .zst)
inline bool isCompressedFile(const QString &filePath) {
    return isCompressionFormat(detectFormat(filePath));
}

/// Check if file path indicates an archive file (.zip, .tar, .tar.gz, etc.)
inline bool isArchiveFile(const QString &filePath) {
    return isArchiveFormat(detectFormat(filePath));
}

/// Get path with compression extension stripped (.gz, .xz, .zst removed)
/// Returns original path if not a compressed file format
QString strippedPath(const QString &filePath);

/// Get the detected archive format name from the last operation (thread-local)
/// Returns names like "ZIP 2.0", "POSIX ustar", "RAW", etc.
/// Empty string if no format was detected
QString detectedFormatName();

/// Get the detected compression filter name from the last operation (thread-local)
/// Returns names like "gzip", "xz", "zstd", "bzip2", "none", etc.
/// Empty string if no filter was detected
QString detectedFilterName();

// ============================================================================
// URL Utilities
// ============================================================================

/// Convert a QUrl to a local file path
/// Handles file://, qrc:/, and plain paths from QML
/// @param url URL to convert (file://, qrc:/, or local path)
/// @return Local file path, or empty string if URL scheme is not supported
/// @note Remote URLs (http://, https://) return empty string - not supported
QString toLocalPath(const QUrl &url);

/// Check if a URL refers to a local file (file://, qrc:/, or plain path)
/// @param url URL to check
/// @return true if URL can be converted to a local path
bool isLocalUrl(const QUrl &url);

/// Detect format from URL (convenience overload)
/// @see detectFormat(const QString&, bool)
inline Format detectFormat(const QUrl &url, bool useContentFallback = true) {
    return detectFormat(toLocalPath(url), useContentFallback);
}

// ============================================================================
// Single-File Decompression
// ============================================================================

/// Decompress a single file
/// @param inputPath Path to compressed file (or Qt resource path :/)
/// @param outputPath Path for decompressed output (if empty, strips compression extension)
/// @param format Compression format (Auto = detect from inputPath)
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return true on success, false on failure or if size limit exceeded
bool decompressFile(const QString &inputPath, const QString &outputPath = QString(),
                    Format format = Format::Auto,
                    ProgressCallback progress = nullptr,
                    qint64 maxDecompressedBytes = 0);

/// Decompress file if it's a compressed format, otherwise return original path
/// @param filePath Input file (may or may not be compressed)
/// @param outputPath Output path (if empty, uses strippedPath)
/// @param removeOriginal Remove compressed file after successful decompression (default: false)
/// @return Path to result file (decompressed or original), empty string on failure
QString decompressIfNeeded(const QString &filePath,
                           const QString &outputPath = QString(),
                           bool removeOriginal = false);

/// Decompress data in memory
/// @param data Compressed input data
/// @param format Compression format (Auto = detect from magic bytes)
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return Decompressed data, or empty on failure or if size limit exceeded
QByteArray decompressData(const QByteArray &data, Format format = Format::Auto,
                          qint64 maxDecompressedBytes = 0);

/// Decompress a single file (QUrl convenience overload)
/// @see decompressFile(const QString&, const QString&, Format, ProgressCallback, qint64)
inline bool decompressFile(const QUrl &inputUrl, const QString &outputPath = QString(),
                           Format format = Format::Auto,
                           ProgressCallback progress = nullptr,
                           qint64 maxDecompressedBytes = 0) {
    return decompressFile(toLocalPath(inputUrl), outputPath, format, progress, maxDecompressedBytes);
}

// ============================================================================
// Archive Extraction (Multiple Files)
// ============================================================================

/// Extract an archive to a directory
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param outputDirectoryPath Output directory path
/// @param format Archive format (Auto = detect from archivePath)
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return true on success, false on failure or if size limit exceeded
bool extractArchive(const QString &archivePath, const QString &outputDirectoryPath,
                    Format format = Format::Auto,
                    ProgressCallback progress = nullptr,
                    qint64 maxDecompressedBytes = 0);

/// Extract an archive with per-entry filtering
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param outputDirectoryPath Output directory path
/// @param filter Callback to decide which entries to extract (return true to extract)
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return true on success (even if filter skipped all entries), false on failure
bool extractArchiveFiltered(const QString &archivePath, const QString &outputDirectoryPath,
                            EntryFilter filter,
                            ProgressCallback progress = nullptr,
                            qint64 maxDecompressedBytes = 0);

/// List contents of an archive without extracting
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param format Archive format (Auto = detect from archivePath)
/// @return List of file paths in archive, empty list on failure
QStringList listArchive(const QString &archivePath, Format format = Format::Auto);

/// List contents of an archive with detailed metadata
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param format Archive format (Auto = detect from archivePath)
/// @return List of ArchiveEntry structs, empty list on failure
QList<ArchiveEntry> listArchiveDetailed(const QString &archivePath, Format format = Format::Auto);

/// Get summary statistics for an archive without extracting
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param format Archive format (Auto = detect from archivePath)
/// @return ArchiveStats struct (all zeros on failure)
ArchiveStats getArchiveStats(const QString &archivePath, Format format = Format::Auto);

/// Validate an archive's integrity without extracting
/// @param archivePath Path to archive file
/// @param format Archive format (Auto = detect from archivePath)
/// @return true if archive is valid and all entries can be read
bool validateArchive(const QString &archivePath, Format format = Format::Auto);

/// Check if a file exists in an archive without extracting
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileName Name of file to check for (as shown by listArchive)
/// @param format Archive format (Auto = detect from archivePath)
/// @return true if file exists in archive
bool fileExists(const QString &archivePath, const QString &fileName,
                Format format = Format::Auto);

/// Extract a single file from an archive to disk
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileName Name of file inside archive (as shown by listArchive)
/// @param outputPath Output file path (if empty, extracts to current directory)
/// @param format Archive format (Auto = detect from archivePath)
/// @return true on success, false if file not found or extraction failed
bool extractFile(const QString &archivePath, const QString &fileName,
                 const QString &outputPath = QString(),
                 Format format = Format::Auto);

/// Extract a single file from an archive directly to memory
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileName Name of file inside archive (as shown by listArchive)
/// @param format Archive format (Auto = detect from archivePath)
/// @return File contents, or empty QByteArray if not found or extraction failed
QByteArray extractFileData(const QString &archivePath, const QString &fileName,
                           Format format = Format::Auto);

/// Extract multiple files from an archive in one pass
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileNames Names of files inside archive
/// @param outputDirectoryPath Output directory (files extracted with their archive names)
/// @param format Archive format (Auto = detect from archivePath)
/// @return true if all files found and extracted, false otherwise
bool extractFiles(const QString &archivePath, const QStringList &fileNames,
                  const QString &outputDirectoryPath,
                  Format format = Format::Auto);

/// Extract files matching glob patterns from an archive
/// Supports wildcards: * (any chars), ? (single char), [...] (char class)
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param patterns Glob patterns to match (e.g., "*.json", "subdir/*", "file.txt")
/// @param outputDirectoryPath Output directory (files extracted with their archive names)
/// @param extractedFiles Optional output list of files that were extracted
/// @param format Archive format (Auto = detect from archivePath)
/// @return true if at least one file matched and extracted, false on error or no matches
bool extractByPattern(const QString &archivePath, const QStringList &patterns,
                      const QString &outputDirectoryPath,
                      QStringList *extractedFiles = nullptr,
                      Format format = Format::Auto);

// --- QUrl convenience overloads for archive functions ---

/// Extract an archive (QUrl convenience overload)
/// @see extractArchive(const QString&, const QString&, Format, ProgressCallback, qint64)
inline bool extractArchive(const QUrl &archiveUrl, const QString &outputDirectoryPath,
                           Format format = Format::Auto,
                           ProgressCallback progress = nullptr,
                           qint64 maxDecompressedBytes = 0) {
    return extractArchive(toLocalPath(archiveUrl), outputDirectoryPath, format, progress, maxDecompressedBytes);
}

/// List archive contents (QUrl convenience overload)
/// @see listArchive(const QString&, Format)
inline QStringList listArchive(const QUrl &archiveUrl, Format format = Format::Auto) {
    return listArchive(toLocalPath(archiveUrl), format);
}

/// List archive contents with details (QUrl convenience overload)
/// @see listArchiveDetailed(const QString&, Format)
inline QList<ArchiveEntry> listArchiveDetailed(const QUrl &archiveUrl, Format format = Format::Auto) {
    return listArchiveDetailed(toLocalPath(archiveUrl), format);
}

/// Extract single file from archive (QUrl convenience overload)
/// @see extractFile(const QString&, const QString&, const QString&, Format)
inline bool extractFile(const QUrl &archiveUrl, const QString &fileName,
                        const QString &outputPath = QString(),
                        Format format = Format::Auto) {
    return extractFile(toLocalPath(archiveUrl), fileName, outputPath, format);
}

/// Extract single file to memory (QUrl convenience overload)
/// @see extractFileData(const QString&, const QString&, Format)
inline QByteArray extractFileData(const QUrl &archiveUrl, const QString &fileName,
                                  Format format = Format::Auto) {
    return extractFileData(toLocalPath(archiveUrl), fileName, format);
}

/// Check if file exists in archive (QUrl convenience overload)
/// @see fileExists(const QString&, const QString&, Format)
inline bool fileExists(const QUrl &archiveUrl, const QString &fileName,
                       Format format = Format::Auto) {
    return fileExists(toLocalPath(archiveUrl), fileName, format);
}

/// Validate archive integrity (QUrl convenience overload)
/// @see validateArchive(const QString&, Format)
inline bool validateArchive(const QUrl &archiveUrl, Format format = Format::Auto) {
    return validateArchive(toLocalPath(archiveUrl), format);
}

// ============================================================================
// QIODevice-based Operations (streaming)
// ============================================================================

/// Decompress from QIODevice to file
/// Enables streaming decompression from QNetworkReply, QBuffer, etc.
/// @param device QIODevice to read from (must be open and readable)
/// @param outputPath Output file path
/// @param progress Optional progress callback
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return true on success, false on failure or if size limit exceeded
bool decompressFromDevice(QIODevice *device, const QString &outputPath,
                          ProgressCallback progress = nullptr,
                          qint64 maxDecompressedBytes = 0);

/// Decompress from QIODevice to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return Decompressed data, or empty QByteArray on failure or if size limit exceeded
QByteArray decompressFromDevice(QIODevice *device, qint64 maxDecompressedBytes = 0);

/// Extract archive from QIODevice to directory
/// @param device QIODevice to read from (must be open and readable)
/// @param outputDirectoryPath Output directory path
/// @param progress Optional progress callback
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return true on success, false on failure or if size limit exceeded
bool extractFromDevice(QIODevice *device, const QString &outputDirectoryPath,
                       ProgressCallback progress = nullptr,
                       qint64 maxDecompressedBytes = 0);

/// Extract a single file from archive device to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param fileName Name of file inside archive
/// @return File contents, or empty QByteArray if not found
QByteArray extractFileDataFromDevice(QIODevice *device, const QString &fileName);

} // namespace QGCCompression
