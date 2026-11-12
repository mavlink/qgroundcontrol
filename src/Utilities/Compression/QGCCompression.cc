#include "QGCCompression.h"

#include <algorithm>
#include <iterator>

#include <QtCore/QCollator>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeType>
#include <QtCore/QtEndian>

#include "QGCDecompressDevice.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGClibarchive.h"

QGC_LOGGING_CATEGORY(QGCCompressionLog, "Utilities.QGCCompression")

namespace QGCCompression {

// ============================================================================
// Formats
// ============================================================================

namespace {

// Minimum bytes needed for magic byte detection
constexpr size_t kMinMagicBytes = 6;

// TAR format detection offsets
constexpr size_t kTarUstarOffset = 257;
constexpr size_t kTarUstarMagicLen = 5;
constexpr size_t kMinBytesForTar = 263;  // kTarUstarOffset + kTarUstarMagicLen + 1

/// What is known of a format: every function that tells about formats looks it up here
struct FormatInfo
{
    Format format;
    bool isArchive;                ///< Holds several files; otherwise it is a single compressed stream
    const char* name;              ///< formatName()
    const char* suffixes[3];       ///< In lower case. The first one is formatExtension()
    const char* mimeTypes[2];      ///< Names Qt's MIME database has for the format
    QByteArrayView signatures[3];  ///< What a file of the format starts with (TAR: see kTarUstarOffset)
};

// Looked through in this order. The compressed TAR formats come first, because their suffixes end in those of
// the compression, and TAR last, because its signature is not at the start of the file.
constexpr FormatInfo kFormats[] = {
    {Format::TAR_GZ, true, "TAR.GZ", {".tar.gz", ".tgz"}, {"application/x-compressed-tar"}, {}},
    {Format::TAR_XZ, true, "TAR.XZ", {".tar.xz", ".txz"}, {"application/x-xz-compressed-tar"}, {}},
    {Format::TAR_ZSTD, true, "TAR.ZSTD", {".tar.zst", ".tar.zstd"}, {"application/x-zstd-compressed-tar"}, {}},
    {Format::TAR_BZ2,
     true,
     "TAR.BZ2",
     {".tar.bz2", ".tbz2", ".tbz"},
     {"application/x-bzip2-compressed-tar", "application/x-bzip-compressed-tar"},
     {}},
    {Format::TAR_LZ4, true, "TAR.LZ4", {".tar.lz4"}, {"application/x-lz4-compressed-tar"}, {}},
    // Local file header, empty archive, spanned archive
    {Format::ZIP,
     true,
     "ZIP",
     {".zip"},
     {"application/zip", "application/x-zip-compressed"},
     {"PK\x03", "PK\x05", "PK\x07"}},
    {Format::SEVENZ, true, "7-Zip", {".7z"}, {"application/x-7z-compressed"}, {"7z\xBC\xAF\x27\x1C"}},
    {Format::GZIP, false, "GZIP", {".gz", ".gzip"}, {"application/gzip", "application/x-gzip"}, {"\x1F\x8B"}},
    // "\xFD7zXZ" and a zero byte, which a view of the literal alone would leave out
    {Format::XZ, false, "XZ/LZMA", {".xz", ".lzma"}, {"application/x-xz"}, {{"\xFD\x37\x7A\x58\x5A\x00", 6}}},
    {Format::ZSTD,
     false,
     "Zstandard",
     {".zst", ".zstd"},
     {"application/zstd", "application/x-zstd"},
     {"\x28\xB5\x2F\xFD"}},
    {Format::BZIP2, false, "BZip2", {".bz2", ".bzip2"}, {"application/x-bzip2", "application/bzip2"}, {"BZh"}},
    {Format::LZ4, false, "LZ4", {".lz4"}, {"application/x-lz4"}, {"\x04\x22\x4D\x18"}},
    {Format::TAR, true, "TAR", {".tar"}, {"application/x-tar"}, {}},
};

/// @return What is known of @a format, nullptr for Format::Auto
const FormatInfo* formatInfo(Format format)
{
    const auto info = std::ranges::find(kFormats, format, &FormatInfo::format);
    return (info != std::end(kFormats)) ? &*info : nullptr;
}

/// Convert Qt MIME type name to Format
/// @param mimeType MIME type name (e.g., "application/gzip")
/// @return Corresponding Format, or Format::Auto if not recognized
Format formatFromMimeType(const QString& mimeType)
{
    for (const FormatInfo& info : kFormats) {
        for (const char* name : info.mimeTypes) {
            if ((name != nullptr) && (mimeType == QLatin1StringView(name))) {
                return info.format;
            }
        }
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
    QGClibarchive::clearLastDetectedFormat();
}

/// Forget what the operation before this one left behind
/// For an operation that can end before it validates its input, which is where the others do this.
void beginOperation()
{
    clearError();
    clearFormatInfo();
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
    if (!QGCFileHelper::isQtResource(filePath) && !QFileInfo(filePath).isReadable()) {
        qCWarning(QGCCompressionLog) << "No permission to read:" << filePath;
        setError(Error::PermissionDenied, QStringLiteral("No permission to read: ") + filePath);
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

/// Record the failure of a QGClibarchive operation that takes a size limit
/// @param result How the operation ended
/// @param message What could not be done, which the reason is appended to if it has a code of its own
static void setOperationError(QGClibarchive::ReadResult result, const QString& message)
{
    if (QGClibarchive::takeSizeLimitExceeded()) {
        setError(Error::SizeLimitExceeded, message + QStringLiteral(": size limit exceeded"));
    } else if (result == QGClibarchive::ReadResult::Cancelled) {
        setError(Error::Cancelled, message + QStringLiteral(": cancelled"));
    } else if (result == QGClibarchive::ReadResult::WriteDenied) {
        setError(Error::PermissionDenied, message + QStringLiteral(": no permission to write the output"));
    } else if (result == QGClibarchive::ReadResult::WriteFailed) {
        setError(Error::IoError, message);
    } else if (result == QGClibarchive::ReadResult::NotCompressed) {
        setError(Error::UnsupportedFormat, message + QStringLiteral(": the input is not compressed"));
    } else if (result == QGClibarchive::ReadResult::Unsupported) {
        setError(Error::UnsupportedFormat,
                 message + QStringLiteral(": the compression is not supported by this build"));
    } else {
        // What is left is the input, which cannot be read or is damaged
        setError(Error::InvalidArchive, message);
    }
}

/// Record that an archive cannot be read or is damaged
static void setArchiveReadError(const QString& archivePath)
{
    setError(Error::InvalidArchive, QStringLiteral("Invalid or corrupt archive: ") + archivePath);
}

/// Record why a function that writes entries of an archive to files failed
/// @param message What could not be done, which the reason is appended to
static void setExtractionError(QGClibarchive::ReadResult result, const QString& message)
{
    switch (result) {
        case QGClibarchive::ReadResult::EntryNotFound:
            setError(Error::FileNotInArchive, message + QStringLiteral(": not in the archive"));
            return;
        case QGClibarchive::ReadResult::EntrySkipped:
            setError(Error::UnsupportedFormat,
                     message + QStringLiteral(": a directory, a link, a special file or a name that leaves the "
                                              "output directory is not extracted"));
            return;
        case QGClibarchive::ReadResult::WriteFailed:
            setError(Error::IoError, message + QStringLiteral(": the output cannot be written"));
            return;
        case QGClibarchive::ReadResult::WriteDenied:
            setError(Error::PermissionDenied, message + QStringLiteral(": no permission to write the output"));
            return;
        case QGClibarchive::ReadResult::Completed:
        case QGClibarchive::ReadResult::Cancelled:
        case QGClibarchive::ReadResult::NotCompressed:
        case QGClibarchive::ReadResult::Unsupported:
        case QGClibarchive::ReadResult::Failed:
            break;
    }
    setError(Error::InvalidArchive, message + QStringLiteral(": the archive cannot be read or is damaged"));
}

// ============================================================================
// Format Detection
// ============================================================================

/// Extension-based format detection (internal helper)
static Format detectFormatFromExtension(const QString& filePath)
{
    const QString lower = filePath.toLower();
    for (const FormatInfo& info : kFormats) {
        for (const char* suffix : info.suffixes) {
            if ((suffix != nullptr) && lower.endsWith(QLatin1StringView(suffix))) {
                return info.format;
            }
        }
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

    for (const FormatInfo& info : kFormats) {
        for (const QByteArrayView signature : info.signatures) {
            if (!signature.isEmpty() && data.startsWith(signature)) {
                return info.format;
            }
        }
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
    const FormatInfo* const info = formatInfo(format);
    return (info != nullptr) ? QString::fromLatin1(info->suffixes[0]) : QString();
}

QString formatName(Format format)
{
    const FormatInfo* const info = formatInfo(format);
    return (info != nullptr) ? QString::fromLatin1(info->name) : QStringLiteral("Auto");
}

bool isArchiveFormat(Format format)
{
    const FormatInfo* const info = formatInfo(format);
    return (info != nullptr) && info->isArchive;
}

bool isCompressionFormat(Format format)
{
    const FormatInfo* const info = formatInfo(format);
    return (info != nullptr) && !info->isArchive;
}

QString strippedPath(const QString& filePath)
{
    if (!isCompressionFormat(detectFormat(filePath))) {
        return filePath;
    }

    // Every single-stream extension the detection accepts is the suffix after the last dot
    if (detectFormatFromExtension(filePath) != Format::Auto) {
        return filePath.left(filePath.lastIndexOf(QLatin1Char('.')));
    }

    // Detected from the content: the name has nothing to strip, and the result must not be the input
    return filePath + QStringLiteral(".decompressed");
}

// ============================================================================
// Single-File Decompression
// ============================================================================

/// decompressFile(), for decompressIfNeeded() and decompressOrCopyFile() too
/// @param copyPlainInput Whether a file that is taken for a single compressed stream and is not compressed is
///        written to the output as it is instead of being refused
/// @param notCompressed Receives whether it failed because such a file was refused
static bool decompressFileOrTell(const QString& inputPath, const QString& outputPath, Format format,
                                 const ProgressCallback& progress, qint64 maxDecompressedBytes, bool copyPlainInput,
                                 bool& notCompressed)
{
    notCompressed = false;

    if (!validateFileInput(inputPath, format)) {
        return false;
    }

    // Determine output path
    QString actualOutput = outputPath;
    if (actualOutput.isEmpty()) {
        QString ext = formatExtension(format);
        if (inputPath.endsWith(ext, Qt::CaseInsensitive)) {
            actualOutput = inputPath.left(inputPath.size() - ext.size());
        } else if (isCompressionFormat(format) && (detectFormatFromExtension(inputPath) == format)) {
            // Another extension of the format (.gzip, .lzma, .zstd, .bzip2), which strippedPath() removes too
            actualOutput = inputPath.left(inputPath.lastIndexOf(QLatin1Char('.')));
        } else {
            actualOutput = inputPath + ".decompressed";
        }
    }

    qCDebug(QGCCompressionLog) << "Decompressing" << inputPath << "to" << actualOutput << "using" << formatName(format);

    // Single-file compression formats
    if (isCompressionFormat(format)) {
        QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
        const bool success = QGClibarchive::decompressSingleFile(inputPath, actualOutput, progress,
                                                                 maxDecompressedBytes, &result, copyPlainInput);
        captureFormatInfo();
        if (!success) {
            notCompressed = (result == QGClibarchive::ReadResult::NotCompressed);
            setOperationError(result, QStringLiteral("Decompression failed: ") + inputPath);
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

bool decompressFile(const QString& inputPath, const QString& outputPath, Format format, ProgressCallback progress,
                    qint64 maxDecompressedBytes)
{
    bool notCompressed = false;
    return decompressFileOrTell(inputPath, outputPath, format, progress, maxDecompressedBytes, false, notCompressed);
}

bool decompressOrCopyFile(const QString& inputPath, const QString& outputPath, ProgressCallback progress,
                          qint64 maxDecompressedBytes)
{
    bool notCompressed = false;
    return decompressFileOrTell(inputPath, outputPath, Format::Auto, progress, maxDecompressedBytes, true,
                                notCompressed);
}

QString decompressIfNeeded(const QString& filePath, const QString& outputPath, bool removeOriginal,
                           qint64 maxDecompressedBytes)
{
    beginOperation();

    // If not a compressed file, return original path unchanged
    if (!isCompressedFile(filePath)) {
        return filePath;
    }

    // Determine output path
    const QString actualOutput = outputPath.isEmpty() ? strippedPath(filePath) : outputPath;

    // Attempt decompression
    bool notCompressed = false;
    if (!decompressFileOrTell(filePath, actualOutput, Format::Auto, nullptr, maxDecompressedBytes, false,
                              notCompressed)) {
        if (notCompressed) {
            // Only its name said that the file is compressed: it is the result as it is, and stays
            clearError();
            return filePath;
        }
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
    clearError();
    clearFormatInfo();

    if (data.isEmpty()) {
        qCWarning(QGCCompressionLog) << "Cannot decompress empty data";
        setError(Error::UnsupportedFormat, QStringLiteral("Cannot decompress empty data"));
        return {};
    }

    if (format == Format::Auto) {
        format = detectFormatFromData(data);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format from data";
            setError(Error::UnsupportedFormat, QStringLiteral("Could not detect format from data"));
            return {};
        }
    }

    if (!isCompressionFormat(format)) {
        qCWarning(QGCCompressionLog) << "Invalid decompression format:" << formatName(format);
        setError(Error::UnsupportedFormat, formatName(format) + QStringLiteral(" is not a compression format"));
        return {};
    }

    // The data does not tell a failure: a stream of an empty file has none
    QGClibarchive::ReadResult read = QGClibarchive::ReadResult::Failed;
    QByteArray result = QGClibarchive::decompressDataFromMemory(data, maxDecompressedBytes, &read);
    captureFormatInfo();
    if (read != QGClibarchive::ReadResult::Completed) {
        setOperationError(read, QStringLiteral("Failed to decompress data"));
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

    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success =
        QGClibarchive::extractAnyArchive(archivePath, outputDirectoryPath, progress, maxDecompressedBytes, &result);
    captureFormatInfo();
    if (!success) {
        setOperationError(result, QStringLiteral("Failed to extract archive: ") + archivePath);
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

    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success =
        QGClibarchive::extractArchiveAtomic(archivePath, outputDirectoryPath, progress, maxDecompressedBytes, &result);
    captureFormatInfo();
    if (!success) {
        setOperationError(result, QStringLiteral("Failed to atomically extract archive: ") + archivePath);
    }
    return success;
}

bool extractArchiveFiltered(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                            ProgressCallback progress, qint64 maxDecompressedBytes)
{
    beginOperation();

    if (!filter) {
        setError(Error::InternalError, QStringLiteral("No filter callback provided"));
        return false;
    }
    Format format = Format::Auto;
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }

    qCDebug(QGCCompressionLog) << "Extracting archive with filter:" << archivePath;

    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success = QGClibarchive::extractWithFilter(archivePath, outputDirectoryPath, filter, progress,
                                                          maxDecompressedBytes, &result);
    captureFormatInfo();
    if (!success) {
        setOperationError(result, QStringLiteral("Failed to extract archive: ") + archivePath);
    }
    return success;
}

QStringList listArchive(const QString& archivePath, Format format)
{
    if (!validateArchiveInput(archivePath, format)) {
        return {};
    }
    bool listed = false;
    QStringList entries = QGClibarchive::listArchiveEntries(archivePath, &listed);
    captureFormatInfo();
    if (!listed) {
        setArchiveReadError(archivePath);
    }

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
    bool listed = false;
    QList<ArchiveEntry> entries = QGClibarchive::listArchiveEntriesDetailed(archivePath, &listed);
    captureFormatInfo();
    if (!listed) {
        setArchiveReadError(archivePath);
    }

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
    bool counted = false;
    const ArchiveStats stats = QGClibarchive::getArchiveStats(archivePath, &counted);
    captureFormatInfo();
    if (!counted) {
        setArchiveReadError(archivePath);
    }
    return stats;
}

bool validateArchive(const QString& archivePath, Format format)
{
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Validating archive:" << archivePath;
    const bool valid = QGClibarchive::validateArchive(archivePath);
    captureFormatInfo();
    if (!valid) {
        setArchiveReadError(archivePath);
    }
    return valid;
}

bool fileExists(const QString& archivePath, const QString& fileName, Format format)
{
    beginOperation();

    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        setError(Error::FileNotInArchive, QStringLiteral("File name cannot be empty"));
        return false;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    const QGClibarchive::NextEntry found = QGClibarchive::findEntryInArchive(archivePath, fileName);
    captureFormatInfo();
    switch (found) {
        case QGClibarchive::NextEntry::Found:
            return true;
        case QGClibarchive::NextEntry::End:
            setError(Error::FileNotInArchive, QStringLiteral("File not found in archive: ") + fileName);
            return false;
        case QGClibarchive::NextEntry::Error:
            break;
    }
    setArchiveReadError(archivePath);
    return false;
}

bool extractFile(const QString& archivePath, const QString& fileName, const QString& outputPath, Format format)
{
    beginOperation();

    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        setError(Error::FileNotInArchive, QStringLiteral("File name cannot be empty"));
        return false;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }

    const QString actualOutput = outputPath.isEmpty() ? QFileInfo(fileName).fileName() : outputPath;
    qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from" << archivePath << "to" << actualOutput;
    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success = QGClibarchive::extractSingleFile(archivePath, fileName, actualOutput, &result);
    captureFormatInfo();
    if (!success) {
        setExtractionError(result, QStringLiteral("Could not extract %1 from %2").arg(fileName, archivePath));
    }
    return success;
}

QByteArray extractFileData(const QString& archivePath, const QString& fileName, Format format,
                           qint64 maxDecompressedBytes)
{
    beginOperation();

    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        setError(Error::FileNotInArchive, QStringLiteral("File name cannot be empty"));
        return {};
    }
    if (!validateArchiveInput(archivePath, format)) {
        return {};
    }
    qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from" << archivePath << "to memory";
    QGClibarchive::ReadResult read = QGClibarchive::ReadResult::Failed;
    QByteArray result = QGClibarchive::extractFileToMemory(archivePath, fileName, maxDecompressedBytes, &read);
    captureFormatInfo();
    // The data does not tell a failure: an entry can have none
    if (read != QGClibarchive::ReadResult::Completed) {
        if (QGClibarchive::takeSizeLimitExceeded()) {
            setError(Error::SizeLimitExceeded,
                     QStringLiteral("%1 in %2 exceeds the size limit").arg(fileName, archivePath));
        } else if (read == QGClibarchive::ReadResult::EntrySkipped) {
            setError(Error::UnsupportedFormat,
                     QStringLiteral("%1 in %2 is a directory, a link or a special file, which is not extracted")
                         .arg(fileName, archivePath));
        } else {
            const bool notFound = (read == QGClibarchive::ReadResult::EntryNotFound);
            setError(notFound ? Error::FileNotInArchive : Error::InvalidArchive,
                     QStringLiteral("Could not extract %1 from %2").arg(fileName, archivePath));
        }
    }
    return result;
}

bool extractFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath,
                  Format format)
{
    beginOperation();

    if (fileNames.isEmpty()) {
        return true;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Extracting" << fileNames.size() << "files from" << archivePath;
    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success = QGClibarchive::extractMultipleFiles(archivePath, fileNames, outputDirectoryPath, &result);
    captureFormatInfo();
    if (!success) {
        setExtractionError(result, QStringLiteral("Could not extract all requested files from ") + archivePath);
    }
    return success;
}

bool extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                      QStringList* extractedFiles, Format format)
{
    beginOperation();

    if (patterns.isEmpty()) {
        setError(Error::FileNotInArchive, QStringLiteral("No patterns provided"));
        return false;
    }
    if (!validateArchiveInput(archivePath, format)) {
        return false;
    }
    qCDebug(QGCCompressionLog) << "Extracting files matching patterns" << patterns << "from" << archivePath;
    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success =
        QGClibarchive::extractByPattern(archivePath, patterns, outputDirectoryPath, extractedFiles, &result);
    captureFormatInfo();
    if (!success) {
        setExtractionError(result,
                           QStringLiteral("Could not extract files that match the patterns from ") + archivePath);
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
    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success =
        QGClibarchive::decompressFromDevice(device, outputPath, progress, maxDecompressedBytes, &result);
    captureFormatInfo();
    if (!success) {
        setOperationError(result, QStringLiteral("Failed to decompress from device"));
    }
    return success;
}

QByteArray decompressFromDevice(QIODevice* device, qint64 maxDecompressedBytes)
{
    if (!validateDeviceInput(device)) {
        return {};
    }
    qCDebug(QGCCompressionLog) << "Decompressing from device to memory";
    // The data does not tell a failure: a stream of an empty file has none
    QGClibarchive::ReadResult read = QGClibarchive::ReadResult::Failed;
    QByteArray result = QGClibarchive::decompressDataFromDevice(device, maxDecompressedBytes, &read);
    captureFormatInfo();
    if (read != QGClibarchive::ReadResult::Completed) {
        setOperationError(read, QStringLiteral("Failed to decompress from device"));
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
    QGClibarchive::ReadResult result = QGClibarchive::ReadResult::Failed;
    const bool success =
        QGClibarchive::extractFromDevice(device, outputDirectoryPath, progress, maxDecompressedBytes, &result);
    captureFormatInfo();
    if (!success) {
        setOperationError(result, QStringLiteral("Failed to extract archive from device"));
    }
    return success;
}

QByteArray extractFileDataFromDevice(QIODevice* device, const QString& fileName, qint64 maxDecompressedBytes)
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
    QGClibarchive::ReadResult read = QGClibarchive::ReadResult::Failed;
    QByteArray result = QGClibarchive::extractFileDataFromDevice(device, fileName, maxDecompressedBytes, &read);
    captureFormatInfo();
    // The data does not tell a failure: an entry can have none
    if (read != QGClibarchive::ReadResult::Completed) {
        if (QGClibarchive::takeSizeLimitExceeded()) {
            setError(Error::SizeLimitExceeded, fileName + QStringLiteral(" exceeds the size limit"));
        } else if (read == QGClibarchive::ReadResult::EntrySkipped) {
            setError(Error::UnsupportedFormat,
                     fileName + QStringLiteral(" is a directory, a link or a special file, which is not extracted"));
        } else if (read == QGClibarchive::ReadResult::EntryNotFound) {
            setError(Error::FileNotInArchive, QStringLiteral("File not found: ") + fileName);
        } else {
            setError(Error::InvalidArchive, QStringLiteral("Could not extract ") + fileName);
        }
    }
    return result;
}

// ============================================================================
// In-Memory Compression
// ============================================================================

QByteArray compress(const QByteArray &data, CompressionLevel level)
{
    if (data.isEmpty() || level == CompressionLevel::None) {
        return data;
    }
    return qCompress(data, static_cast<int>(level));
}

QByteArray uncompress(const QByteArray &data)
{
    if (data.isEmpty()) {
        return {};
    }
    return qUncompress(data);
}

static constexpr quint8 kHeaderUncompressed = 0x00;
static constexpr quint8 kHeaderCompressed   = 0x01;

static thread_local int s_lastCompressionRatio = 100; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

QByteArray compressData(const QByteArray &data, CompressionLevel level, int minSize)
{
    if (level == CompressionLevel::None || data.size() < minSize) {
        s_lastCompressionRatio = 100;
        QByteArray result;
        result.reserve(1 + data.size());
        result.append(static_cast<char>(kHeaderUncompressed));
        result.append(data);
        return result;
    }

    const QByteArray compressed = qCompress(data, static_cast<int>(level));

    if (!compressed.isEmpty() && compressed.size() < data.size()) {
        s_lastCompressionRatio = (compressed.size() * 100) / data.size();
        QByteArray result;
        result.reserve(1 + compressed.size());
        result.append(static_cast<char>(kHeaderCompressed));
        result.append(compressed);
        return result;
    }

    s_lastCompressionRatio = 100;
    QByteArray result;
    result.reserve(1 + data.size());
    result.append(static_cast<char>(kHeaderUncompressed));
    result.append(data);
    return result;
}

QByteArray uncompressData(const QByteArray &data, qint64 maxDecompressedSize)
{
    if (data.isEmpty()) {
        return {};
    }

    const auto header = static_cast<quint8>(data[0]);
    const QByteArray payload = data.mid(1);

    if (header == kHeaderUncompressed) {
        return payload;
    }

    if (header == kHeaderCompressed) {
        if (maxDecompressedSize > 0 && payload.size() >= 4) {
            const quint32 declaredSize = qFromBigEndian<quint32>(payload.constData());
            if (static_cast<qint64>(declaredSize) > maxDecompressedSize) {
                qCWarning(QGCCompressionLog) << "Rejected decompression: declared size"
                                             << declaredSize << "exceeds limit" << maxDecompressedSize;
                return {};
            }
        }

        const QByteArray result = qUncompress(payload);
        if (result.isEmpty() && !payload.isEmpty()) {
            qCWarning(QGCCompressionLog) << "Decompression failed";
        }
        return result;
    }

    qCWarning(QGCCompressionLog) << "Unknown compression header byte:" << header;
    return {};
}

bool isDataCompressed(const QByteArray &data)
{
    return !data.isEmpty() && static_cast<quint8>(data[0]) == kHeaderCompressed;
}

int lastCompressionRatio()
{
    return s_lastCompressionRatio;
}

QByteArray readFile(const QString& filePath, QString* errorString, qint64 maxBytes, qint64 maxDecompressedBytes)
{
    if (!isCompressedFile(filePath)) {
        return QGCFileHelper::readFile(filePath, errorString, maxBytes);
    }

    QGCDecompressDevice decompressor(filePath);
    if (!decompressor.open(QIODevice::ReadOnly)) {
        if (errorString != nullptr) {
            *errorString = QObject::tr("Failed to open compressed file: %1").arg(filePath);
        }
        return {};
    }

    // Read in chunks so that a stream expanding beyond the limit is given up on before it is all in memory
    QByteArray data;
    QByteArray chunk(static_cast<qsizetype>(QGCFileHelper::kBufferSizeMax), Qt::Uninitialized);
    while (maxBytes <= 0 || data.size() < maxBytes) {
        qint64 chunkSize = chunk.size();
        if (maxBytes > 0) {
            chunkSize = qMin(chunkSize, maxBytes - data.size());
        }
        // The count tells a damaged stream (-1) from one that has ended (0)
        const qint64 bytesRead = decompressor.read(chunk.data(), chunkSize);
        if (bytesRead < 0) {
            if (errorString != nullptr) {
                *errorString = QObject::tr("Failed to decompress %1: %2").arg(filePath, decompressor.errorString());
            }
            return {};
        }
        if (bytesRead == 0) {
            break;
        }
        if (maxDecompressedBytes > 0 && (data.size() + bytesRead) > maxDecompressedBytes) {
            if (errorString != nullptr) {
                *errorString = QObject::tr("Decompressed size of %1 exceeds the limit of %2 bytes")
                                   .arg(filePath)
                                   .arg(maxDecompressedBytes);
            }
            return {};
        }
        data.append(chunk.constData(), static_cast<qsizetype>(bytesRead));
    }
    decompressor.close();
    return data;
}

QString computeFileHash(const QString &filePath, QCryptographicHash::Algorithm algorithm)
{
    if (filePath.isEmpty()) {
        qCWarning(QGCCompressionLog) << "computeFileHash: empty file path";
        return {};
    }

    if (!isCompressedFile(filePath)) {
        return QGCFileHelper::computeFileHash(filePath, algorithm);
    }

    QGCDecompressDevice decompressor(filePath);
    if (!decompressor.open(QIODevice::ReadOnly)) {
        qCWarning(QGCCompressionLog) << "computeFileHash: failed to open:" << filePath;
        return {};
    }

    QCryptographicHash hash(algorithm);
    QByteArray buffer(65536, Qt::Uninitialized);
    while (true) {
        // atEnd() is also true after a read error: the count is what tells one (-1) from the end (0)
        const qint64 bytesRead = decompressor.read(buffer.data(), buffer.size());
        if (bytesRead < 0) {
            qCWarning(QGCCompressionLog) << "computeFileHash: read error";
            decompressor.close();
            return {};
        }
        if (bytesRead == 0) {
            break;
        }
        hash.addData(QByteArrayView(buffer.constData(), static_cast<qsizetype>(bytesRead)));
    }

    decompressor.close();
    return QString::fromLatin1(hash.result().toHex());
}

// ============================================================================
// Compressed JSON Helpers
// ============================================================================

bool looksLikeCompressedData(const QByteArray &data)
{
    return isCompressionFormat(detectFormatFromData(data));
}

QJsonDocument parseCompressedJson(const QByteArray& data, QJsonParseError* error, qint64 maxDecompressedBytes)
{
    QByteArray jsonData = data;

    if (looksLikeCompressedData(data)) {
        jsonData = decompressData(data, Format::Auto, maxDecompressedBytes);
        if (jsonData.isEmpty()) {
            if (error != nullptr) {
                error->error = QJsonParseError::IllegalValue;
                error->offset = 0;
            }
            return {};
        }
    }

    return QJsonDocument::fromJson(jsonData, error);
}

}  // namespace QGCCompression
