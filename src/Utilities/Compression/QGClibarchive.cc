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

thread_local bool t_sizeLimitExceeded = false;

void reportSizeLimitExceeded(qint64 maxBytes)
{
    qCWarning(QGClibarchiveLog) << "Size limit exceeded:" << maxBytes << "bytes";
    t_sizeLimitExceeded = true;
}

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

void clearLastDetectedFormat()
{
    updateFormatState(nullptr);
}

bool takeSizeLimitExceeded()
{
    const bool exceeded = t_sizeLimitExceeded;
    t_sizeLimitExceeded = false;
    return exceeded;
}

}  // namespace QGClibarchive

// ============================================================================
// libarchive callbacks for QIODevice streaming
// ============================================================================

namespace QGClibarchive {

la_ssize_t deviceReadCallback(struct archive*, void* clientData, const void** buffer)
{
    auto* source = static_cast<DeviceSource*>(clientData);

    const qint64 bytesRead = source->device->read(source->block.data(), source->block.size());
    if (bytesRead < 0) {
        return ARCHIVE_FATAL;
    }

    *buffer = source->block.constData();
    return static_cast<la_ssize_t>(bytesRead);
}

la_int64_t deviceSkipCallback(struct archive*, void* clientData, la_int64_t request)
{
    QIODevice* device = static_cast<DeviceSource*>(clientData)->device;

    if (!device->isSequential()) {
        // A file can be positioned beyond its end, so no more is skipped than is there. libarchive
        // takes a short skip for what it is: an archive cut inside the data that was to be skipped.
        const qint64 currentPos = device->pos();
        const qint64 skipped = qBound(qint64(0), device->size() - currentPos, static_cast<qint64>(request));
        if (device->seek(currentPos + skipped)) {
            return skipped;
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
    QIODevice* device = static_cast<DeviceSource*>(clientData)->device;

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

NextEntry readNextEntry(struct archive* a, struct archive_entry** entry)
{
    switch (archive_read_next_header(a, entry)) {
        case ARCHIVE_OK:
            return NextEntry::Found;
        case ARCHIVE_EOF:
            return NextEntry::End;
        case ARCHIVE_WARN:
            qCWarning(QGClibarchiveLog) << "Archive entry read with a warning:" << archive_error_string(a);
            return NextEntry::Found;
        default:
            qCWarning(QGClibarchiveLog) << "Failed to read archive entry:" << archive_error_string(a);
            return NextEntry::Error;
    }
}

EntryWalk forEachEntry(struct archive* a, const EntryVisitor& visitor)
{
    struct archive_entry* entry = nullptr;
    bool formatKnown = false;
    NextEntry next;
    while ((next = readNextEntry(a, &entry)) == NextEntry::Found) {
        // libarchive knows the format once it has read the first header
        if (!formatKnown) {
            updateFormatState(a);
            formatKnown = true;
        }
        if (!visitor(entry)) {
            return EntryWalk::Stopped;
        }
    }
    return (next == NextEntry::End) ? EntryWalk::Completed : EntryWalk::Failed;
}

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

struct DiskWriterDeleter
{
    void operator()(struct archive* writer) const { archive_write_free(writer); }
};

using DiskWriterPtr = std::unique_ptr<struct archive, DiskWriterDeleter>;

struct EntryMatchDeleter
{
    void operator()(struct archive* match) const { archive_match_free(match); }
};

using EntryMatchPtr = std::unique_ptr<struct archive, EntryMatchDeleter>;

/// Size limit and progress reporting of one operation, shared by all the entries whose data it copies
struct DataTransfer
{
    qint64 maxBytes = 0;                            ///< Bytes the operation may write (0 = unlimited)
    QGClibarchive::ProgressCallback progress = {};  ///< Optional; cancels the operation by returning false
    qint64 totalSize = 0;                           ///< Size of the input for the progress callback (0 if unknown)
    qint64 bytesWritten = 0;                        ///< Bytes written so far
};

/// How copyEntryData() ended
enum class CopyResult
{
    Completed,          ///< The data ended cleanly and all of it was written
    ReadFailed,         ///< The archive is truncated or corrupt: archive_error_string() tells how
    WriteFailed,        ///< The sink could not take a block (it has logged a warning)
    SizeLimitExceeded,  ///< The data did not fit in the operation's limit (a warning has been logged)
    Cancelled           ///< The progress callback returned false
};

/// Takes one block of an entry's data, which belongs at @a offset of the entry. Returns false if it cannot.
using DataSink = std::function<bool(const char* data, qint64 size, qint64 offset)>;

/// Copy the data of the current entry to a sink
/// Every loop over entry data is this one: it holds the operation to its size limit, reports progress,
/// lets the caller cancel, and only completes on data that ends cleanly.
/// @param a Archive handle (must have entry header already read)
/// @param transfer Limit and progress of the operation, updated with the bytes written
/// @param sink Called with each block, in the order of the offsets
CopyResult copyEntryData(struct archive* a, DataTransfer& transfer, const DataSink& sink)
{
    const void* buff = nullptr;
    size_t size = 0;
    la_int64_t offset = 0;
    int r;

    while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
        const qint64 blockSize = static_cast<qint64>(size);
        if (transfer.maxBytes > 0 && (transfer.bytesWritten + blockSize) > transfer.maxBytes) {
            reportSizeLimitExceeded(transfer.maxBytes);
            return CopyResult::SizeLimitExceeded;
        }

        if (!sink(static_cast<const char*>(buff), blockSize, offset)) {
            return CopyResult::WriteFailed;
        }
        transfer.bytesWritten += blockSize;

        // Progress is the position in the input, whose size is all that is known beforehand
        if (transfer.progress && !transfer.progress(archive_filter_bytes(a, -1), transfer.totalSize)) {
            qCDebug(QGClibarchiveLog) << "Cancelled by the progress callback";
            return CopyResult::Cancelled;
        }
    }

    return (r == ARCHIVE_EOF) ? CopyResult::Completed : CopyResult::ReadFailed;
}

/// Read all data from archive into QByteArray
/// @param a Archive handle (must have header already read)
/// @param expectedSize Size the entry's header declares (0 if unknown). Memory is set aside for it up to
///        kMaxReservedBytes only: a damaged or hostile archive declares any size.
/// @param maxBytes Maximum bytes to read (0 = unlimited)
/// @param completed If not null, receives whether the data was read to its end: there may be none to read
/// @return Data read, or empty QByteArray on error or size limit exceeded
QByteArray readArchiveToMemory(struct archive* a, qint64 expectedSize = 0, qint64 maxBytes = 0,
                               bool* completed = nullptr)
{
    constexpr qint64 kMaxReservedBytes = 16 * 1024 * 1024;

    QByteArray result;
    if (expectedSize > 0) {
        result.reserve(static_cast<qsizetype>(qMin(expectedSize, kMaxReservedBytes)));
    }

    DataTransfer transfer{.maxBytes = maxBytes};
    const CopyResult copied =
        copyEntryData(a, transfer, [&result, maxBytes](const char* data, qint64 size, qint64 offset) {
            // The holes of a sparse entry read as zeros. They take memory, so they count towards the limit.
            const qint64 hole = offset - result.size();
            if (hole < 0) {
                qCWarning(QGClibarchiveLog) << "Read error: out-of-order sparse blocks";
                return false;
            }
            if (maxBytes > 0 && (offset + size) > maxBytes) {
                reportSizeLimitExceeded(maxBytes);
                return false;
            }
            result.append(static_cast<qsizetype>(hole), '\0');
            result.append(data, static_cast<qsizetype>(size));
            return true;
        });

    if (copied == CopyResult::ReadFailed) {
        qCWarning(QGClibarchiveLog) << "Read error:" << archive_error_string(a);
    }
    if (completed != nullptr) {
        *completed = (copied == CopyResult::Completed);
    }

    return (copied == CopyResult::Completed) ? result : QByteArray();
}

/// Read the data of the archive entry whose header was just read into a QByteArray
/// An entry that says it does not fit in @a maxBytes is not started. One that says less than it holds, or
/// nothing, is stopped at the limit while it is read.
/// @param maxBytes Maximum size of the entry (0 = unlimited)
/// @param completed Receives whether the entry was read to its end: it may have no data
/// @return Data read, or empty QByteArray on error or size limit exceeded
QByteArray readEntryToMemory(struct archive* a, struct archive_entry* entry, qint64 maxBytes, bool& completed)
{
    completed = false;
    const qint64 declaredSize = archive_entry_size(entry);
    if (maxBytes > 0 && declaredSize > maxBytes) {
        reportSizeLimitExceeded(maxBytes);
        return {};
    }
    return readArchiveToMemory(a, declaredSize, maxBytes, &completed);
}

/// Write the data of the current archive entry to a file atomically
/// @param a Archive handle (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @param transfer Limit and progress of the operation the entry is part of
/// @return Completed on success; otherwise no partial file is left
/// @note Uses QSaveFile for atomic writes - writes to temp file, then renames on commit
/// @note Writes each block at its offset to preserve sparse file structure
CopyResult copyEntryToFile(struct archive* a, const QString& outputPath, DataTransfer& transfer)
{
    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCWarning(QGClibarchiveLog) << "Failed to open output file:" << outputPath << outFile.errorString();
        return CopyResult::WriteFailed;
    }

    const CopyResult copied = copyEntryData(a, transfer, [&outFile](const char* data, qint64 size, qint64 offset) {
        // Seek to offset - creates sparse hole if gap from previous position
        if (offset != outFile.pos() && !outFile.seek(offset)) {
            qCWarning(QGClibarchiveLog) << "Failed to seek:" << outFile.errorString();
            return false;
        }
        if (outFile.write(data, size) != size) {
            qCWarning(QGClibarchiveLog) << "Failed to write output:" << outFile.errorString();
            return false;
        }
        return true;
    });

    if (copied != CopyResult::Completed) {
        outFile.cancelWriting();
        return copied;
    }

    // Atomically commit - renames temp file to final path
    if (!outFile.commit()) {
        qCWarning(QGClibarchiveLog) << "Failed to commit output file:" << outFile.errorString();
        return CopyResult::WriteFailed;
    }

    return CopyResult::Completed;
}

/// Whether an entry is a hard or symbolic link, which is then logged as skipped
/// The functions that write an entry with writeArchiveEntryToFile() leave links out: QSaveFile makes regular
/// files, so a link would become an empty one. extractArchiveEntries() creates the symbolic links that stay
/// inside its output directory instead.
bool skipLinkEntry(struct archive_entry* entry)
{
    if (archive_entry_hardlink(entry) != nullptr) {
        qCWarning(QGClibarchiveLog) << "Skipping hard link:" << archive_entry_pathname(entry) << "->"
                                    << archive_entry_hardlink(entry);
        return true;
    }
    if (archive_entry_filetype(entry) == AE_IFLNK) {
        qCWarning(QGClibarchiveLog) << "Skipping symlink:" << archive_entry_pathname(entry) << "->"
                                    << archive_entry_symlink(entry);
        return true;
    }
    return false;
}

/// Write current archive entry data to a file atomically
/// @param a Archive handle (must have entry header already read)
/// @param outputPath Output file path (parent directory must exist)
/// @param transfer Limit and progress of the operation the entry is part of
/// @return true on success, false on read/write error (no partial file left on failure)
bool writeArchiveEntryToFile(struct archive* a, const QString& outputPath, DataTransfer& transfer)
{
    const CopyResult copied = copyEntryToFile(a, outputPath, transfer);
    if (copied == CopyResult::ReadFailed) {
        qCWarning(QGClibarchiveLog) << "Read error:" << archive_error_string(a);
    }
    return copied == CopyResult::Completed;
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
    DataTransfer transfer{.maxBytes = maxBytes, .progress = progress, .totalSize = totalSize};
    const CopyResult copied = copyEntryToFile(a, outputPath, transfer);
    if (copied == CopyResult::ReadFailed) {
        qCWarning(QGClibarchiveLog) << "Decompression error:" << archive_error_string(a);
    }
    if (copied != CopyResult::Completed) {
        return false;
    }

    qCDebug(QGClibarchiveLog) << "Decompressed" << transfer.bytesWritten << "bytes to" << outputPath;
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

/// Canonical path of an extraction root. Resolving it first keeps a symlinked parent (for example
/// /var -> /private/var on macOS) from tripping the containment and secure symlink checks.
QString canonicalDirectoryPath(const QString& directoryPath)
{
    const QString canonicalPath = QFileInfo(directoryPath).canonicalFilePath();
    return canonicalPath.isEmpty() ? QFileInfo(directoryPath).absoluteFilePath() : canonicalPath;
}

/// Whether @a path, absolute and free of "." and "..", is inside the extraction root and stays there
/// on disk: its nearest existing component, which may be the path itself, has to resolve inside the
/// root, because a symbolic link there would redirect the write.
bool isContainedPath(const QString& canonicalOutputDir, const QString& path)
{
    const auto isInsideRoot = [&canonicalOutputDir](const QString& candidate) {
        return candidate == canonicalOutputDir || candidate.startsWith(canonicalOutputDir + QLatin1Char('/'));
    };

    if (!isInsideRoot(path)) {
        return false;
    }

    QFileInfo existing(path);
    while (!existing.exists() && !existing.isSymLink()) {
        const QString parentPath = existing.path();
        if (parentPath == existing.filePath()) {
            return false;
        }
        existing.setFile(parentPath);
    }
    return isInsideRoot(existing.canonicalFilePath());
}

/// Path an archive entry is extracted to. Every extraction path resolves entry names through this.
/// @param canonicalOutputDir Extraction root as returned by canonicalDirectoryPath()
/// @param entryName Entry path as stored in the archive
/// @return Absolute path inside the root, or an empty string if the entry would land outside it
QString containedEntryPath(const QString& canonicalOutputDir, const QString& entryName)
{
    const QString path = QDir::cleanPath(QGCFileHelper::joinPath(canonicalOutputDir, entryName));
    return isContainedPath(canonicalOutputDir, path) ? path : QString();
}

/// Path an entry is extracted to by libarchive's disk writer, which creates links as well as files
/// @param entry Entry as read from the archive
/// @param canonicalOutputDir Extraction root as returned by canonicalDirectoryPath()
/// @return Absolute path inside the root, or an empty string (after a warning) if the entry has to be skipped
QString diskEntryPath(struct archive_entry* entry, const QString& canonicalOutputDir)
{
    const char* currentFile = archive_entry_pathname(entry);
    const QString entryName = QString::fromUtf8(currentFile);

    // Prevent path traversal attacks
    const QString outputPath = containedEntryPath(canonicalOutputDir, entryName);
    if (outputPath.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Skipping path traversal attempt:" << currentFile;
        return {};
    }

    // Security: reject filenames with embedded null bytes (libarchive issue #2774)
    if (entryName.contains(QChar('\0'))) {
        qCWarning(QGClibarchiveLog) << "Skipping path with embedded null byte";
        return {};
    }

    // Security: libarchive resolves a hard link's target against the working directory, not the
    // output directory, and writes the entry's data through the link
    if (archive_entry_hardlink(entry) != nullptr) {
        qCWarning(QGClibarchiveLog) << "Skipping hard link:" << currentFile << "->" << archive_entry_hardlink(entry);
        return {};
    }

    // Security: validate symlink targets don't escape output directory
    if (archive_entry_filetype(entry) == AE_IFLNK) {
        const char* symlinkTarget = archive_entry_symlink(entry);
        if (symlinkTarget) {
            QString target = QString::fromUtf8(symlinkTarget);

            if (QFileInfo(target).isAbsolute()) {
                // Absolute symlink - reject outright
                qCWarning(QGClibarchiveLog)
                    << "Skipping symlink with absolute target:" << currentFile << "->" << target;
                return {};
            }

            // Where ".." leads depends on the links before it in the target, which the archive can
            // still create after this check, so no target with ".." is accepted
            target.replace(QLatin1Char('\\'), QLatin1Char('/'));
            if (target.split(QLatin1Char('/')).contains(QLatin1String(".."))) {
                qCWarning(QGClibarchiveLog)
                    << "Skipping symlink with \"..\" in its target:" << currentFile << "->" << symlinkTarget;
                return {};
            }

            // Verify resolved target stays within output directory
            const QString resolvedTarget = QDir::cleanPath(QFileInfo(outputPath).path() + QLatin1Char('/') + target);
            if (!isContainedPath(canonicalOutputDir, resolvedTarget)) {
                qCWarning(QGClibarchiveLog)
                    << "Skipping symlink escaping output directory:" << currentFile << "->" << symlinkTarget;
                return {};
            }
        }
    }

    return outputPath;
}

/// Extract entries from an open archive to a directory
/// @param reader Archive reader (already opened with format support configured)
/// @param outputDirectoryPath Directory to extract to (must exist)
/// @param progress Optional progress callback
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @return true on success
bool extractArchiveEntries(const QGClibarchive::ArchiveReader& reader, const QString& outputDirectoryPath,
                           const QGClibarchive::ProgressCallback& progress, qint64 maxBytes)
{
    DiskWriterPtr writer(archive_write_disk_new());
    if (!writer) {
        qCWarning(QGClibarchiveLog) << "Failed to create disk writer";
        return false;
    }
    struct archive* const a = reader.handle();
    struct archive* const ext = writer.get();
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
    DataTransfer transfer{.maxBytes = maxBytes, .progress = progress, .totalSize = reader.dataSize()};
    CopyResult copied = CopyResult::Completed;

    // Track created entries for safe cleanup on failure
    QStringList createdFiles;
    QStringList createdDirs;
    QSet<QString> existingDirs;

    const QString canonicalOutputDir = canonicalDirectoryPath(outputDirectoryPath);
    existingDirs.insert(canonicalOutputDir);

    const QGClibarchive::EntryWalk walk = QGClibarchive::forEachEntry(a, [&](struct archive_entry* entry) {
        const QString outputPath = diskEntryPath(entry, canonicalOutputDir);
        if (outputPath.isEmpty()) {
            return true;
        }

        // Security: mask permissions for cross-platform safety
        // Files: 0644 (rw-r--r--), Directories: 0755 (rwxr-xr-x)
        const auto fileType = archive_entry_filetype(entry);
        if (fileType == AE_IFDIR) {
            archive_entry_set_perm(entry, 0755);
        } else if (fileType == AE_IFREG) {
            archive_entry_set_perm(entry, 0644);
        }
        // Note: symlinks don't have meaningful permissions on most systems

        archive_entry_set_pathname(entry, outputPath.toUtf8().constData());

        // Create parent directories and track newly created ones
        trackAndCreateParentDirs(outputPath, canonicalOutputDir, existingDirs, createdDirs);

        if (archive_write_header(ext, entry) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to write header for" << outputPath << ":"
                                        << archive_error_string(ext);
            success = false;
            return false;
        }

        // Track created file/directory for cleanup
        if (fileType == AE_IFDIR) {
            if (!existingDirs.contains(outputPath)) {
                createdDirs.append(outputPath);
                existingDirs.insert(outputPath);
            }
        } else {
            createdFiles.append(outputPath);
        }

        // An entry of a streamed ZIP has data but no size in its header
        if (!archive_entry_size_is_set(entry) || archive_entry_size(entry) > 0) {
            copied = copyEntryData(a, transfer, [ext](const char* data, qint64 size, qint64 offset) {
                if (archive_write_data_block(ext, data, static_cast<size_t>(size), offset) != ARCHIVE_OK) {
                    qCWarning(QGClibarchiveLog) << "Failed to write data:" << archive_error_string(ext);
                    return false;
                }
                return true;
            });

            if (copied == CopyResult::ReadFailed) {
                qCWarning(QGClibarchiveLog) << "Failed to read data:" << archive_error_string(a);
            }
            if (copied != CopyResult::Completed) {
                success = false;
                return false;
            }
        }

        if (archive_write_finish_entry(ext) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to finish entry:" << archive_error_string(ext);
            success = false;
            return false;
        }

        return true;
    });

    if (walk == QGClibarchive::EntryWalk::Failed) {
        success = false;
    }

    // Closes the file being written, which has to happen before it can be removed
    writer.reset();

    // Clean up only created entries on cancellation or size limit exceeded
    if (copied == CopyResult::Cancelled || copied == CopyResult::SizeLimitExceeded) {
        cleanupCreatedEntries(createdFiles, createdDirs);
    }

    return success;
}

/// Extract all of an archive file to a directory. The archive is closed again on return, so the
/// caller can move the directory even if the archive is inside it.
bool extractArchiveFile(const QString& archivePath, const QString& outputDirectoryPath,
                        const QGClibarchive::ProgressCallback& progress, qint64 maxBytes)
{
    QGClibarchive::ArchiveReader reader;
    return reader.open(archivePath, QGClibarchive::ReaderMode::AllFormats) &&
           extractArchiveEntries(reader, outputDirectoryPath, progress, maxBytes);
}

/// Advance to the entry called @a fileName, whose data can then be read
/// @param entry If not null, receives the entry when Found is returned
/// @return Found, End if the archive has no such entry, or Error if the archive is damaged
QGClibarchive::NextEntry findEntry(struct archive* a, const QString& fileName, struct archive_entry** entry = nullptr)
{
    const QGClibarchive::EntryWalk walk = QGClibarchive::forEachEntry(a, [&](struct archive_entry* candidate) {
        if (QString::fromUtf8(archive_entry_pathname(candidate)) != fileName) {
            return true;
        }
        if (entry != nullptr) {
            *entry = candidate;
        }
        return false;
    });

    switch (walk) {
        case QGClibarchive::EntryWalk::Stopped:
            return QGClibarchive::NextEntry::Found;
        case QGClibarchive::EntryWalk::Completed:
            return QGClibarchive::NextEntry::End;
        case QGClibarchive::EntryWalk::Failed:
            break;
    }
    return QGClibarchive::NextEntry::Error;
}

/// As findEntry(), for a caller that reads the entry: a warning is logged if the archive does not have it
/// @return Found if the reader is on the entry
QGClibarchive::NextEntry findEntryToRead(struct archive* a, const QString& fileName,
                                         struct archive_entry** entry = nullptr)
{
    const QGClibarchive::NextEntry found = findEntry(a, fileName, entry);
    if (found == QGClibarchive::NextEntry::End) {
        qCWarning(QGClibarchiveLog) << "File not found in archive:" << fileName;
    }
    return found;
}

/// Read the entry called @a fileName, which the reader has not passed yet, into a QByteArray
/// @param maxBytes Maximum size of the entry (0 = unlimited)
/// @param result Receives how the read ended
/// @return Data of the entry, or empty QByteArray unless @a result is Completed
QByteArray readNamedEntryToMemory(struct archive* a, const QString& fileName, qint64 maxBytes,
                                  QGClibarchive::ReadResult& result)
{
    struct archive_entry* entry = nullptr;
    switch (findEntryToRead(a, fileName, &entry)) {
        case QGClibarchive::NextEntry::Found:
            break;
        case QGClibarchive::NextEntry::End:
            result = QGClibarchive::ReadResult::EntryNotFound;
            return {};
        case QGClibarchive::NextEntry::Error:
            result = QGClibarchive::ReadResult::Failed;
            return {};
    }

    bool completed = false;
    QByteArray data = readEntryToMemory(a, entry, maxBytes, completed);
    result = completed ? QGClibarchive::ReadResult::Completed : QGClibarchive::ReadResult::Failed;
    return data;
}

/// Read the header of the one entry that a single compressed stream is read as
/// @return false, after logging a warning, if the stream cannot be read
bool readStreamHeader(struct archive* a)
{
    struct archive_entry* entry = nullptr;
    if (archive_read_next_header(a, &entry) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to read header:" << archive_error_string(a);
        return false;
    }

    updateFormatState(a);
    return true;
}

}  // namespace

// ============================================================================
// QGClibarchive Implementation
// ============================================================================

namespace QGClibarchive {

// ----------------------------------------------------------------------------
// ArchiveReader
// ----------------------------------------------------------------------------

qint64 ArchiveReader::dataSize() const
{
    if (!_memoryData.isEmpty()) {
        return _memoryData.size();
    }
    if (_source.device) {
        return _source.device->size() > 0 ? _source.device->size() : 0;
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
    const char* name = archive_format_name(_archive.get());
    return name ? QString::fromUtf8(name) : QString();
}

QString ArchiveReader::filterName() const
{
    if (!_archive) {
        return {};
    }
    // Get the outermost compression filter (index 0 is the format, 1+ are filters)
    // archive_filter_count returns total filters including format at index 0
    const int filterCount = archive_filter_count(_archive.get());
    if (filterCount <= 1) {
        return QStringLiteral("none");
    }
    // Index 0 is innermost (format), higher indices are outer filters
    // We want the first compression filter (index 1)
    const char* name = archive_filter_name(_archive.get(), 0);
    return name ? QString::fromUtf8(name) : QStringLiteral("none");
}

void ArchiveReader::close()
{
    _archive.reset();
    _filePath.clear();
    _memoryData.clear();
    _source = {};
}

bool ArchiveReader::_create(ReaderMode mode, const QByteArray& signature)
{
    close();

    if (mode == ReaderMode::RawFormat && !canDecodeStream(signature)) {
        return false;
    }

    _archive.reset(archive_read_new());
    if (!_archive) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive reader";
        return false;
    }

    configureReader(_archive.get(), mode);
    return true;
}

bool ArchiveReader::open(const QString& path, ReaderMode mode)
{
    QByteArray signature;
    if (mode == ReaderMode::RawFormat) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            signature = file.read(kStreamSignatureSize);
        }
    }

    if (!_create(mode, signature)) {
        return false;
    }
    _filePath = path;

    if (!openArchiveForReading(_archive.get(), path, _memoryData)) {
        close();
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

    QString errorString;
    if (!open(device, mode, errorString)) {
        if (!errorString.isEmpty()) {
            qCWarning(QGClibarchiveLog).noquote() << "Failed to open device:" << errorString;
        }
        return false;
    }

    return true;
}

bool ArchiveReader::open(QIODevice* device, ReaderMode mode, QString& errorString)
{
    if (!device || !device->isOpen() || !device->isReadable()) {
        errorString = QStringLiteral("Device is null, not open, or not readable");
        return false;
    }

    const QByteArray signature = (mode == ReaderMode::RawFormat) ? device->peek(kStreamSignatureSize) : QByteArray();
    if (!_create(mode, signature)) {
        return false;
    }
    _source.device = device;
    _source.block = QByteArray(static_cast<qsizetype>(QGCFileHelper::kBufferSizeMax), Qt::Uninitialized);

    // Enable seek callback for random-access devices (improves ZIP performance)
    // Must be set before opening the archive
    if (!device->isSequential()) {
        archive_read_set_seek_callback(_archive.get(), deviceSeekCallback);
    }

    const int result = archive_read_open2(_archive.get(), &_source,
                                          nullptr,  // open callback (not needed)
                                          deviceReadCallback, deviceSkipCallback, deviceCloseCallback);

    if (result != ARCHIVE_OK) {
        errorString = QString::fromUtf8(archive_error_string(_archive.get()));
        if (errorString.isEmpty()) {
            errorString = QStringLiteral("Unknown error");
        }
        close();
        return false;
    }

    return true;
}

bool ArchiveReader::openMemory(const QByteArray& data, ReaderMode mode)
{
    if (!_create(mode, data)) {
        return false;
    }
    // A shared copy keeps the bytes libarchive reads from alive as long as the reader
    _memoryData = data;

    if (archive_read_open_memory(_archive.get(), _memoryData.constData(), static_cast<size_t>(_memoryData.size())) !=
        ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to open compressed data:" << archive_error_string(_archive.get());
        close();
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

static bool bzip2DecoderAvailable()
{
#ifdef QGC_ENABLE_BZIP2
    return archive_bzlib_version() != nullptr;
#else
    return false;
#endif
}

static bool lz4DecoderAvailable()
{
#ifdef QGC_ENABLE_LZ4
    return archive_liblz4_version() != nullptr;
#else
    return false;
#endif
}

void configureReader(struct archive* a, ReaderMode mode)
{
    // archive_read_support_filter_all() and _format_all() are deliberately not used: the input is
    // untrusted, and a filter without its library compiled in is decoded by an external program.
    if (archive_zlib_version() != nullptr) {
        archive_read_support_filter_gzip(a);
    }
    if (archive_liblzma_version() != nullptr) {
        archive_read_support_filter_xz(a);
        archive_read_support_filter_lzma(a);
    }
    if (archive_libzstd_version() != nullptr) {
        archive_read_support_filter_zstd(a);
    }
    if (bzip2DecoderAvailable()) {
        archive_read_support_filter_bzip2(a);
    }
    if (lz4DecoderAvailable()) {
        archive_read_support_filter_lz4(a);
    }

    switch (mode) {
        case ReaderMode::AllFormats:
            archive_read_support_format_zip(a);
            archive_read_support_format_tar(a);
            archive_read_support_format_7zip(a);
            break;
        case ReaderMode::RawFormat:
            archive_read_support_format_raw(a);
            break;
    }
}

bool canDecodeStream(const QByteArray& signature)
{
    if (!bzip2DecoderAvailable() && signature.startsWith("BZh")) {
        qCWarning(QGClibarchiveLog) << "bzip2 decompression is not supported by this build";
        return false;
    }
    if (!lz4DecoderAvailable() && signature.startsWith(QByteArrayView("\x04\x22\x4D\x18", 4))) {
        qCWarning(QGClibarchiveLog) << "LZ4 decompression is not supported by this build";
        return false;
    }
    return true;
}

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

    return extractArchiveFile(archivePath, outputDirectoryPath, progress, maxBytes);
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
    if (!extractArchiveFile(archivePath, stagingPath, progress, maxBytes)) {
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

    struct archive_entry* entry = nullptr;
    if (findEntryToRead(reader.handle(), fileName, &entry) != NextEntry::Found) {
        return false;
    }
    if (skipLinkEntry(entry)) {
        return false;
    }

    QGCFileHelper::ensureParentExists(outputPath);
    DataTransfer unlimited;
    return writeArchiveEntryToFile(reader.handle(), outputPath, unlimited);
}

QByteArray extractFileToMemory(const QString& archivePath, const QString& fileName, qint64 maxBytes, ReadResult* result)
{
    ReadResult unused = ReadResult::Failed;
    ReadResult& outcome = (result != nullptr) ? *result : unused;
    outcome = ReadResult::Failed;

    if (fileName.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Empty file name";
        return QByteArray();
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return QByteArray();
    }

    QByteArray data = readNamedEntryToMemory(reader.handle(), fileName, maxBytes, outcome);
    if (outcome == ReadResult::Completed) {
        qCDebug(QGClibarchiveLog) << "Extracted" << fileName << "to memory:" << data.size() << "bytes";
    }
    return data;
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

    const QString canonicalOutputDir = canonicalDirectoryPath(outputDirectoryPath);
    QSet<QString> targetFiles(fileNames.begin(), fileNames.end());
    QSet<QString> extractedFiles;
    DataTransfer unlimited;
    bool failed = false;

    // The walk stops at the last of the files asked for, without reading the rest of the archive
    const EntryWalk walk = forEachEntry(reader.handle(), [&](struct archive_entry* entry) {
        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));

        if (!targetFiles.contains(entryName)) {
            return true;
        }

        // Asked for by name, so an entry that is left out fails the extraction
        if (skipLinkEntry(entry)) {
            failed = true;
            return false;
        }

        const QString outputPath = containedEntryPath(canonicalOutputDir, entryName);
        if (outputPath.isEmpty()) {
            qCWarning(QGClibarchiveLog) << "Path traversal attempt:" << entryName;
            failed = true;
            return false;
        }
        QGCFileHelper::ensureParentExists(outputPath);

        if (!writeArchiveEntryToFile(reader.handle(), outputPath, unlimited)) {
            failed = true;
            return false;
        }

        extractedFiles.insert(entryName);

        return extractedFiles.size() != targetFiles.size();
    });

    if (failed || walk == EntryWalk::Failed) {
        return false;
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

    const EntryMatchPtr match(archive_match_new());
    if (!match) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive_match";
        return false;
    }

    for (const QString& pattern : patterns) {
        if (archive_match_include_pattern(match.get(), pattern.toUtf8().constData()) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Invalid pattern:" << pattern;
            return false;
        }
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    const QString canonicalOutputDir = canonicalDirectoryPath(outputDirectoryPath);
    DataTransfer unlimited;
    int matchCount = 0;

    const EntryWalk walk = forEachEntry(reader.handle(), [&](struct archive_entry* entry) {
        if (archive_match_excluded(match.get(), entry)) {
            return true;
        }

        if (archive_entry_filetype(entry) == AE_IFDIR) {
            return true;
        }

        if (skipLinkEntry(entry)) {
            return true;
        }

        const QString entryName = QString::fromUtf8(archive_entry_pathname(entry));
        const QString outputPath = containedEntryPath(canonicalOutputDir, entryName);
        if (outputPath.isEmpty()) {
            qCWarning(QGClibarchiveLog) << "Skipping path traversal attempt:" << entryName;
            return true;
        }

        QGCFileHelper::ensureParentExists(outputPath);

        if (!writeArchiveEntryToFile(reader.handle(), outputPath, unlimited)) {
            return false;
        }

        matchCount++;
        if (extractedFiles) {
            extractedFiles->append(entryName);
        }
        return true;
    });

    // The walk is only stopped by an entry that could not be written
    if (walk != EntryWalk::Completed) {
        return false;
    }

    if (archive_match_path_unmatched_inclusions(match.get()) > 0) {
        const char* unmatched;
        while (archive_match_path_unmatched_inclusions_next(match.get(), &unmatched) == ARCHIVE_OK) {
            qCDebug(QGClibarchiveLog) << "Pattern not matched:" << unmatched;
        }
    }

    qCDebug(QGClibarchiveLog) << "Extracted" << matchCount << "files matching patterns from" << archivePath;
    return matchCount > 0;
}

bool validateArchive(const QString& archivePath)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return false;
    }

    // Read through all entries and their data to validate
    DataTransfer unlimited;
    const EntryWalk walk = forEachEntry(reader.handle(), [&reader, &unlimited](struct archive_entry* entry) {
        // Read and discard data to verify it can be decompressed
        const CopyResult copied =
            copyEntryData(reader.handle(), unlimited, [](const char*, qint64, qint64) { return true; });
        if (copied != CopyResult::Completed) {
            qCWarning(QGClibarchiveLog) << "Validation failed for entry:" << archive_entry_pathname(entry)
                                        << archive_error_string(reader.handle());
            return false;
        }
        return true;
    });

    return walk == EntryWalk::Completed;
}

bool fileExistsInArchive(const QString& archivePath, const QString& fileName)
{
    if (fileName.isEmpty()) {
        return false;
    }

    return findEntryInArchive(archivePath, fileName) == NextEntry::Found;
}

NextEntry findEntryInArchive(const QString& archivePath, const QString& fileName)
{
    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return NextEntry::Error;
    }

    return findEntry(reader.handle(), fileName);
}

QStringList listArchiveEntries(const QString& archivePath, bool* ok)
{
    if (ok != nullptr) {
        *ok = false;
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return {};
    }

    QStringList entries;
    const EntryWalk walk = forEachEntry(reader.handle(), [&entries](struct archive_entry* entry) {
        entries.append(QString::fromUtf8(archive_entry_pathname(entry)));
        return true;
    });
    if (walk != EntryWalk::Completed) {
        return {};
    }
    if (ok != nullptr) {
        *ok = true;
    }

    qCDebug(QGClibarchiveLog) << "Listed" << entries.size() << "entries in" << archivePath;
    return entries;
}

QList<ArchiveEntry> listArchiveEntriesDetailed(const QString& archivePath, bool* ok)
{
    if (ok != nullptr) {
        *ok = false;
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return {};
    }

    QList<ArchiveEntry> entries;
    const EntryWalk walk = forEachEntry(reader.handle(), [&entries](struct archive_entry* entry) {
        entries.append(toArchiveEntry(entry));
        return true;
    });
    if (walk != EntryWalk::Completed) {
        return {};
    }
    if (ok != nullptr) {
        *ok = true;
    }

    qCDebug(QGClibarchiveLog) << "Listed" << entries.size() << "entries with metadata in" << archivePath;
    return entries;
}

ArchiveStats getArchiveStats(const QString& archivePath, bool* ok)
{
    if (ok != nullptr) {
        *ok = false;
    }

    ArchiveReader reader;
    if (!reader.open(archivePath, ReaderMode::AllFormats)) {
        return {};
    }

    ArchiveStats stats;

    const EntryWalk walk = forEachEntry(reader.handle(), [&stats](struct archive_entry* entry) {
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
        return true;
    });

    if (walk != EntryWalk::Completed) {
        return {};
    }
    if (ok != nullptr) {
        *ok = true;
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

    const QString canonicalOutputDir = canonicalDirectoryPath(outputDirectoryPath);
    DataTransfer transfer{.maxBytes = maxBytes, .progress = progress, .totalSize = reader.dataSize()};
    qint64 bytesExtracted = 0;
    int extractedCount = 0;
    int skippedCount = 0;

    const EntryWalk walk = forEachEntry(reader.handle(), [&](struct archive_entry* entry) {
        // Build ArchiveEntry for filter callback
        const ArchiveEntry info = toArchiveEntry(entry);

        // Check filter
        if (!filter(info)) {
            skippedCount++;
            return true;
        }

        // Skip directories for extraction (they're created by ensureParentExists)
        if (info.isDirectory) {
            return true;
        }

        if (skipLinkEntry(entry)) {
            return true;
        }

        // An entry that says it does not fit is not started. One that says less than it holds, or
        // nothing, is stopped at the limit while it is written.
        if (maxBytes > 0 && (bytesExtracted + info.size) > maxBytes) {
            reportSizeLimitExceeded(maxBytes);
            return false;
        }

        const QString outputPath = containedEntryPath(canonicalOutputDir, info.name);
        if (outputPath.isEmpty()) {
            qCWarning(QGClibarchiveLog) << "Skipping path traversal attempt:" << info.name;
            return true;
        }
        QGCFileHelper::ensureParentExists(outputPath);

        if (!writeArchiveEntryToFile(reader.handle(), outputPath, transfer)) {
            return false;
        }

        bytesExtracted += info.size;
        extractedCount++;
        return true;
    });

    // The walk is only stopped by the size limit, a failed write or a cancellation
    if (walk != EntryWalk::Completed) {
        return false;
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

    if (!readStreamHeader(reader.handle())) {
        return false;
    }

    QGCFileHelper::ensureParentExists(outputPath);

    return decompressStreamToFile(reader.handle(), outputPath, progress, reader.dataSize(), maxBytes);
}

QByteArray decompressDataFromMemory(const QByteArray& data, qint64 maxBytes)
{
    if (data.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Cannot decompress empty data";
        return {};
    }

    ArchiveReader reader;
    if (!reader.openMemory(data, ReaderMode::RawFormat)) {
        return {};
    }

    if (!readStreamHeader(reader.handle())) {
        return {};
    }

    QByteArray result = readArchiveToMemory(reader.handle(), 0, maxBytes);
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

    return extractArchiveEntries(reader, outputDirectoryPath, progress, maxBytes);
}

QByteArray extractFileDataFromDevice(QIODevice* device, const QString& fileName, qint64 maxBytes, ReadResult* result)
{
    ReadResult unused = ReadResult::Failed;
    ReadResult& outcome = (result != nullptr) ? *result : unused;
    outcome = ReadResult::Failed;

    if (fileName.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Empty file name";
        return {};
    }

    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::AllFormats)) {
        return {};
    }

    QByteArray data = readNamedEntryToMemory(reader.handle(), fileName, maxBytes, outcome);
    if (outcome == ReadResult::Completed) {
        qCDebug(QGClibarchiveLog) << "Extracted" << fileName << "from device:" << data.size() << "bytes";
    }
    return data;
}

bool decompressFromDevice(QIODevice* device, const QString& outputPath, ProgressCallback progress, qint64 maxBytes)
{
    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::RawFormat)) {
        return false;
    }

    QGCFileHelper::ensureParentExists(outputPath);

    if (!readStreamHeader(reader.handle())) {
        return false;
    }

    return decompressStreamToFile(reader.handle(), outputPath, progress, reader.dataSize(), maxBytes);
}

QByteArray decompressDataFromDevice(QIODevice* device, qint64 maxBytes)
{
    ArchiveReader reader;
    if (!reader.open(device, ReaderMode::RawFormat)) {
        return {};
    }

    if (!readStreamHeader(reader.handle())) {
        return {};
    }

    QByteArray result = readArchiveToMemory(reader.handle(), 0, maxBytes);
    if (!result.isEmpty()) {
        qCDebug(QGClibarchiveLog) << "Decompressed from device:" << result.size() << "bytes";
    }
    return result;
}

}  // namespace QGClibarchive
