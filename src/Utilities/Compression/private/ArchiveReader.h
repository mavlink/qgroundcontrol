#pragma once

/// @file ArchiveReader.h
/// @brief The libarchive reader that every Compression operation reads from
/// @note Private to QGCCompression, like every header in this directory: use Compression.h

#include <archive.h>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include "CompressionTypes.h"
#include "Formats.h"

Q_DECLARE_LOGGING_CATEGORY(LibArchiveLog)

namespace LibArchive {

// ============================================================================
// Types (aliases of those of Compression)
// ============================================================================

using Error = Compression::Error;
using ProgressCallback = Compression::ProgressCallback;

// ============================================================================
// Results
// ============================================================================

/// How an operation, or a step of one, ended. The public function builds its Outcome from it
struct [[nodiscard]] Result
{
    Error error = Error::None;
    /// What libarchive or the system reported, in English. Becomes Outcome::detail
    QString detail = {};
    /// Set by ArchiveReader::open() for an input that could be read but that no registered format recognized, or
    /// whose start a filter found damaged: libarchive has no public error code that tells the two apart
    bool unrecognized = false;
    /// Compression filter that the reader of an operation detected (ArchiveReader::filterName()). Set by the
    /// operation, also when it fails
    QString filterName = {};

    explicit operator bool() const { return error == Error::None; }
};

/// A failed Result, logged at debug level only: the Outcome reports it
Result failure(Error error, const QString& detail = {});

// ============================================================================
// File Source
// ============================================================================

class GzipCheck;

/// Client data of the read callbacks of a reader on a file
struct FileSource
{
    QFile* file = nullptr;

    /// Takes the bytes that are read, to check them if they are a gzip stream
    GzipCheck* gzip = nullptr;

    /// Whether reading the file failed
    bool readFailed = false;

    /// Block last handed to libarchive by the read callback. libarchive reads from it until it asks for the next
    /// one, and another reader on the same thread can be read in between, so each reader has its own.
    QByteArray block;
};

// ============================================================================
// RAII Archive Reader
// ============================================================================

/// \brief RAII wrapper for a libarchive reader of one input
///
/// An operation creates its own reader and opens it once. The open functions create the libarchive reader and
/// register the filters and formats that this build reads (configureReader()). For a single stream they refuse a
/// compression that this build cannot decode.
class ArchiveReader
{
public:
    ArchiveReader();
    ~ArchiveReader();

    ArchiveReader(const ArchiveReader&) = delete;
    ArchiveReader& operator=(const ArchiveReader&) = delete;

    /// Open a file or a Qt resource (:/...) for reading
    /// The reader opens the file with QFile and reads it through its own callbacks. A file that cannot seek, such
    /// as a pipe, is read in order.
    /// @return Success, UnsupportedFormat for a single stream whose compression this build cannot decode, or
    ///         InvalidArchive
    Result open(const QString& path, Input input);

    /// Open data in memory for reading
    /// @param data Archive or compressed stream (shared with the reader, not copied)
    /// @return As for a file
    Result open(const QByteArray& data, Input input);

    struct archive* handle() const { return _archive.get(); }

    /// Size in bytes of the input, for progress reporting (0 while the reader is not open)
    qint64 dataSize() const;

    /// Record the compression filter of the input
    /// libarchive knows the filter only once it has read the first header, so whoever reads the first header
    /// calls this then. The extraction does so at the first entry.
    void captureFilter();

    /// Check what libarchive does not: the checksums of an input that is a gzip stream
    /// libarchive reads the checksum and the length that end a gzip member without comparing them, so data that
    /// was changed and still inflates is not found by it. Call this once the input has been read with success,
    /// and before the result is committed. Any other input is sound.
    /// @return Success, or InvalidArchive if the data does not match its checksums
    Result verifyInput();

    /// Detected compression filter: "gzip", "xz", "zstd", ... or "none"
    /// Empty if no header has been read
    QString filterName() const { return _filterName; }

private:
    struct Deleter
    {
        void operator()(struct archive* reader) const { archive_read_free(reader); }
    };

    /// Create the libarchive reader
    /// @param signature Start of the input, which shows whether this build can decode a single stream
    Result _create(Input input, const QByteArray& signature);

    // Declared before _archive, which reads from them, so that _archive is destroyed first
    QByteArray _memoryData;            // The bytes given to open()
    QFile _file;                       // The file of a path given to open()
    FileSource _source;                // What the read callbacks are given of the file
    std::unique_ptr<GzipCheck> _gzip;  // Checks the bytes read from the file
    std::unique_ptr<struct archive, Deleter> _archive;
    QString _filterName;
};

// ============================================================================
// Failures
// ============================================================================

/// libarchive's message for its last failure, or a generic one if it has none
/// libarchive sets no message for some failures, such as a read callback that fails
QString libarchiveError(struct archive* a);

// ============================================================================
// Operations
// ============================================================================

// The operations in ArchiveExtraction.h and StreamDecompression.h do the work of the public functions of
// Compression.h after those have checked their arguments. Compression.h documents their behaviour. An operation reads
// its input with a reader of its own, and returns the filter it detected in Result::filterName and an error that is
// one of:
//   None               Everything requested was read to its end, and written if it was to be written
//   UnsupportedFormat  Single stream only: this build cannot decode the compression of the input. An archive
//                      whose format this build lacks fails with InvalidArchive, and Result::unrecognized set
//   IoError            The output directory or a file in it could not be written
//   PermissionDenied   As IoError, because writing to the output is not permitted
//   Cancelled          The progress callback returned false
//   SizeLimitExceeded  The output is larger than maxBytes (0 = unlimited)
//   DiskSpaceExceeded  Writing to disk only: the disk has no room for the output
//   InvalidArchive     The input cannot be read or is damaged
// Result::detail holds what libarchive or the system reported, if anything. Output data is empty unless the
// operation succeeds.

}  // namespace LibArchive
