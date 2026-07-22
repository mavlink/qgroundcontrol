#include "QGClibarchive.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QSet>
#include <QtCore/QTemporaryDir>
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <limits>

#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGClibarchiveLog, "Utilities.QGClibarchive")

// ============================================================================
// Thread-Local Format Detection State
// ============================================================================

namespace {

thread_local QGClibarchive::OperationResult t_lastOperationResult = QGClibarchive::OperationResult::Success;

struct FormatState
{
    QString formatName;
    QString filterName;
};

thread_local FormatState t_formatState;

void updateFormatState(struct archive* a)
{
    if (!a) {
        t_formatState.formatName.clear();
        t_formatState.filterName.clear();
        return;
    }

    // Get format name
    const char* fmt = archive_format_name(a);
    t_formatState.formatName = fmt ? QString::fromUtf8(fmt) : QString();

    // Get compression filter name (index 0 is the outermost filter)
    const char* flt = archive_filter_name(a, 0);
    t_formatState.filterName = flt ? QString::fromUtf8(flt) : QStringLiteral("none");

    if (!t_formatState.formatName.isEmpty()) {
        qCDebug(QGClibarchiveLog) << "Detected format:" << t_formatState.formatName
                                  << "filter:" << t_formatState.filterName;
    }
}

}  // namespace

namespace QGClibarchive {

OperationResult lastOperationResult()
{
    return t_lastOperationResult;
}

QString lastDetectedFormatName()
{
    return t_formatState.formatName;
}

QString lastDetectedFilterName()
{
    return t_formatState.filterName;
}

}  // namespace QGClibarchive

// ============================================================================
// libarchive callbacks for QIODevice streaming
// ============================================================================

namespace QGClibarchive {

la_ssize_t deviceReadCallback(struct archive*, void* clientData, const void** buffer)
{
    static thread_local char readBuffer[QGCFileHelper::kBufferSizeMax];
    auto* device = static_cast<QIODevice*>(clientData);

    const qint64 bytesRead = device->read(readBuffer, sizeof(readBuffer));
    if (bytesRead < 0) {
        return ARCHIVE_FATAL;
    }

    *buffer = readBuffer;
    return static_cast<la_ssize_t>(bytesRead);
}

la_int64_t deviceSkipCallback(struct archive*, void* clientData, la_int64_t request)
{
    auto* device = static_cast<QIODevice*>(clientData);

    if (!device->isSequential()) {
        const qint64 currentPos = device->pos();
        const qint64 newPos = currentPos + request;
        if (device->seek(newPos)) {
            return request;
        }
    }

    // For sequential devices or failed seek, read and discard
    char discardBuffer[QGCFileHelper::kBufferSizeMax];
    la_int64_t skipped = 0;
    while (skipped < request) {
        const qint64 toRead = qMin(static_cast<qint64>(sizeof(discardBuffer)), request - skipped);
        const qint64 bytesRead = device->read(discardBuffer, toRead);
        if (bytesRead <= 0)
            break;
        skipped += bytesRead;
    }
    return skipped;
}

int deviceCloseCallback(struct archive*, void*)
{
    // Don't close the device - caller owns it
    return ARCHIVE_OK;
}

la_int64_t deviceSeekCallback(struct archive*, void* clientData, la_int64_t offset, int whence)
{
    auto* device = static_cast<QIODevice*>(clientData);

    if (device->isSequential()) {
        return ARCHIVE_FATAL;
    }

    qint64 newPos;
    switch (whence) {
        case SEEK_SET:
            newPos = offset;
            break;
        case SEEK_CUR:
            newPos = device->pos() + offset;
            break;
        case SEEK_END:
            newPos = device->size() + offset;
            break;
        default:
            return ARCHIVE_FATAL;
    }

    if (!device->seek(newPos)) {
        return ARCHIVE_FATAL;
    }

    return newPos;
}

// ============================================================================
// Utility Functions
// ============================================================================

ArchiveEntry toArchiveEntry(struct archive_entry* entry)
{
    ArchiveEntry info;
    info.name = QString::fromUtf8(archive_entry_pathname(entry));
    info.size = archive_entry_size(entry);
    info.isDirectory = (archive_entry_filetype(entry) == AE_IFDIR);
    info.permissions = static_cast<quint32>(archive_entry_perm(entry));

    const time_t mtime = archive_entry_mtime(entry);
    if (mtime > 0) {
        info.modified = QDateTime::fromSecsSinceEpoch(mtime);
    }

    return info;
}

}  // namespace QGClibarchive

// ============================================================================
// Internal helpers (anonymous namespace)
// ============================================================================

namespace {

/// Read all data from archive into QByteArray
/// @param a Archive handle (must have header already read)
/// @param expectedSize Expected size for pre-allocation (0 if unknown)
/// @param maxBytes Maximum bytes to read (0 = unlimited)
/// @return Data read, or empty QByteArray on error or size limit exceeded
QByteArray readArchiveToMemory(struct archive* a, qint64 expectedSize = 0, qint64 maxBytes = 0)
{
    t_lastOperationResult = QGClibarchive::OperationResult::Failed;
    QByteArray result;
    if (expectedSize > 0) {
        if ((maxBytes > 0) && (expectedSize > maxBytes)) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            t_lastOperationResult = QGClibarchive::OperationResult::SizeLimitExceeded;
            return {};
        }
        if (expectedSize <= (std::numeric_limits<qsizetype>::max)()) {
        result.reserve(static_cast<qsizetype>(expectedSize));
        }
    }

    char buffer[QGCFileHelper::kBufferSizeMax];
    la_ssize_t size;
    while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
        if ((maxBytes > 0) && (size > (maxBytes - result.size()))) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            t_lastOperationResult = QGClibarchive::OperationResult::SizeLimitExceeded;
            return {};
        }
        result.append(buffer, static_cast<qsizetype>(size));
    }

    if (size < 0) {
        qCWarning(QGClibarchiveLog) << "Read error:" << archive_error_string(a);
        return {};
    }

    t_lastOperationResult = QGClibarchive::OperationResult::Success;
    return result;
}

QString canonicalExtractionRoot(const QString& outputDirectoryPath)
{
    QString root = QFileInfo(outputDirectoryPath).canonicalFilePath();
    if (root.isEmpty()) {
        root = QFileInfo(outputDirectoryPath).absoluteFilePath();
    }
    return QDir::cleanPath(root);
}

bool isWithinExtractionRoot(const QString& root, const QString& path)
{
    const QString relative = QDir::fromNativeSeparators(QDir(root).relativeFilePath(path));
    return !QDir::isAbsolutePath(relative) && (relative != QLatin1String("..")) &&
           !relative.startsWith(QLatin1String("../"));
}

bool resolveSecureExtractionPath(const QString& root, const QString& entryName, QString& outputPath)
{
    if (entryName.isEmpty() || entryName.contains(QChar('\0'))) {
        qCWarning(QGClibarchiveLog) << "Rejecting empty path or path with embedded null byte";
        return false;
    }

    const QString normalizedName = QDir::fromNativeSeparators(entryName);
    if (QDir::isAbsolutePath(normalizedName)) {
        qCWarning(QGClibarchiveLog) << "Rejecting absolute archive path:" << entryName;
        return false;
    }

    const QString cleanName = QDir::cleanPath(normalizedName);
    if ((cleanName == QLatin1String(".")) || (cleanName == QLatin1String("..")) ||
        cleanName.startsWith(QLatin1String("../"))) {
        qCWarning(QGClibarchiveLog) << "Rejecting archive path traversal:" << entryName;
        return false;
    }

    outputPath = QDir(root).absoluteFilePath(cleanName);
    if (!isWithinExtractionRoot(root, outputPath)) {
        qCWarning(QGClibarchiveLog) << "Rejecting archive path outside extraction root:" << entryName;
        return false;
    }

    QString currentPath = root;
    const QStringList components = cleanName.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString& component : components) {
        currentPath = QDir(currentPath).filePath(component);
        const QFileInfo componentInfo(currentPath);
        if (componentInfo.isSymLink()) {
            qCWarning(QGClibarchiveLog) << "Rejecting archive path through symlink:" << entryName;
            return false;
        }
    }

    return true;
}

struct SelectionCommitEntry
{
    QString stagedPath;
    QString outputPath;
    QString backupPath;
    bool hadExistingOutput = false;
    bool committed = false;
};

bool commitStagedSelection(const QString& stagingRoot, const QString& outputDirectoryPath)
{
    const QFileInfo outputInfo(outputDirectoryPath);
    if (outputInfo.isSymLink() || (outputInfo.exists() && !outputInfo.isDir())) {
        qCWarning(QGClibarchiveLog) << "Selective extraction output is not a regular directory:" << outputDirectoryPath;
        return false;
    }

    const bool createdOutputRoot = !outputInfo.exists();
    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }

    const QString outputRoot = canonicalExtractionRoot(outputDirectoryPath);
    const QString outputParent = QFileInfo(outputRoot).absolutePath();
    const QString outputName = QFileInfo(outputRoot).fileName();
    QTemporaryDir backupDirectory(
        QGCFileHelper::joinPath(outputParent, QStringLiteral(".%1.qgc_select_backup_XXXXXX").arg(outputName)));
    if (!backupDirectory.isValid()) {
        if (createdOutputRoot) {
            (void) QDir(outputRoot).removeRecursively();
        }
        qCWarning(QGClibarchiveLog) << "Failed to create selective extraction backup directory:"
                                    << backupDirectory.errorString();
        return false;
    }

    QList<SelectionCommitEntry> entries;
    QDirIterator iterator(stagingRoot, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString stagedPath = iterator.next();
        const QFileInfo stagedInfo(stagedPath);
        if (stagedInfo.isSymLink()) {
            qCWarning(QGClibarchiveLog) << "Refusing to commit staged symlink:" << stagedPath;
            if (createdOutputRoot) {
                (void) QDir(outputRoot).removeRecursively();
            }
            return false;
        }

        const QString relativePath = QDir::fromNativeSeparators(QDir(stagingRoot).relativeFilePath(stagedPath));
        QString outputPath;
        if (!resolveSecureExtractionPath(outputRoot, relativePath, outputPath)) {
            if (createdOutputRoot) {
                (void) QDir(outputRoot).removeRecursively();
            }
            return false;
        }

        const QFileInfo destinationInfo(outputPath);
        if (destinationInfo.isSymLink() || (destinationInfo.exists() && !destinationInfo.isFile())) {
            qCWarning(QGClibarchiveLog) << "Refusing to replace non-regular extraction target:" << outputPath;
            if (createdOutputRoot) {
                (void) QDir(outputRoot).removeRecursively();
            }
            return false;
        }

        entries.append({stagedPath, outputPath, QDir(backupDirectory.path()).filePath(relativePath),
                        destinationInfo.exists(), false});
    }

    QStringList createdDirectories;
    const auto rollback = [&entries, &createdDirectories, createdOutputRoot, &outputRoot]() {
        bool restored = true;
        for (auto iterator = entries.rbegin(); iterator != entries.rend(); ++iterator) {
            if (iterator->committed && !QFile::remove(iterator->outputPath)) {
                    qCWarning(QGClibarchiveLog)
                    << "Failed to remove selective extraction target during rollback:" << iterator->outputPath;
                restored = false;
            }
            if (iterator->hadExistingOutput && QFileInfo::exists(iterator->backupPath)) {
                if (!QGCFileHelper::ensureParentExists(iterator->outputPath) ||
                    !QFile::rename(iterator->backupPath, iterator->outputPath)) {
                    qCWarning(QGClibarchiveLog)
                        << "Failed to restore selective extraction backup:" << iterator->outputPath;
                    restored = false;
                }
            }
        }
        if (createdOutputRoot) {
            if (!QDir(outputRoot).removeRecursively()) {
                    qCWarning(QGClibarchiveLog)
                    << "Failed to remove selective extraction output during rollback:" << outputRoot;
                restored = false;
            }
        } else {
            for (auto iterator = createdDirectories.crbegin(); iterator != createdDirectories.crend(); ++iterator) {
                (void) QDir().rmdir(*iterator);
            }
        }
        return restored;
};
    const auto rollbackAndPreserveBackups = [&rollback, &backupDirectory]() {
        if (!rollback()) {
            backupDirectory.setAutoRemove(false);
            qCWarning(QGClibarchiveLog) << "Selective extraction recovery files retained at:" << backupDirectory.path();
        }
};

    for (SelectionCommitEntry& entry : entries) {
        QString currentDirectory = QFileInfo(entry.outputPath).absolutePath();
        QStringList missingDirectories;
        while (!QFileInfo::exists(currentDirectory) && isWithinExtractionRoot(outputRoot, currentDirectory)) {
            missingDirectories.prepend(currentDirectory);
            currentDirectory = QFileInfo(currentDirectory).absolutePath();
        }
        if (!QGCFileHelper::ensureParentExists(entry.outputPath)) {
            rollbackAndPreserveBackups();
            return false;
        }
        createdDirectories.append(missingDirectories);

        if (entry.hadExistingOutput) {
            if (!QGCFileHelper::ensureParentExists(entry.backupPath) ||
                !QFile::rename(entry.outputPath, entry.backupPath)) {
                qCWarning(QGClibarchiveLog) << "Failed to back up selective extraction target:" << entry.outputPath;
                rollbackAndPreserveBackups();
                return false;
            }
        }
        if (!QFile::rename(entry.stagedPath, entry.outputPath)) {
            qCWarning(QGClibarchiveLog) << "Failed to commit selective extraction target:" << entry.outputPath;
            rollbackAndPreserveBackups();
            return false;
        }
        entry.committed = true;
    }

    return true;
}

template <typename Extractor>
bool extractSelectionAtomically(const QString& outputDirectoryPath, Extractor extractor)
{
    const QFileInfo outputInfo(outputDirectoryPath);
    const QString outputName = outputInfo.fileName();
    const QString outputParent = outputInfo.absolutePath();
    if (outputName.isEmpty() || !QGCFileHelper::ensureDirectoryExists(outputParent)) {
        qCWarning(QGClibarchiveLog) << "Invalid selective extraction output directory:" << outputDirectoryPath;
        t_lastOperationResult = QGClibarchive::OperationResult::Failed;
        return false;
    }

    QTemporaryDir stagingDirectory(
        QGCFileHelper::joinPath(outputParent, QStringLiteral(".%1.qgc_select_XXXXXX").arg(outputName)));
    if (!stagingDirectory.isValid()) {
        qCWarning(QGClibarchiveLog) << "Failed to create selective extraction staging directory:"
                                    << stagingDirectory.errorString();
        t_lastOperationResult = QGClibarchive::OperationResult::Failed;
        return false;
    }

    if (!extractor(stagingDirectory.path())) {
        return false;
    }

    t_lastOperationResult = QGClibarchive::OperationResult::Failed;
    if (!commitStagedSelection(stagingDirectory.path(), outputDirectoryPath)) {
        return false;
    }

    t_lastOperationResult = QGClibarchive::OperationResult::Success;
    return true;
}

/// Write current archive entry data to a file atomically
/// @param a Archive handle (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @return true on success, false on read/write error (no partial file left on failure)
/// @note Uses QSaveFile for atomic writes - writes to temp file, then renames on commit
/// @note Uses archive_read_data_block() with seeking to preserve sparse file structure
bool writeArchiveEntryToFile(struct archive* a, const QString& outputPath,
                             const QGClibarchive::ProgressCallback& progress = nullptr, qint64 totalSize = 0,
                             qint64 maxBytes = 0, qint64* aggregateBytes = nullptr, qint64 declaredSize = -1)
{
    t_lastOperationResult = QGClibarchive::OperationResult::Failed;
    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCWarning(QGClibarchiveLog) << "Failed to open output file:" << outputPath << outFile.errorString();
        return false;
    }

    const void* buff;
    size_t size;
    la_int64_t offset;
    int r;
    const qint64 aggregateBase = aggregateBytes ? *aggregateBytes : 0;
    qint64 logicalFileSize = (std::max) (declaredSize, qint64{0});

    if ((maxBytes > 0) && ((aggregateBase > maxBytes) || (logicalFileSize > (maxBytes - aggregateBase)))) {
        qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
        outFile.cancelWriting();
        t_lastOperationResult = QGClibarchive::OperationResult::SizeLimitExceeded;
        return false;
    }

    while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
        if ((offset < 0) || (size > static_cast<size_t>((std::numeric_limits<qint64>::max)())) ||
            (static_cast<quint64>(offset) + static_cast<quint64>(size) >
             static_cast<quint64>((std::numeric_limits<qint64>::max)()))) {
            qCWarning(QGClibarchiveLog) << "Invalid archive data block range:" << offset << size;
            outFile.cancelWriting();
            return false;
        }

        const qint64 blockEnd = offset + static_cast<qint64>(size);
        const qint64 newLogicalFileSize = (std::max) (logicalFileSize, blockEnd);
        if ((maxBytes > 0) && ((aggregateBase > maxBytes) || (newLogicalFileSize > (maxBytes - aggregateBase)))) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            outFile.cancelWriting();
            t_lastOperationResult = QGClibarchive::OperationResult::SizeLimitExceeded;
            return false;
        }

        // Seek to offset - creates sparse hole if gap from previous position
        if (!outFile.seek(offset)) {
            qCWarning(QGClibarchiveLog) << "Failed to seek:" << outFile.errorString();
            outFile.cancelWriting();
            return false;
        }
        if (outFile.write(static_cast<const char*>(buff), static_cast<qint64>(size)) != static_cast<qint64>(size)) {
            qCWarning(QGClibarchiveLog) << "Failed to write output:" << outFile.errorString();
            outFile.cancelWriting();
            return false;
        }
        logicalFileSize = newLogicalFileSize;
        if (progress && !progress(archive_filter_bytes(a, -1), totalSize)) {
            qCDebug(QGClibarchiveLog) << "Extraction cancelled by user";
            outFile.cancelWriting();
            t_lastOperationResult = QGClibarchive::OperationResult::Cancelled;
            return false;
        }
    }

    if (r != ARCHIVE_EOF) {
        qCWarning(QGClibarchiveLog) << "Read error:" << archive_error_string(a);
        outFile.cancelWriting();
        return false;
    }

    if (!outFile.resize(logicalFileSize)) {
        qCWarning(QGClibarchiveLog) << "Failed to resize sparse output:" << outFile.errorString();
        outFile.cancelWriting();
        return false;
    }

    // Atomically commit - renames temp file to final path
    if (!outFile.commit()) {
        qCWarning(QGClibarchiveLog) << "Failed to commit output file:" << outFile.errorString();
        return false;
    }

    if (aggregateBytes) {
        *aggregateBytes = aggregateBase + logicalFileSize;
    }
    t_lastOperationResult = QGClibarchive::OperationResult::Success;
    return true;
}

/// Decompress archive stream to file atomically with progress and size limit
/// @param a Archive handle (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @param progress Optional progress callback
/// @param totalSize Total size for progress reporting (0 if unknown)
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @return Typed result; failures leave no partial file
QGClibarchive::OperationResult decompressStreamToFile(struct archive* a, const QString& outputPath,
                                                      const QGClibarchive::ProgressCallback& progress, qint64 totalSize,
                                                      qint64 maxBytes)
{
    t_lastOperationResult = QGClibarchive::OperationResult::Failed;
    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCWarning(QGClibarchiveLog) << "Failed to open output file:" << outputPath << outFile.errorString();
        return QGClibarchive::OperationResult::Failed;
    }

    qint64 totalBytesWritten = 0;
    char buffer[QGCFileHelper::kBufferSizeMax];
    la_ssize_t size;

    while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
        if (maxBytes > 0 && size > (maxBytes - totalBytesWritten)) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            outFile.cancelWriting();
            t_lastOperationResult = QGClibarchive::OperationResult::SizeLimitExceeded;
            return t_lastOperationResult;
        }

        if (outFile.write(buffer, size) != size) {
            qCWarning(QGClibarchiveLog) << "Failed to write output:" << outFile.errorString();
            outFile.cancelWriting();
            return QGClibarchive::OperationResult::Failed;
        }
        totalBytesWritten += size;

        if (progress) {
            const qint64 bytesRead = archive_filter_bytes(a, -1);
            if (!progress(bytesRead, totalSize)) {
                qCDebug(QGClibarchiveLog) << "Decompression cancelled by user";
                outFile.cancelWriting();
                t_lastOperationResult = QGClibarchive::OperationResult::Cancelled;
                return t_lastOperationResult;
            }
        }
    }

    if (size < 0) {
        qCWarning(QGClibarchiveLog) << "Decompression error:" << archive_error_string(a);
        outFile.cancelWriting();
        return QGClibarchive::OperationResult::Failed;
    }

    if (!outFile.commit()) {
        qCWarning(QGClibarchiveLog) << "Failed to commit output file:" << outFile.errorString();
        return QGClibarchive::OperationResult::Failed;
    }

    qCDebug(QGClibarchiveLog) << "Decompressed" << totalBytesWritten << "bytes to" << outputPath;
    t_lastOperationResult = QGClibarchive::OperationResult::Success;
    return t_lastOperationResult;
}

/// Remove only files/directories created during extraction (safe cleanup)
/// @param createdFiles Files created during extraction (removed first)
/// @param createdDirs Directories created during extraction (removed in reverse order, only if empty)
void cleanupCreatedEntries(const QStringList& createdFiles, const QStringList& createdDirs)
{
    // Remove files first
    for (const QString& file : createdFiles) {
        QFile::remove(file);
    }

    // Remove directories in reverse order (deepest first), only if empty
    for (auto it = createdDirs.crbegin(); it != createdDirs.crend(); ++it) {
        QDir dir(*it);
        if (dir.isEmpty()) {
            dir.rmdir(*it);
        }
    }
}

/// Track directories that need to be created for a path
/// @param path Target file path
/// @param outputDir Base output directory (won't be tracked)
/// @param existingDirs Set of directories known to exist (updated)
/// @param createdDirs List of newly created directories (appended to)
void trackAndCreateParentDirs(const QString& path, const QString& outputDir, QSet<QString>& existingDirs,
                              QStringList& createdDirs)
{
    QStringList dirsToCreate;
    QString current = QFileInfo(path).absolutePath();

    // Walk up the tree finding directories that don't exist
    while (current != outputDir && !current.isEmpty() && !existingDirs.contains(current)) {
        if (!QDir(current).exists()) {
            dirsToCreate.prepend(current);
        } else {
            existingDirs.insert(current);
            break;
        }
        current = QFileInfo(current).absolutePath();
    }

    // Create directories and track them
    for (const QString& dir : dirsToCreate) {
        if (QDir().mkdir(dir)) {
            createdDirs.append(dir);
            existingDirs.insert(dir);
        }
    }
}

/// Extract entries from an open archive to a directory
/// @param a Archive reader (already opened with format support configured)
/// @param outputDirectoryPath Directory to extract to (must exist)
/// @param progress Optional progress callback
/// @param totalSize Total archive size for progress (0 if unknown)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @return true on success
bool extractArchiveEntries(struct archive* a, const QString& outputDirectoryPath,
                           const QGClibarchive::ProgressCallback& progress, qint64 totalSize, qint64 maxBytes)
{
    t_lastOperationResult = QGClibarchive::OperationResult::Failed;
    struct archive* ext = archive_write_disk_new();
    if (!ext) {
    archive_read_close(a);
        archive_read_free(a);
        return false;
    }
    // Security flags:
    // - ARCHIVE_EXTRACT_TIME: Restore file modification times
    // - ARCHIVE_EXTRACT_SECURE_NODOTDOT: Reject paths containing ".."
    // - ARCHIVE_EXTRACT_SECURE_SYMLINKS: Don't follow symlinks when setting perms
    // Note: We don't use ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS because we set
    // absolute output paths manually (after validating the original entry path)
    archive_write_disk_set_options(
        ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS);
    archive_write_disk_set_standard_lookup(ext);

    bool success = true;
    bool cancelled = false;
    bool sizeLimitExceeded = false;
    qint64 totalBytesWritten = 0;
    struct archive_entry* entry;

    // Track created entries for safe cleanup on failure
    QStringList createdFiles;
    QStringList createdDirs;
    QSet<QString> existingDirs;

    // Resolve the extraction root to a canonical path when possible so platforms
    // with symlinked temp roots (for example, /var -> /private/var on macOS)
    // do not trip libarchive's secure symlink checks for parent directories.
    const QString canonicalOutputDir = canonicalExtractionRoot(outputDirectoryPath);
    existingDirs.insert(canonicalOutputDir);

    bool formatLogged = false;
    int headerResult = ARCHIVE_OK;
    while ((headerResult = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        // Log detected format after first header read
        if (!formatLogged) {
            updateFormatState(a);
            formatLogged = true;
        }

        const char* currentFile = archive_entry_pathname(entry);
        const QString entryName = QString::fromUtf8(currentFile);
        QString outputPath;
        if (!resolveSecureExtractionPath(canonicalOutputDir, entryName, outputPath)) {
            continue;
        }
        const QString canonicalOutput = QFileInfo(outputPath).absoluteFilePath();

        // Security: validate symlink targets don't escape output directory
        const auto fileType = archive_entry_filetype(entry);
        if (fileType == AE_IFLNK) {
            const char* symlinkTarget = archive_entry_symlink(entry);
            if (symlinkTarget) {
                QString target = QString::fromUtf8(symlinkTarget);
                QString resolvedTarget;

                if (QFileInfo(target).isAbsolute()) {
                    // Absolute symlink - reject outright
                    qCWarning(QGClibarchiveLog)
                        << "Skipping symlink with absolute target:" << currentFile << "->" << target;
                    continue;
                } else {
                    // Relative symlink - resolve against entry's parent directory
                    QString entryDir = QFileInfo(outputPath).absolutePath();
                    resolvedTarget = QFileInfo(entryDir + "/" + target).absoluteFilePath();
                }

                // Verify resolved target stays within output directory
                if (!isWithinExtractionRoot(canonicalOutputDir, resolvedTarget)) {
                    qCWarning(QGClibarchiveLog)
                        << "Skipping symlink escaping output directory:" << currentFile << "->" << symlinkTarget;
                    continue;
                }
            }
        }

        // Security: mask permissions for cross-platform safety
        // Files: 0644 (rw-r--r--), Directories: 0755 (rwxr-xr-x)
        if (fileType == AE_IFDIR) {
            archive_entry_set_perm(entry, 0755);
        } else if (fileType == AE_IFREG) {
            archive_entry_set_perm(entry, 0644);
        }
        // Note: symlinks don't have meaningful permissions on most systems

        const qint64 declaredSize = archive_entry_size(entry);
        if ((fileType == AE_IFREG) && (maxBytes > 0) && (declaredSize >= 0) &&
            ((totalBytesWritten > maxBytes) || (declaredSize > (maxBytes - totalBytesWritten)))) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
                    sizeLimitExceeded = true;
            success = false;
            break;
        }

        const bool outputExisted = QFileInfo::exists(canonicalOutput);
        archive_entry_set_pathname(entry, outputPath.toUtf8().constData());

        // Create parent directories and track newly created ones
        trackAndCreateParentDirs(outputPath, canonicalOutputDir, existingDirs, createdDirs);

        int r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to write header for" << outputPath << ":"
                                        << archive_error_string(ext);
            success = false;
            break;
        }

        // Track created file/directory for cleanup
        if (fileType == AE_IFDIR) {
            if (!existingDirs.contains(canonicalOutput)) {
                createdDirs.append(canonicalOutput);
                existingDirs.insert(canonicalOutput);
            }
        } else if (!outputExisted) {
            createdFiles.append(canonicalOutput);
        }

        qint64 entryLogicalSize = (fileType == AE_IFREG) && (declaredSize > 0) ? declaredSize : 0;
        if (declaredSize > 0) {
            const void* buff;
            size_t size;
            la_int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if ((offset < 0) || (size > static_cast<size_t>((std::numeric_limits<qint64>::max)())) ||
                    (static_cast<quint64>(offset) + static_cast<quint64>(size) >
                     static_cast<quint64>((std::numeric_limits<qint64>::max)()))) {
                    qCWarning(QGClibarchiveLog) << "Invalid archive data block range:" << offset << size;
                    success = false;
                    break;
                }

                const qint64 blockEnd = offset + static_cast<qint64>(size);
                const qint64 newLogicalSize = (std::max) (entryLogicalSize, blockEnd);
                if ((maxBytes > 0) &&
                    ((totalBytesWritten > maxBytes) || (newLogicalSize > (maxBytes - totalBytesWritten)))) {
                    qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
                    sizeLimitExceeded = true;
                    success = false;
                    break;
                }

                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    qCWarning(QGClibarchiveLog) << "Failed to write data:" << archive_error_string(ext);
                    success = false;
                    break;
                }

                entryLogicalSize = newLogicalSize;

                // Report progress based on compressed bytes read
                if (progress) {
                    const qint64 compressedBytesRead = archive_filter_bytes(a, -1);
                    if (!progress(compressedBytesRead, totalSize)) {
                        qCDebug(QGClibarchiveLog) << "Extraction cancelled by user";
                        cancelled = true;
                        success = false;
                        break;
                    }
                }
            }

            if (!success)
                break;

            if (r != ARCHIVE_EOF && r != ARCHIVE_OK) {
                qCWarning(QGClibarchiveLog) << "Failed to read data:" << archive_error_string(a);
                success = false;
                break;
            }
        }

        if (archive_write_finish_entry(ext) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to finish entry:" << archive_error_string(ext);
            success = false;
            break;
        }

        if ((maxBytes > 0) && (fileType == AE_IFREG) && (entryLogicalSize > 0)) {
            totalBytesWritten += entryLogicalSize;
        }
    }

    if (success && (headerResult != ARCHIVE_EOF)) {
        qCWarning(QGClibarchiveLog) << "Failed to read archive header:" << archive_error_string(a);
        success = false;
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    // Never expose newly-created partial output from a failed extraction.
    if (!success) {
        cleanupCreatedEntries(createdFiles, createdDirs);
    }

    if (sizeLimitExceeded) {
        t_lastOperationResult = QGClibarchive::OperationResult::SizeLimitExceeded;
    } else if (cancelled) {
        t_lastOperationResult = QGClibarchive::OperationResult::Cancelled;
    } else if (success) {
        t_lastOperationResult = QGClibarchive::OperationResult::Success;
    }

    return success;
}

}  // namespace

// ============================================================================
// QGClibarchive Implementation
// ============================================================================

namespace QGClibarchive {

// ----------------------------------------------------------------------------
// ArchiveReader
// ----------------------------------------------------------------------------

ArchiveReader::~ArchiveReader()
{
    if (_archive) {
        archive_read_free(_archive);
    }
}

qint64 ArchiveReader::dataSize() const
{
    if (!_resourceData.isEmpty()) {
        return _resourceData.size();
    }
    if (_device) {
        return _device->size() > 0 ? _device->size() : 0;
    }
    if (!_filePath.isEmpty()) {
        return QFileInfo(_filePath).size();
    }
    return 0;
}

QString ArchiveReader::formatName() const
{
    if (!_archive) {
        return {};
    }
    const char* name = archive_format_name(_archive);
    return name ? QString::fromUtf8(name) : QString();
}

QString ArchiveReader::filterName() const
{
    if (!_archive) {
        return {};
    }
    // Get the outermost compression filter (index 0 is the format, 1+ are filters)
    // archive_filter_count returns total filters including format at index 0
    const int filterCount = archive_filter_count(_archive);
    if (filterCount <= 1) {
        return QStringLiteral("none");
    }
    // Index 0 is innermost (format), higher indices are outer filters
    // We want the first compression filter (index 1)
    const char* name = archive_filter_name(_archive, 0);
    return name ? QString::fromUtf8(name) : QStringLiteral("none");
}

bool ArchiveReader::open(const QString& path, ReaderMode mode)
{
    if (_archive) {
        archive_read_free(_archive);
        _archive = nullptr;
    }
    _device = nullptr;
    _filePath = path;

    _archive = archive_read_new();
    if (!_archive) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive reader";
        return false;
    }

    archive_read_support_filter_all(_archive);

    switch (mode) {
        case ReaderMode::AllFormats:
            archive_read_support_format_all(_archive);
            break;
        case ReaderMode::RawFormat:
            archive_read_support_format_raw(_archive);
            break;
    }

    if (!openArchiveForReading(_archive, path, _resourceData)) {
        archive_read_free(_archive);
        _archive = nullptr;
        return false;
    }

    return true;
}

bool ArchiveReader::open(QIODevice* device, ReaderMode mode)
{
    if (!device || !device->isOpen() || !device->isReadable()) {
        qCWarning(QGClibarchiveLog) << "Device is null, not open, or not readable";
        return false;
    }

    if (_archive) {
        archive_read_free(_archive);
        _archive = nullptr;
    }
    _device = device;
    _resourceData.clear();

    _archive = archive_read_new();
    if (!_archive) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive reader";
        return false;
    }

    archive_read_support_filter_all(_archive);

    switch (mode) {
        case ReaderMode::AllFormats:
            archive_read_support_format_all(_archive);
            break;
        case ReaderMode::RawFormat:
            archive_read_support_format_raw(_archive);
            break;
    }

    // Enable seek callback for random-access devices (improves ZIP performance)
    // Must be set before opening the archive
    if (!device->isSequential()) {
        archive_read_set_seek_callback(_archive, deviceSeekCallback);
    }

    const int result = archive_read_open2(_archive, device,
                                          nullptr,  // open callback (not needed)
                                          deviceReadCallback, deviceSkipCallback, deviceCloseCallback);

    if (result != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to open device:" << archive_error_string(_archive);
        archive_read_free(_archive);
        _archive = nullptr;
        _device = nullptr;
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

bool openArchiveForReading(struct archive* a, const QString& filePath, QByteArray& resourceData)
{
    const bool isResource = QGCFileHelper::isQtResource(filePath);

    if (isResource) {
        // Qt resources must be loaded into memory
        QFile inputFile(filePath);
        if (!inputFile.open(QIODevice::ReadOnly)) {
            qCWarning(QGClibarchiveLog) << "Failed to open file:" << filePath << inputFile.errorString();
            return false;
        }
        resourceData = inputFile.readAll();
        inputFile.close();

        if (resourceData.isEmpty()) {
            qCWarning(QGClibarchiveLog) << "File is empty:" << filePath;
            return false;
        }

        if (archive_read_open_memory(a, resourceData.constData(), static_cast<size_t>(resourceData.size())) !=
            ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to open data:" << archive_error_string(a);
            return false;
        }
    } else {
        // Stream directly from file (memory efficient)
        if (archive_read_open_filename(a, filePath.toLocal8Bit().constData(),
                                       QGCFileHelper::optimalBufferSize(filePath)) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to open file:" << filePath << archive_error_string(a);
            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// Archive Extraction
// ----------------------------------------------------------------------------

bool extractAnyArchive(const QString& archivePath, const QString& outputDirectoryPath, ProgressCallback progress,
                       qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }

    // Pre-check: verify sufficient disk space before extraction
    const ArchiveStats stats = getArchiveStats(archivePath);
    if (stats.totalUncompressedSize > 0) {
        if (!QGCFileHelper::hasSufficientDiskSpace(outputDirectoryPath, stats.totalUncompressedSize)) {
            return false;
        }
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    const qint64 totalSize = reader.dataSize();
    return extractArchiveEntries(reader.release(), outputDirectoryPath, progress, totalSize, maxBytes);
}

bool extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath, ProgressCallback progress,
                          qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    const QFileInfo outputInfo(outputDirectoryPath);
    if (outputInfo.fileName().isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Invalid output directory path:" << outputDirectoryPath;
        return false;
    }

    const QString parentDirectoryPath = outputInfo.absoluteDir().absolutePath();
    if (!QGCFileHelper::ensureDirectoryExists(parentDirectoryPath)) {
        qCWarning(QGClibarchiveLog) << "Failed to create output parent directory:" << parentDirectoryPath;
        return false;
    }

    // Pre-check: verify archive can be opened and get stats for disk space check
    const ArchiveStats stats = getArchiveStats(archivePath);
    if (stats.totalEntries == 0) {
        qCWarning(QGClibarchiveLog) << "Archive is empty or invalid:" << archivePath;
        return false;
    }

    // Need ~2x space: one for temp extraction, one for final location
    // (though move on same filesystem doesn't need extra space)
    if (stats.totalUncompressedSize > 0) {
        if (!QGCFileHelper::hasSufficientDiskSpace(outputDirectoryPath, stats.totalUncompressedSize * 2)) {
            return false;
        }
    }

    const QString outputDirectoryName = outputInfo.fileName();

    // Stage extraction in the same parent directory to keep commit/rollback renames on one filesystem.
    QTemporaryDir stagingDir(
        QGCFileHelper::joinPath(parentDirectoryPath, QStringLiteral("%1.qgc_stage_XXXXXX").arg(outputDirectoryName)));
    if (!stagingDir.isValid()) {
        qCWarning(QGClibarchiveLog) << "Failed to create staging directory:" << stagingDir.errorString();
        return false;
    }
    stagingDir.setAutoRemove(false);

    const QString stagingPath = stagingDir.path();
    const QFileInfo stagingInfo(stagingPath);
    const QString stagingName = stagingInfo.fileName();

    qCDebug(QGClibarchiveLog) << "Atomic extraction: staging to" << stagingPath;

    // Extract to temporary directory
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        (void) QDir(stagingPath).removeRecursively();
        return false;
    }

    const qint64 totalSize = reader.dataSize();
    if (!extractArchiveEntries(reader.release(), stagingPath, progress, totalSize, maxBytes)) {
        const OperationResult extractionResult = t_lastOperationResult;
        qCDebug(QGClibarchiveLog) << "Staged extraction failed, cleaning up";
        (void) QDir(stagingPath).removeRecursively();
        t_lastOperationResult = extractionResult;
        return false;
    }
    t_lastOperationResult = OperationResult::Failed;

    QDir parentDir(parentDirectoryPath);
    QString backupPath;
    QString backupName;

    const QFileInfo existingOutputInfo(outputDirectoryPath);
    if (existingOutputInfo.exists()) {
        if (!existingOutputInfo.isDir()) {
            qCWarning(QGClibarchiveLog) << "Output path exists and is not a directory:" << outputDirectoryPath;
            (void) QDir(stagingPath).removeRecursively();
            return false;
        }

        QTemporaryDir backupDir(QGCFileHelper::joinPath(
            parentDirectoryPath, QStringLiteral("%1.qgc_backup_XXXXXX").arg(outputDirectoryName)));
        if (!backupDir.isValid()) {
            qCWarning(QGClibarchiveLog) << "Failed to create backup directory placeholder:" << backupDir.errorString();
            (void) QDir(stagingPath).removeRecursively();
            return false;
        }
        backupDir.setAutoRemove(false);
        backupPath = backupDir.path();
        backupName = QFileInfo(backupPath).fileName();
        (void) backupDir.remove();

        if (!parentDir.rename(outputDirectoryName, backupName)) {
            qCWarning(QGClibarchiveLog) << "Failed to move existing output to backup:" << outputDirectoryPath;
            (void) QDir(stagingPath).removeRecursively();
            return false;
        }
    }

    if (!parentDir.rename(stagingName, outputDirectoryName)) {
        qCWarning(QGClibarchiveLog) << "Failed to commit staged extraction for" << outputDirectoryPath;
        (void) QDir(stagingPath).removeRecursively();

        if (!backupName.isEmpty()) {
            if (!parentDir.rename(backupName, outputDirectoryName)) {
                qCWarning(QGClibarchiveLog) << "Failed to rollback backup for" << outputDirectoryPath;
            }
        }

        return false;
    }

    if (!backupPath.isEmpty()) {
        (void) QDir(backupPath).removeRecursively();
    }

    qCDebug(QGClibarchiveLog) << "Atomic extraction committed:" << outputDirectoryPath;
    t_lastOperationResult = OperationResult::Success;
    return true;
}

bool extractSingleFile(const QString& archivePath, const QString& fileName, const QString& outputPath,
                       ProgressCallback progress, qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    const qint64 totalSize = QFileInfo(archivePath).size();
    if (progress && !progress(0, totalSize)) {
        t_lastOperationResult = OperationResult::Cancelled;
        return false;
    }

    struct archive_entry* entry;
    int headerResult = ARCHIVE_OK;

    while ((headerResult = archive_read_next_header(reader.handle(), &entry)) == ARCHIVE_OK) {
        if (progress && !progress(archive_filter_bytes(reader.handle(), -1), totalSize)) {
            t_lastOperationResult = OperationResult::Cancelled;
            return false;
        }
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (entryName == fileName) {
            const qint64 declaredSize = archive_entry_size(entry);
            if ((maxBytes > 0) && (declaredSize >= 0) && (declaredSize > maxBytes)) {
                qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
                t_lastOperationResult = OperationResult::SizeLimitExceeded;
                return false;
            }
            if (!QGCFileHelper::ensureParentExists(outputPath)) {
                return false;
            }
            qint64 extractedBytes = 0;
            return writeArchiveEntryToFile(reader.handle(), outputPath, progress, totalSize, maxBytes, &extractedBytes,
                                           declaredSize);
        }

        archive_read_data_skip(reader.handle());
    }

    if (headerResult != ARCHIVE_EOF) {
        qCWarning(QGClibarchiveLog) << "Failed to scan archive:" << archive_error_string(reader.handle());
        t_lastOperationResult = OperationResult::Failed;
    return false;
}

    qCWarning(QGClibarchiveLog) << "File not found in archive:" << fileName;
    t_lastOperationResult = OperationResult::NotFound;
    return false;
}

QByteArray extractFileToMemory(const QString& archivePath, const QString& fileName, qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (fileName.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Empty file name";
        return QByteArray();
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return QByteArray();
    }

    struct archive_entry* entry;
    int headerResult = ARCHIVE_OK;

    while ((headerResult = archive_read_next_header(reader.handle(), &entry)) == ARCHIVE_OK) {
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (entryName == fileName) {
    QByteArray result = readArchiveToMemory(reader.handle(), archive_entry_size(entry), maxBytes);
            if (!result.isEmpty()) {
                qCDebug(QGClibarchiveLog) << "Extracted" << fileName << "to memory:" << result.size() << "bytes";
            }
            return result;
        }

        archive_read_data_skip(reader.handle());
    }

    if (headerResult != ARCHIVE_EOF) {
        qCWarning(QGClibarchiveLog) << "Failed to scan archive:" << archive_error_string(reader.handle());
        t_lastOperationResult = OperationResult::Failed;
    return QByteArray();
}

    qCWarning(QGClibarchiveLog) << "File not found in archive:" << fileName;
    t_lastOperationResult = OperationResult::NotFound;
    return QByteArray();
}

static bool extractMultipleFilesToDirectory(const QString& archivePath, const QStringList& fileNames,
                                            const QString& outputDirectoryPath, ProgressCallback progress,
                                            qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (fileNames.isEmpty()) {
        t_lastOperationResult = OperationResult::Success;
        return true;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }
    const QString extractionRoot = canonicalExtractionRoot(outputDirectoryPath);

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    const qint64 totalSize = QFileInfo(archivePath).size();
    if (progress && !progress(0, totalSize)) {
        t_lastOperationResult = OperationResult::Cancelled;
        return false;
    }

    QSet<QString> targetFiles(fileNames.begin(), fileNames.end());
    QSet<QString> extractedFiles;
    qint64 extractedBytes = 0;
    struct archive_entry* entry;
    int headerResult = ARCHIVE_OK;

    while ((headerResult = archive_read_next_header(reader.handle(), &entry)) == ARCHIVE_OK) {
        if (progress && !progress(archive_filter_bytes(reader.handle(), -1), totalSize)) {
            t_lastOperationResult = OperationResult::Cancelled;
            return false;
        }
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (!targetFiles.contains(entryName)) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        const qint64 declaredSize = archive_entry_size(entry);
        if ((maxBytes > 0) && (declaredSize >= 0) &&
            ((extractedBytes > maxBytes) || (declaredSize > (maxBytes - extractedBytes)))) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            t_lastOperationResult = OperationResult::SizeLimitExceeded;
            return false;
        }

        QString outputPath;
        if (!resolveSecureExtractionPath(extractionRoot, entryName, outputPath) ||
            !QGCFileHelper::ensureParentExists(outputPath)) {
            return false;
        }

        if (!writeArchiveEntryToFile(reader.handle(), outputPath, progress, totalSize, maxBytes, &extractedBytes,
                                     declaredSize)) {
            return false;
        }

        extractedFiles.insert(entryName);

        if (extractedFiles.size() == targetFiles.size()) {
            break;
        }
    }

    if ((extractedFiles.size() != targetFiles.size()) && (headerResult != ARCHIVE_EOF)) {
        qCWarning(QGClibarchiveLog) << "Failed to scan archive:" << archive_error_string(reader.handle());
        t_lastOperationResult = OperationResult::Failed;
        return false;
    }

    if (extractedFiles.size() != targetFiles.size()) {
        t_lastOperationResult = OperationResult::NotFound;
        for (const QString& name : fileNames) {
            if (!extractedFiles.contains(name)) {
                qCWarning(QGClibarchiveLog) << "File not found in archive:" << name;
            }
        }
        return false;
    }

    t_lastOperationResult = OperationResult::Success;
    return true;
}

bool extractMultipleFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath,
                       ProgressCallback progress, qint64 maxBytes)
{
    if (fileNames.isEmpty()) {
        t_lastOperationResult = OperationResult::Success;
        return true;
    }

    return extractSelectionAtomically(outputDirectoryPath, [&](const QString& stagingPath) {
        return extractMultipleFilesToDirectory(archivePath, fileNames, stagingPath, progress, maxBytes);
    });
}

static bool extractByPatternToDirectory(const QString& archivePath, const QStringList& patterns,
                                        const QString& outputDirectoryPath, QStringList* extractedFiles,
                                        qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (patterns.isEmpty()) {
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }
    const QString extractionRoot = canonicalExtractionRoot(outputDirectoryPath);

    struct archive* match = archive_match_new();
    if (!match) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive_match";
        return false;
    }

    for (const QString& pattern : patterns) {
        if (archive_match_include_pattern(match, pattern.toUtf8().constData()) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Invalid pattern:" << pattern;
            archive_match_free(match);
            return false;
        }
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        archive_match_free(match);
        return false;
    }

    struct archive_entry* entry;
    int headerResult = ARCHIVE_OK;
    int matchCount = 0;
    qint64 extractedBytes = 0;

    while ((headerResult = archive_read_next_header(reader.handle(), &entry)) == ARCHIVE_OK) {
        if (archive_match_excluded(match, entry)) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        if (archive_entry_filetype(entry) == AE_IFDIR) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        const qint64 declaredSize = archive_entry_size(entry);
        if ((maxBytes > 0) && (declaredSize >= 0) &&
            ((extractedBytes > maxBytes) || (declaredSize > (maxBytes - extractedBytes)))) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            t_lastOperationResult = OperationResult::SizeLimitExceeded;
            archive_match_free(match);
            return false;
        }

        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));
        QString outputPath;
        if (!resolveSecureExtractionPath(extractionRoot, entryName, outputPath) ||
            !QGCFileHelper::ensureParentExists(outputPath)) {
            archive_match_free(match);
            return false;
        }

        if (!writeArchiveEntryToFile(reader.handle(), outputPath, nullptr, 0, maxBytes, &extractedBytes,
                                     declaredSize)) {
            archive_match_free(match);
            return false;
        }

        matchCount++;
        if (extractedFiles) {
            extractedFiles->append(entryName);
        }
    }

    if (headerResult != ARCHIVE_EOF) {
        qCWarning(QGClibarchiveLog) << "Failed to scan archive:" << archive_error_string(reader.handle());
        t_lastOperationResult = OperationResult::Failed;
        archive_match_free(match);
        return false;
    }

    if (archive_match_path_unmatched_inclusions(match) > 0) {
        const char* unmatched;
        while (archive_match_path_unmatched_inclusions_next(match, &unmatched) == ARCHIVE_OK) {
            qCDebug(QGClibarchiveLog) << "Pattern not matched:" << unmatched;
        }
    }

    archive_match_free(match);

    qCDebug(QGClibarchiveLog) << "Extracted" << matchCount << "files matching patterns from" << archivePath;
    t_lastOperationResult = matchCount > 0 ? OperationResult::Success : OperationResult::NotFound;
    return matchCount > 0;
}

bool extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                      QStringList* extractedFiles, qint64 maxBytes)
{
    QStringList stagedFiles;
    const bool success = extractSelectionAtomically(outputDirectoryPath, [&](const QString& stagingPath) {
        return extractByPatternToDirectory(archivePath, patterns, stagingPath, &stagedFiles, maxBytes);
    });
    if (success && extractedFiles) {
        extractedFiles->append(stagedFiles);
    }
    return success;
}

bool validateArchive(const QString& archivePath)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    struct archive_entry* entry;

    // Read through all entries and their data to validate
    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        // Read and discard data to verify it can be decompressed
        char buffer[QGCFileHelper::kBufferSizeMax];
        la_ssize_t size;
        while ((size = archive_read_data(reader.handle(), buffer, sizeof(buffer))) > 0) {
            // Just reading to validate
        }

        if (size < 0) {
            qCWarning(QGClibarchiveLog) << "Validation failed for entry:" << archive_entry_pathname(entry)
                                        << archive_error_string(reader.handle());
            return false;
        }
    }

    return true;
}

bool fileExistsInArchive(const QString& archivePath, const QString& fileName)
{
    if (fileName.isEmpty()) {
        return false;
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    struct archive_entry* entry;
    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        if (QString::fromUtf8(archive_entry_pathname(entry)) == fileName) {
            return true;
        }
        archive_read_data_skip(reader.handle());
    }

    return false;
}

QStringList listArchiveEntries(const QString& archivePath)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return {};
    }

    QStringList entries;
    struct archive_entry* entry;
    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        entries.append(QString::fromUtf8(archive_entry_pathname(entry)));
        archive_read_data_skip(reader.handle());
    }

    qCDebug(QGClibarchiveLog) << "Listed" << entries.size() << "entries in" << archivePath;
    return entries;
}

QList<ArchiveEntry> listArchiveEntriesDetailed(const QString& archivePath)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return {};
    }

    QList<ArchiveEntry> entries;
    struct archive_entry* entry;
    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        entries.append(toArchiveEntry(entry));
        archive_read_data_skip(reader.handle());
    }

    qCDebug(QGClibarchiveLog) << "Listed" << entries.size() << "entries with metadata in" << archivePath;
    return entries;
}

ArchiveStats getArchiveStats(const QString& archivePath)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return {};
    }

    ArchiveStats stats;
    struct archive_entry* entry;
    int headerResult = ARCHIVE_OK;

    while ((headerResult = archive_read_next_header(reader.handle(), &entry)) == ARCHIVE_OK) {
        stats.totalEntries++;

        if (archive_entry_filetype(entry) == AE_IFDIR) {
            stats.directoryCount++;
        } else {
            stats.fileCount++;
            const qint64 size = archive_entry_size(entry);
            stats.totalUncompressedSize += size;

            if (size > stats.largestFileSize) {
                stats.largestFileSize = size;
                stats.largestFileName = QString::fromUtf8(archive_entry_pathname(entry));
            }
        }

        archive_read_data_skip(reader.handle());
    }

    qCDebug(QGClibarchiveLog) << "Archive stats:" << stats.fileCount << "files," << stats.directoryCount << "dirs,"
                              << stats.totalUncompressedSize << "bytes total";
    return stats;
}

static bool extractWithFilterToDirectory(const QString& archivePath, const QString& outputDirectoryPath,
                                         EntryFilter filter, ProgressCallback progress, qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (!filter) {
        qCWarning(QGClibarchiveLog) << "No filter provided";
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }
    const QString extractionRoot = canonicalExtractionRoot(outputDirectoryPath);

    // Pre-check: verify sufficient disk space (uses total archive size as upper bound)
    const ArchiveStats stats = getArchiveStats(archivePath);
    if (stats.totalUncompressedSize > 0) {
        if (!QGCFileHelper::hasSufficientDiskSpace(outputDirectoryPath, stats.totalUncompressedSize)) {
            return false;
        }
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    const qint64 totalSize = reader.dataSize();
    qint64 bytesExtracted = 0;
    int extractedCount = 0;
    int skippedCount = 0;
    struct archive_entry* entry;
    int headerResult = ARCHIVE_OK;

    while ((headerResult = archive_read_next_header(reader.handle(), &entry)) == ARCHIVE_OK) {
        // Build ArchiveEntry for filter callback
        const ArchiveEntry info = toArchiveEntry(entry);

        // Check filter
        if (!filter(info)) {
            skippedCount++;
            archive_read_data_skip(reader.handle());
            continue;
        }

        // Skip directories for extraction (they're created by ensureParentExists)
        if (info.isDirectory) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        // Check size limit before extraction
        if ((maxBytes > 0) && (info.size >= 0) &&
            ((bytesExtracted > maxBytes) || (info.size > (maxBytes - bytesExtracted)))) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            t_lastOperationResult = OperationResult::SizeLimitExceeded;
            return false;
        }

        QString outputPath;
        if (!resolveSecureExtractionPath(extractionRoot, info.name, outputPath) ||
            !QGCFileHelper::ensureParentExists(outputPath)) {
            return false;
        }

        if (!writeArchiveEntryToFile(reader.handle(), outputPath, progress, totalSize, maxBytes, &bytesExtracted,
                                     info.size)) {
                return false;
            }

        extractedCount++;
        }

    if (headerResult != ARCHIVE_EOF) {
        qCWarning(QGClibarchiveLog) << "Failed to scan archive:" << archive_error_string(reader.handle());
        t_lastOperationResult = OperationResult::Failed;
        return false;
    }

    qCDebug(QGClibarchiveLog) << "Extracted" << extractedCount << "files, skipped" << skippedCount;
    t_lastOperationResult = OperationResult::Success;
    return true;
}

bool extractWithFilter(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                       ProgressCallback progress, qint64 maxBytes)
{
    if (!filter) {
        t_lastOperationResult = OperationResult::Failed;
        qCWarning(QGClibarchiveLog) << "No filter provided";
        return false;
    }

    return extractSelectionAtomically(outputDirectoryPath, [&](const QString& stagingPath) {
        return extractWithFilterToDirectory(archivePath, stagingPath, filter, progress, maxBytes);
    });
}

// ----------------------------------------------------------------------------
// Single-File Decompression
// ----------------------------------------------------------------------------

OperationResult decompressSingleFile(const QString& inputPath, const QString& outputPath, ProgressCallback progress,
                          qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    ArchiveReader reader;
    if (!reader.open(inputPath, ReaderMode::RawFormat)) {
        return OperationResult::Failed;
    }

    struct archive_entry* entry = nullptr;
    if (archive_read_next_header(reader.handle(), &entry) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to read header:" << archive_error_string(reader.handle());
        return OperationResult::Failed;
    }

    updateFormatState(reader.handle());
    QGCFileHelper::ensureParentExists(outputPath);

    return decompressStreamToFile(reader.handle(), outputPath, progress, reader.dataSize(), maxBytes);
}

QByteArray decompressDataFromMemory(const QByteArray& data, qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (data.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Cannot decompress empty data";
        return {};
    }

    struct archive* a = archive_read_new();
    if (!a) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive reader";
        return {};
    }

    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);

    if (archive_read_open_memory(a, data.constData(), static_cast<size_t>(data.size())) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to open compressed data:" << archive_error_string(a);
        archive_read_free(a);
        return {};
    }

    struct archive_entry* entry;
    if (archive_read_next_header(a, &entry) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to read header:" << archive_error_string(a);
        archive_read_free(a);
        return {};
    }

    updateFormatState(a);

    QByteArray result = readArchiveToMemory(a, 0, maxBytes);
    archive_read_free(a);
    qCDebug(QGClibarchiveLog) << "Decompressed" << data.size() << "bytes to" << result.size() << "bytes";
    return result;
}

// ----------------------------------------------------------------------------
// QIODevice-based Operations
// ----------------------------------------------------------------------------

bool extractFromDevice(QIODevice* device, const QString& outputDirectoryPath, ProgressCallback progress,
                       qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }

    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::AllFormats)) {
        return false;
    }

    // For device-based reads, we don't know the total size upfront
    const qint64 totalSize = device->size() > 0 ? device->size() : 0;
    return extractArchiveEntries(reader.release(), outputDirectoryPath, progress, totalSize, maxBytes);
}

QByteArray extractFileDataFromDevice(QIODevice* device, const QString& fileName)
{
    if (fileName.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Empty file name";
        return {};
    }

    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::AllFormats)) {
        return {};
    }

    struct archive_entry* entry;
    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        if (QString::fromUtf8(archive_entry_pathname(entry)) == fileName) {
            QByteArray result = readArchiveToMemory(reader.handle(), archive_entry_size(entry));
            if (!result.isEmpty()) {
                qCDebug(QGClibarchiveLog) << "Extracted" << fileName << "from device:" << result.size() << "bytes";
            }
            return result;
        }
        archive_read_data_skip(reader.handle());
    }

    qCWarning(QGClibarchiveLog) << "File not found in archive:" << fileName;
    return QByteArray();
}

bool decompressFromDevice(QIODevice* device, const QString& outputPath, ProgressCallback progress, qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::RawFormat)) {
        return false;
    }

    QGCFileHelper::ensureParentExists(outputPath);

    struct archive_entry* entry = nullptr;
    if (archive_read_next_header(reader.handle(), &entry) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to read header:" << archive_error_string(reader.handle());
        return false;
    }

    updateFormatState(reader.handle());

    const qint64 totalSize = device->size() > 0 ? device->size() : 0;
    return decompressStreamToFile(reader.handle(), outputPath, progress, totalSize, maxBytes) ==
           OperationResult::Success;
}

QByteArray decompressDataFromDevice(QIODevice* device, qint64 maxBytes)
{
    t_lastOperationResult = OperationResult::Failed;
    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::RawFormat)) {
        return {};
    }

    struct archive_entry* entry;
    if (archive_read_next_header(reader.handle(), &entry) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to read header:" << archive_error_string(reader.handle());
        return {};
    }

    updateFormatState(reader.handle());

    QByteArray result = readArchiveToMemory(reader.handle(), archive_entry_size(entry), maxBytes);
    if (!result.isEmpty()) {
        qCDebug(QGClibarchiveLog) << "Decompressed from device:" << result.size() << "bytes";
    }
    return result;
}

}  // namespace QGClibarchive
