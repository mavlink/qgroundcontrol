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

/// Size of the stream signature canDecodeStream() looks at
constexpr qsizetype kStreamSignatureSize = 4;

/// Check that this build can decode a single compressed stream
/// With RawFormat libarchive hands data through unchanged when no registered filter
/// recognizes it, so bzip2 and LZ4 streams have to be refused when their decoder is missing.
/// @param signature At least the first kStreamSignatureSize bytes of the stream
/// @return false (after logging a warning) for a bzip2 or LZ4 stream that cannot be decoded
bool canDecodeStream(const QByteArray& signature);

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
/// formats of its mode by configureReader() and, in RawFormat mode, refused by canDecodeStream()
/// if this build cannot decode the stream.
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
    /// @param errorString Receives the reason if the device cannot be read or libarchive cannot open it,
    ///        and that reason is then not logged
    /// @return true on success
    /// @note The device must remain valid and open until the reader is closed
    bool open(QIODevice* device, ReaderMode mode, QString& errorString);

    /// Open archive for reading from memory
    /// @param data Archive or compressed stream (shared with the reader, not copied)
    /// @param mode Format support mode (AllFormats or RawFormat)
    /// @return true on success
    bool openMemory(const QByteArray& data, ReaderMode mode);

    /// Close the archive. The reader can be opened again.
    void close();

    /// Get the underlying archive handle
    struct archive* handle() const { return _archive.get(); }

    /// Release ownership of archive handle (caller must free)
    /// @note The handle keeps reading through this object, which has to outlive it
    struct archive* release() { return _archive.release(); }

    /// Check if archive is open
    bool isOpen() const { return _archive != nullptr; }

    /// Get the total data size (for progress reporting)
    /// @return Size in bytes: size of the data in memory, file size, or device size (0 if unknown)
    qint64 dataSize() const;

    /// Get the detected archive format name (after reading first header)
    /// @return Format name like "ZIP 2.0 (deflation)", "POSIX ustar", "RAW", or empty if not detected
    QString formatName() const;

    /// Get the detected compression filter name
    /// @return Filter name like "gzip", "xz", "zstd", "none", or empty if not detected
    QString filterName() const;

private:
    struct Deleter
    {
        void operator()(struct archive* reader) const { archive_read_free(reader); }
    };

    /// Close the archive and create the reader for the next one
    /// @param signature Start of the input (see canDecodeStream()), looked at in RawFormat mode
    /// @return false, after logging a warning, if the reader was not created
    bool _create(ReaderMode mode, const QByteArray& signature);

    // Declared before the reader, which reads from them and so is the first to go
    QByteArray _memoryData;  // Data read from memory: a Qt resource, or the bytes given to openMemory()
    DeviceSource _source;    // Device for streaming reads
    std::unique_ptr<struct archive, Deleter> _archive;
    QString _filePath;  // Stored path for size queries
};

// ============================================================================
// QIODevice Streaming Callbacks
// ============================================================================

/// Read callback for QIODevice streaming
/// @param clientData DeviceSource pointer
/// @param buffer Output pointer to buffer
/// @return Bytes read, or ARCHIVE_FATAL on error
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

/// Outcome of advancing to the next entry of an archive
enum class NextEntry
{
    Found,  ///< An entry header was read
    End,    ///< The archive ended cleanly
    Error   ///< The archive is truncated or corrupt (a warning has been logged)
};

/// Advance to the next entry header, first skipping whatever is unread of the current entry's data
/// forEachEntry() is the loop over this function. Testing archive_read_next_header() against
/// ARCHIVE_OK instead makes a truncated or corrupt archive look like one that has ended.
/// ARCHIVE_WARN is logged and the entry returned: libarchive documents it as success with a
/// warning (for example a pathname it could not convert) and the entry is usable.
/// ARCHIVE_RETRY, ARCHIVE_FAILED and ARCHIVE_FATAL are all reported as Error.
/// @param a Archive reader
/// @param entry Receives the entry when Found is returned
NextEntry readNextEntry(struct archive* a, struct archive_entry** entry);

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
/// its end, so a caller cannot take a read error for the end of the archive. The first entry
/// also sets what lastDetectedFormatName() and lastDetectedFilterName() return.
/// @param a Archive reader
/// @param visitor Called with each entry header, when the data of that entry can be read from @a a
EntryWalk forEachEntry(struct archive* a, const EntryVisitor& visitor);

// ============================================================================
// Utility Functions
// ============================================================================

/// What an entry is, if it is one whose data is neither returned nor written to a file
/// A hard or symbolic link has no data of its own, and a FIFO, a device node or a socket has none at all.
/// The functions that extract an entry by name fail for one, and those that walk the archive skip it.
/// @return "hard link", "symbolic link", "FIFO", "character device", "block device" or "socket"; nullptr for
///         a regular file or a directory
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

/// How an operation that reads an archive or a compressed stream, and returns its data or writes it to files,
/// ended
/// An entry without data is returned as an empty QByteArray, which is also what a failure returns.
enum class ReadResult
{
    Completed,      ///< Every entry asked for was read to its end, and written if it was to be
    EntryNotFound,  ///< The archive was read to its end without an entry that was asked for
    EntrySkipped,   ///< An entry asked for by name is one that is not extracted: a link, a special file, or a
                    ///< name that would be written outside the output directory (a warning has been logged)
    WriteFailed,    ///< The output directory or a file in it could not be written
    WriteDenied,    ///< As WriteFailed, and the reason is that the output may not be written to
    Cancelled,      ///< The progress callback returned false
    Failed          ///< The input cannot be read or is damaged, or what it holds is larger than maxBytes
};

/// Extract any supported archive format using libarchive's auto-detection
/// Supports: ZIP, 7z, TAR (plain or compressed)
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param outputDirectoryPath Path where files will be extracted
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @param result If not null, receives how the extraction ended
/// @return true on success, false on failure or size limit exceeded
bool extractAnyArchive(const QString& archivePath, const QString& outputDirectoryPath,
                       ProgressCallback progress = nullptr, qint64 maxBytes = 0, ReadResult* result = nullptr);

/// Extract archive atomically using a temporary staging directory
/// Provides all-or-nothing semantics: if extraction fails, no partial files remain.
/// Uses a staging directory and atomic rename/swap in the output parent directory.
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param outputDirectoryPath Path where files will be extracted
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @param result If not null, receives how the extraction ended
/// @return true on success, false on failure (no partial files left on failure)
/// @note Requires ~2x disk space during extraction for staging
bool extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath,
                          ProgressCallback progress = nullptr, qint64 maxBytes = 0, ReadResult* result = nullptr);

/// Extract a single file from any archive by name
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of file inside archive
/// @param outputPath Output file path
/// @param result If not null, receives how the extraction ended
/// @return true on success, false if not found, a link or a special file, or extraction failed
bool extractSingleFile(const QString& archivePath, const QString& fileName, const QString& outputPath,
                       ReadResult* result = nullptr);

/// Extract a single file from any archive directly to memory
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of file inside archive
/// @param maxBytes Maximum size of the file (0 = unlimited)
/// @param result If not null, receives how the extraction ended
/// @return File contents, or empty QByteArray if not found, larger than maxBytes or extraction failed
QByteArray extractFileToMemory(const QString& archivePath, const QString& fileName, qint64 maxBytes = 0,
                               ReadResult* result = nullptr);

/// Extract multiple files from any archive by name in one pass
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileNames Names of files inside archive
/// @param outputDirectoryPath Output directory
/// @param result If not null, receives how the extraction ended
/// @return true if all files found and extracted. A link or a special file is not extracted.
bool extractMultipleFiles(const QString& archivePath, const QStringList& fileNames, const QString& outputDirectoryPath,
                          ReadResult* result = nullptr);

/// Extract files matching glob patterns from archive (uses libarchive archive_match)
/// Supports wildcards: * (any chars), ? (single char), [...] (char class)
/// A link or a special file that matches is skipped with a warning.
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param patterns Glob patterns to match (e.g., "*.json", "dir/*", "file.txt")
/// @param outputDirectoryPath Output directory
/// @param extractedFiles Optional output list of files that were extracted
/// @param result If not null, receives how the extraction ended: EntryNotFound if no file matched
/// @return true if at least one file matched and extracted, false on error or no matches
bool extractByPattern(const QString& archivePath, const QStringList& patterns, const QString& outputDirectoryPath,
                      QStringList* extractedFiles = nullptr, ReadResult* result = nullptr);

/// Validate archive integrity by reading all entries
/// @param archivePath Path to the archive file
/// @return true if archive is valid
bool validateArchive(const QString &archivePath);

/// Check if a file exists in an archive without extracting
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of file to check for
/// @return true if file exists in archive
bool fileExistsInArchive(const QString &archivePath, const QString &fileName);

/// Look for an entry in an archive without extracting it
/// Unlike fileExistsInArchive(), this tells an archive that does not have the entry from one that could
/// not be searched.
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param fileName Name of the entry to look for
/// @return Found, End if the archive was read to its end without the entry, or Error (after a warning) if
///         the archive cannot be opened or is damaged before the entry
NextEntry findEntryInArchive(const QString& archivePath, const QString& fileName);

/// List all entries in an archive
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param ok If not null, receives whether the archive was read to its end: one without entries lists as
///        empty too
/// @return List of entry names, empty on failure
QStringList listArchiveEntries(const QString& archivePath, bool* ok = nullptr);

/// List all entries in an archive with detailed metadata
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param ok If not null, receives whether the archive was read to its end: one without entries lists as
///        empty too
/// @return List of ArchiveEntry structs, empty on failure
QList<ArchiveEntry> listArchiveEntriesDetailed(const QString& archivePath, bool* ok = nullptr);

/// Get summary statistics for an archive
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param ok If not null, receives whether the archive was read to its end: one without entries has all
///        zeros too
/// @return ArchiveStats struct (all zeros on failure)
ArchiveStats getArchiveStats(const QString& archivePath, bool* ok = nullptr);

/// Extract archive with per-entry filtering
/// A link or a special file the filter accepts is skipped with a warning.
/// @param archivePath Path to the archive file (or Qt resource path)
/// @param outputDirectoryPath Path where files will be extracted
/// @param filter Callback to decide which entries to extract
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @param result If not null, receives how the extraction ended
/// @return true on success (even if all entries filtered out), false on failure
bool extractWithFilter(const QString& archivePath, const QString& outputDirectoryPath, EntryFilter filter,
                       ProgressCallback progress = nullptr, qint64 maxBytes = 0, ReadResult* result = nullptr);

// ============================================================================
// Single-File Decompression
// ============================================================================

/// Decompress a single compressed file (.gz, .xz, .zst, etc.) to disk
/// @param inputPath Path to compressed file (or Qt resource path)
/// @param outputPath Path for decompressed output
/// @param progress Optional progress callback
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @param result If not null, receives how the decompression ended
/// @return true on success, false on failure or size limit exceeded
bool decompressSingleFile(const QString& inputPath, const QString& outputPath, ProgressCallback progress = nullptr,
                          qint64 maxBytes = 0, ReadResult* result = nullptr);

/// Decompress data in memory
/// @param data Compressed input data
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @param ok If not null, receives whether the data was decompressed: a stream of an empty file gives
///        nothing too
/// @return Decompressed data, or empty on failure or size limit exceeded
QByteArray decompressDataFromMemory(const QByteArray& data, qint64 maxBytes = 0, bool* ok = nullptr);

// ============================================================================
// QIODevice-based Operations (streaming)
// ============================================================================

/// Extract archive from QIODevice to directory
/// @param device QIODevice to read from (must be open and readable)
/// @param outputDirectoryPath Path where files will be extracted
/// @param progress Optional progress callback (return false to cancel)
/// @param maxBytes Maximum total decompressed bytes (0 = unlimited)
/// @param result If not null, receives how the extraction ended
/// @return true on success, false on failure or size limit exceeded
bool extractFromDevice(QIODevice* device, const QString& outputDirectoryPath, ProgressCallback progress = nullptr,
                       qint64 maxBytes = 0, ReadResult* result = nullptr);

/// Extract a single file from archive device directly to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param fileName Name of file inside archive
/// @param maxBytes Maximum size of the file (0 = unlimited)
/// @param result If not null, receives how the extraction ended
/// @return File contents, or empty QByteArray if not found or larger than maxBytes
QByteArray extractFileDataFromDevice(QIODevice* device, const QString& fileName, qint64 maxBytes = 0,
                                     ReadResult* result = nullptr);

/// Decompress single-file format from QIODevice to disk
/// @param device QIODevice to read from (must be open and readable)
/// @param outputPath Output file path
/// @param progress Optional progress callback
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @param result If not null, receives how the decompression ended
/// @return true on success, false on failure or size limit exceeded
bool decompressFromDevice(QIODevice* device, const QString& outputPath, ProgressCallback progress = nullptr,
                          qint64 maxBytes = 0, ReadResult* result = nullptr);

/// Decompress single-file format from QIODevice to memory
/// @param device QIODevice to read from (must be open and readable)
/// @param maxBytes Maximum decompressed bytes (0 = unlimited)
/// @param ok If not null, receives whether the stream was decompressed: one of an empty file gives nothing too
/// @return Decompressed data, or empty QByteArray on failure or size limit exceeded
QByteArray decompressDataFromDevice(QIODevice* device, qint64 maxBytes = 0, bool* ok = nullptr);

// ============================================================================
// Format Detection (Thread-Local State)
// ============================================================================

/// Get the detected format name from the last archive operation (thread-local)
/// @return Format name like "ZIP 2.0 (deflation)", "POSIX ustar", "RAW", or empty
QString lastDetectedFormatName();

/// Get the detected filter name from the last archive operation (thread-local)
/// @return Filter name like "gzip", "xz", "zstd", "none", or empty
QString lastDetectedFilterName();

/// Forget the format and filter detected on this thread (thread-local)
/// They are recorded when the first header of an archive is read. An operation that fails before that
/// records nothing, and would be described by what the operation before it detected.
void clearLastDetectedFormat();

/// Tell whether an operation on this thread stopped at its maxBytes limit since the last call
/// (thread-local). Resets the flag, so call it once for each failed operation that takes a limit.
bool takeSizeLimitExceeded();

} // namespace QGClibarchive
