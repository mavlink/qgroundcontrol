#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
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
/// - maxDecompressedBytes: the maximum number of bytes the decompressed stream may have (0 = unlimited). Data read
///   into memory has no default: the caller chooses the limit for what it reads.
///   ExtractOptions::maxBytes limits an extraction. The output of an operation that writes to disk is also limited
///   by the free space on the disk (see extractArchive())
///
/// The decompression and extraction functions return an Outcome (CompressionTypes.h). Their output parameters are
/// empty after a failure. They log a failure at debug level only: the Outcome reports it, and the caller decides
/// whether it is worth a warning.
///
/// libarchive detects the format from the content, among the formats this build supports. The function you call
/// decides whether the input is read as a single stream or as an archive. No external program is ever run to
/// decompress, and an encrypted archive fails with Error::InvalidArchive. The functions block until they are done:
/// CompressionJob runs an extraction or the decompression of a file on a thread of its own pool.
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

// Rules for the functions that decompress a single stream from a file or from memory:
// - Input that is not compressed is not an error. It is copied or returned unchanged, within the size limit, and
//   Outcome::filterName is "none". This handles a file that has the name of a compressed file but was already
//   decompressed in transit, such as a download whose content encoding the HTTP client removed. Empty input is
//   also taken unchanged.
// - A compressed stream that holds no data (what a compressor writes for an empty file) gives an empty file or
//   empty data.
// - Compressed input that is truncated or corrupt fails. So does a legacy lzma, Zstandard, bzip2 or LZ4 stream in
//   a build without that decoder (Error::UnsupportedFormat), also one inside a gzip or xz stream (data.lzma.gz),
//   which a build with the decoder decodes. That includes a gzip stream whose data does not match the checksum and
//   the length it ends with, which libarchive does not compare: the module checks them itself. After a failure
//   nothing is written and no data is returned, but by readFileChunks(), whose caller must discard the chunks it
//   was given.
// - Input that starts with the signature of a stream format is compressed. A signature is the whole magic number:
//   1F 8B (gzip), FD 37 7A 58 5A 00 (xz), 28 B5 2F FD (Zstandard), 04 22 4D 18 or 02 21 4C 18 (LZ4), 50 to 5F
//   2A 4D 18 (a skippable frame of Zstandard or LZ4), "BZh" and a digit from 1 to 9 (bzip2). If no decoder takes
//   such input, because its header is truncated or damaged, it fails with Error::InvalidArchive, or with
//   Error::UnsupportedFormat as above, and is not taken unchanged. So does decompressed data that starts so, and
//   data after a gzip member that starts with 1F 8B but is no whole member. Input shorter than a signature, or with
//   a byte of it changed, is taken as not compressed: a byte or two cannot be told from data, and a changed
//   signature names no format. A legacy lzma stream has no signature.
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
Outcome readFile(const QString& filePath, QByteArray& data, qint64 maxDecompressedBytes);

/// Read the content of a file, decompressed if it is compressed, in chunks, so that the caller need not hold all of it
/// It reads what readFile() reads, by the same rules and within the same limit, and gives it to @a sink a chunk at a
/// time, in order, on the calling thread. A failure can be found only after chunks were given: a stream that is
/// truncated or corrupt, a gzip checksum that does not match, the size limit, a stream inside a stream that the
/// build cannot decode. Whatever the caller built from the chunks must then be discarded: only a success says that
/// they were all of the content, and sound.
/// @param sink Takes the next chunk, which is valid only during the call. Returning false cancels the read, which
///        then fails with Error::Cancelled
Outcome readFileChunks(const QString& filePath, qint64 maxDecompressedBytes,
                       const std::function<bool(QByteArrayView chunk)>& sink);

/// Decompress data in memory
/// Data that is not compressed is returned shared with @a data, not copied, if that array owns its bytes. Those of
/// QByteArray::fromRawData() are copied, so that the caller may free them after the call.
/// @param decompressed Receives the decompressed data. It can be the same object as @a data
Outcome decompressData(const QByteArray& data, QByteArray& decompressed, qint64 maxDecompressedBytes);

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
/// a warning, and counted in Outcome::skippedEntries:
/// - an entry whose path leads outside the output directory
/// - every link, hard or symbolic, and every special file (FIFO, device node, socket)
/// - a sparse file (TAR only), whose holes the archive does not store
/// - a non-directory entry that names the output directory itself ("."), which stays a directory
/// - an entry whose name some system would write differently or not at all, on every system: a name longer than 4096
///   bytes in UTF-8, or one with a part between slashes that ends with a dot or a space, holds a control character
///   or one of < > : " | ? *, is longer than 255 bytes in UTF-8, or is a device name of Windows (CON, PRN, AUX, NUL,
///   COM0 to COM9, LPT0 to LPT9, CONIN$, CONOUT$), also with an extension ("nul.txt")
/// - an entry whose path, or a directory above it, is that of an earlier entry written in another spelling, as a
///   file system that ignores case or Unicode normalisation takes it ("Data/a" after "data/b"). An entry that the
///   filter refuses writes nothing, and so clashes with none
/// - an entry whose path in the output directory is longer than the system allows (PATH_MAX). A build for Windows
///   defines no PATH_MAX and has no such check
///
/// A ZIP entry that the filter accepts is extracted only if it is stored or deflated, as archivers write them. One
/// compressed with any other method (xz, LZMA, bzip2, Zstandard, PPMd, Deflate64, ...) fails the extraction with
/// Error::UnsupportedFormat, and the detail names the entry and the method: libarchive decodes those without a memory
/// limit, and the dictionary of an xz or LZMA entry, up to 4 GiB, takes as much memory as the data written from it.
/// An entry that the filter refuses is not decoded. A 7z archive is compressed with LZMA or LZMA2 by nature, which
/// libarchive decodes in the same way, without a memory limit: its decoder can take as much memory as the archive
/// declares. A TAR archive compressed as a whole is a single stream.
///
/// The name of an entry is read as UTF-8 unless the archive declares another charset for it. A backslash in it is
/// a separator, as archivers on Windows write it, on every system: "Tiles\a.db" is "Tiles/a.db", and "..\a" leads
/// outside. An entry replaces the file that an earlier entry of the same name wrote.
///
/// A failed extraction leaves nothing, whatever the reason, also an exception that the filter or the progress
/// callback throws, which is passed on to the caller: an output directory that the extraction created is removed,
/// and one that was there is emptied (missing directories above it, which the extraction also creates, stay). Each
/// file is written at its own path while its entry is extracted, and a failure removes it with the rest.
///
/// The output directory is checked only at the start. Nothing guards against another process that changes it
/// during the extraction, so extract into a directory that only QGC writes to.
///
/// The size limit counts the sizes of the files written. An entry that declares more than the rest of the limit
/// fails the extraction before it is written. An entry that declares no size (as in a ZIP archive written to a
/// stream) is stopped while it is written. Reading a ZIP archive also takes memory for its central directory, which
/// libarchive holds whole: up to about 3.4 times the size of an archive that holds nothing but directory records.
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
