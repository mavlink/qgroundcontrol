#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "CompressionTypes.h"

/// Decompression of a single compressed stream (gzip and xz; legacy lzma, Zstandard, bzip2 and LZ4 if the build
/// enables them) from a file or from memory, and extraction of a ZIP archive (7z and TAR, plain or compressed, if the
/// build enables them) into a directory. All operations use libarchive. The module only decompresses: QGC does not
/// create compressed files.
///
/// Parameters that several functions share:
/// - archivePath, inputPath, filePath: the file to read. It can be a Qt resource (:/...), or a file that cannot
///   seek, such as a pipe, which is read in order. Only a 7z archive needs a file that can seek. An operation
///   waits for the writer of a pipe, and cannot be cancelled while it waits
/// - maxDecompressedBytes: the maximum number of bytes the decompressed stream may have (0 = unlimited).
///   ExtractOptions::maxBytes limits an extraction. The output of an operation that writes to disk is also limited
///   by the free space on the disk (see extractArchive())
///
/// The decompression and extraction functions return an Outcome (CompressionTypes.h). Their output parameters are
/// empty after a failure. They log a failure at debug level only: the Outcome reports it, and the caller decides
/// whether it is worth a warning.
///
/// libarchive detects the format from the content, among the formats this build supports. The function you call
/// decides whether the input is read as a single stream or as an archive.
namespace Compression {

// ============================================================================
// Error Handling
// ============================================================================

/// Translated, human-readable name of an error code
[[nodiscard]] QString errorName(Error error);

// ============================================================================
// Kind of a File
// ============================================================================

/// Whether a file is an archive (.zip, .7z, .tar), including a TAR compressed as a whole (.tar.gz, .tgz, ...)
/// The file name is checked first, in any case. If it is not recognized, the signature the content starts with is
/// checked. A recognized name wins over the content: a name of a single compressed stream (.gz, .xz, ...) is no
/// archive. A file that is not recognized, or does not exist, is no archive. Only the content of a regular file is
/// checked, so a pipe is told by its name alone. Every build knows the names and signatures of all formats, also of
/// those it does not read.
[[nodiscard]] bool isArchiveFile(const QString& filePath);

/// Name patterns of the archives that this build extracts, for a file dialog: "*.zip", and "*.7z", "*.tar",
/// "*.tar.gz", ... with the build options of those formats
[[nodiscard]] QStringList archiveNamePatterns();

/// Whether a file is a single compressed stream (.gz, .gzip, .xz, .lzma, .zst, .zstd, .bz2, .bzip2, .lz4)
/// The name and the content are checked as isArchiveFile() checks them, so a compressed archive (.tar.gz) is no
/// compressed file. A legacy lzma stream has no signature: only its name tells it.
[[nodiscard]] bool isCompressedFile(const QString& filePath);

/// Path a compressed file decompresses to: the path without its compression extension, or with ".decompressed"
/// appended if only the content shows that it is compressed or the name is only the extension (".gz"), so that
/// the result differs from the input and is not its directory.
/// Returns @a filePath unchanged if isCompressedFile() is false
[[nodiscard]] QString strippedPath(const QString& filePath);

// ============================================================================
// Single-File Decompression
// ============================================================================

// Rules for the three functions that decompress a single stream from a file or from memory:
// - Input that is not compressed is not an error. It is copied or returned unchanged, within the size limit, and
//   Outcome::filterName is "none". This handles a file that has the name of a compressed file but was already
//   decompressed in transit, such as a download whose content encoding the HTTP client removed. Empty input is
//   also taken unchanged.
// - A compressed stream that holds no data (what a compressor writes for an empty file) gives an empty file or
//   empty data.
// - Compressed input that is truncated or corrupt fails. So does a legacy lzma, Zstandard, bzip2 or LZ4 stream in
//   a build without that decoder (Error::UnsupportedFormat), also one inside a gzip or xz stream (data.lzma.gz),
//   which a build with the decoder decodes. After a failure nothing is written and no data is returned.
//   That includes a gzip stream whose data does not match the checksum and the length it ends with, which
//   libarchive does not compare: the module checks them itself.
//
// They ignore the file name: the content decides whether the input is compressed. A compressed archive (.tar.gz)
// gives the uncompressed archive, also in a build that does not extract TAR, and a ZIP or 7z archive is taken
// unchanged.

/// Decompress a file to another file
/// The output is written atomically: a file already at the output path is replaced only once the output is
/// complete. Missing directories above the output path are created, and stay after a failure. Output that does not
/// fit on the disk fails with Error::DiskSpaceExceeded, and nothing is written (see extractArchive()).
/// @param outputPath Path for the output. If empty: @a inputPath without its compression extension (.gz, .gzip,
///        .xz, ...; a.tar.gz gives a.tar), or with ".decompressed" appended if it has none or the name is only the
///        extension. A path that names the input file itself, also through a link or in another spelling, fails
///        with Error::IoError, and the file stays as it is
/// @param progress Optional. Returning false cancels the operation, also the copy of an input that is not
///        compressed
Outcome decompressFile(const QString& inputPath, const QString& outputPath = QString(),
                       ProgressCallback progress = nullptr,
                       qint64 maxDecompressedBytes = kDefaultMaxDecompressedFileBytes);

/// Read the content of a file, decompressed if it is compressed
/// A truncated or corrupt compressed file gives no data, not the part that could be read.
/// @param data Receives the content
Outcome readFile(const QString& filePath, QByteArray& data,
                 qint64 maxDecompressedBytes = kDefaultMaxDecompressedMemoryBytes);

/// Decompress data in memory
/// @param decompressed Receives the decompressed data. It can be the same object as @a data
Outcome decompressData(const QByteArray& data, QByteArray& decompressed,
                       qint64 maxDecompressedBytes = kDefaultMaxDecompressedMemoryBytes);

// ============================================================================
// Archive Extraction
// ============================================================================

/// Extract an archive into a new or empty directory
/// The file is read as an archive whatever its name is: ZIP, and 7z or TAR (the last also compressed as a whole) if
/// the build enables them. Any other file fails with Error::InvalidArchive, except one whose name or signature shows
/// an archive format, or the compression of a TAR, that this build does not read: that fails with
/// Error::UnsupportedFormat.
///
/// The output directory must not exist, or be an empty directory. Missing directories above it are created. Any
/// other path (a directory that has anything in it or cannot be read, a file, a link that leads nowhere) fails
/// with Error::IoError before the archive is opened, and nothing is changed. The path is made absolute first, and
/// "." and ".." in it are resolved by name.
///
/// Only files and directories are written, and only inside the output directory. These entries are skipped with
/// a warning:
/// - an entry whose path leads outside the output directory
/// - every link, hard or symbolic, and every special file (FIFO, device node, socket)
/// - a sparse file (TAR only), whose holes the archive does not store
/// - a non-directory entry that names the output directory itself ("."), which stays a directory
/// - an entry whose path in the output directory is longer than the system allows (PATH_MAX). A build for Windows
///   defines no PATH_MAX and has no such check
///
/// The name of an entry is read as UTF-8 unless the archive declares another charset for it. An entry replaces the
/// file that an earlier entry of the same name wrote.
///
/// A failed extraction leaves nothing, whatever the reason, also an exception that the filter or the progress
/// callback throws, which is passed on to the caller: an output directory that the extraction created is removed,
/// and one that was there is emptied (missing directories above it, which the extraction also creates, stay). Each
/// file is moved into place only once it is complete.
///
/// The output directory is checked only at the start. Nothing guards against another process that changes it
/// during the extraction, so extract into a directory that only QGC writes to.
///
/// The size limit counts the sizes of the files written. An entry that declares more than the rest of the limit
/// fails the extraction before it is written. An entry that declares no size (as in a ZIP archive written to a
/// stream) is stopped while it is written.
///
/// Whatever the limit, an extraction does not fill its disk: it leaves free at least a tenth of what it writes.
/// This second limit is counted the same way, and exceeding it fails with Error::DiskSpaceExceeded. The archive
/// is read only once, so an entry that does not fit is found after the entries before it were written; those
/// are then removed. If the free space cannot be determined, there is no disk limit. The limit counts the
/// sizes of the files, not the blocks the file system gives them: an archive of very many small files can
/// still fill a disk, and then fails with Error::IoError.
Outcome extractArchive(const QString& archivePath, const QString& outputDirectoryPath,
                       const ExtractOptions& options = {});

}  // namespace Compression
