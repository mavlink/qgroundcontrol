#pragma once

#include "QGCCompressionTypes.h"

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QIODevice>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QString>
#include <QtCore/QStringList>

/// Unified decompression interface for archives and single-file compression
/// Uses libarchive for all operations (ZIP, GZIP, XZ, ZSTD, TAR, etc.)
/// NOTE: This module is decompression-only; QGC does not create compressed files
namespace QGCCompression {

// ============================================================================
// Error Handling
// ============================================================================

// An operation returns its Outcome (QGCCompressionTypes.h): the error code, the message, and the format and
// the filter that were detected. readFile(), computeFileHash() and parseCompressedJson() report in their own way.

/// Get a human-readable name for an error code
QString errorName(Error error);

// ============================================================================
// Format Definitions
// ============================================================================

/// Archive and compression format types (for decompression)
///
/// A function that takes a format uses it for the kind of its input: an archive (isArchiveFormat()) or a
/// single compressed stream (isCompressionFormat()). It refuses the kind it does not handle with
/// Error::UnsupportedFormat. With Format::Auto the kind is detected, from the file name and failing that from
/// the content (detectFormat()), or from the content alone for data in memory, and an input whose format is
/// not recognized is refused. Any other value is taken as given and nothing is detected.
///
/// The format does not choose the decoder. libarchive recognizes what it reads from the content, among all
/// the formats this build supports, whichever format of that kind was passed: ZIP for a 7-Zip archive, or
/// GZIP for an xz stream, reads the input all the same. Pass a format for an input whose kind Auto gets
/// wrong, such as a .tar.gz archive under a name that does not tell, which Auto takes for a gzip stream.
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

/// Get the path a compressed file decompresses to: its compression extension removed
/// (.gz, .gzip, .xz, .lzma, .zst, ...), or ".decompressed" appended when the format was
/// detected from the content and the name has no such extension, so that the result
/// differs from the input. Returns original path if not a compressed file format
QString strippedPath(const QString &filePath);

// ============================================================================
// Single-File Decompression
// ============================================================================

/// Default limit for decompressed data that is held in memory (256 MiB)
constexpr qint64 kDefaultMaxDecompressedMemoryBytes = 256LL * 1024 * 1024;

/// Default limit for a compressed file that is decompressed to another file (1 GiB)
constexpr qint64 kDefaultMaxDecompressedFileBytes = 1024LL * 1024 * 1024;

/// What decompressFile() does with a file that is taken for a single compressed stream, by its name or by the
/// format passed, and is not compressed
enum class PlainInput
{
    Refuse,  ///< Fail with Error::UnsupportedFormat, and write nothing
    Copy     ///< Write the file to the output as it is
};

/// Decompress a single file
/// A compressed file that holds no data, as a compressor writes for an empty file, gives an empty output file.
/// A file without any bytes is not a compressed file, and is refused. So is a bzip2 or LZ4 file in a build
/// that cannot decode it, and a file that is not compressed unless plainInput says to copy it: nothing is
/// written, and the error is Error::UnsupportedFormat.
/// PlainInput::Copy is for a file that has the name of a compressed one and can have been decompressed on its
/// way: the body of a download is, when its server sends it with a content encoding, which the HTTP client
/// takes off. The size limit holds for the copy too, and the callback can cancel it. The filter name of the
/// outcome is "none" after a copy. A compressed file that is truncated or corrupt fails either way.
/// @param inputPath Path to compressed file (or Qt resource path :/)
/// @param outputPath Path for decompressed output. If empty: inputPath without the extension of the format
///        (for GZIP .gz or .gzip), or with ".decompressed" appended if it does not end in one
/// @param format Kind of input, see Format (Auto = detect from inputPath). A single-stream format decompresses
///        the file and decides the default output path. An archive format extracts the file as extractArchive()
///        does, after a warning, into the directory outputPath.
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum size of the output in bytes (0 = unlimited)
/// @param plainInput What to do with a file that is not compressed
/// @return Success, or why it failed
Outcome decompressFile(const QString& inputPath, const QString& outputPath = QString(), Format format = Format::Auto,
                       ProgressCallback progress = nullptr, qint64 maxDecompressedBytes = 0,
                       PlainInput plainInput = PlainInput::Refuse);

/// Decompress file if it's a compressed format, otherwise return original path
/// The original path is also returned for a file that has the name of a compressed one and is not compressed.
/// @param filePath Input file (may or may not be compressed)
/// @param outputPath Output path (if empty, uses strippedPath)
/// @param removeOriginal Remove compressed file after successful decompression (default: false)
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return Path to result file (decompressed or original), empty string on failure or if size limit exceeded
QString decompressIfNeeded(const QString& filePath, const QString& outputPath = QString(), bool removeOriginal = false,
                           qint64 maxDecompressedBytes = kDefaultMaxDecompressedFileBytes);

/// As above
/// @param outcome Receives how the decompression ended. A file that is returned as it is, is a success
QString decompressIfNeeded(const QString& filePath, Outcome& outcome, const QString& outputPath = QString(),
                           bool removeOriginal = false, qint64 maxDecompressedBytes = kDefaultMaxDecompressedFileBytes);

/// Decompress data in memory
/// @param data Compressed input data
/// @param decompressed Receives the decompressed data, none for a stream that holds no data. It can be @a data
/// @param format Kind of input, a single-stream format here: see Format (Auto = detect from magic bytes)
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return Success, or why it failed. Data that is not compressed is not returned as it is: that is a failure,
///         with Error::UnsupportedFormat
Outcome decompressData(const QByteArray& data, QByteArray& decompressed, Format format = Format::Auto,
                       qint64 maxDecompressedBytes = 0);

// ============================================================================
// Archive Extraction (Multiple Files)
// ============================================================================

/// Extract an archive to a directory
/// Nothing is written outside the output directory: entries whose path leaves it, hard links, and
/// symbolic links with an absolute target or ".." in it are skipped with a warning. So are special files:
/// a FIFO, a device node or a socket is not created.
/// A failed extraction is undone only if it stopped at the size limit or was cancelled: the files and
/// directories it created are then removed again. If the archive turns out to be truncated or corrupt, or an
/// entry cannot be written, they stay: the entries before the failure, and the one it happened in, which
/// holds what could be read of it and can be filled up with zeros to the size the archive declares for it.
/// extractArchiveAtomic() leaves nothing behind.
/// The size limit counts the bytes of entry data that are read from the archive. The holes of a sparse entry are
/// not stored, so they do not count, and the size an entry declares is not held against the limit: a file is
/// written with the size its entry declares, which can be more than the limit.
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param outputDirectoryPath Output directory path
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return Success, or why it failed
Outcome extractArchive(const QString& archivePath, const QString& outputDirectoryPath, Format format = Format::Auto,
                       ProgressCallback progress = nullptr, qint64 maxDecompressedBytes = 0);

/// Extract an archive to a directory atomically (all-or-nothing)
/// Uses staged extraction and an atomic directory swap on success.
/// If extraction or commit fails, the pre-existing output directory is restored.
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param outputDirectoryPath Output directory path
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return Success, or why it failed
Outcome extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath,
                             Format format = Format::Auto, ProgressCallback progress = nullptr,
                             qint64 maxDecompressedBytes = 0);

/// Extract an archive with per-entry filtering
/// The entries the filter accepts are written as extractArchive() writes them, and a failed extraction leaves
/// behind what a failed extractArchive() leaves. The size limit is that of extractArchive(): an entry is not
/// refused for the size it declares.
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param outputDirectoryPath Output directory path
/// @param filter Callback to decide which entries to extract (return true to extract). It is asked for the
///        directories too: one it accepts is created, one it refuses is only made for the files in it
/// @param progress Optional progress callback (return false to cancel)
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return Success (even if filter skipped all entries), or why it failed
Outcome extractArchiveFiltered(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                               ProgressCallback progress = nullptr, qint64 maxDecompressedBytes = 0);

/// List contents of an archive without extracting
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param entries Receives the file paths in the archive, none for an archive without entries
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success, or why it failed: Error::InvalidArchive if the archive cannot be read or is damaged
Outcome listArchive(const QString& archivePath, QStringList& entries, Format format = Format::Auto);

/// List contents of an archive with detailed metadata
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param entries Receives the entries, none for an archive without entries
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success, or why it failed, as for listArchive()
Outcome listArchiveDetailed(const QString& archivePath, QList<ArchiveEntry>& entries, Format format = Format::Auto);

/// Get summary statistics for an archive without extracting
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param stats Receives the statistics, all zeros for an archive without entries
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success, or why it failed, as for listArchive()
Outcome getArchiveStats(const QString& archivePath, ArchiveStats& stats, Format format = Format::Auto);

/// Validate an archive's integrity without extracting
/// @param archivePath Path to archive file
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success if archive is valid and all entries can be read
Outcome validateArchive(const QString& archivePath, Format format = Format::Auto);

/// Check if a file exists in an archive without extracting
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileName Name of file to check for (as shown by listArchive)
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success if file exists in archive. Otherwise Error::FileNotInArchive only for an archive that was read
///         to its end, and Error::InvalidArchive for one that cannot be read or is damaged before the entry
Outcome fileExists(const QString& archivePath, const QString& fileName, Format format = Format::Auto);

/// Extract a single file from an archive to disk
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileName Name of file inside archive (as shown by listArchive)
/// @param outputPath Output file path (if empty, extracts to current directory)
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success, or why it failed: Error::FileNotInArchive if the archive was read to its end without the
///         file, Error::UnsupportedFormat if it is a directory, a hard or symbolic link or a special file (FIFO,
///         device node, socket), which are not extracted, Error::InvalidArchive if the archive cannot be read
///         or is damaged, and Error::IoError if the output cannot be written, or Error::PermissionDenied if
///         that is for lack of permission
Outcome extractFile(const QString& archivePath, const QString& fileName, const QString& outputPath = QString(),
                    Format format = Format::Auto);

/// Extract a single file from an archive directly to memory
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileName Name of file inside archive (as shown by listArchive)
/// @param data Receives the file contents, none for a file without data
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @param maxDecompressedBytes Fail if the file is larger than this many bytes (0 = unlimited)
/// @return Success, or why it failed: Error::FileNotInArchive if the archive was read to its end without the
///         file, Error::UnsupportedFormat if it is a directory, a hard or symbolic link or a special file, which
///         have no data to extract, Error::SizeLimitExceeded, or Error::InvalidArchive if the archive cannot be
///         read or is damaged
Outcome extractFileData(const QString& archivePath, const QString& fileName, QByteArray& data,
                        Format format = Format::Auto, qint64 maxDecompressedBytes = kDefaultMaxDecompressedMemoryBytes);

/// Extract multiple files from an archive in one pass
/// The files are written as extractArchive() writes its entries: a symbolic link that stays inside the output
/// directory is created. The archive is read up to the last of the files. A failed extraction leaves behind
/// what a failed extractArchive() leaves.
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param fileNames Names of files inside archive
/// @param outputDirectoryPath Output directory (files extracted with their archive names)
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success if all files found and extracted. It fails also if one of them is a directory, or an entry
///         extractArchive() skips (a hard link, a special file, a symbolic link that leads out of the output
///         directory, a name that leaves it). The error tells which, as for extractFile()
Outcome extractFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath,
                     Format format = Format::Auto);

/// Extract files matching glob patterns from an archive
/// Supports wildcards: * (any chars), ? (single char), [...] (char class)
/// What matches is written as extractArchive() writes its entries, and an entry it skips is skipped here. A
/// directory that matches is passed over: the files in it are what makes it. A failed extraction leaves behind
/// what a failed extractArchive() leaves.
/// @param archivePath Path to archive file (or Qt resource path :/)
/// @param patterns Glob patterns to match (e.g., "*.json", "subdir/*", "file.txt")
/// @param outputDirectoryPath Output directory (files extracted with their archive names)
/// @param extractedFiles Optional output list of files that were extracted
/// @param format Kind of input, an archive format here: see Format (Auto = detect from archivePath)
/// @return Success if at least one file matched and extracted. Otherwise Error::FileNotInArchive if the archive
///         was read to its end without a file that matches, Error::InvalidArchive if it cannot be read or is
///         damaged, and Error::IoError if the output cannot be written, or Error::PermissionDenied if that is
///         for lack of permission
Outcome extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                         QStringList* extractedFiles = nullptr, Format format = Format::Auto);

// ============================================================================
// QIODevice-based Operations (streaming)
// ============================================================================

/// Decompress from QIODevice to file
/// Enables streaming decompression from QNetworkReply, QBuffer, etc.
/// What is read has to be a compressed stream, as for decompressFile(): one that is not compressed is refused
/// with Error::UnsupportedFormat.
/// @param device QIODevice to read from (must be open and readable)
/// @param outputPath Output file path
/// @param progress Optional progress callback
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return Success, or why it failed
Outcome decompressFromDevice(QIODevice* device, const QString& outputPath, ProgressCallback progress = nullptr,
                             qint64 maxDecompressedBytes = 0);

/// Decompress from QIODevice to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param data Receives the decompressed data, none for a stream that holds no data
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
/// @return Success, or why it failed: Error::UnsupportedFormat for a stream that is not compressed
Outcome decompressFromDevice(QIODevice* device, QByteArray& data, qint64 maxDecompressedBytes = 0);

/// Extract archive from QIODevice to directory
/// What is written, and what a failed extraction leaves behind, is as for extractArchive().
/// @param device QIODevice to read from (must be open and readable)
/// @param outputDirectoryPath Output directory path
/// @param progress Optional progress callback
/// @param maxDecompressedBytes Maximum total decompressed size in bytes (0 = unlimited)
/// @return Success, or why it failed
Outcome extractFromDevice(QIODevice* device, const QString& outputDirectoryPath, ProgressCallback progress = nullptr,
                          qint64 maxDecompressedBytes = 0);

/// Extract a single file from archive device to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param fileName Name of file inside archive
/// @param data Receives the file contents, none for a file without data
/// @param maxDecompressedBytes Fail if the file is larger than this many bytes (0 = unlimited)
/// @return Success, or why it failed, as for extractFileData()
Outcome extractFileDataFromDevice(QIODevice* device, const QString& fileName, QByteArray& data,
                                  qint64 maxDecompressedBytes = kDefaultMaxDecompressedMemoryBytes);

// ============================================================================
// In-Memory Compression (zlib via qCompress/qUncompress)
// ============================================================================

enum class CompressionLevel {
    None    = 0,
    Fast    = 1,
    Default = 6,
    Best    = 9
};

/// Compress data using qCompress (4-byte size header + zlib payload).
/// @param data Raw data to compress
/// @param level Compression level
/// @return Compressed data in qCompress format, or empty on failure
QByteArray compress(const QByteArray &data,
                    CompressionLevel level = CompressionLevel::Default);

/// Decompress data using qUncompress (expects 4-byte size header + zlib payload).
/// @param data Compressed data in qCompress format
/// @return Decompressed data, or empty on failure
QByteArray uncompress(const QByteArray &data);

/// Compress data in memory with a framing header byte.
/// Returns header(1) + payload. If compression doesn't reduce size, returns uncompressed with header.
/// @param data Raw data to compress
/// @param level Compression level
/// @param minSize Minimum data size to attempt compression (smaller data returned uncompressed)
QByteArray compressData(const QByteArray &data,
                        CompressionLevel level = CompressionLevel::Default,
                        int minSize = 256);

/// Decompress data produced by compressData().
/// @param data Header byte + payload (as produced by compressData)
/// @param maxDecompressedSize Maximum allowed decompressed size (0 = no limit)
QByteArray uncompressData(const QByteArray &data, qint64 maxDecompressedSize = 64LL * 1024 * 1024);

/// Check if data has the compressed framing header.
bool isDataCompressed(const QByteArray &data);

/// Compression ratio from the last compressData() call (thread-local, percentage of original size).
int lastCompressionRatio();

/// Read file contents, transparently decompressing .gz/.xz/.zst/.bz2/.lz4 files.
/// A compressed file that is truncated or corrupt yields nothing, not the part that could be read.
/// A file with such a name that is not compressed is read as it is.
/// @param errorString Receives the reason if the file cannot be read, and is left as it is otherwise: pass an
///        empty string to tell a file without content from a failure
/// @param maxBytes Read at most this many bytes of the (decompressed) content (0 = all of it)
/// @param maxDecompressedBytes Fail, returning nothing, if a compressed file expands beyond this
///        many bytes before maxBytes is reached (0 = unlimited)
QByteArray readFile(const QString& filePath, QString* errorString = nullptr, qint64 maxBytes = 0,
                    qint64 maxDecompressedBytes = kDefaultMaxDecompressedMemoryBytes);

/// Hash file contents post-decompression.
/// @return Hex digest, or an empty string if the file cannot be read to its end
QString computeFileHash(const QString &filePath,
                        QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256);

// ============================================================================
// Compressed JSON Helpers
// ============================================================================

/// Quick check whether `data` begins with a recognized compression magic
/// (gzip / xz / zstd / bzip2 / lz4). Wraps detectFormatFromData + isCompressionFormat.
bool looksLikeCompressedData(const QByteArray &data);

/// Parse JSON from data that may be compressed. Auto-detects gzip/xz/zstd/bzip2/lz4.
/// @param maxDecompressedBytes Maximum decompressed size in bytes (0 = unlimited)
QJsonDocument parseCompressedJson(const QByteArray& data, QJsonParseError* error = nullptr,
                                  qint64 maxDecompressedBytes = kDefaultMaxDecompressedMemoryBytes);

} // namespace QGCCompression
