#include "QGCCompression.h"
#include "QGCLoggingCategory.h"
#include "QGClibarchive.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <archive.h>
#include <archive_entry.h>

QGC_LOGGING_CATEGORY(QGCCompressionLog, "Utilities.QGCCompression")

namespace QGCCompression {

// ============================================================================
// Libarchive-based Decompression (handles gzip, xz, zstd, bz2, lz4)
// ============================================================================

/// Decompress a single compressed file using libarchive's "raw" format
/// This handles standalone compressed files (not archives) like .gz, .xz, .zst
/// Also handles Qt resources (:/path) by reading through QFile first
static bool decompressWithLibarchive(const QString &inputPath, const QString &outputPath)
{
    // Read input file (handles Qt resources and regular files)
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qCWarning(QGCCompressionLog) << "Failed to open input file:" << inputPath
                                      << inputFile.errorString();
        return false;
    }
    const QByteArray compressedData = inputFile.readAll();
    inputFile.close();

    if (compressedData.isEmpty()) {
        qCWarning(QGCCompressionLog) << "Input file is empty:" << inputPath;
        return false;
    }

    struct archive *a = archive_read_new();
    if (!a) {
        qCWarning(QGCCompressionLog) << "Failed to create archive reader";
        return false;
    }

    // Support all compression filters
    archive_read_support_filter_all(a);
    // Use "raw" format to treat input as a single compressed file
    archive_read_support_format_raw(a);

    // Open from memory buffer (works with Qt resources)
    if (archive_read_open_memory(a, compressedData.constData(),
                                  static_cast<size_t>(compressedData.size())) != ARCHIVE_OK) {
        qCWarning(QGCCompressionLog) << "Failed to open compressed data:" << archive_error_string(a);
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry;
    if (archive_read_next_header(a, &entry) != ARCHIVE_OK) {
        qCWarning(QGCCompressionLog) << "Failed to read header:" << archive_error_string(a);
        archive_read_free(a);
        return false;
    }

    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(QGCCompressionLog) << "Failed to open output file:" << outputPath
                                      << outputFile.errorString();
        archive_read_free(a);
        return false;
    }

    char buffer[16384];
    la_ssize_t size;
    while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
        if (outputFile.write(buffer, size) != size) {
            qCWarning(QGCCompressionLog) << "Failed to write output:" << outputFile.errorString();
            archive_read_free(a);
            return false;
        }
    }

    if (size < 0) {
        qCWarning(QGCCompressionLog) << "Decompression error:" << archive_error_string(a);
        archive_read_free(a);
        return false;
    }

    outputFile.close();
    archive_read_free(a);

    qCDebug(QGCCompressionLog) << "Successfully decompressed" << inputPath << "to" << outputPath;
    return true;
}

// ============================================================================
// Format Detection
// ============================================================================

Format detectFormat(const QString &filePath)
{
    const QString lower = filePath.toLower();

    // Check compound extensions first
    if (lower.endsWith(".tar.gz") || lower.endsWith(".tgz")) {
        return Format::TAR_GZ;
    }
    if (lower.endsWith(".tar.xz") || lower.endsWith(".txz")) {
        return Format::TAR_XZ;
    }

    // Check single extensions
    if (lower.endsWith(".zip")) {
        return Format::ZIP;
    }
    if (lower.endsWith(".gz") || lower.endsWith(".gzip")) {
        return Format::GZIP;
    }
    if (lower.endsWith(".xz") || lower.endsWith(".lzma")) {
        return Format::XZ;
    }
    if (lower.endsWith(".zst") || lower.endsWith(".zstd")) {
        return Format::ZSTD;
    }
    if (lower.endsWith(".tar")) {
        return Format::TAR;
    }

    return Format::Auto;
}

Format detectFormatFromData(const QByteArray &data)
{
    if (data.size() < 6) {
        return Format::Auto;
    }

    const auto* bytes = reinterpret_cast<const unsigned char*>(data.constData());

    // ZIP: PK\x03\x04 or PK\x05\x06 (empty) or PK\x07\x08 (spanned)
    if (bytes[0] == 0x50 && bytes[1] == 0x4B &&
        (bytes[2] == 0x03 || bytes[2] == 0x05 || bytes[2] == 0x07)) {
        return Format::ZIP;
    }

    // GZIP: \x1f\x8b
    if (bytes[0] == 0x1F && bytes[1] == 0x8B) {
        return Format::GZIP;
    }

    // XZ: \xfd7zXZ\x00
    if (bytes[0] == 0xFD && bytes[1] == 0x37 && bytes[2] == 0x7A &&
        bytes[3] == 0x58 && bytes[4] == 0x5A && bytes[5] == 0x00) {
        return Format::XZ;
    }

    // ZSTD: 0x28 0xB5 0x2F 0xFD (little-endian magic)
    if (bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
        return Format::ZSTD;
    }

    // TAR: Check for ustar magic at offset 257
    if (data.size() >= 263) {
        if (data.mid(257, 5) == "ustar") {
            return Format::TAR;
        }
    }

    return Format::Auto;
}

QString formatExtension(Format format)
{
    switch (format) {
    case Format::ZIP:    return QStringLiteral(".zip");
    case Format::GZIP:   return QStringLiteral(".gz");
    case Format::XZ:     return QStringLiteral(".xz");
    case Format::ZSTD:   return QStringLiteral(".zst");
    case Format::TAR:    return QStringLiteral(".tar");
    case Format::TAR_GZ: return QStringLiteral(".tar.gz");
    case Format::TAR_XZ: return QStringLiteral(".tar.xz");
    case Format::Auto:   return QString();
    }
    return QString();
}

QString formatName(Format format)
{
    switch (format) {
    case Format::Auto:   return QStringLiteral("Auto");
    case Format::ZIP:    return QStringLiteral("ZIP");
    case Format::GZIP:   return QStringLiteral("GZIP");
    case Format::XZ:     return QStringLiteral("XZ/LZMA");
    case Format::ZSTD:   return QStringLiteral("Zstandard");
    case Format::TAR:    return QStringLiteral("TAR");
    case Format::TAR_GZ: return QStringLiteral("TAR.GZ");
    case Format::TAR_XZ: return QStringLiteral("TAR.XZ");
    }
    return QStringLiteral("Unknown");
}

bool isArchiveFormat(Format format)
{
    switch (format) {
    case Format::ZIP:
    case Format::TAR:
    case Format::TAR_GZ:
    case Format::TAR_XZ:
        return true;
    default:
        return false;
    }
}

bool isCompressionFormat(Format format)
{
    switch (format) {
    case Format::GZIP:
    case Format::XZ:
    case Format::ZSTD:
        return true;
    default:
        return false;
    }
}

QString strippedPath(const QString &filePath)
{
    const Format format = detectFormat(filePath);
    if (!isCompressionFormat(format)) {
        return filePath;
    }

    const QString ext = formatExtension(format);
    if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
        return filePath.left(filePath.size() - ext.size());
    }

    return filePath;
}

// ============================================================================
// Single-File Compression/Decompression
// ============================================================================

bool compressFile(const QString &inputPath, const QString &outputPath, Format format)
{
    QFileInfo inputInfo(inputPath);
    if (!inputInfo.exists()) {
        qCWarning(QGCCompressionLog) << "Input file does not exist:" << inputPath;
        return false;
    }

    // Determine output path
    QString actualOutput = outputPath;
    if (actualOutput.isEmpty()) {
        if (format == Format::Auto) {
            format = Format::GZIP;  // Default to GZIP for single-file compression
        }
        actualOutput = inputPath + formatExtension(format);
    }

    // Detect format from output path if Auto
    if (format == Format::Auto) {
        format = detectFormat(actualOutput);
        if (format == Format::Auto) {
            format = Format::GZIP;
        }
    }

    qCDebug(QGCCompressionLog) << "Compressing" << inputPath << "to" << actualOutput
                                << "using" << formatName(format);

    // TODO: Implement compression using libarchive
    qCWarning(QGCCompressionLog) << "Single-file compression not yet implemented";
    return false;
}

bool decompressFile(const QString &inputPath, const QString &outputPath, Format format)
{
    QFileInfo inputInfo(inputPath);
    if (!inputInfo.exists()) {
        qCWarning(QGCCompressionLog) << "Input file does not exist:" << inputPath;
        return false;
    }

    // Detect format if Auto
    if (format == Format::Auto) {
        format = detectFormat(inputPath);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format for:" << inputPath;
            return false;
        }
    }

    // Determine output path
    QString actualOutput = outputPath;
    if (actualOutput.isEmpty()) {
        // Strip compression extension
        QString ext = formatExtension(format);
        if (inputPath.endsWith(ext, Qt::CaseInsensitive)) {
            actualOutput = inputPath.left(inputPath.size() - ext.size());
        } else {
            actualOutput = inputPath + ".decompressed";
        }
    }

    qCDebug(QGCCompressionLog) << "Decompressing" << inputPath << "to" << actualOutput
                                << "using" << formatName(format);

    switch (format) {
    case Format::GZIP:
    case Format::XZ:
    case Format::ZSTD:
        return decompressWithLibarchive(inputPath, actualOutput);

    case Format::ZIP:
        qCWarning(QGCCompressionLog) << "Use extractArchive() for ZIP files";
        return false;

    case Format::TAR:
    case Format::TAR_GZ:
    case Format::TAR_XZ:
        qCWarning(QGCCompressionLog) << "Use extractArchive() for TAR archives";
        return false;

    default:
        qCWarning(QGCCompressionLog) << "Unsupported decompression format:" << formatName(format);
        return false;
    }
}

QString decompressIfNeeded(const QString &filePath, const QString &outputPath, bool removeOriginal)
{
    // If not a compressed file, return original path unchanged
    if (!isCompressedFile(filePath)) {
        return filePath;
    }

    // Determine output path
    const QString actualOutput = outputPath.isEmpty() ? strippedPath(filePath) : outputPath;

    // Attempt decompression
    if (!decompressFile(filePath, actualOutput)) {
        qCWarning(QGCCompressionLog) << "Decompression failed:" << filePath;
        return QString();
    }

    if (removeOriginal && !QFile::remove(filePath)) {
        qCWarning(QGCCompressionLog) << "Failed to remove original file:" << filePath;
    }

    return actualOutput;
}

QByteArray compressData(const QByteArray &data, Format format)
{
    Q_UNUSED(data)
    Q_UNUSED(format)
    // TODO: Implement in-memory compression
    qCWarning(QGCCompressionLog) << "In-memory compression not yet implemented";
    return QByteArray();
}

QByteArray decompressData(const QByteArray &data, Format format)
{
    Q_UNUSED(data)

    if (format == Format::Auto) {
        format = detectFormatFromData(data);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format from data";
            return QByteArray();
        }
    }

    // TODO: Implement in-memory decompression
    qCWarning(QGCCompressionLog) << "In-memory decompression not yet implemented for" << formatName(format);
    return QByteArray();
}

// ============================================================================
// Archive Operations
// ============================================================================

bool createArchive(const QString &directoryPath, const QString &archivePath, Format format)
{
    // Detect format from path if Auto
    if (format == Format::Auto) {
        format = detectFormat(archivePath);
        if (format == Format::Auto) {
            format = Format::ZIP;  // Default to ZIP
        }
    }

    qCDebug(QGCCompressionLog) << "Creating" << formatName(format) << "archive"
                                << archivePath << "from" << directoryPath;

    switch (format) {
    case Format::ZIP:
        return QGClibarchive::zipDirectory(directoryPath, archivePath);

    case Format::TAR:
    case Format::TAR_GZ:
    case Format::TAR_XZ:
        // TODO: Add TAR support to libarchive wrapper
        qCWarning(QGCCompressionLog) << "TAR archive creation not yet implemented";
        return false;

    default:
        qCWarning(QGCCompressionLog) << "Unsupported archive format:" << formatName(format);
        return false;
    }
}

bool extractArchive(const QString &archivePath, const QString &outputDirectoryPath, Format format)
{
    QFileInfo archiveInfo(archivePath);
    if (!archiveInfo.exists()) {
        qCWarning(QGCCompressionLog) << "Archive file does not exist:" << archivePath;
        return false;
    }

    // Detect format from path if Auto
    if (format == Format::Auto) {
        format = detectFormat(archivePath);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect archive format for:" << archivePath;
            return false;
        }
    }

    qCDebug(QGCCompressionLog) << "Extracting" << formatName(format) << "archive"
                                << archivePath << "to" << outputDirectoryPath;

    switch (format) {
    case Format::ZIP:
        return QGClibarchive::unzipFile(archivePath, outputDirectoryPath);

    case Format::TAR:
    case Format::TAR_GZ:
    case Format::TAR_XZ:
        // TODO: Add TAR support to libarchive wrapper
        qCWarning(QGCCompressionLog) << "TAR archive extraction not yet implemented";
        return false;

    default:
        qCWarning(QGCCompressionLog) << "Unsupported archive format:" << formatName(format);
        return false;
    }
}

} // namespace QGCCompression
