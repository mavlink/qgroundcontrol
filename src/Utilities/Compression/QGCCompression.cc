#include "QGCCompression.h"

#include <algorithm>
#include <iterator>
#include <utility>

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
// Error Handling
// ============================================================================

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

// ============================================================================
// Internal Helpers
// ============================================================================

/// Validate file input: check existence and detect format
/// @param filePath Path to validate
/// @param format Format to use/detect (modified if Auto)
/// @return Success if file exists and format was detected, or why the file is refused
static Outcome validateFileInput(const QString& filePath, Format& format)
{
    if (!QGCFileHelper::exists(filePath)) {
        qCWarning(QGCCompressionLog) << "File does not exist:" << filePath;
        return {.error = Error::FileNotFound, .message = QStringLiteral("File does not exist: ") + filePath};
    }
    if (!QGCFileHelper::isQtResource(filePath) && !QFileInfo(filePath).isReadable()) {
        qCWarning(QGCCompressionLog) << "No permission to read:" << filePath;
        return {.error = Error::PermissionDenied, .message = QStringLiteral("No permission to read: ") + filePath};
    }
    if (format == Format::Auto) {
        format = detectFormat(filePath);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format for:" << filePath;
            return {.error = Error::UnsupportedFormat,
                    .message = QStringLiteral("Could not detect format: ") + filePath};
        }
    }
    return {};
}

/// Validate archive input: file exists, format detected, and is archive format
static Outcome validateArchiveInput(const QString& archivePath, Format& format)
{
    Outcome validated = validateFileInput(archivePath, format);
    if (validated && !isArchiveFormat(format)) {
        qCWarning(QGCCompressionLog) << "Not an archive format:" << formatName(format);
        validated = {.error = Error::UnsupportedFormat,
                     .message = formatName(format) + QStringLiteral(" is not an archive format")};
    }
    return validated;
}

/// Validate device input for streaming operations
static Outcome validateDeviceInput(QIODevice* device)
{
    if (!device || !device->isOpen() || !device->isReadable()) {
        qCWarning(QGCCompressionLog) << "Device is null, not open, or not readable";
        return {.error = Error::IoError, .message = QStringLiteral("Device is null, not open, or not readable")};
    }
    return {};
}

/// Validate the name of an entry that is asked for
static Outcome validateEntryName(const QString& fileName)
{
    if (fileName.isEmpty()) {
        qCWarning(QGCCompressionLog) << "File name cannot be empty";
        return {.error = Error::FileNotInArchive, .message = QStringLiteral("File name cannot be empty")};
    }
    return {};
}

/// Why an operation of QGClibarchive failed, for the end of the message, if its code has more to say than the
/// message does
static QString failureReason(Error error, const QGClibarchive::ArchiveReader& reader)
{
    switch (error) {
        case Error::SizeLimitExceeded:
            return QStringLiteral(": size limit exceeded");
        case Error::Cancelled:
            return QStringLiteral(": cancelled");
        case Error::PermissionDenied:
            return QStringLiteral(": no permission to write the output");
        case Error::FileNotInArchive:
            return QStringLiteral(": not in the archive");
        case Error::UnsupportedFormat:
            if (reader.unsupportedStream()) {
                return QStringLiteral(": the compression is not supported by this build");
            }
            if (reader.mode() == QGClibarchive::ReaderMode::RawFormat) {
                return QStringLiteral(": the input is not compressed");
            }
            return QStringLiteral(
                ": a directory, a link, a special file or a name that leaves the output directory "
                "is not extracted");
        default:
            // The input cannot be read or is damaged, or the output cannot be written
            return {};
    }
}

/// Run an operation of QGClibarchive
/// @param what What could not be done, for the message of a failure
/// @param operation Called with the reader for it to open. Returns the Error of the operation
template <typename Operation>
static Outcome runOperation(const QString& what, const Operation& operation)
{
    QGClibarchive::ArchiveReader reader;
    Outcome outcome;
    outcome.error = operation(reader);
    if (!outcome) {
        outcome.message = what + failureReason(outcome.error, reader);
    }
    outcome.formatName = reader.formatName();
    outcome.filterName = reader.filterName();
    return outcome;
}

/// As runOperation(), for an archive file, which is validated first
template <typename Operation>
static Outcome archiveOperation(const QString& archivePath, Format& format, const QString& what,
                                const Operation& operation)
{
    const Outcome validated = validateArchiveInput(archivePath, format);
    return validated ? runOperation(what, operation) : validated;
}

/// As runOperation(), for a device, which is validated first
template <typename Operation>
static Outcome deviceOperation(QIODevice* device, const QString& what, const Operation& operation)
{
    const Outcome validated = validateDeviceInput(device);
    return validated ? runOperation(what, operation) : validated;
}

/// As archiveOperation(), for the functions that extract the entries they are asked for, by name or by pattern:
/// the message of a failure also tells the output that cannot be written from the archive that cannot be read
template <typename Operation>
static Outcome selectedEntriesOperation(const QString& archivePath, Format& format, const QString& what,
                                        const Operation& operation)
{
    Outcome outcome = archiveOperation(archivePath, format, what, operation);
    if (outcome.error == Error::IoError) {
        outcome.message += QStringLiteral(": the output cannot be written");
    } else if (outcome.error == Error::InvalidArchive) {
        outcome.message += QStringLiteral(": the archive cannot be read or is damaged");
    }
    return outcome;
}

/// What could not be done with an archive whose entries were to be read, and none of them written
static QString unreadableArchive(const QString& archivePath)
{
    return QStringLiteral("Invalid or corrupt archive: ") + archivePath;
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

Outcome decompressFile(const QString& inputPath, const QString& outputPath, Format format, ProgressCallback progress,
                       qint64 maxDecompressedBytes, PlainInput plainInput)
{
    if (Outcome refused = validateFileInput(inputPath, format); !refused) {
        return refused;
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
        return runOperation(QStringLiteral("Decompression failed: ") + inputPath, [&](auto& reader) {
            return QGClibarchive::decompressSingleFile(reader, inputPath, actualOutput, progress, maxDecompressedBytes,
                                                       plainInput == PlainInput::Copy);
        });
    }

    // Archive formats - delegate to extractArchive with a warning
    if (isArchiveFormat(format)) {
        qCWarning(QGCCompressionLog) << formatName(format) << "is an archive format; use extractArchive() instead";
        return extractArchive(inputPath, actualOutput, format, progress, maxDecompressedBytes);
    }

    qCWarning(QGCCompressionLog) << "Unsupported decompression format:" << formatName(format);
    return {.error = Error::UnsupportedFormat,
            .message = QStringLiteral("Unsupported decompression format: ") + formatName(format)};
}

QString decompressIfNeeded(const QString& filePath, const QString& outputPath, bool removeOriginal,
                           qint64 maxDecompressedBytes)
{
    Outcome outcome;
    return decompressIfNeeded(filePath, outcome, outputPath, removeOriginal, maxDecompressedBytes);
}

QString decompressIfNeeded(const QString& filePath, Outcome& outcome, const QString& outputPath, bool removeOriginal,
                           qint64 maxDecompressedBytes)
{
    outcome = {};

    // If not a compressed file, return original path unchanged
    if (!isCompressedFile(filePath)) {
        return filePath;
    }

    // Determine output path
    const QString actualOutput = outputPath.isEmpty() ? strippedPath(filePath) : outputPath;

    // Attempt decompression
    outcome = decompressFile(filePath, actualOutput, Format::Auto, nullptr, maxDecompressedBytes);
    if (!outcome) {
        if ((outcome.error == Error::UnsupportedFormat) && (outcome.filterName == QLatin1String("none"))) {
            // Only its name said that the file is compressed, and no decompression filter took it: it is the
            // result as it is, and stays
            outcome.error = Error::None;
            outcome.message.clear();
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

/// Validate data input: there is data, and its format is that of a single compressed stream
/// @param format Format to use/detect (modified if Auto)
static Outcome validateDataInput(const QByteArray& data, Format& format)
{
    if (data.isEmpty()) {
        qCWarning(QGCCompressionLog) << "Cannot decompress empty data";
        return {.error = Error::UnsupportedFormat, .message = QStringLiteral("Cannot decompress empty data")};
    }

    if (format == Format::Auto) {
        format = detectFormatFromData(data);
        if (format == Format::Auto) {
            qCWarning(QGCCompressionLog) << "Could not detect format from data";
            return {.error = Error::UnsupportedFormat, .message = QStringLiteral("Could not detect format from data")};
        }
    }

    if (!isCompressionFormat(format)) {
        qCWarning(QGCCompressionLog) << "Invalid decompression format:" << formatName(format);
        return {.error = Error::UnsupportedFormat,
                .message = formatName(format) + QStringLiteral(" is not a compression format")};
    }
    return {};
}

Outcome decompressData(const QByteArray& data, QByteArray& decompressed, Format format, qint64 maxDecompressedBytes)
{
    if (Outcome refused = validateDataInput(data, format); !refused) {
        decompressed.clear();
        return refused;
    }
    return runOperation(QStringLiteral("Failed to decompress data"), [&](auto& reader) {
        return QGClibarchive::decompressToMemory(reader, data, maxDecompressedBytes, decompressed);
    });
}

// ============================================================================
// Archive Extraction
// ============================================================================

/// extractArchive(), and extractArchiveFiltered() with its filter
static Outcome extractEntries(const QString& archivePath, const QString& outputDirectoryPath, Format format,
                              const EntryFilter& filter, const ProgressCallback& progress, qint64 maxDecompressedBytes)
{
    const QString what = QStringLiteral("Failed to extract archive: ") + archivePath;
    return archiveOperation(archivePath, format, what, [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Extracting" << formatName(format) << "archive" << archivePath << "to"
                                   << outputDirectoryPath;
        return QGClibarchive::extractAnyArchive(reader, archivePath, outputDirectoryPath, filter, progress,
                                                maxDecompressedBytes);
    });
}

Outcome extractArchive(const QString& archivePath, const QString& outputDirectoryPath, Format format,
                       ProgressCallback progress, qint64 maxDecompressedBytes)
{
    return extractEntries(archivePath, outputDirectoryPath, format, {}, progress, maxDecompressedBytes);
}

Outcome extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath, Format format,
                             ProgressCallback progress, qint64 maxDecompressedBytes)
{
    const QString what = QStringLiteral("Failed to atomically extract archive: ") + archivePath;
    return archiveOperation(archivePath, format, what, [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Atomically extracting" << formatName(format) << "archive" << archivePath << "to"
                                   << outputDirectoryPath;
        return QGClibarchive::extractArchiveAtomic(reader, archivePath, outputDirectoryPath, progress,
                                                   maxDecompressedBytes);
    });
}

Outcome extractArchiveFiltered(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                               ProgressCallback progress, qint64 maxDecompressedBytes)
{
    if (!filter) {
        return {.error = Error::InternalError, .message = QStringLiteral("No filter callback provided")};
    }
    return extractEntries(archivePath, outputDirectoryPath, Format::Auto, filter, progress, maxDecompressedBytes);
}

Outcome listArchive(const QString& archivePath, QStringList& entries, Format format)
{
    QList<ArchiveEntry> detailed;
    const Outcome outcome = listArchiveDetailed(archivePath, detailed, format);
    entries.clear();
    for (const ArchiveEntry& entry : std::as_const(detailed)) {
        entries.append(entry.name);
    }
    return outcome;
}

Outcome listArchiveDetailed(const QString& archivePath, QList<ArchiveEntry>& entries, Format format)
{
    entries.clear();
    const Outcome outcome = archiveOperation(archivePath, format, unreadableArchive(archivePath), [&](auto& reader) {
        return QGClibarchive::listArchiveEntriesDetailed(reader, archivePath, entries);
    });

    // Natural sort by name: "file2.txt" before "file10.txt"
    // Use English locale for consistent cross-platform numeric collation
    QCollator collator{QLocale{QLocale::English}};
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(entries.begin(), entries.end(), [&collator](const ArchiveEntry& a, const ArchiveEntry& b) {
        return collator.compare(a.name, b.name) < 0;
    });

    return outcome;
}

Outcome getArchiveStats(const QString& archivePath, ArchiveStats& stats, Format format)
{
    stats = {};
    return archiveOperation(archivePath, format, unreadableArchive(archivePath),
                            [&](auto& reader) { return QGClibarchive::getArchiveStats(reader, archivePath, stats); });
}

Outcome validateArchive(const QString& archivePath, Format format)
{
    return archiveOperation(archivePath, format, unreadableArchive(archivePath), [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Validating archive:" << archivePath;
        return QGClibarchive::validateArchive(reader, archivePath);
    });
}

Outcome fileExists(const QString& archivePath, const QString& fileName, Format format)
{
    if (Outcome refused = validateEntryName(fileName); !refused) {
        return refused;
    }
    Outcome outcome = archiveOperation(archivePath, format, unreadableArchive(archivePath), [&](auto& reader) {
        return QGClibarchive::findEntryInArchive(reader, archivePath, fileName);
    });
    if (outcome.error == Error::FileNotInArchive) {
        // The archive was read to its end: it is not what is wrong
        outcome.message = QStringLiteral("File not found in archive: ") + fileName;
    }
    return outcome;
}

Outcome extractFile(const QString& archivePath, const QString& fileName, const QString& outputPath, Format format)
{
    if (Outcome refused = validateEntryName(fileName); !refused) {
        return refused;
    }
    const QString what = QStringLiteral("Could not extract %1 from %2").arg(fileName, archivePath);
    return selectedEntriesOperation(archivePath, format, what, [&](auto& reader) {
        const QString actualOutput = outputPath.isEmpty() ? QFileInfo(fileName).fileName() : outputPath;
        qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from" << archivePath << "to" << actualOutput;
        return QGClibarchive::extractSingleFile(reader, archivePath, fileName, actualOutput);
    });
}

Outcome extractFileData(const QString& archivePath, const QString& fileName, QByteArray& data, Format format,
                        qint64 maxDecompressedBytes)
{
    data.clear();
    if (Outcome refused = validateEntryName(fileName); !refused) {
        return refused;
    }
    const QString what = QStringLiteral("Could not extract %1 from %2").arg(fileName, archivePath);
    return archiveOperation(archivePath, format, what, [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from" << archivePath << "to memory";
        return QGClibarchive::extractFileToMemory(reader, archivePath, fileName, maxDecompressedBytes, data);
    });
}

Outcome extractFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath,
                     Format format)
{
    if (fileNames.isEmpty()) {
        return {};
    }
    const QString what = QStringLiteral("Could not extract all requested files from ") + archivePath;
    return selectedEntriesOperation(archivePath, format, what, [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Extracting" << fileNames.size() << "files from" << archivePath;
        return QGClibarchive::extractMultipleFiles(reader, archivePath, fileNames, outputDirectoryPath);
    });
}

Outcome extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                         QStringList* extractedFiles, Format format)
{
    if (patterns.isEmpty()) {
        return {.error = Error::FileNotInArchive, .message = QStringLiteral("No patterns provided")};
    }
    const QString what = QStringLiteral("Could not extract files that match the patterns from ") + archivePath;
    return selectedEntriesOperation(archivePath, format, what, [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Extracting files matching patterns" << patterns << "from" << archivePath;
        return QGClibarchive::extractByPattern(reader, archivePath, patterns, outputDirectoryPath, extractedFiles);
    });
}

// ============================================================================
// QIODevice-based Operations
// ============================================================================

Outcome decompressFromDevice(QIODevice* device, const QString& outputPath, ProgressCallback progress,
                             qint64 maxDecompressedBytes)
{
    return deviceOperation(device, QStringLiteral("Failed to decompress from device"), [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Decompressing from device to" << outputPath;
        return QGClibarchive::decompressSingleFile(reader, device, outputPath, progress, maxDecompressedBytes);
    });
}

Outcome decompressFromDevice(QIODevice* device, QByteArray& data, qint64 maxDecompressedBytes)
{
    data.clear();
    return deviceOperation(device, QStringLiteral("Failed to decompress from device"), [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Decompressing from device to memory";
        return QGClibarchive::decompressToMemory(reader, device, maxDecompressedBytes, data);
    });
}

Outcome extractFromDevice(QIODevice* device, const QString& outputDirectoryPath, ProgressCallback progress,
                          qint64 maxDecompressedBytes)
{
    return deviceOperation(device, QStringLiteral("Failed to extract archive from device"), [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Extracting archive from device to" << outputDirectoryPath;
        return QGClibarchive::extractAnyArchive(reader, device, outputDirectoryPath, {}, progress,
                                                maxDecompressedBytes);
    });
}

Outcome extractFileDataFromDevice(QIODevice* device, const QString& fileName, QByteArray& data,
                                  qint64 maxDecompressedBytes)
{
    data.clear();
    Outcome validated = validateDeviceInput(device);
    if (validated) {
        validated = validateEntryName(fileName);
    }
    if (!validated) {
        return validated;
    }
    return runOperation(QStringLiteral("Could not extract ") + fileName, [&](auto& reader) {
        qCDebug(QGCCompressionLog) << "Extracting" << fileName << "from device to memory";
        return QGClibarchive::extractFileToMemory(reader, device, fileName, maxDecompressedBytes, data);
    });
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
        if (!decompressData(data, jsonData, Format::Auto, maxDecompressedBytes) || jsonData.isEmpty()) {
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
