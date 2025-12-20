#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>

/// Unified compression interface for archives and single-file compression
/// Uses libarchive for all operations (ZIP, GZIP, XZ, ZSTD, TAR, etc.)
namespace QGCCompression {

// ============================================================================
// Format Definitions
// ============================================================================

/// Compression/archive format types
enum class Format {
    Auto,       ///< Auto-detect from file extension or magic bytes
    ZIP,        ///< ZIP archive (multiple files)
    GZIP,       ///< GZIP compression (single file, .gz)
    XZ,         ///< XZ/LZMA compression (single file, .xz)
    ZSTD,       ///< Zstandard compression (single file, .zst)
    TAR,        ///< TAR archive (uncompressed, multiple files)
    TAR_GZ,     ///< TAR + GZIP (.tar.gz, .tgz)
    TAR_XZ,     ///< TAR + XZ (.tar.xz, .txz)
};

// ============================================================================
// Format Detection
// ============================================================================

/// Detect format from file path (extension-based)
Format detectFormat(const QString &filePath);

/// Detect format from data (magic bytes)
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

// ============================================================================
// Single-File Compression/Decompression
// ============================================================================

/// Compress a single file
/// @param inputPath Path to file to compress
/// @param outputPath Path for compressed output (if empty, appends format extension)
/// @param format Compression format (Auto = detect from outputPath extension)
/// @return true on success
bool compressFile(const QString &inputPath, const QString &outputPath = QString(),
                  Format format = Format::Auto);

/// Decompress a single file
/// @param inputPath Path to compressed file
/// @param outputPath Path for decompressed output (if empty, strips compression extension)
/// @param format Compression format (Auto = detect from inputPath)
/// @return true on success
bool decompressFile(const QString &inputPath, const QString &outputPath = QString(),
                    Format format = Format::Auto);

/// Decompress file if it's a compressed format, otherwise return original path
/// @param filePath Input file (may or may not be compressed)
/// @param outputPath Output path (if empty, uses strippedPath)
/// @param removeOriginal Remove compressed file after successful decompression (default: false)
/// @return Path to result file (decompressed or original), empty string on failure
QString decompressIfNeeded(const QString &filePath,
                           const QString &outputPath = QString(),
                           bool removeOriginal = false);

/// Compress data in memory
/// @param data Input data to compress
/// @param format Compression format
/// @return Compressed data, or empty on failure
QByteArray compressData(const QByteArray &data, Format format);

/// Decompress data in memory
/// @param data Compressed input data
/// @param format Compression format (Auto = detect from magic bytes)
/// @return Decompressed data, or empty on failure
QByteArray decompressData(const QByteArray &data, Format format = Format::Auto);

// ============================================================================
// Archive Operations (Multiple Files)
// ============================================================================

/// Create an archive from a directory
/// @param directoryPath Path to directory to archive
/// @param archivePath Output archive path
/// @param format Archive format (Auto = detect from archivePath, default ZIP)
/// @return true on success
bool createArchive(const QString &directoryPath, const QString &archivePath,
                   Format format = Format::Auto);

/// Extract an archive to a directory
/// @param archivePath Path to archive file
/// @param outputDirectoryPath Output directory path
/// @param format Archive format (Auto = detect from archivePath)
/// @return true on success
bool extractArchive(const QString &archivePath, const QString &outputDirectoryPath,
                    Format format = Format::Auto);

// ============================================================================
// Convenience Aliases
// ============================================================================

/// Compress a directory into a ZIP archive
inline bool zipDirectory(const QString &directoryPath, const QString &zipFilePath) {
    return createArchive(directoryPath, zipFilePath, Format::ZIP);
}

/// Extract a ZIP archive to a directory
inline bool unzipFile(const QString &zipFilePath, const QString &outputDirectoryPath) {
    return extractArchive(zipFilePath, outputDirectoryPath, Format::ZIP);
}

/// Decompress a GZIP file
inline bool inflateGzipFile(const QString &gzippedFileName, const QString &decompressedFilename) {
    return decompressFile(gzippedFileName, decompressedFilename, Format::GZIP);
}

/// Decompress an XZ/LZMA file
inline bool inflateLZMAFile(const QString &lzmaFilename, const QString &decompressedFilename) {
    return decompressFile(lzmaFilename, decompressedFilename, Format::XZ);
}

} // namespace QGCCompression
