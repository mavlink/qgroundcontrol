#include "ArchiveFixtures.h"

#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QIODevice>

#include "FileFixtures.h"

namespace ArchiveFixtures {

QByteArray makeStreamedZip(const QList<ZipEntry>& entries, bool endRecord)
{
    constexpr quint16 lengthAtEnd = 0x0008;
    constexpr quint16 deflate = 8;

    QByteArray zip;
    QDataStream out(&zip, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    QByteArray centralDirectory;
    QDataStream directory(&centralDirectory, QIODevice::WriteOnly);
    directory.setByteOrder(QDataStream::LittleEndian);

    for (const ZipEntry& entry : entries) {
        const QByteArray deflated = TestFixtures::deflate(entry.data);
        const quint32 localHeaderOffset = zip.size();
        // Local file header, data, data descriptor
        out << quint32(0x04034B50) << quint16(20) << lengthAtEnd << deflate << quint32(0) << quint32(0) << quint32(0)
            << quint32(0) << quint16(entry.name.size()) << quint16(0);
        out.writeRawData(entry.name.constData(), entry.name.size());
        out.writeRawData(deflated.constData(), deflated.size());
        out << quint32(0x08074B50) << TestFixtures::crc32(entry.data) << quint32(deflated.size())
            << quint32(entry.data.size());
        // Central directory header. An entry made on Unix (3) has its mode in the upper half of the external
        // attributes.
        const quint16 madeBy = (entry.unixMode != 0) ? ((3 << 8) | 20) : 20;
        directory << quint32(0x02014B50) << madeBy << quint16(20) << lengthAtEnd << deflate << quint32(0)
                  << TestFixtures::crc32(entry.data) << quint32(deflated.size()) << quint32(entry.data.size())
                  << quint16(entry.name.size()) << quint16(0) << quint16(0) << quint16(0) << quint16(0)
                  << quint32(entry.unixMode << 16) << localHeaderOffset;
        directory.writeRawData(entry.name.constData(), entry.name.size());
    }

    const quint32 centralDirectoryOffset = zip.size();
    out.writeRawData(centralDirectory.constData(), centralDirectory.size());
    if (endRecord) {
        out << quint32(0x06054B50) << quint16(0) << quint16(0) << quint16(entries.size()) << quint16(entries.size())
            << quint32(centralDirectory.size()) << centralDirectoryOffset << quint16(0);
    }
    return zip;
}

QByteArray xzStream(const QByteArray& data)
{
    const auto littleEndian32 = [](quint32 value) {
        QByteArray bytes(4, '\0');
        qToLittleEndian(value, bytes.data());
        return bytes;
    };
    const auto withCrc = [&](const QByteArray& bytes) { return bytes + littleEndian32(TestFixtures::crc32(bytes)); };
    const auto padded = [](QByteArray bytes) { return bytes.append(QByteArray((4 - (bytes.size() % 4)) % 4, '\0')); };
    const auto varint = [](quint64 value) {
        QByteArray bytes;
        for (; value >= 0x80; value >>= 7) {
            bytes.append(static_cast<char>((value & 0x7F) | 0x80));
        }
        return bytes.append(static_cast<char>(value));
    };

    // Stream header: the magic and the flags of a stream without a check
    QByteArray stream = QByteArray::fromHex("fd377a585a00") + withCrc(QByteArray(2, '\0'));
    // Block header of 12 bytes: one filter, LZMA2 (0x21) with one property byte, and padding
    const QByteArray blockHeader = withCrc(QByteArray::fromHex("0200210100000000"));
    // A chunk that resets the dictionary and is not compressed, with its size less one, then the end of LZMA2
    const quint16 sizeLessOne = static_cast<quint16>(data.size() - 1);
    const QByteArray chunk = QByteArray(1, '\x01') + static_cast<char>(sizeLessOne >> 8) +
                             static_cast<char>(sizeLessOne & 0xFF) + data + QByteArray(1, '\0');
    stream += blockHeader + padded(chunk);
    // Index of the one block: its size without padding and the size of its data
    const QByteArray index = withCrc(padded(QByteArray(1, '\0') + varint(1) +
                                            varint(static_cast<quint64>(blockHeader.size() + chunk.size())) +
                                            varint(static_cast<quint64>(data.size()))));
    stream += index;
    // Stream footer: the size of the index in units of four bytes, less one, and the flags again
    const QByteArray backward = littleEndian32(static_cast<quint32>((index.size() / 4) - 1)) + QByteArray(2, '\0');
    return stream + littleEndian32(TestFixtures::crc32(backward)) + backward + "YZ";
}

bool writeGzipOfZeros(const QString& path, int memberCount)
{
    const QByteArray member = TestFixtures::gzipMember(QByteArray(64 * 1024, '\0'));
    return TestFixtures::writeFile(path, member.repeated(memberCount));
}

namespace {

/// @a a times @a b modulo the polynomial of CRC-32, with the bits in the order of the CRC
quint32 multiplyModuloCrc32(quint32 a, quint32 b)
{
    quint32 product = 0;
    for (quint32 bit = 1U << 31; bit != 0; bit >>= 1) {
        if ((a & bit) != 0) {
            product ^= b;
        }
        b = ((b & 1U) != 0) ? ((b >> 1) ^ 0xEDB88320U) : (b >> 1);
    }
    return product;
}

/// CRC-32 of @a size zero bytes, without reading them: each zero byte multiplies the register by x^8, so all of
/// them multiply it by x^(8 * size). zlib's crc32_combine() works the same way
quint32 crc32OfZeros(qint64 size)
{
    quint32 power = 1U << 31;   // x^0
    quint32 square = 1U << 23;  // x^8
    for (qint64 remaining = size; remaining != 0; remaining >>= 1) {
        if ((remaining & 1) != 0) {
            power = multiplyModuloCrc32(square, power);
        }
        square = multiplyModuloCrc32(square, square);
    }
    return ~multiplyModuloCrc32(power, 0xFFFFFFFFU);
}

}  // namespace

bool writeZipOfZeros(const QString& path, const QByteArray& entryName, qint64 size)
{
    if (size < 1) {
        return false;
    }

    // One deflate block with the fixed codes: a zero, copies of the 258 bytes before, the longest a copy can be,
    // and a zero for each byte left. The bits of each code are in the order deflate writes them
    constexpr quint32 kLastFixedBlock = 0b011;  // 3 bits
    constexpr quint32 kZero = 0x0C;             // 8 bits: literal 0
    constexpr quint32 kCopy = 0xA3;             // 13 bits: length 258 (code 285), distance 1 (code 0)
    constexpr quint32 kEndOfBlock = 0;          // 7 bits
    constexpr qint64 kCopyLength = 258;

    QByteArray stream;
    stream.reserve(static_cast<qsizetype>((size / kCopyLength) * 13 / 8) + 300);
    quint64 pending = 0;
    int pendingBits = 0;
    const auto put = [&](quint32 bits, int count) {
        pending |= static_cast<quint64>(bits) << pendingBits;
        pendingBits += count;
        while (pendingBits >= 8) {
            stream.append(static_cast<char>(pending & 0xFFU));
            pending >>= 8;
            pendingBits -= 8;
        }
    };
    put(kLastFixedBlock, 3);
    put(kZero, 8);
    for (qint64 copy = 0; copy < ((size - 1) / kCopyLength); ++copy) {
        put(kCopy, 13);
    }
    for (qint64 zero = 0; zero < ((size - 1) % kCopyLength); ++zero) {
        put(kZero, 8);
    }
    put(kEndOfBlock, 7);
    if (pendingBits > 0) {
        stream.append(static_cast<char>(pending));
    }

    ZipWriter writer;
    writer.add({.name = entryName}, stream, crc32OfZeros(size), static_cast<quint64>(size));
    return TestFixtures::writeFile(path, writer.finish());
}

QStringList entriesBelow(const QString& directoryPath)
{
    const QDir directory(directoryPath);
    QStringList entries;
    QDirIterator it(directoryPath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        entries.append(directory.relativeFilePath(it.next()));
    }
    entries.sort();
    return entries;
}

}  // namespace ArchiveFixtures
