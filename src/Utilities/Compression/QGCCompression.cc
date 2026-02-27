#include "QGCCompression.h"

#include <QtCore/QCollator>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeType>

#include <algorithm>
#include <cstring>

#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGClibarchive.h"

QGC_LOGGING_CATEGORY(QGCCompressionLog, "Utilities.QGCCompression")

namespace QGCCompression {

// ============================================================================
// Format Detection Constants
// ============================================================================

namespace {

// Minimum bytes needed for magic byte detection
constexpr size_t kMinMagicBytes = 6;

// TAR format detection offsets
constexpr size_t kTarUstarOffset = 257;
constexpr size_t kTarUstarMagicLen = 5;
constexpr size_t kMinBytesForTar = 263;  // kTarUstarOffset + kTarUstarMagicLen + 1

// Magic byte sequences for format detection
constexpr unsigned char kMagicZip[] = {0x50, 0x4B};                         // "PK"
constexpr unsigned char kZipLocalFile = 0x03;                               // PK\x03\x04 - local file header
constexpr unsigned char kZipEmptyArchive = 0x05;                            // PK\x05\x06 - empty archive
constexpr unsigned char kZipSpannedArchive = 0x07;                          // PK\x07\x08 - spanned archive
constexpr unsigned char kMagic7z[] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};  // "7z\xBC\xAF'\x1C"
constexpr unsigned char kMagicGzip[] = {0x1F, 0x8B};
constexpr unsigned char kMagicXz[] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};  // "\xFD7zXZ\0"
constexpr unsigned char kMagicZstd[] = {0x28, 0xB5, 0x2F, 0xFD};
constexpr unsigned char kMagicBzip2[] = {0x42, 0x5A, 0x68};                 // "BZh"
constexpr unsigned char kMagicLz4[] = {0x04, 0x22, 0x4D, 0x18};

/// Convert Qt MIME type name to Format
/// @param mimeType MIME type name (e.g., "application/gzip")
/// @return Corresponding Format, or Format::Auto if not recognized
Format formatFromMimeType(const QString& mimeType)
{
    // Archive formats
    if (mimeType == QLatin1String("application/zip") || mimeType == QLatin1String("application/x-zip-compressed")) {
        return Format::ZIP;
    }
    if (mimeType == QLatin1String("application/x-7z-compressed")) {
        return Format::SEVENZ;
    }
    if (mimeType == QLatin1String("application/x-tar")) {
        return Format::TAR;
    }

    // Compression formats
    if (mimeType == QLatin1String("application/gzip") || mimeType == QLatin1String("application/x-gzip")) {
        return Format::GZIP;
    }
    if (mimeType == QLatin1String("application/x-xz")) {
        return Format::XZ;
    }
    if (mimeType == QLatin1String("application/zstd") || mimeType == QLatin1String("application/x-zstd")) {
        return Format::ZSTD;
    }
    if (mimeType == QLatin1String("application/x-bzip2") || mimeType == QLatin1String("application/bzip2")) {
        return Format::BZIP2;
    }
    if (mimeType == QLatin1String("application/x-lz4")) {
        return Format::LZ4;
    }

    // Compound TAR formats (Qt may detect these as compressed TAR)
    if (mimeType == QLatin1String("application/x-compressed-tar")) {
        return Format::TAR_GZ;  // Most common, fallback will refine
    }
    if (mimeType == QLatin1String("application/x-xz-compressed-tar")) {
        return Format::TAR_XZ;
    }
    if (mimeType == QLatin1String("application/x-zstd-compressed-tar")) {
        return Format::TAR_ZSTD;
    }
    if (mimeType == QLatin1String("application/x-bzip2-compressed-tar") ||
        mimeType == QLatin1String("application/x-bzip-compressed-tar")) {
        return Format::TAR_BZ2;
    }
    if (mimeType == QLatin1String("application/x-lz4-compressed-tar")) {
        return Format::TAR_LZ4;
    }

    return Format::Auto;
}

}  // namespace

// ============================================================================
// Thread-Local Error State
// ============================================================================

namespace {

struct ThreadState
{
    Error error = Error::None;
    QString errorString;
    QString formatName;
    QString filterName;
};

thread_local ThreadState t_state;

void clearError()
{
    t_state.error = Error::None;
    t_state.errorString.clear();
}

void setError(Error error, const QString& message = QString())
{
    t_state.error = error;
    t_state.errorString = message;
}

void setFormatInfo(const QString& format, const QString& filter)
{
    t_state.formatName = format;
    t_state.filterName = filter;
}

void clearFormatInfo()
{
    t_state.formatName.clear();
    t_state.filterName.clear();
}

}  // namespace

// ============================================================================
// Error Handling
// ============================================================================

Error lastError()
{
    return t_state.error;
}

QString lastErrorString()
{
    if (!t_state.errorString.isEmpty()) {
        return t_state.errorString;
    }
    return errorName(t_state.error);
}

QString errorName(Error error)
{
    switch (error) {
        case Error::None:
            return QStringLiteral("No error");
        case Error::FileNotFound:
            return QStringLiteral("File not found");
        case Error::PermissionDenied:
            return QStringLiteral("Permission denied");
        case Error::InvalidArchive:
            return QStringLiteral("Invalid or corrupt archive");
        case Error::UnsupportedFormat:
            return QStringLiteral("Unsupported format");
        case Error::SizeLimitExceeded:
            return QStringLiteral("Size limit exceeded");
        case Error::Cancelled:
            return QStringLiteral("Operation cancelled");
        case Error::FileNotInArchive:
            return QStringLiteral("File not found in archive");
        case Error::IoError:
            return QStringLiteral("I/O error");
        case Error::InternalError:
            return QStringLiteral("Internal error");
    }
    return QStringLiteral("Unknown error");
}

QString detectedFormatName()
{
    return t_state.formatName;
}

QString detectedFilterName()
{
    return t_state.filterName;
}

// ============================================================================
// Internal Helpers
// ============================================================================

/// Validate file input: check existence and detect format
/// @param filePath Path to validate
/// @param format Format to use/detect (modified if Auto)
/// @return true if file exists and format was detected
static bool validateFileInput(const QString& filePath, Format& format)
{
    clearError();
    clearFormatInfo();

    if (!QGCFileHelper::exists(filePath)) {
        qCWarning(QGCCompressionLog) << "File does not exist:" << filePath;
        setError(Error::FileNotFound, QStringLiteral("File does not exist: ") + filePath);
        return false;
    }
    if (format == Format::Auto) {
        format = detectFormat(filePath);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format for:" << filePath;
            setError(Error::UnsupportedFormat, QStringLiteral("Could not detect format: ") + filePath);
            return false;
        }
    }
    return true;
}

/// Validate archive input: file exists, format detected, and is archive format
static bool validateArchiveInput(const QString& archivePath, Format& format)
{
    if (!validateFileInput(archivePath, format)) {
        return false;
    }
    if (!isArchiveFormat(format)) {
        qCWarning(QGCCompressionLog) << "Not an archive format:" << formatName(format);
        setError(Error::UnsupportedFormat, formatName(format) + QStringLiteral(" is not an archive format"));
        return false;
    }
    return true;
}

/// Validate device input for streaming operations
static bool validateDeviceInput(QIODevice* device)
{
    clearError();
    clearFormatInfo();

    if (!device || !device->isOpen() || !device->isReadable()) {
        qCWarning(QGCCompressionLog) << "Device is null, not open, or not readable";
        setError(Error::IoError, QStringLiteral("Device is null, not open, or not readable"));
        return false;
    }
    return true;
}

/// Capture format detection info from QGClibarchive after an operation
static void captureFormatInfo()
{
    setFormatInfo(QGClibarchive::lastDetectedFormatName(), QGClibarchive::lastDetectedFilterName());
}

// ============================================================================
// Format Detection
// ============================================================================

/// Extension-based format detection (internal helper)
static Format detectFormatFromExtension(const QString& filePath)
{
    const QString lower = filePath.toLower();

    // Check compound extensions first
    if (lower.endsWith(".tar.gz") || lower.endsWith(".tgz")) {
        return Format::TAR_GZ;
    }
    if (lower.endsWith(".tar.xz") || lower.endsWith(".txz")) {
        return Format::TAR_XZ;
    }
    if (lower.endsWith(".tar.zst") || lower.endsWith(".tar.zstd")) {
        return Format::TAR_ZSTD;
    }
    if (lower.endsWith(".tar.bz2") || lower.endsWith(".tbz2") || lower.endsWith(".tbz")) {
        return Format::TAR_BZ2;
    }
    if (lower.endsWith(".tar.lz4")) {
        return Format::TAR_LZ4;
    }

    // Check single extensions
    if (lower.endsWith(".zip")) {
        return Format::ZIP;
    }
    if (lower.endsWith(".7z")) {
        return Format::SEVENZ;
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
    if (lower.endsWith(".bz2") || lower.endsWith(".bzip2")) {
        return Format::BZIP2;
    }
    if (lower.endsWith(".lz4")) {
        return Format::LZ4;
    }
    if (lower.endsWith(".tar")) {
        return Format::TAR;
    }

    return Format::Auto;
}

Format detectFormat(const QString& filePath, bool useContentFallback)
{
    // Try extension-based detection first
    Format format = detectFormatFromExtension(filePath);
    if (format != Format::Auto) {
        return format;
    }

    // If extension detection failed and content fallback is enabled, try content-based
    if (useContentFallback && QGCFileHelper::exists(filePath)) {
        format = detectFormatFromFile(filePath);
        if (format != Format::Auto) {
            qCDebug(QGCCompressionLog) << "Format detected from content:" << formatName(format) << "for" << filePath;
        }
    }

    return format;
}

Format detectFormatFromFile(const QString& filePath)
{
    // Handle Qt resources
    if (QGCFileHelper::isQtResource(filePath)) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return Format::Auto;
        }
        return detectFormatFromData(file.read(512));
    }

    // Try QMimeDatabase first (uses both filename and content)
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);

    if (mimeType.isValid() && mimeType.name() != QLatin1String("application/octet-stream")) {
        Format format = formatFromMimeType(mimeType.name());
        if (format != Format::Auto) {
            qCDebug(QGCCompressionLog) << "MIME detection:" << mimeType.name() << "->" << formatName(format);
            return format;
        }
    }

    // Fall back to reading raw bytes and using magic byte detection
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Format::Auto;
    }
    return detectFormatFromData(file.read(512));
}

Format detectFormatFromData(const QByteArray& data)
{
    if (static_cast<size_t>(data.size()) < kMinMagicBytes) {
        return Format::Auto;
    }

    const auto* bytes = reinterpret_cast<const unsigned char*>(data.constData());

    // ZIP: PK\x03\x04 (local file) or PK\x05\x06 (empty) or PK\x07\x08 (spanned)
    if (bytes[0] == kMagicZip[0] && bytes[1] == kMagicZip[1] &&
        (bytes[2] == kZipLocalFile || bytes[2] == kZipEmptyArchive || bytes[2] == kZipSpannedArchive)) {
        return Format::ZIP;
    }

    // 7-Zip
    if (memcmp(bytes, kMagic7z, sizeof(kMagic7z)) == 0) {
        return Format::SEVENZ;
    }

    // GZIP
    if (memcmp(bytes, kMagicGzip, sizeof(kMagicGzip)) == 0) {
        return Format::GZIP;
    }

    // XZ
    if (memcmp(bytes, kMagicXz, sizeof(kMagicXz)) == 0) {
        return Format::XZ;
    }

    // ZSTD
    if (memcmp(bytes, kMagicZstd, sizeof(kMagicZstd)) == 0) {
        return Format::ZSTD;
    }

    // BZip2
    if (memcmp(bytes, kMagicBzip2, sizeof(kMagicBzip2)) == 0) {
        return Format::BZIP2;
    }

    // LZ4
    if (memcmp(bytes, kMagicLz4, sizeof(kMagicLz4)) == 0) {
        return Format::LZ4;
    }

    // TAR: Check for ustar magic at offset 257
    if (static_cast<size_t>(data.size()) >= kMinBytesForTar) {
        if (data.mid(kTarUstarOffset, kTarUstarMagicLen) == "ustar") {
            return Format::TAR;
        }
    }

    // Fallback: Use QMimeDatabase for content-based detection
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForData(data);
    if (mimeType.isValid() && mimeType.name() != QLatin1String("application/octet-stream")) {
        Format format = formatFromMimeType(mimeType.name());
        if (format != Format::Auto) {
            qCDebug(QGCCompressionLog) << "MIME fallback detection:" << mimeType.name() << "->" << formatName(format);
            return format;
        }
    }

    return Format::Auto;
}

QString formatExtension(Format format)
{
    switch (format) {
        case Format::ZIP:
            return QStringLiteral(".zip");
        case Format::SEVENZ:
            return QStringLiteral(".7z");
        case Format::GZIP:
            return QStringLiteral(".gz");
        case Format::XZ:
            return QStringLiteral(".xz");
        case Format::ZSTD:
            return QStringLiteral(".zst");
        case Format::BZIP2:
            return QStringLiteral(".bz2");
        case Format::LZ4:
            return QStringLiteral(".lz4");
        case Format::TAR:
            return QStringLiteral(".tar");
        case Format::TAR_GZ:
            return QStringLiteral(".tar.gz");
        case Format::TAR_XZ:
            return QStringLiteral(".tar.xz");
        case Format::TAR_ZSTD:
            return QStringLiteral(".tar.zst");
        case Format::TAR_BZ2:
            return QStringLiteral(".tar.bz2");
        case Format::TAR_LZ4:
            return QStringLiteral(".tar.lz4");
        case Format::Auto:
            return QString();
    }
    Q_UNREACHABLE();
}

QString formatName(Format format)
{
    switch (format) {
        case Format::Auto:
            return QStringLiteral("Auto");
        case Format::ZIP:
            return QStringLiteral("ZIP");
        case Format::SEVENZ:
            return QStringLiteral("7-Zip");
        case Format::GZIP:
            return QStringLiteral("GZIP");
        case Format::XZ:
            return QStringLiteral("XZ/LZMA");
        case Format::ZSTD:
            return QStringLiteral("Zstandard");
        case Format::BZIP2:
            return QStringLiteral("BZip2");
        case Format::LZ4:
            return QStringLiteral("LZ4");
        case Format::TAR:
            return QStringLiteral("TAR");
        case Format::TAR_GZ:
            return QStringLiteral("TAR.GZ");
        case Format::TAR_XZ:
            return QStringLiteral("TAR.XZ");
        case Format::TAR_ZSTD:
            return QStringLiteral("TAR.ZSTD");
        case Format::TAR_BZ2:
            return QStringLiteral("TAR.BZ2");
        case Format::TAR_LZ4:
            return QStringLiteral("TAR.LZ4");
    }
    Q_UNREACHABLE();
}

bool isArchiveFormat(Format format)
{
    switch (format) {
        case Format::ZIP:
        case Format::SEVENZ:
        case Format::TAR:
        case Format::TAR_GZ:
        case Format::TAR_XZ:
        case Format::TAR_ZSTD:
        case Format::TAR_BZ2:
        case Format::TAR_LZ4:
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
        case Format::BZIP2:
        case Format::LZ4:
            return true;
        default:
            return false;
    }
}

QString strippedPath(const QString& filePath)
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
// Single-File Decompression
// ============================================================================

bool decompressFile(const QString& inputPath, const QString& outputPath, Format format, ProgressCallback progress,
                    qint64 maxDecompressedBytes)
{
    if (!validateFileInput(inputPath, format)) {
        return false;
    }

    // Determine output path
    QString actualOutput = outputPath;
    if (actualOutput.isEmpty()) {
        QString ext = formatExtension(format);
        if (inputPath.endsWith(ext, Qt::CaseInsensitive)) {
            actualOutput = inputPath.left(inputPath.size() - ext.size());
        } else {
            actualOutput = inputPath + ".decompressed";
        }
    }

    qCDebug(QGCCompressionLog) << "Decompressing" << inputPath << "to" << actualOutput << "using" << formatName(format);

    // Single-file compression formats
    if (isCompressionFormat(format)) {
        const bool success =
            QGClibarchive::decompressSingleFile(inputPath, actualOutput, progress, maxDecompressedBytes);
        captureFormatInfo();
        if (!success) {
            setError(Error::IoError, QStringLiteral("Decompression failed: ") + inputPath);
        }
        return success;
    }

    // Archive formats - delegate to extractArchive with a warning
    if (isArchiveFormat(format)) {
        qCWarning(QGCCompressionLog) << formatName(format) << "is an archive format; use extractArchive() instead";
        return extractArchive(inputPath, actualOutput, format, progress, maxDecompressedBytes);
    }

    qCWarning(QGCCompressionLog) << "Unsupported decompression format:" << formatName(format);
    setError(Error::UnsupportedFormat, QStringLiteral("Unsupported decompression format: ") + formatName(format));
    return false;
}

QString decompressIfNeeded(const QString& filePath, const QString& outputPath, bool removeOriginal)
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

QByteArray decompressData(const QByteArray& data, Format format, qint64 maxDecompressedBytes)
{
    if (data.isEmpty()) {
        qCWarning(QGCCompressionLog) << "Cannot decompress empty data";
        return {};
    }

    if (format == Format::Auto) {
        format = detectFormatFromData(data);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format from data";
            return {};
        }
    }

    if (!isCompressionFormat(format)) {
        qCWarning(QGCCompressionLog) << "Invalid decompression format:" << formatName(format);
        setError(Error::UnsupportedFormat, formatName(format) + QStringLiteral(" is not a compression format"));
        return {};
    }

    QByteArray result = QGClibarchive::decompressDataFromMemory(data, maxDecompressedBytes);
    captureFormatInfo();
    if (result.isEmpty()) {
        setError(Error::IoError, QStringLiteral("Failed to decompress data"));
    }
    return result;
}

// ============================================================================
// Archive Extraction
// ============================================================================

bool extractArchive(const QString& archivePath, const QString& outputDirectoryPath, Format format,
                    ProgressCallback progress, qint64 maxDecompressedBytes)
{
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }

    qCDebug(QGCCompressionLog) << "Extracting" << formatName(format) << "archive" << archivePath << "to"
                               << outputDirectoryPath;

    const bool success =
        QGClibarchive::extractAnyArchive(archivePath, outputDirectoryPath, progress, maxDecompressedBytes);
    captureFormatInfo();
    if (!success) {
        setError(Error::IoError, QStringLiteral("Failed to extract archive: ") + archivePath);
    }
    return success;
}

bool extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath, Format format,
                          ProgressCallback progress, qint64 maxDecompressedBytes)
{
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }

    qCDebug(QGCCompressionLog) << "Atomically extracting" << formatName(format) << "archive" << archivePath
                               << "to" << outputDirectoryPath;

    const bool success =
        QGClibarchive::extractArchiveAtomic(archivePath, outputDirectoryPath, progress, maxDecompressedBytes);
    captureFormatInfo();
    if (!success) {
        setError(Error::IoError, QStringLiteral("Failed to atomically extract archive: ") + archivePath);
    }
    return success;
}

bool extractArchiveFiltered(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                            ProgressCallback progress, qint64 maxDecompressedBytes)
{
    if (!filter) {
        setError(Error::InternalError, QStringLiteral("No filter callback provided"));
        return false;
    }
    Format format = Format::Auto;
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }

    qCDebug(QGCCompressionLog) << "Extracting archive with filter:" << archivePath;

    const bool success =
        QGClibarchive::extractWithFilter(archivePath, outputDirectoryPath, filter, progress, maxDecompressedBytes);
    captureFormatInfo();
    if (!success) {
        setError(Error::IoError, QStringLiteral("Failed to extract archive: ") + archivePath);
    }
    return success;
}

QStringList listArchive(const QString& archivePath, Format format)
{
    if (!validateArchiveInput(archivePath, format)) {
        return {};
    }
    QStringList entries = QGClibarchive::listArchiveEntries(archivePath);

    // Natural sort: "file2.txt" before "file10.txt"
    // Use English locale for consistent cross-platform numeric collation
    QCollator collator{QLocale{QLocale::English}};
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(entries.begin(), entries.end(), collator);

    return entries;
}

QList<ArchiveEntry> listArchiveDetailed(const QString& archivePath, Format format)
{
    if (!validateArchiveInput(archivePath, format)) {
        return {};
    }
    QList<ArchiveEntry> entries = QGClibarchive::listArchiveEntriesDetailed(archivePath);

    // Natural sort by name: "file2.txt" before "file10.txt"
    // Use English locale for consistent cross-platform numeric collation
    QCollator collator{QLocale{QLocale::English}};
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(entries.begin(), entries.end(), [&collator](const ArchiveEntry& a, const ArchiveEntry& b) {
        return collator.compare(a.name, b.name) < 0;
    });

    return entries;
}

ArchiveStats getArchiveStats(const QString& archivePath, Format format)
{
    if (!validateArchiveInput(archivePath, format)) {
        return {};
    }
    return QGClibarchive::getArchiveStats(archivePath);
}

bool validateArchive(const QString& archivePath, Format format)
{
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Validating archive:" << archivePath;
    return QGClibarchive::validateArchive(archivePath);
}

bool fileExists(const QString& archivePath, const QString& fileName, Format format)
{
    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        return false;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    return QGClibarchive::fileExistsInArchive(archivePath, fileName);
}

bool extractFile(const QString& archivePath, const QString& fileName, const QString& outputPath, Format format)
{
    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        return false;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }

    const QString actualOutput = outputPath.isEmpty() ? QFileInfo(fileName).fileName() : outputPath;
    qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from" << archivePath << "to" << actualOutput;
    return QGClibarchive::extractSingleFile(archivePath, fileName, actualOutput);
}

QByteArray extractFileData(const QString& archivePath, const QString& fileName, Format format)
{
    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        return {};
    }
    if (!validateArchiveInput(archivePath, format)) {
        return {};
    }
    qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from" << archivePath << "to memory";
    return QGClibarchive::extractFileToMemory(archivePath, fileName);
}

bool extractFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath,
                  Format format)
{
    if (fileNames.isEmpty()) {
        return true;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Extracting" << fileNames.size() << "files from" << archivePath;
    return QGClibarchive::extractMultipleFiles(archivePath, fileNames, outputDirectoryPath);
}

bool extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                      QStringList* extractedFiles, Format format)
{
    if (patterns.isEmpty()) {
        setError(Error::FileNotInArchive, QStringLiteral("No patterns provided"));
        return false;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Extracting files matching patterns" << patterns << "from" << archivePath;
    const bool success = QGClibarchive::extractByPattern(archivePath, patterns, outputDirectoryPath, extractedFiles);
    if (!success) {
        setError(Error::FileNotInArchive, QStringLiteral("No files matched patterns"));
    }
    return success;
}

// ============================================================================
// QIODevice-based Operations
// ============================================================================

bool decompressFromDevice(QIODevice* device, const QString& outputPath, ProgressCallback progress,
                          qint64 maxDecompressedBytes)
{
    if (!validateDeviceInput(device)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Decompressing from device to" << outputPath;
    const bool success = QGClibarchive::decompressFromDevice(device, outputPath, progress, maxDecompressedBytes);
    captureFormatInfo();
    if (!success) {
        setError(Error::IoError, QStringLiteral("Failed to decompress from device"));
    }
    return success;
}

QByteArray decompressFromDevice(QIODevice* device, qint64 maxDecompressedBytes)
{
    if (!validateDeviceInput(device)) {
        return {};
    }
    qCDebug(QGCCompressionLog) << "Decompressing from device to memory";
    QByteArray result = QGClibarchive::decompressDataFromDevice(device, maxDecompressedBytes);
    captureFormatInfo();
    if (result.isEmpty()) {
        setError(Error::IoError, QStringLiteral("Failed to decompress from device"));
    }
    return result;
}

bool extractFromDevice(QIODevice* device, const QString& outputDirectoryPath, ProgressCallback progress,
                       qint64 maxDecompressedBytes)
{
    if (!validateDeviceInput(device)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Extracting archive from device to" << outputDirectoryPath;
    const bool success = QGClibarchive::extractFromDevice(device, outputDirectoryPath, progress, maxDecompressedBytes);
    captureFormatInfo();
    if (!success) {
        setError(Error::IoError, QStringLiteral("Failed to extract archive from device"));
    }
    return success;
}

QByteArray extractFileDataFromDevice(QIODevice* device, const QString& fileName)
{
    if (!validateDeviceInput(device)) {
        return {};
    }
    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        setError(Error::FileNotInArchive, QStringLiteral("File name cannot be empty"));
        return {};
    }
    qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from device to memory";
    QByteArray result = QGClibarchive::extractFileDataFromDevice(device, fileName);
    captureFormatInfo();
    if (result.isEmpty()) {
        setError(Error::FileNotInArchive, QStringLiteral("File not found: ") + fileName);
    }
    return result;
}

}  // namespace QGCCompression
