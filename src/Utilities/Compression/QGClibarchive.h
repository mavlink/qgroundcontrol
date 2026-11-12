#pragma once

/// @file QGClibarchive.h
/// @brief Private implementation details for QGCCompression
/// @note This is an internal header - use QGCCompression.h for public API

#include <archive.h>
#include <functional>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "QGCCompressionTypes.h"

namespace QGClibarchive {

// ============================================================================
// Types (re-export from QGCCompression for internal use)
// ============================================================================

using Error = QGCCompression::Error;
using ProgressCallback = QGCCompression::ProgressCallback;
using ArchiveEntry = QGCCompression::ArchiveEntry;
using ArchiveStats = QGCCompression::ArchiveStats;
using EntryFilter = QGCCompression::EntryFilter;

using QGCCompression::kDefaultFilePermissions;

// ============================================================================
// Archive Reader Mode
// ============================================================================

/// Mode for ArchiveReader format support
enum class ReaderMode {
    AllFormats,  ///< Support the archive formats QGC reads (ZIP, TAR, 7z)
    RawFormat    ///< Raw format for single-file decompression (.gz, .xz, etc.)
};

// ============================================================================
// Reader Set-up
// ============================================================================

/// Register the filters and formats QGC reads on a new libarchive reader
/// Only decoders built into libarchive are registered. Its reader for a filter whose
/// library is not compiled in runs an external program (e.g. "bzip2 -d") on the input.
/// @param a Reader created by archive_read_new(), not yet opened
/// @param mode Format support mode (AllFormats or RawFormat)
void configureReader(struct archive* a, ReaderMode mode);

/// Size of the stream signature undecodableStreamReason() looks at
constexpr qsizetype kStreamSignatureSize = 4;

/// Tell why this build cannot decode a single compressed stream
/// With RawFormat libarchive hands data through unchanged when no registered filter
/// recognizes it, so bzip2 and LZ4 streams have to be refused when their decoder is missing.
/// @param signature At least the first kStreamSignatureSize bytes of the stream
/// @return The reason for a bzip2 or LZ4 stream that cannot be decoded, or an empty string
QString undecodableStreamReason(const QByteArray& signature);

/// Tell a compressed stream that decompresses to nothing from an input that cannot be read
/// libarchive's raw format reads a stream as one entry, and has none for an input that is empty once it is
/// decompressed. A RawFormat reader takes such an input with libarchive's format for an empty file, whose
/// first header is the end of the archive. That is a valid gzip, xz, ... stream of an empty file, or an
/// input that has no bytes at all, which is not a compressed stream. Nothing can be read from the reader
/// after it.
/// @param a RawFormat reader on which archive_read_next_header() was just called for the first time
/// @param headerResult What that call returned
/// @return true if the input is a compressed stream that holds no data
bool isEmptyStream(struct archive* a, int headerResult);

/// Why the first header of a RawFormat reader could not be read
/// @param a Reader on which archive_read_next_header() was just called for the first time
/// @param headerResult What that call returned: not ARCHIVE_OK, and not what isEmptyStream() accepts
QString streamHeaderError(struct archive* a, int headerResult);

// ============================================================================
// QIODevice Source
// ============================================================================

/// What a reader on a QIODevice gives the device callbacks as their client data
struct DeviceSource
{
    QIODevice* device = nullptr;

    /// Block last handed to libarchive by the read callback. libarchive reads from it until it
    /// asks for the next one, and another reader of the thread can be read from in between, so
    /// each reader has its own.
    QByteArray block;
};

// ============================================================================
// RAII Archive Reader
// ============================================================================

/// \brief RAII wrapper for libarchive reader with automatic cleanup
///
/// Every libarchive reader is set up by one of the open functions: created, given the filters and
/// formats of its mode by configureReader() and, in RawFormat mode, refused for the reason
/// undecodableStreamReason() gives if this build cannot decode the stream.
class ArchiveReader {
public:
    ArchiveReader() = default;
    ~ArchiveReader() = default;

    ArchiveReader(const ArchiveReader&) = delete;
    ArchiveReader& operator=(const ArchiveReader&) = delete;

    /// Open archive for reading (supports Qt resources and regular files)
    /// @param path File path or Qt resource path (:/...)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @return true on success
    bool open(const QString &path, ReaderMode mode = ReaderMode::AllFormats);

    /// Open archive for reading from a QIODevice (streaming)
    /// @param device QIODevice to read from (must be open and readable)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @return true on success
    /// @note The device must remain valid and open until the reader is closed
    bool open(QIODevice *device, ReaderMode mode = ReaderMode::AllFormats);

    /// Open archive for reading from a QIODevice, for a caller that reports the failure itself
    /// @param device QIODevice to read from (must be open and readable)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @param errorString Receives the reason if the device cannot be read, this build cannot decode the stream
    ///        or libarchive cannot open it, and that reason is then not logged
    /// @return true on success
    /// @note The device must remain valid and open until the reader is closed
    bool open(QIODevice* device, ReaderMode mode, QString& errorString);

    /// Open archive for reading from memory
    /// @param data Archive or compressed stream (shared with the reader, not copied)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @return true on success
    bool open(const QByteArray& data, ReaderMode mode);

    /// Close the archive. The reader can be opened again.
    void close();

    /// Get the underlying archive handle
    struct archive* handle() const { return _archive.get(); }

    /// Check if archive is open
    bool isOpen() const { return _archive != nullptr; }

    /// Whether the last open refused the input because this build cannot decode its compression
    bool unsupportedStream() const { return _unsupportedStream; }

    /// Mode of the last open
    ReaderMode mode() const { return _mode; }

    /// Get the total data size (for progress reporting)
    /// @return Size in bytes: size of the data in memory, file size, or device size (0 if unknown)
    qint64 dataSize() const;

    /// Record the format and the filter of the input, which libarchive knows once it has read the first header
    /// forEachEntry() does it. A caller that reads the first header of the open reader itself does it then.
    void captureFormat();

    /// Get the detected archive format name
    /// Everything that tells the format or the filter of an input asks here.
    /// @return Format name like "ZIP 2.0 (deflation)", "POSIX ustar", "raw", or empty if no header has been read
    ///         since the last open. It stays when the reader is closed
    QString formatName() const { return _formatName; }

    /// Get the detected compression filter name
    /// @return Filter name like "gzip", "xz", "zstd", "none", or empty as formatName() is
    QString filterName() const { return _filterName; }

private:
    struct Deleter
    {
        void operator()(struct archive* reader) const { archive_read_free(reader); }
    };

    /// Close the archive and create the reader for the next one
    /// @param signature Start of the input (see undecodableStreamReason()), looked at in RawFormat mode
    /// @param reason Receives why the reader was not created, which is not logged
    /// @return false if the reader was not created
    bool _create(ReaderMode mode, const QByteArray& signature, QString& reason);

    // Declared before the reader, which reads from them and so is the first to go
    QByteArray _memoryData;  // Data read from memory: a Qt resource, or the bytes given to open()
    DeviceSource _source;    // Device for streaming reads
    std::unique_ptr<struct archive, Deleter> _archive;
    QString _filePath;  // Stored path for size queries
    QString _formatName;
    QString _filterName;
    ReaderMode _mode = ReaderMode::AllFormats;
    bool _unsupportedStream = false;
};

// ============================================================================
// QIODevice Streaming Callbacks
// ============================================================================

/// Read callback for QIODevice streaming
/// @param clientData DeviceSource pointer
/// @param buffer Output pointer to buffer
/// @return Bytes read, 0 at the end of the device (which a sequential device can answer a read with -1 at), or
///         ARCHIVE_FATAL on error
la_ssize_t deviceReadCallback(struct archive *a, void *clientData, const void **buffer);

/// Skip callback for QIODevice streaming
/// @param clientData DeviceSource pointer
/// @param request Bytes to skip
/// @return Bytes actually skipped
la_int64_t deviceSkipCallback(struct archive *a, void *clientData, la_int64_t request);

/// Close callback for QIODevice streaming (does not close device)
/// @param clientData DeviceSource pointer
/// @return ARCHIVE_OK
int deviceCloseCallback(struct archive *a, void *clientData);

/// Seek callback for random-access QIODevice (improves ZIP performance)
/// @param clientData DeviceSource pointer
/// @param offset Seek offset
/// @param whence SEEK_SET, SEEK_CUR, or SEEK_END
/// @return New position, or ARCHIVE_FATAL on error
la_int64_t deviceSeekCallback(struct archive *a, void *clientData, la_int64_t offset, int whence);

// ============================================================================
// Entry Iteration
// ============================================================================

/// How a walk over the entries of an archive ended
enum class EntryWalk
{
    Completed,  ///< Every entry was visited and the archive ended cleanly
    Stopped,    ///< The visitor returned false
    Failed      ///< The archive is truncated or corrupt (a warning has been logged)
};

/// Called by forEachEntry() with each entry of the archive. Returns false to stop the walk.
using EntryVisitor = std::function<bool(struct archive_entry* entry)>;

/// Visit the entries of an archive in order
/// Every loop over archive entries is this one. Only Completed says that the archive was read to
/// its end, so a caller cannot take a read error for the end of the archive. What is unread of the
/// data of an entry is skipped before the next one. An entry libarchive reads with a warning (for
/// example a pathname it could not convert) is usable: the warning is logged and the entry visited.
/// The first entry also sets what the reader tells of the format.
/// @param reader Open archive reader
/// @param visitor Called with each entry header, when the data of that entry can be read from @a reader
EntryWalk forEachEntry(ArchiveReader& reader, const EntryVisitor& visitor);

// ============================================================================
// Utility Functions
// ============================================================================

/// What an entry is, if it is one that is not extracted when it is asked for by name
/// A hard or symbolic link has no data of its own, and a directory, a FIFO, a device node or a socket has
/// none at all: read as a file, each would look like one that is empty. The functions that walk the archive
/// skip the links and the special files, and make the directories.
/// @return "directory", "hard link", "symbolic link", "FIFO", "character device", "block device" or "socket";
///         nullptr for a regular file
const char* skippedEntryKind(struct archive_entry* entry);

/// Convert libarchive entry to ArchiveEntry struct
/// @param entry libarchive entry pointer (from archive_read_next_header)
/// @return Populated ArchiveEntry with name, size, modified time, isDirectory, permissions
ArchiveEntry toArchiveEntry(struct archive_entry *entry);

// ============================================================================
// Low-level Operations
// ============================================================================

/// Open an archive for reading, handling both Qt resources and regular files
/// Qt resources are loaded into resourceData; regular files are streamed
/// @param a Initialized archive reader (call archive_read_support_* first)
/// @param filePath Path to file (Qt resource :/ or filesystem path)
/// @param resourceData Output buffer for Qt resource data (must stay alive until archive is closed)
/// @return true on success
bool openArchiveForReading(struct archive *a, const QString &filePath, QByteArray &resourceData);

// Each operation below is what the function of QGCCompression that calls it does once its arguments are
// checked: that function documents it. The operation opens the reader it is given, which tells the caller
// the format of the input afterwards and more of a failure (mode(), unsupportedStream()), and returns how it
// ended:
//   None               Every entry asked for was read to its end, and written if it was to be
//   FileNotInArchive   The archive was read to its end without an entry that was asked for
//   UnsupportedFormat  An entry asked for by name is one that is not extracted: a directory, a link, a special
//                      file, or a name that would be written outside the output directory. Of a RawFormat
//                      reader: the input is not compressed, or this build cannot decode its compression
//   IoError            The output directory or a file in it could not be written
//   PermissionDenied   As IoError, and the reason is that the output may not be written to
//   Cancelled          The progress callback returned false
//   SizeLimitExceeded  What the input holds is larger than maxBytes (0 = unlimited)
//   InvalidArchive     The input cannot be read or is damaged
// A failure has been logged as a warning, but for Cancelled and for patterns that match nothing. Data that an
// operation returns is empty unless it returns None.
//
// An operation that takes a Source is written once for what ArchiveReader::open() takes. QGClibarchive.cc
// defines it for a path and for a QIODevice, and decompressToMemory() for a QByteArray instead of a path.

/// Extract an archive (ZIP, 7z, TAR plain or compressed) to a directory
/// @param filter Which entries to extract (not set = all of them)
template <typename Source>
Error extractAnyArchive(ArchiveReader& reader, const Source& source, const QString& outputDirectoryPath,
                        const EntryFilter& filter, const ProgressCallback& progress, qint64 maxBytes);

/// Extract an archive into a staging directory next to the output directory, which it then takes the place of
/// A failure leaves the output directory as it was. Needs about twice the space of the entries meanwhile.
Error extractArchiveAtomic(ArchiveReader& reader, const QString& archivePath, const QString& outputDirectoryPath,
                           const ProgressCallback& progress, qint64 maxBytes);

/// Extract the file called @a fileName to @a outputPath
Error extractSingleFile(ArchiveReader& reader, const QString& archivePath, const QString& fileName,
                        const QString& outputPath);

/// Read the file called @a fileName (not empty) into @a data
template <typename Source>
Error extractFileToMemory(ArchiveReader& reader, const Source& source, const QString& fileName, qint64 maxBytes,
                          QByteArray& data);

/// Extract the files called @a fileNames (at least one) in one pass, as extractAnyArchive() writes its entries
Error extractMultipleFiles(ArchiveReader& reader, const QString& archivePath, const QStringList& fileNames,
                           const QString& outputDirectoryPath);

/// Extract the files that match one of the glob patterns (at least one), which libarchive's archive_match reads,
/// as extractAnyArchive() writes its entries
/// @param extractedFiles If not null, the names of the files that were extracted are appended to it
/// @return FileNotInArchive if no file matched
Error extractByPattern(ArchiveReader& reader, const QString& archivePath, const QStringList& patterns,
                       const QString& outputDirectoryPath, QStringList* extractedFiles);

/// Read every entry of an archive to its end
Error validateArchive(ArchiveReader& reader, const QString& archivePath);

/// Look for an entry in an archive without reading its data
/// @return None if the archive has the entry, FileNotInArchive if it was read to its end without it
Error findEntryInArchive(ArchiveReader& reader, const QString& archivePath, const QString& fileName);

/// List the entries of an archive in the order they have in it
Error listArchiveEntriesDetailed(ArchiveReader& reader, const QString& archivePath, QList<ArchiveEntry>& entries);

/// Count the entries of an archive
/// @param stats Receives the statistics, all zeros unless None is returned
Error getArchiveStats(ArchiveReader& reader, const QString& archivePath, ArchiveStats& stats);

/// Decompress a single compressed stream (.gz, .xz, .zst, etc.) to a file
/// @param copyPlainInput Whether an input that is not compressed is written to the output as it is, within
///        maxBytes, instead of being refused as UnsupportedFormat
template <typename Source>
Error decompressSingleFile(ArchiveReader& reader, const Source& source, const QString& outputPath,
                           const ProgressCallback& progress, qint64 maxBytes, bool copyPlainInput = false);

/// Decompress a single compressed stream (not empty) into @a data
template <typename Source>
Error decompressToMemory(ArchiveReader& reader, const Source& source, qint64 maxBytes, QByteArray& data);

} // namespace QGClibarchive
