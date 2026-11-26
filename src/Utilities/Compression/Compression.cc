#include "Compression.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "ArchiveExtraction.h"
#include "Formats.h"
#include "Operation.h"
#include "QGCLoggingCategory.h"
#include "StreamDecompression.h"

QGC_LOGGING_CATEGORY(CompressionLog, "Utilities.Compression")

namespace Compression {

namespace {

/// Why an extraction may not write into @a outputDirectoryPath, as the end of its failure message
/// @return Empty if the path does not exist or is an empty directory
QString unusableOutputDirectory(const QString& outputDirectoryPath)
{
    const QFileInfo output(outputDirectoryPath);
    if (outputDirectoryPath.isEmpty() || (output.isSymLink() && !output.exists()) ||
        (output.exists() && !output.isDir())) {
        qCDebug(CompressionLog) << "Output path is not a directory:" << outputDirectoryPath;
        return QCoreApplication::translate("Compression", ": the output path %1 is not a directory",
                                           "Ends a message that tells what failed")
            .arg(outputDirectoryPath);
    }
    if (!output.exists()) {
        return {};
    }
    // A directory that cannot be listed may hold files that an entry would replace
    if (!output.isReadable()) {
        qCDebug(CompressionLog) << "Output directory cannot be read:" << outputDirectoryPath;
        return QCoreApplication::translate("Compression", ": the output directory %1 cannot be read",
                                           "Ends a message that tells what failed")
            .arg(outputDirectoryPath);
    }
    if (!QDir(outputDirectoryPath).isEmpty(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)) {
        qCDebug(CompressionLog) << "Output directory is not empty:" << outputDirectoryPath;
        return QCoreApplication::translate("Compression", ": the output directory %1 is not empty",
                                           "Ends a message that tells what failed")
            .arg(outputDirectoryPath);
    }
    return {};
}

/// Output path when the caller gives none: @a filePath without its compression extension, or with ".decompressed"
/// appended if it has none, so that the output is never the input. A name that is only the extension (".gz"), or
/// dots and the extension ("..gz"), also gets ".decompressed": without it, the path would be a directory
QString defaultOutputPath(const QString& filePath)
{
    const qsizetype suffixSize = compressionSuffixSize(filePath);
    const QString name = QFileInfo(filePath.chopped(suffixSize)).fileName();
    if ((suffixSize > 0) && !name.isEmpty() && (name != QLatin1String(".")) && (name != QLatin1String(".."))) {
        return filePath.chopped(suffixSize);
    }
    return filePath + QStringLiteral(".decompressed");
}

}  // namespace

// ============================================================================
// Error Handling
// ============================================================================

QString errorName(Error error)
{
    switch (error) {
        case Error::None:
            return QCoreApplication::translate("Compression", "No error");
        case Error::FileNotFound:
            return QCoreApplication::translate("Compression", "File not found");
        case Error::PermissionDenied:
            return QCoreApplication::translate("Compression", "Permission denied");
        case Error::InvalidArchive:
            return QCoreApplication::translate("Compression", "Invalid or corrupt archive");
        case Error::UnsupportedFormat:
            return QCoreApplication::translate("Compression", "Unsupported format");
        case Error::SizeLimitExceeded:
            return QCoreApplication::translate("Compression", "Size limit exceeded");
        case Error::Cancelled:
            return QCoreApplication::translate("Compression", "Operation cancelled");
        case Error::IoError:
            return QCoreApplication::translate("Compression", "I/O error");
        case Error::InternalError:
            return QCoreApplication::translate("Compression", "Internal error");
        case Error::DiskSpaceExceeded:
            return QCoreApplication::translate("Compression", "Not enough disk space");
    }
    return QCoreApplication::translate("Compression", "Unknown error");
}

// ============================================================================
// Kind of a File
// ============================================================================

bool isCompressedFile(const QString& filePath)
{
    return kindOfFile(filePath) == Kind::Stream;
}

bool isArchiveFile(const QString& filePath)
{
    return kindOfFile(filePath) == Kind::Archive;
}

QStringList archiveNamePatterns()
{
    return builtArchiveNamePatterns();
}

QString strippedPath(const QString& filePath)
{
    return isCompressedFile(filePath) ? defaultOutputPath(filePath) : filePath;
}

// ============================================================================
// Single-File Decompression
// ============================================================================

Outcome decompressFile(const QString& inputPath, const QString& outputPath, ProgressCallback progress,
                       qint64 maxDecompressedBytes)
{
    if (Outcome refused = validateFileInput(inputPath); !refused) {
        return refused;
    }

    const QString actualOutput = outputPath.isEmpty() ? defaultOutputPath(inputPath) : outputPath;
    if (isSameFile(inputPath, actualOutput)) {
        qCDebug(CompressionLog) << "The output is the input file:" << inputPath;
        return {
            .error = Error::IoError,
            .message =
                QCoreApplication::translate("Compression", "Cannot decompress a file onto itself: %1").arg(inputPath)};
    }

    qCDebug(CompressionLog) << "Decompressing" << inputPath << "to" << actualOutput;
    return operationOutcome(LibArchive::decompressToFile(inputPath, actualOutput, progress, maxDecompressedBytes),
                            QCoreApplication::translate("Compression", "Decompression failed: %1").arg(inputPath));
}

Outcome readFile(const QString& filePath, QByteArray& data, qint64 maxDecompressedBytes)
{
    data.clear();
    if (Outcome refused = validateFileInput(filePath); !refused) {
        return refused;
    }
    return operationOutcome(LibArchive::decompressToMemory(filePath, maxDecompressedBytes, data),
                            QCoreApplication::translate("Compression", "Failed to read: %1").arg(filePath));
}

Outcome decompressData(const QByteArray& data, QByteArray& decompressed, qint64 maxDecompressedBytes)
{
    return operationOutcome(LibArchive::decompressToMemory(data, maxDecompressedBytes, decompressed),
                            QCoreApplication::translate("Compression", "Failed to decompress data"));
}

// ============================================================================
// Archive Extraction
// ============================================================================

Outcome extractArchive(const QString& archivePath, const QString& outputDirectoryPath, const ExtractOptions& options)
{
    if (Outcome refused = validateFileInput(archivePath); !refused) {
        return refused;
    }

    // Every step takes the directory by this one path. Until "missing" exists, the system finds no file at
    // "full/missing/..": the directory would pass as new, and after a failure "full" would be removed as created
    const QString outputPath =
        outputDirectoryPath.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(outputDirectoryPath).absoluteFilePath());

    const QString what = QCoreApplication::translate("Compression", "Failed to extract archive: %1").arg(archivePath);
    if (const QString unusable = unusableOutputDirectory(outputPath); !unusable.isEmpty()) {
        return {.error = Error::IoError, .message = what + unusable};
    }

    const LibArchive::Result extracted = LibArchive::extractArchive(archivePath, outputPath, options);
    // libarchive cannot tell an archive of a format it was built without from a file that is no archive
    if (extracted.unrecognized && isFormatNotBuilt(archivePath)) {
        return {.error = Error::UnsupportedFormat,
                .message = what + QCoreApplication::translate("Compression",
                                                              ": the archive format is not supported by this build",
                                                              "Ends a message that tells what failed"),
                .detail = extracted.detail};
    }
    return operationOutcome(extracted, what);
}

}  // namespace Compression
