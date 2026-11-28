#include "BidAgreement.h"

#include <archive.h>

#include <QtCore/QtEndian>

namespace BidAgreement {

namespace {

const char* bidderName(Compression::OptionalFormat filter)
{
    switch (filter) {
        case Compression::OptionalFormat::Lzma:
            return "lzma";
        case Compression::OptionalFormat::Bzip2:
            return "bzip2";
        case Compression::OptionalFormat::Lz4:
            return "lz4";
        case Compression::OptionalFormat::Zstd:
            return "zstd";
        case Compression::OptionalFormat::Tar:
        case Compression::OptionalFormat::SevenZip:
            break;
    }
    return "";
}

/// Registers the filter on a reader. Only called for a filter whose decoder the build has: without it, libarchive
/// would run an external program
int registerFilter(struct archive* a, Compression::OptionalFormat filter)
{
    switch (filter) {
        case Compression::OptionalFormat::Lzma:
            return archive_read_support_filter_lzma(a);
        case Compression::OptionalFormat::Bzip2:
            return archive_read_support_filter_bzip2(a);
        case Compression::OptionalFormat::Lz4:
            return archive_read_support_filter_lz4(a);
        case Compression::OptionalFormat::Zstd:
            return archive_read_support_filter_zstd(a);
        case Compression::OptionalFormat::Tar:
        case Compression::OptionalFormat::SevenZip:
            break;
    }
    return ARCHIVE_FATAL;
}

QByteArray le32(quint32 value)
{
    QByteArray bytes(4, '\0');
    qToLittleEndian(value, bytes.data());
    return bytes;
}

QByteArray le64(quint64 value)
{
    QByteArray bytes(8, '\0');
    qToLittleEndian(value, bytes.data());
    return bytes;
}

}  // namespace

QList<Compression::OptionalFormat> comparableFilters()
{
    QList<Compression::OptionalFormat> filters;
    for (const Compression::OptionalFormat filter :
         {Compression::OptionalFormat::Lzma, Compression::OptionalFormat::Bzip2, Compression::OptionalFormat::Lz4,
          Compression::OptionalFormat::Zstd}) {
        if (Compression::readFormats().testFlag(filter)) {
            filters.append(filter);
        }
    }
    return filters;
}

bool libarchiveBids(Compression::OptionalFormat filter, const QByteArray& input)
{
    // With nothing registered but the filter, the raw format and the empty one, the reader opens any input as it is.
    // It has a filter, or fails while the filter decodes the start of the data, only if the filter took the input
    struct archive* const a = archive_read_new();
    (void) registerFilter(a, filter);
    (void) archive_read_support_format_raw(a);
    (void) archive_read_support_format_empty(a);
    const bool opened = archive_read_open_memory(a, input.constData(), static_cast<size_t>(input.size())) == ARCHIVE_OK;
    const bool bid = !opened || (archive_filter_count(a) > 1);
    (void) archive_read_free(a);
    return bid;
}

bool rulesRefuse(Compression::OptionalFormat filter, const QByteArray& input)
{
    const Compression::OptionalFormats everyDecoder =
        Compression::OptionalFormat::Lzma | Compression::OptionalFormat::Bzip2 | Compression::OptionalFormat::Lz4 |
        Compression::OptionalFormat::Zstd;
    const QByteArray signature = input.left(LibArchive::kStreamSignatureSize);
    return !LibArchive::undecodableStreamReason(signature, everyDecoder & ~Compression::OptionalFormats(filter))
                .isEmpty();
}

QString disagreement(const QByteArray& input)
{
    for (const Compression::OptionalFormat filter : comparableFilters()) {
        const bool bids = libarchiveBids(filter, input);
        const bool refused = rulesRefuse(filter, input);
        // Zstandard and LZ4 have magic numbers, which the rules go by alone: they may refuse more than libarchive
        // takes, such as a frame whose header is damaged
        const bool exact =
            (filter == Compression::OptionalFormat::Lzma) || (filter == Compression::OptionalFormat::Bzip2);
        if ((bids && !refused) || (exact && refused && !bids)) {
            return QStringLiteral("%1: libarchive %2 the input, the copied rules %3 it (starts with %4)")
                .arg(QLatin1String(bidderName(filter)),
                     bids ? QStringLiteral("takes") : QStringLiteral("does not take"),
                     refused ? QStringLiteral("refuse") : QStringLiteral("do not refuse"),
                     QString::fromLatin1(input.left(24).toHex(' ')));
        }
    }
    return {};
}

QList<QByteArray> edgeInputs()
{
    QList<QByteArray> inputs;

    // Legacy lzma: a properties byte, the dictionary size and the uncompressed size, then data
    const QByteArray lzmaData = QByteArray::fromHex("003d8889c65436c3174fe29e63242ecb7bfc7f5aa5a6fffffc6a3000");
    for (const quint8 properties : {0x00, 0x01, 0x5C, 0x5D, 0x5E, 0x5F, 0xE0, 0xE1, 0xFF}) {
        for (const quint32 dictionary :
             {0U,         1U,         0x800U,     0xFFFU,     0x1000U,     0x1001U,    0x10000U,
              0x2FFFFFU,  0x300000U,  0x300001U,  0x380000U,  0x500000U,   0x3E00000U, 0x3F00000U,
              0x3F00001U, 0x4000000U, 0x8000000U, 0x8000001U, 0x10000000U, 0xFFFFFFFFU}) {
            for (const quint64 size : {~quint64{0}, quint64{0x11}, quint64{0}}) {
                const QByteArray header = QByteArray(1, static_cast<char>(properties)) + le32(dictionary) + le64(size);
                inputs.append(header);
                inputs.append(header + lzmaData.left(1));
                inputs.append(header + lzmaData);
            }
        }
    }

    // bzip2: the signature, a block size and the marker of a block or of the end of the stream
    const QByteArray blockMarker = QByteArray::fromHex("314159265359");
    const QByteArray endMarker = QByteArray::fromHex("177245385090");
    for (const char blockSize : {'0', '1', '5', '9', ':', 'a'}) {
        for (const QByteArray& marker : {blockMarker, endMarker, QByteArray::fromHex("314159265358")}) {
            const QByteArray start = QByteArray("BZh") + blockSize + marker;
            inputs.append(start);
            inputs.append(start + QByteArray(4, '\0'));
            inputs.append(start + QByteArray(32, '\x55'));
        }
    }

    // Zstandard and LZ4 frames, after skippable frames of either format that hold some bytes or more than the input
    const QList<QByteArray> frames = {
        le32(0xFD2FB528),                                    // Zstandard
        le32(0xFD2FB528) + QByteArray::fromHex("2000"),      // Zstandard, with a frame header
        le32(0x184D2204) + QByteArray::fromHex("644070"),    // LZ4, with a frame descriptor
        le32(0x184D2204) + QByteArray::fromHex("c44070"),    // LZ4, with a descriptor of an unknown version
        le32(0x184C2102) + QByteArray::fromHex("00000000"),  // LZ4 before version 1.4
        QByteArray("plain text"),
        QByteArray(),
    };
    const QList<QByteArray> skippable = {
        QByteArray(),
        le32(0x184D2A50) + le32(0),
        le32(0x184D2A5F) + le32(4) + QByteArray(4, 'x'),
        le32(0x184D2A50) + le32(0x10000),
    };
    for (const QByteArray& skipped : skippable) {
        for (const QByteArray& frame : frames) {
            for (const int padding : {0, 3, 7, 20}) {
                inputs.append(skipped + frame + QByteArray(padding, '\0'));
            }
        }
    }

    return inputs;
}

}  // namespace BidAgreement
