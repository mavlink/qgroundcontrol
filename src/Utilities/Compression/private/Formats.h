#pragma once

/// @file Formats.h
/// @brief What the formats look like, and which of them this build reads
/// @note Private to QGCCompression

#include <QtCore/QByteArray>
#include <QtCore/QFlags>
#include <QtCore/QString>
#include <QtCore/QStringList>

struct archive;

namespace Compression {

/// Kind of a file, which decides how the functions read it
enum class Kind
{
    Unknown,  ///< Neither its name nor its content is recognized
    Stream,   ///< A single compressed stream
    Archive   ///< An archive of several files, possibly compressed as a whole
};

/// The formats that a build may leave out. gzip, xz and ZIP are always read
enum class OptionalFormat : unsigned
{
    Tar = 1U << 0,
    SevenZip = 1U << 1,
    Zstd = 1U << 2,
    Bzip2 = 1U << 3,
    Lz4 = 1U << 4,
    Lzma = 1U << 5  ///< Legacy .lzma streams
};
Q_DECLARE_FLAGS(OptionalFormats, OptionalFormat)
Q_DECLARE_OPERATORS_FOR_FLAGS(OptionalFormats)

/// The optional formats this build reads: those that its options enable, and of those a filter only if the
/// libarchive it runs with has its decoder, which it would otherwise run as an external program. Everything that
/// tells which formats are read asks this
OptionalFormats readFormats();

/// Kind of a file from its name, or from its content if the name is not recognized and the file can be read
Kind kindOfFile(const QString& filePath);

/// Length of the suffix of a single compressed stream (".gz", ".xz", ...) that ends a file name, also the name of a
/// compressed archive (".gz" of "a.tar.gz"), or 0 if none does
qsizetype compressionSuffixSize(const QString& filePath);

/// Whether the name or the content of a file shows a format that this build does not read: an archive format, or
/// the compression of an archive that is compressed as a whole
bool isFormatNotBuilt(const QString& filePath);

/// Name patterns of the archives this build extracts
QStringList builtArchiveNamePatterns();

}  // namespace Compression

namespace LibArchive {

/// How a reader reads its input
enum class Input
{
    Stream,  ///< As a single stream (.gz, .xz, ...), with libarchive's raw format
    Archive  ///< As an archive of one of the formats this build extracts
};

/// Bytes of a stream that undecodableStreamReason() looks at: as far as libarchive's Zstandard and LZ4 readers
/// look for the frame that follows skippable frames
constexpr qsizetype kStreamSignatureSize = qsizetype{64} * 1024;

/// Register on a new libarchive reader that is not yet opened the filters and, for an archive, the archive formats
/// that this build reads
void configureReader(struct archive* a, Input input);

/// Why this build cannot decode a single compressed stream
/// The raw format passes data through unchanged when no registered filter recognizes it, so legacy lzma, bzip2,
/// LZ4 and Zstandard streams must be refused explicitly when their decoder is missing. The conditions are copied
/// from libarchive's bidders: a stream is refused whenever a build with the decoder would take it, and a legacy
/// lzma or bzip2 stream exactly then. test/Utilities/Compression/Fuzz/BidAgreement.h compares them with libarchive.
/// @param signature The first kStreamSignatureSize bytes of the stream, or all of a shorter one
/// @param decoders The optional filters whose decoders the build has
/// @return The reason for a stream of an optional format that cannot be decoded, or an empty string
QString undecodableStreamReason(const QByteArray& signature,
                                Compression::OptionalFormats decoders = Compression::readFormats());

}  // namespace LibArchive
