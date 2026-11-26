#include "Formats.h"

#include <algorithm>
#include <archive.h>
#include <initializer_list>

#include <QtCore/QByteArrayView>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QtEndian>

namespace Compression {

namespace {

// Content shorter than this is not recognized
constexpr qsizetype kMinMagicBytes = 6;

// Offset of the "ustar" magic in a TAR header
constexpr qsizetype kTarUstarOffset = 257;

// Bytes read from the start of a file to recognize its content: the size of a TAR header
constexpr qsizetype kHeaderBytes = 512;

/// The libarchive reader of an optional format that the build options enable
struct OptionalReader
{
    OptionalFormat format;
    bool isArchiveFormat;
    int (*registerOn)(struct archive*);
    /// Version of the decoder library that libarchive was built with, which gives nullptr without one. Not set for
    /// an archive format
    const char* (*decoderVersion)() = nullptr;
};

// Only what the options enable is named, so that libarchive links nothing else
constexpr std::initializer_list<OptionalReader> kOptionalReaders = {
#ifdef QGC_ENABLE_TAR
    {OptionalFormat::Tar, true, archive_read_support_format_tar},
#endif
#ifdef QGC_ENABLE_7ZIP
    {OptionalFormat::SevenZip, true, archive_read_support_format_7zip},
#endif
#ifdef QGC_ENABLE_ZSTD
    {OptionalFormat::Zstd, false, archive_read_support_filter_zstd, archive_libzstd_version},
#endif
#ifdef QGC_ENABLE_BZIP2
    {OptionalFormat::Bzip2, false, archive_read_support_filter_bzip2, archive_bzlib_version},
#endif
#ifdef QGC_ENABLE_LZ4
    {OptionalFormat::Lz4, false, archive_read_support_filter_lz4, archive_liblz4_version},
#endif
#ifdef QGC_ENABLE_LZMA
    {OptionalFormat::Lzma, false, archive_read_support_filter_lzma, archive_liblzma_version},
#endif
};

// Bytes that streams start with, which kFormats, LibArchive::undecodableStreamReason() and
// LibArchive::streamSignatureFormat() look for
constexpr QByteArrayView kGzipSignature = "\x1F\x8B";
// "\xFD7zXZ" and a zero byte. The size is explicit because a view of a literal stops at its first zero
constexpr QByteArrayView kXzSignature = {"\xFD\x37\x7A\x58\x5A\x00", 6};
constexpr QByteArrayView kBzip2Signature = "BZh";
constexpr QByteArrayView kZstdSignature = "\x28\xB5\x2F\xFD";
constexpr QByteArrayView kLz4Signature = "\x04\x22\x4D\x18";
constexpr QByteArrayView kLz4LegacySignature = "\x02\x21\x4C\x18";  // The frame of LZ4 before version 1.4

/// How to recognize a file of one format
struct FormatInfo
{
    Kind kind;
    QByteArrayView suffixes[3];    ///< In lower case
    QByteArrayView signatures[3];  ///< Bytes a file of the format starts with (TAR: see kTarUstarOffset)
    OptionalFormats needs = {};    ///< The formats a build needs to read it, of those it may leave out
};

// Searched in this order. The compressed TAR formats come before the compressions, because their suffixes end
// with a compression suffix (.tar.gz ends with .gz). The names and signatures of every format are known, also in a
// build that does not read it.
constexpr FormatInfo kFormats[] = {
    {Kind::Archive, {".tar.gz", ".tgz"}, {}, OptionalFormat::Tar},
    {Kind::Archive, {".tar.xz", ".txz"}, {}, OptionalFormat::Tar},
    {Kind::Archive, {".tar.zst", ".tar.zstd"}, {}, OptionalFormat::Tar | OptionalFormat::Zstd},
    {Kind::Archive, {".tar.bz2", ".tbz2", ".tbz"}, {}, OptionalFormat::Tar | OptionalFormat::Bzip2},
    {Kind::Archive, {".tar.lz4"}, {}, OptionalFormat::Tar | OptionalFormat::Lz4},
    // ZIP: local file header, empty archive, spanned archive
    {Kind::Archive, {".zip"}, {"PK\x03", "PK\x05", "PK\x07"}},
    {Kind::Archive, {".7z"}, {"7z\xBC\xAF\x27\x1C"}, OptionalFormat::SevenZip},
    {Kind::Stream, {".gz", ".gzip"}, {kGzipSignature}},
    {Kind::Stream, {".xz"}, {kXzSignature}},
    // A legacy lzma stream starts with no signature
    {Kind::Stream, {".lzma"}, {}, OptionalFormat::Lzma},
    {Kind::Stream, {".zst", ".zstd"}, {kZstdSignature}, OptionalFormat::Zstd},
    {Kind::Stream, {".bz2", ".bzip2"}, {kBzip2Signature}, OptionalFormat::Bzip2},
    {Kind::Stream, {".lz4"}, {kLz4Signature, kLz4LegacySignature}, OptionalFormat::Lz4},
    {Kind::Archive, {".tar"}, {}, OptionalFormat::Tar},
};

/// Whether this build reads a format
bool isBuilt(const FormatInfo& info)
{
    return (readFormats() & info.needs) == info.needs;
}

/// Suffix of a format that ends a file name, or an empty view if none does
QByteArrayView suffixOfName(const FormatInfo& info, const QString& filePath)
{
    for (const QByteArrayView suffix : info.suffixes) {
        if (!suffix.isEmpty() && filePath.endsWith(QLatin1StringView(suffix), Qt::CaseInsensitive)) {
            return suffix;
        }
    }
    return {};
}

/// Format whose suffix ends a file name, or nullptr if no suffix does
const FormatInfo* formatOfName(const QString& filePath)
{
    for (const FormatInfo& info : kFormats) {
        if (!suffixOfName(info, filePath).isEmpty()) {
            return &info;
        }
    }
    return nullptr;
}

// The functions below are given the start of a file's content: its first kHeaderBytes, or all of it if it is shorter

/// Format whose signature the content starts with, or nullptr if it starts with none
const FormatInfo* formatOfData(const QByteArray& data)
{
    if (data.size() < kMinMagicBytes) {
        return nullptr;
    }
    for (const FormatInfo& info : kFormats) {
        for (const QByteArrayView signature : info.signatures) {
            if (!signature.isEmpty() && data.startsWith(signature)) {
                return &info;
            }
        }
    }
    return nullptr;
}

/// Whether the content is a TAR archive, which has its signature in the header of its first entry
bool isTarData(const QByteArray& data)
{
    return (data.size() >= kMinMagicBytes) &&
           QByteArrayView(data).sliced(std::min(kTarUstarOffset, data.size())).startsWith("ustar");
}

/// Kind of a file from the start of its content
Kind kindOfData(const QByteArray& data)
{
    if (const FormatInfo* const format = formatOfData(data)) {
        return format->kind;
    }
    return isTarData(data) ? Kind::Archive : Kind::Unknown;
}

/// The start of the content of a file, or nothing if it is not a regular file or cannot be read
/// Only a regular file is read: bytes read from a pipe are lost to the operation that reads it next
QByteArray headerOfFile(const QString& filePath)
{
    if (!QFileInfo(filePath).isFile()) {
        return {};
    }
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? file.read(kHeaderBytes) : QByteArray();
}

}  // namespace

OptionalFormats readFormats()
{
    static const OptionalFormats formats = [] {
        OptionalFormats read;
        for (const OptionalReader& reader : kOptionalReaders) {
            if ((reader.decoderVersion == nullptr) || (reader.decoderVersion() != nullptr)) {
                read |= reader.format;
            }
        }
        return read;
    }();
    return formats;
}

Kind kindOfFile(const QString& filePath)
{
    if (const FormatInfo* const named = formatOfName(filePath)) {
        return named->kind;
    }
    return kindOfData(headerOfFile(filePath));
}

qsizetype compressionSuffixSize(const QString& filePath)
{
    for (const FormatInfo& info : kFormats) {
        if (info.kind == Kind::Stream) {
            if (const QByteArrayView suffix = suffixOfName(info, filePath); !suffix.isEmpty()) {
                return suffix.size();
            }
        }
    }
    return 0;
}

bool isFormatNotBuilt(const QString& filePath)
{
    if (const FormatInfo* const named = formatOfName(filePath); (named != nullptr) && !isBuilt(*named)) {
        return true;
    }
    const QByteArray header = headerOfFile(filePath);
    if (const FormatInfo* const format = formatOfData(header)) {
        return !isBuilt(*format);
    }
    return !readFormats().testFlag(OptionalFormat::Tar) && isTarData(header);
}

QStringList builtArchiveNamePatterns()
{
    QStringList patterns;
    for (const FormatInfo& info : kFormats) {
        if ((info.kind != Kind::Archive) || !isBuilt(info)) {
            continue;
        }
        for (const QByteArrayView suffix : info.suffixes) {
            if (!suffix.isEmpty()) {
                patterns.append(QLatin1Char('*') + QLatin1StringView(suffix));
            }
        }
    }
    return patterns;
}

}  // namespace Compression

namespace LibArchive {

namespace {

/// The number that a frame starting with @a signature starts with: Zstandard and LZ4 store it low byte first
constexpr quint32 frameMagic(QByteArrayView signature)
{
    quint32 magic = 0;
    for (qsizetype i = 3; i >= 0; --i) {
        magic = (magic << 8) | static_cast<quint8>(signature[i]);
    }
    return magic;
}

constexpr quint32 kLz4Magic = frameMagic(Compression::kLz4Signature);
constexpr quint32 kLz4LegacyMagic = frameMagic(Compression::kLz4LegacySignature);
constexpr quint32 kZstdMagic = frameMagic(Compression::kZstdSignature);
constexpr quint32 kSkippableMagic = 0x184D2A50;  // To 0x184D2A5F
constexpr quint32 kSkippableMask = 0xFFFFFFF0;

/// Magic number of the first frame of a stream that is not a skippable frame
/// A Zstandard or an LZ4 stream can start with skippable frames, which hold data for other programs. Both
/// formats give them the same magic numbers, so the frame after them tells the format.
/// @param onlySkippable Set if @a stream holds skippable frames and no frame after them
/// @return The magic number, or 0 if there is no such frame in @a stream
quint32 firstFrameMagic(QByteArrayView stream, bool& onlySkippable)
{
    qint64 offset = 0;
    while ((stream.size() - offset) >= 4) {
        const quint32 magic = qFromLittleEndian<quint32>(stream.data() + offset);
        if ((magic & kSkippableMask) != kSkippableMagic) {
            onlySkippable = false;
            return magic;
        }
        if ((stream.size() - offset) < 8) {
            break;
        }
        // A skippable frame is its magic number, the size of its data, and the data
        offset += 8 + static_cast<qint64>(qFromLittleEndian<quint32>(stream.data() + offset + 4));
    }
    onlySkippable = (offset > 0) || (stream.size() >= 4);
    return 0;
}

/// Whether a stream starts like a legacy lzma stream, which has no magic number
/// The conditions are those of lzma_bidder_bid() in libarchive 3.8.11, so that a build without the lzma filter
/// refuses exactly what a build with it decodes: a properties byte of at most 224, and a dictionary size that is a
/// power of two from 4 KiB to 128 MiB, or a whole number of MiB from 3 to 63 MiB after the byte 0x5D or 0x5E and
/// with an unknown uncompressed size
bool isLegacyLzmaStream(QByteArrayView stream)
{
    // libarchive bids only on that many bytes: the 13 of the header and one more
    constexpr qsizetype kBidSize = 14;
    if (stream.size() < kBidSize) {
        return false;
    }
    const quint8 properties = static_cast<quint8>(stream.at(0));
    if (properties > (((4 * 5) + 4) * 9) + 8) {
        return false;
    }
    const quint32 dictionarySize = qFromLittleEndian<quint32>(stream.data() + 1);
    if ((dictionarySize >= 0x1000) && (dictionarySize <= 0x8000000) && ((dictionarySize & (dictionarySize - 1)) == 0)) {
        return true;
    }
    const bool usualProperties = (properties == 0x5D) || (properties == 0x5E);
    const bool unknownSize = (qFromLittleEndian<quint64>(stream.data() + 5) == ~quint64{0});
    return usualProperties && unknownSize && (dictionarySize >= 0x300000) && (dictionarySize <= 0x3F00000) &&
           ((dictionarySize & 0xFFFFF) == 0);
}

/// Whether a stream starts like a bzip2 stream, by the conditions of bzip2_reader_bid() in libarchive 3.8.11: the
/// signature, a block size from 1 to 9, and the marker of a block or of the end of the stream. Text that happens
/// to start with the signature is so not taken for a stream
bool isBzip2Stream(QByteArrayView stream)
{
    // libarchive bids on that many bytes, the smallest bzip2 stream
    constexpr qsizetype kBzip2BidSize = 14;
    if ((stream.size() < kBzip2BidSize) || !stream.startsWith(Compression::kBzip2Signature)) {
        return false;
    }
    const char blockSize = stream.at(3);
    const QByteArrayView marker = stream.sliced(4, 6);
    return (blockSize >= '1') && (blockSize <= '9') &&
           ((marker == "\x31\x41\x59\x26\x53\x59") || (marker == "\x17\x72\x45\x38\x50\x90"));
}

}  // namespace

void configureReader(struct archive* a, Input input)
{
    // archive_read_support_filter_all() and _format_all() are deliberately not used: the input is
    // untrusted, and a filter without its library compiled in is decoded by an external program. The build
    // checks that libarchive has zlib and liblzma; a libarchive without them gets neither filter.
    if (archive_zlib_version() != nullptr) {
        archive_read_support_filter_gzip(a);
    }
    if (archive_liblzma_version() != nullptr) {
        archive_read_support_filter_xz(a);
    }
    const Compression::OptionalFormats read = Compression::readFormats();
    for (const Compression::OptionalReader& reader : Compression::kOptionalReaders) {
        if (read.testFlag(reader.format) && (!reader.isArchiveFormat || (input == Input::Archive))) {
            (void) reader.registerOn(a);
        }
    }

    if (input == Input::Archive) {
        archive_read_support_format_zip(a);
#ifdef Q_OS_WIN
        // A name whose charset the archive does not declare (a TAR that is not pax, a ZIP entry without the UTF-8
        // flag) is UTF-8 in practice. libarchive reads it in the charset of the C locale, which Qt makes UTF-8 on
        // every platform but Windows.
        (void) archive_read_set_options(a, "hdrcharset=UTF-8");
#endif
    } else {
        archive_read_support_format_raw(a);
        // The raw format refuses input that is empty once it is decompressed: the empty format reads that
        archive_read_support_format_empty(a);
    }
}

QString streamSignatureFormat(QByteArrayView start)
{
    struct Signature
    {
        QByteArrayView bytes;
        const char* format;
    };

    constexpr Signature kSignatures[] = {
        {Compression::kGzipSignature, "gzip"},      {Compression::kXzSignature, "xz"},
        {Compression::kZstdSignature, "Zstandard"}, {Compression::kLz4Signature, "LZ4"},
        {Compression::kLz4LegacySignature, "LZ4"},
    };
    for (const Signature& signature : kSignatures) {
        if (start.startsWith(signature.bytes)) {
            return QString::fromLatin1(signature.format);
        }
    }
    if (start.size() < 4) {
        return {};
    }
    if ((qFromLittleEndian<quint32>(start.data()) & kSkippableMask) == kSkippableMagic) {
        return QStringLiteral("Zstandard or LZ4");
    }
    // "BZh" alone also starts text, and the block size that follows it is a part of the signature
    if (start.startsWith(Compression::kBzip2Signature) && (start.at(3) >= '1') && (start.at(3) <= '9')) {
        return QStringLiteral("bzip2");
    }
    return {};
}

QString undecodableStreamReason(const QByteArray& signature, Compression::OptionalFormats decoders)
{
    if (!decoders.testFlag(Compression::OptionalFormat::Lzma) && isLegacyLzmaStream(signature)) {
        return QStringLiteral("LZMA decompression is not supported by this build");
    }
    if (!decoders.testFlag(Compression::OptionalFormat::Bzip2) && isBzip2Stream(signature)) {
        return QStringLiteral("bzip2 decompression is not supported by this build");
    }

    bool onlySkippable = false;
    const quint32 magic = firstFrameMagic(signature, onlySkippable);
    if (!decoders.testFlag(Compression::OptionalFormat::Lz4) && ((magic == kLz4Magic) || (magic == kLz4LegacyMagic))) {
        return QStringLiteral("LZ4 decompression is not supported by this build");
    }
    if (!decoders.testFlag(Compression::OptionalFormat::Zstd) && (magic == kZstdMagic)) {
        return QStringLiteral("Zstandard decompression is not supported by this build");
    }
    // Skippable frames with nothing in view after them belong to either format
    if (onlySkippable && !(decoders.testFlag(Compression::OptionalFormat::Lz4) &&
                           decoders.testFlag(Compression::OptionalFormat::Zstd))) {
        return QStringLiteral("Zstandard or LZ4 decompression is not supported by this build");
    }
    return {};
}

}  // namespace LibArchive
