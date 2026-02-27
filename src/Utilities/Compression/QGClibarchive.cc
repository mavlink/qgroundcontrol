#include "QGClibarchive.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QSet>
#include <QtCore/QTemporaryDir>

#include <archive.h>
#include <archive_entry.h>

#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGClibarchiveLog, "Utilities.QGClibarchive")

// ============================================================================
// Thread-Local Format Detection State
// ============================================================================

namespace {

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
    QByteArray result;
    if (expectedSize > 0) {
        result.reserve(static_cast<qsizetype>(expectedSize));
    }

    char buffer[QGCFileHelper::kBufferSizeMax];
    la_ssize_t size;
    while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
        if (maxBytes > 0 && (result.size() + size) > maxBytes) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            return {};
        }
        result.append(buffer, static_cast<qsizetype>(size));
    }

    if (size < 0) {
        qCWarning(QGClibarchiveLog) << "Read error:" << archive_error_string(a);
        return {};
    }

    return result;
}

/// Write current archive entry data to a file atomically
/// @param a Archive handle (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @return true on success, false on read/write error (no partial file left on failure)
/// @note Uses QSaveFile for atomic writes - writes to temp file, then renames on commit
/// @note Uses archive_read_data_block() with seeking to preserve sparse file structure
bool writeArchiveEntryToFile(struct archive* a, const QString& outputPath)
{
    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCWarning(QGClibarchiveLog) << "Failed to open output file:" << outputPath << outFile.errorString();
        return false;
    }

    const void* buff;
    size_t size;
    la_int64_t offset;
    int r;

    while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
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
    }

    if (r != ARCHIVE_EOF) {
        qCWarning(QGClibarchiveLog) << "Read error:" << archive_error_string(a);
        outFile.cancelWriting();
        return false;
    }

    // Atomically commit - renames temp file to final path
    if (!outFile.commit()) {
        qCWarning(QGClibarchiveLog) << "Failed to commit output file:" << outFile.errorString();
        return false;
    }

    return true;
}

/// Decompress archive stream to file atomically with progress and size limit
/// @param a Archive handle (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @param progress Optional progress callback
/// @param totalSize Total size for progress reporting (0 if unknown)
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @return true on success, false on error/cancel/limit exceeded (no partial file)
bool decompressStreamToFile(struct archive* a, const QString& outputPath,
                            const QGClibarchive::ProgressCallback& progress, qint64 totalSize, qint64 maxBytes)
{
    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCWarning(QGClibarchiveLog) << "Failed to open output file:" << outputPath << outFile.errorString();
        return false;
    }

    qint64 totalBytesWritten = 0;
    char buffer[QGCFileHelper::kBufferSizeMax];
    la_ssize_t size;

    while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
        if (maxBytes > 0 && (totalBytesWritten + size) > maxBytes) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            outFile.cancelWriting();
            return false;
        }

        if (outFile.write(buffer, size) != size) {
            qCWarning(QGClibarchiveLog) << "Failed to write output:" << outFile.errorString();
            outFile.cancelWriting();
            return false;
        }
        totalBytesWritten += size;

        if (progress) {
            const qint64 bytesRead = archive_filter_bytes(a, -1);
            if (!progress(bytesRead, totalSize)) {
                qCDebug(QGClibarchiveLog) << "Decompression cancelled by user";
                outFile.cancelWriting();
                return false;
            }
        }
    }

    if (size < 0) {
        qCWarning(QGClibarchiveLog) << "Decompression error:" << archive_error_string(a);
        outFile.cancelWriting();
        return false;
    }

    if (!outFile.commit()) {
        qCWarning(QGClibarchiveLog) << "Failed to commit output file:" << outFile.errorString();
        return false;
    }

    qCDebug(QGClibarchiveLog) << "Decompressed" << totalBytesWritten << "bytes to" << outputPath;
    return true;
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
    struct archive* ext = archive_write_disk_new();
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
    QString canonicalOutputDir = QFileInfo(outputDirectoryPath).canonicalFilePath();
    if (canonicalOutputDir.isEmpty()) {
        canonicalOutputDir = QFileInfo(outputDirectoryPath).absoluteFilePath();
    }
    existingDirs.insert(canonicalOutputDir);

    bool formatLogged = false;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        // Log detected format after first header read
        if (!formatLogged) {
            updateFormatState(a);
            formatLogged = true;
        }

        const char* currentFile = archive_entry_pathname(entry);
        QString entryName = QString::fromUtf8(currentFile);
        QString outputPath = QGCFileHelper::joinPath(canonicalOutputDir, entryName);

        // Prevent path traversal attacks
        QFileInfo outputInfo(outputPath);
        QString canonicalOutput = outputInfo.absoluteFilePath();

        if (!canonicalOutput.startsWith(canonicalOutputDir + "/") && canonicalOutput != canonicalOutputDir) {
            qCWarning(QGClibarchiveLog) << "Skipping path traversal attempt:" << currentFile;
            continue;
        }

        // Security: reject filenames with embedded null bytes (libarchive issue #2774)
        if (entryName.contains(QChar('\0'))) {
            qCWarning(QGClibarchiveLog) << "Skipping path with embedded null byte";
            continue;
        }

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
                if (!resolvedTarget.startsWith(canonicalOutputDir + "/") && resolvedTarget != canonicalOutputDir) {
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
        } else {
            createdFiles.append(canonicalOutput);
        }

        if (archive_entry_size(entry) > 0) {
            const void* buff;
            size_t size;
            la_int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                // Check size limit before writing
                if (maxBytes > 0 && (totalBytesWritten + static_cast<qint64>(size)) > maxBytes) {
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

                totalBytesWritten += static_cast<qint64>(size);

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
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    // Clean up only created entries on cancellation or size limit exceeded
    if (cancelled || sizeLimitExceeded) {
        cleanupCreatedEntries(createdFiles, createdDirs);
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
    QTemporaryDir stagingDir(QGCFileHelper::joinPath(parentDirectoryPath,
                                                     QStringLiteral("%1.qgc_stage_XXXXXX").arg(outputDirectoryName)));
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
        qCDebug(QGClibarchiveLog) << "Staged extraction failed, cleaning up";
        (void) QDir(stagingPath).removeRecursively();
        return false;
    }

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

        QTemporaryDir backupDir(QGCFileHelper::joinPath(parentDirectoryPath,
                                                        QStringLiteral("%1.qgc_backup_XXXXXX").arg(outputDirectoryName)));
        if (!backupDir.isValid()) {
            qCWarning(QGClibarchiveLog) << "Failed to create backup directory placeholder:"
                                        << backupDir.errorString();
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
    return true;
}

bool extractSingleFile(const QString& archivePath, const QString& fileName, const QString& outputPath)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    struct archive_entry* entry;

    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (entryName == fileName) {
            QGCFileHelper::ensureParentExists(outputPath);
            return writeArchiveEntryToFile(reader.handle(), outputPath);
        }

        archive_read_data_skip(reader.handle());
    }

    qCWarning(QGClibarchiveLog) << "File not found in archive:" << fileName;
    return false;
}

QByteArray extractFileToMemory(const QString& archivePath, const QString& fileName)
{
    if (fileName.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Empty file name";
        return QByteArray();
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return QByteArray();
    }

    struct archive_entry* entry;

    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (entryName == fileName) {
            QByteArray result = readArchiveToMemory(reader.handle(), archive_entry_size(entry));
            if (!result.isEmpty()) {
                qCDebug(QGClibarchiveLog) << "Extracted" << fileName << "to memory:" << result.size() << "bytes";
            }
            return result;
        }

        archive_read_data_skip(reader.handle());
    }

    qCWarning(QGClibarchiveLog) << "File not found in archive:" << fileName;
    return QByteArray();
}

bool extractMultipleFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath)
{
    if (fileNames.isEmpty()) {
        return true;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    QSet<QString> targetFiles(fileNames.begin(), fileNames.end());
    QSet<QString> extractedFiles;
    struct archive_entry* entry;

    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (!targetFiles.contains(entryName)) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        const QString outputPath = QGCFileHelper::joinPath(outputDirectoryPath, entryName);
        QGCFileHelper::ensureParentExists(outputPath);

        if (!writeArchiveEntryToFile(reader.handle(), outputPath)) {
            return false;
        }

        extractedFiles.insert(entryName);

        if (extractedFiles.size() == targetFiles.size()) {
            break;
        }
    }

    if (extractedFiles.size() != targetFiles.size()) {
        for (const QString& name : fileNames) {
            if (!extractedFiles.contains(name)) {
                qCWarning(QGClibarchiveLog) << "File not found in archive:" << name;
            }
        }
        return false;
    }

    return true;
}

bool extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                      QStringList* extractedFiles)
{
    if (patterns.isEmpty()) {
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }

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
    int matchCount = 0;

    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
        if (archive_match_excluded(match, entry)) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        if (archive_entry_filetype(entry) == AE_IFDIR) {
            archive_read_data_skip(reader.handle());
            continue;
        }

        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));
        const QString outputPath = QGCFileHelper::joinPath(outputDirectoryPath, entryName);

        QGCFileHelper::ensureParentExists(outputPath);

        if (!writeArchiveEntryToFile(reader.handle(), outputPath)) {
            archive_match_free(match);
            return false;
        }

        matchCount++;
        if (extractedFiles) {
            extractedFiles->append(entryName);
        }
    }

    if (archive_match_path_unmatched_inclusions(match) > 0) {
        const char* unmatched;
        while (archive_match_path_unmatched_inclusions_next(match, &unmatched) == ARCHIVE_OK) {
            qCDebug(QGClibarchiveLog) << "Pattern not matched:" << unmatched;
        }
    }

    archive_match_free(match);

    qCDebug(QGClibarchiveLog) << "Extracted" << matchCount << "files matching patterns from" << archivePath;
    return matchCount > 0;
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

    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
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

bool extractWithFilter(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                       ProgressCallback progress, qint64 maxBytes)
{
    if (!filter) {
        qCWarning(QGClibarchiveLog) << "No filter provided";
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return false;
    }

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
    qint64 bytesProcessed = 0;
    qint64 bytesExtracted = 0;
    int extractedCount = 0;
    int skippedCount = 0;
    struct archive_entry* entry;

    while (archive_read_next_header(reader.handle(), &entry) == ARCHIVE_OK) {
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
        if (maxBytes > 0 && (bytesExtracted + info.size) > maxBytes) {
            qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
            return false;
        }

        const QString outputPath = QGCFileHelper::joinPath(outputDirectoryPath, info.name);
        QGCFileHelper::ensureParentExists(outputPath);

        if (!writeArchiveEntryToFile(reader.handle(), outputPath)) {
            return false;
        }

        bytesExtracted += info.size;
        extractedCount++;

        // Progress callback
        if (progress) {
            bytesProcessed += info.size;
            if (!progress(bytesProcessed, totalSize)) {
                qCDebug(QGClibarchiveLog) << "Extraction cancelled by user";
                return false;
            }
        }
    }

    qCDebug(QGClibarchiveLog) << "Extracted" << extractedCount << "files, skipped" << skippedCount;
    return true;
}

// ----------------------------------------------------------------------------
// Single-File Decompression
// ----------------------------------------------------------------------------

bool decompressSingleFile(const QString& inputPath, const QString& outputPath, ProgressCallback progress,
                          qint64 maxBytes)
{
    ArchiveReader reader;
    if (!reader.open(inputPath, ReaderMode::RawFormat)) {
        return false;
    }

    struct archive_entry* entry = nullptr;
    if (archive_read_next_header(reader.handle(), &entry) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to read header:" << archive_error_string(reader.handle());
        return false;
    }

    updateFormatState(reader.handle());
    QGCFileHelper::ensureParentExists(outputPath);

    return decompressStreamToFile(reader.handle(), outputPath, progress, reader.dataSize(), maxBytes);
}

QByteArray decompressDataFromMemory(const QByteArray& data, qint64 maxBytes)
{
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
    return decompressStreamToFile(reader.handle(), outputPath, progress, totalSize, maxBytes);
}

QByteArray decompressDataFromDevice(QIODevice* device, qint64 maxBytes)
{
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
