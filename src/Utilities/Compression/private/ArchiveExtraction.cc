#include "ArchiveExtraction.h"

#include <algorithm>
#include <archive_entry.h>
#include <climits>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>

#include "EntryData.h"
#include "EntryNames.h"
#include "QGCFileHelper.h"

namespace LibArchive {

namespace {

using ArchiveEntry = Compression::ArchiveEntry;
using ExtractOptions = Compression::ExtractOptions;

// ============================================================================
// Entries
// ============================================================================

/// Path of an entry within the archive
/// libarchive converts a name from the charset of the archive to the one asked for. The UTF-8 form is exact.
/// archive_entry_pathname() gives the name in the charset of the C locale, which on Windows cannot hold every
/// name. A name without a UTF-8 form, because its bytes are not valid in the charset of the archive, is read from
/// those bytes, with a replacement character for each one that is not UTF-8. On Windows libarchive gives no such
/// name at all, and the entry is skipped.
/// A backslash is a separator, as archivers on Windows write it, on every system. libarchive turns it into a slash
/// only in a name of a ZIP archive made on Windows that has no slash.
QString pathnameOf(struct archive_entry* entry)
{
    const char* const name = archive_entry_pathname_utf8(entry);
    return QString::fromUtf8((name != nullptr) ? name : archive_entry_pathname(entry))
        .replace(QLatin1Char('\\'), QLatin1Char('/'));
}

/// An entry name as a warning shows it: a long one is cut
QString shownName(const QString& entryName)
{
    constexpr qsizetype kShownCharacters = 200;
    return (entryName.size() > kShownCharacters) ? (entryName.left(kShownCharacters) + QStringLiteral("..."))
                                                 : entryName;
}

/// The entry as an entry filter sees it
ArchiveEntry toArchiveEntry(struct archive_entry* entry)
{
    return {.name = pathnameOf(entry), .isDirectory = (archive_entry_filetype(entry) == AE_IFDIR)};
}

/// Log that an entry of @a kind is skipped
/// @param target Of a link: what it points to. A symbolic link can have none
/// @return true, so that the functions below can return it for an entry they skip
bool logSkipped(struct archive_entry* entry, const char* kind, const char* target = nullptr)
{
    const QString skipping = QStringLiteral("Skipping %1:").arg(QLatin1String(kind));
    if ((target != nullptr) && (*target != '\0')) {
        qCWarning(LibArchiveLog).noquote() << skipping << pathnameOf(entry) << "->" << target;
    } else {
        qCWarning(LibArchiveLog).noquote() << skipping << pathnameOf(entry);
    }
    return true;
}

/// Whether an entry is a link, hard or symbolic, which is then logged as skipped
/// Links are never created. QGC has no use for a second name of a file, and a link would let a later entry, or
/// whoever reads the output, reach a file outside the output directory.
bool skipLinkEntry(struct archive_entry* entry)
{
    // Not archive_entry_hardlink(): it gives no target, as for a file, if the C locale cannot hold the target
    if (archive_entry_hardlink_is_set(entry) != 0) {
        return logSkipped(entry, "hard link", archive_entry_hardlink_utf8(entry));
    }
    return (archive_entry_filetype(entry) == AE_IFLNK) &&
           logSkipped(entry, "symbolic link", archive_entry_symlink_utf8(entry));
}

/// Kind of special file an entry is, or nullptr if it is none
const char* specialFileKind(struct archive_entry* entry)
{
    switch (archive_entry_filetype(entry)) {
        case AE_IFIFO:
            return "FIFO";
        case AE_IFCHR:
            return "character device";
        case AE_IFBLK:
            return "block device";
        case AE_IFSOCK:
            return "socket";
        default:
            return nullptr;
    }
}

/// Whether an entry is a FIFO, a device node or a socket, which is then logged as skipped
/// Special files are never created. QGC has no use for them, and written as a regular file one would be empty.
bool skipSpecialEntry(struct archive_entry* entry)
{
    const char* const kind = specialFileKind(entry);
    return (kind != nullptr) && logSkipped(entry, kind);
}

/// Whether an entry is a sparse file, which is then logged as skipped
/// libarchive reads the map of the parts that a sparse file stores with its header, before any data. QGC has no
/// use for sparse files, and only a TAR archive holds them.
bool skipSparseEntry(struct archive_entry* entry)
{
    return (archive_entry_sparse_count(entry) > 0) && logSkipped(entry, "sparse file");
}

// ============================================================================
// Extraction
// ============================================================================

/// Canonical path of an extraction root. Resolving it first keeps a symlinked parent (for example
/// /var -> /private/var on macOS) from tripping the containment check.
QString canonicalDirectoryPath(const QString& directoryPath)
{
    const QString canonicalPath = QFileInfo(directoryPath).canonicalFilePath();
    return canonicalPath.isEmpty() ? QFileInfo(directoryPath).absoluteFilePath() : canonicalPath;
}

/// Remove everything in a directory, and keep the directory
void removeDirectoryContent(const QString& directoryPath)
{
    const QFileInfoList entries =
        QDir(directoryPath).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        if (entry.isDir() && !entry.isSymLink()) {
            (void) QDir(entry.filePath()).removeRecursively();
        } else {
            (void) QFile::remove(entry.filePath());
        }
    }
}

/// One extraction of an archive into a directory that does not exist or is empty
class Extraction
{
public:
    explicit Extraction(const ExtractOptions& options)
        : _options(options)
    {}

    /// Extract the archive, as Compression::extractArchive() documents
    Result run(const QString& archivePath, const QString& outputDirectoryPath);

    /// The compression filter of the archive, once its first entry has been read
    QString filterName() const { return _reader.filterName(); }

    /// Entries skipped with a warning so far
    int skippedEntries() const { return _skippedEntries; }

private:
    Result _extractEntries();
    Result _extractEntry(struct archive_entry* entry);
    QString _entryPath(struct archive_entry* entry) const;
    bool _claimSpelling(const QString& path, const QString& entryName);
    Result _makeDirectory(const QString& path);
    Result _writeFile(struct archive_entry* entry, const QString& path);

    ArchiveReader _reader;
    const ExtractOptions& _options;

    QString _root;  ///< Canonical path of the output directory
    /// Spelling of each file and directory written. An entry that the filter refuses writes nothing, and claims none
    EntrySpellings _spellings;
    int _skippedEntries = 0;
    /// Holds each file to what is left of the limit and of the room on the disk: before it is written, by the
    /// size its entry declares, and while it is written, by the data the entry holds
    DataTransfer _transfer;
};

Result Extraction::run(const QString& archivePath, const QString& outputDirectoryPath)
{
    qCDebug(LibArchiveLog) << "Extracting" << archivePath << "to" << outputDirectoryPath;

    const bool madeOutputDirectory = !QFileInfo::exists(outputDirectoryPath);
    if (!QGCFileHelper::ensureDirectoryExists(outputDirectoryPath)) {
        return writeFailure(outputDirectoryPath,
                            QStringLiteral("Failed to create output directory: ") + outputDirectoryPath);
    }
    _root = canonicalDirectoryPath(outputDirectoryPath);

    // Checked again where the cleanup is set up, because the cleanup removes everything in the directory
    if (!QDir(_root).isEmpty(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)) {
        return failure(Error::IoError, QStringLiteral("Output directory is not empty: ") + _root);
    }

    // The directory is empty, so everything in it will be what the extraction wrote. A failure removes it, also
    // one that an exception from the entry filter or the progress callback causes on its way to the caller
    auto removeOutput = qScopeGuard([this, madeOutputDirectory]() {
        if (madeOutputDirectory) {
            (void) QDir(_root).removeRecursively();
        } else {
            removeDirectoryContent(_root);
        }
    });

    if (Result opened = _reader.open(archivePath, Input::Archive); !opened) {
        return opened;
    }
    _transfer = transferInto(outputDirectoryPath, _options.maxBytes, _options.progress, _reader.dataSize(),
                             "Failed to read data:");
    Result extracted = _extractEntries();
    if (extracted) {
        removeOutput.dismiss();
    }
    return extracted;
}

/// Extract the entries of the open archive in order
/// Succeeds only if the archive was read to its end: a read error is never taken for the end of the
/// archive. Unread data of an entry is skipped before the next one. An entry that libarchive reads with a warning
/// (for example a pathname it could not convert) is usable: the warning is logged and the entry extracted. The
/// first entry also records the compression filter in the reader.
Result Extraction::_extractEntries()
{
    struct archive* const a = _reader.handle();
    struct archive_entry* entry = nullptr;
    bool filterKnown = false;
    while (true) {
        // Testing for ARCHIVE_OK alone would make a truncated or corrupt archive look like one that has ended
        switch (archive_read_next_header(a, &entry)) {
            case ARCHIVE_EOF:
                // The entries are written by now: an archive that fails here is removed like any other
                return _reader.verifyInput();
            case ARCHIVE_WARN:
                qCWarning(LibArchiveLog) << "Archive entry read with a warning:" << archive_error_string(a);
                break;
            case ARCHIVE_OK:
                break;
            default:
                return failure(Error::InvalidArchive,
                               QStringLiteral("Failed to read archive entry: ") + libarchiveError(a));
        }
        if (!filterKnown) {
            _reader.captureFilter();
            filterKnown = true;
        }
        if (Result extracted = _extractEntry(entry); !extracted) {
            return extracted;
        }
    }
}

/// Write one entry, whose header was just read, unless it is skipped
/// @return Success for an entry that is written or skipped, or the failure that ends the extraction
Result Extraction::_extractEntry(struct archive_entry* entry)
{
    // Called at each entry as well as after each block of data, because a skipped entry, a directory and an
    // empty file have no block
    if (_options.progress && !_options.progress(archive_filter_bytes(_reader.handle(), -1), _transfer.totalSize)) {
        return failure(Error::Cancelled);
    }
    if (_options.filter && !_options.filter(toArchiveEntry(entry))) {
        return {};
    }

    const QString path = _entryPath(entry);
    if (path.isEmpty() || !_claimSpelling(path, pathnameOf(entry))) {
        ++_skippedEntries;
        return {};
    }
    // A parent that cannot be created, because an earlier entry wrote a file there, makes the entry fail below
    (void) QDir().mkpath(QFileInfo(path).absolutePath());

    if (archive_entry_filetype(entry) == AE_IFDIR) {
        return _makeDirectory(path);
    }
    // A file that an earlier entry wrote is replaced. A directory in the way makes the entry fail
    return _writeFile(entry, path);
}

/// Path an entry of an archive is extracted to, as a file or a directory
/// @return Absolute path inside the root, or an empty string (after a warning) if the entry is skipped
QString Extraction::_entryPath(struct archive_entry* entry) const
{
    const QString entryName = pathnameOf(entry);

    // Skipped on every system, so that an archive extracts the same everywhere. First, because it also bounds the
    // length of the name that every later step works on
    if (const QString reason = nonPortableNameReason(entryName); !reason.isEmpty()) {
        qCWarning(LibArchiveLog).noquote()
            << QStringLiteral("Skipping entry whose name has %1:").arg(reason) << shownName(entryName);
        return {};
    }

    // Prevent path traversal attacks. A name such as "." or "a/.." resolves to the root itself. The extraction
    // creates no links, so a path inside the root stays there on disk
    const QString path = QDir::cleanPath(QGCFileHelper::joinPath(_root, entryName));
    if (!QGCFileHelper::isInDirectory(_root, path)) {
        qCWarning(LibArchiveLog).noquote() << "Skipping path traversal attempt:" << entryName;
        return {};
    }

#ifdef PATH_MAX
    // Skipped with a warning, because failing would end the whole extraction
    if (QFile::encodeName(path).size() >= PATH_MAX) {
        qCWarning(LibArchiveLog) << "Skipping path longer than the system takes:" << entryName.left(64);
        return {};
    }
#endif

    // An empty name, "." or "a/.." names the output directory. Only a directory entry may name it, and it leaves
    // the directory in place. Nothing else may take the place of the directory.
    if ((path == _root) && (archive_entry_filetype(entry) != AE_IFDIR)) {
        qCWarning(LibArchiveLog).noquote() << "Skipping entry that names the output directory:" << entryName;
        return {};
    }

    if (skipLinkEntry(entry) || skipSpecialEntry(entry) || skipSparseEntry(entry)) {
        return {};
    }

    return path;
}

/// Record the spelling of the path of an entry and of each directory above it in the output directory
/// A file system that ignores case or Unicode normalisation takes another spelling of a path that an earlier entry
/// wrote for that path. Such an entry is skipped on every system, so that an archive extracts the same everywhere.
/// @return false, after a warning, for an entry that is skipped
bool Extraction::_claimSpelling(const QString& path, const QString& entryName)
{
    const QStringView relativePath =
        QStringView(path).mid(_root.endsWith(QLatin1Char('/')) ? _root.size() : _root.size() + 1);
    if (const QString clash = _spellings.claim(relativePath); !clash.isEmpty()) {
        qCWarning(LibArchiveLog).noquote() << "Skipping entry whose name clashes with" << clash << ":" << entryName;
        return false;
    }
    return true;
}

/// Create the directory of an entry, unless it already exists
/// @return Success, or why the directory cannot be created
Result Extraction::_makeDirectory(const QString& path)
{
    if (QFileInfo(path).isDir() || QDir().mkdir(path)) {
        return {};
    }
    return writeFailure(path, QStringLiteral("Failed to create directory: ") + path);
}

/// Write the file of an entry, within what is left of the limit
Result Extraction::_writeFile(struct archive_entry* entry, const QString& path)
{
    // Do not start an entry that declares more than is left of the limit
    const qint64 declaredSize = archive_entry_size_is_set(entry) ? std::max<qint64>(0, archive_entry_size(entry)) : 0;
    if (Result fits = _transfer.fits(declaredSize); !fits) {
        return fits;
    }
    const QDateTime modified =
        archive_entry_mtime_is_set(entry) ? QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry)) : QDateTime();
    return copyEntryToFile(_reader, path, _transfer, modified);
}

}  // namespace

Result extractArchive(const QString& archivePath, const QString& outputDirectoryPath,
                      const Compression::ExtractOptions& options)
{
    Extraction extraction(options);
    Result extracted = extraction.run(archivePath, outputDirectoryPath);
    extracted.filterName = extraction.filterName();
    extracted.skippedEntries = extraction.skippedEntries();
    return extracted;
}

}  // namespace LibArchive
