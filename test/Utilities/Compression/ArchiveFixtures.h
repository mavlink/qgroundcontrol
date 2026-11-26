#pragma once

/// @file ArchiveFixtures.h
/// @brief Archives built by hand, so that tests can create archives an archiver would refuse to write
///
/// The tar and ZIP builders are defined in this header and use only Qt Core, so that the fuzz smoke test can use
/// them.

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtEndian>

namespace ArchiveFixtures {

// The test resources: one manifest.json, as an archive and as a single compressed stream
inline const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
inline const QString sevenZipResource = QStringLiteral(":/unittest/manifest.json.7z");
inline const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
inline const QString xzResource = QStringLiteral(":/unittest/manifest.json.xz");

/// Part of a sparse file that is stored in the archive
struct TarSparseData
{
    qint64 offset = 0;
    qint64 size = 0;
};

/// Entry of a hand-built tar archive
struct TarEntry
{
    QByteArray name = {};
    QByteArray data = {};
    /// '0' file, '1' hard link, '2' symbolic link, '3' character device, '4' block device, '5' directory,
    /// '6' FIFO, 'L' the name of the next entry (GNU), 'S' old GNU sparse file
    char type = '0';
    QByteArray linkName = {};
    QList<TarSparseData> sparseData = {};  ///< For 'S': where the data belongs in the file, at most four parts
    qint64 sparseFileSize = 0;             ///< For 'S': size of the file with its holes
    qint64 modified = 0;                   ///< Modification time in seconds since the epoch
};

/// Entry of a hand-built ZIP archive
struct ZipEntry
{
    QByteArray name = {};
    QByteArray data = {};  ///< Of a symbolic link: the path it points to
    /// Unix file type and permissions (0 = none: a file, or a directory if the name ends with '/'). Only the central
    /// directory holds them, which only a reader that can seek sees
    quint32 unixMode = 0;
    bool deflated = true;      ///< makeZip(): whether the data is deflated or stored
    bool utf8Name = true;      ///< makeZip(): whether the flag that declares the name UTF-8 is set
    qint64 declaredSize = -1;  ///< makeZip(): size the headers give the extracted data (-1 = the size of the data)
    qint64 modified = 0;       ///< makeZip(): modification time in seconds since the epoch, in UTC (0 = none)
};

/// Header block of a tar entry that declares @a dataSize bytes of data, which need not be the size of its data
inline QByteArray tarHeader(const TarEntry& entry, qint64 dataSize)
{
    QByteArray header(512, '\0');
    const auto setField = [&header](qsizetype offset, const QByteArray& value) {
        header.replace(offset, value.size(), value);
    };
    const auto octal = [](qint64 value) { return QByteArray::number(value, 8).rightJustified(11, '0'); };
    // A size too large for eleven octal digits is stored as a big-endian binary number, marked by the top bit
    const auto size = [&octal](qint64 value) {
        if (value < (Q_INT64_C(1) << 33)) {
            return octal(value);
        }
        QByteArray binary(12, '\0');
        binary[0] = static_cast<char>(0x80);
        for (int i = 0; i < 8; ++i) {
            binary[11 - i] = static_cast<char>((value >> (8 * i)) & 0xFF);
        }
        return binary;
    };
    // A name longer than its field is split, and its start goes into the ustar prefix field
    const qsizetype prefixSize = (entry.name.size() > 100) ? entry.name.indexOf('/', entry.name.size() - 101) : -1;
    setField(0, entry.name.mid(prefixSize + 1).left(100));
    setField(345, entry.name.left(qMax<qsizetype>(prefixSize, 0)));
    setField(100, "0000644");
    setField(108, "0000000");
    setField(116, "0000000");
    setField(124, size(dataSize));
    setField(136, octal(entry.modified));
    setField(148, "        ");
    header[156] = entry.type;
    setField(157, entry.linkName.left(100));
    setField(257, "ustar");
    setField(263, "00");
    if (entry.type == 'S') {
        // The old GNU header has its own magic, and lists the stored parts where ustar has the prefix
        setField(257, QByteArray("ustar  \0", 8));
        qsizetype field = 386;
        for (const TarSparseData& part : entry.sparseData) {
            setField(field, octal(part.offset));
            setField(field + 12, octal(part.size));
            field += 24;
        }
        setField(483, size(entry.sparseFileSize));
    }

    quint32 checksum = 0;
    for (const char byte : std::as_const(header)) {
        checksum += static_cast<quint8>(byte);
    }
    setField(148, QByteArray::number(checksum, 8).rightJustified(6, '0'));
    header[154] = '\0';
    return header;
}

/// Builds a tar archive
inline QByteArray makeTar(const QList<TarEntry>& entries)
{
    constexpr qsizetype blockSize = 512;
    QByteArray archive;
    for (const TarEntry& entry : entries) {
        archive.append(tarHeader(entry, entry.data.size()));
        archive.append(entry.data);
        archive.append(QByteArray((blockSize - (entry.data.size() % blockSize)) % blockSize, '\0'));
    }
    archive.append(QByteArray(2 * blockSize, '\0'));
    return archive;
}

/// CRC-32 of @a data, as ZIP stores it
inline quint32 crc32Of(QByteArrayView data)
{
    quint32 crc = 0xFFFFFFFFU;
    for (const char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

/// Builds a ZIP archive entry by entry, from data that is already as the archive stores it
class ZipWriter
{
public:
    /// @param stored The data as the archive holds it: deflated if @a entry says so
    /// @param crc CRC-32 of the extracted data
    /// @param size Size the headers give the extracted data
    void add(const ZipEntry& entry, const QByteArray& stored, quint32 crc, quint64 size)
    {
        // Sizes that do not fit in the fields of the headers are in a ZIP64 extra field, in both headers
        const bool zip64 = (size >= 0xFFFFFFFFU) || (static_cast<quint64>(stored.size()) >= 0xFFFFFFFFU);
        QByteArray extra;
        if (zip64) {
            append<quint16>(extra, 0x0001);
            append<quint16>(extra, 16);
            append<quint64>(extra, size);
            append<quint64>(extra, static_cast<quint64>(stored.size()));
        }
        if (entry.modified != 0) {
            // Extended timestamp, which holds the modification time in UTC
            append<quint16>(extra, 0x5455);
            append<quint16>(extra, 5);
            append<quint8>(extra, 0x01);
            append<quint32>(extra, static_cast<quint32>(entry.modified));
        }

        // What the local header and the central directory header share, from the version needed to extract
        const quint16 version = zip64 ? 45 : 20;
        QByteArray shared;
        append<quint16>(shared, version);
        append<quint16>(shared, entry.utf8Name ? 0x0800 : 0x0000);
        append<quint16>(shared, entry.deflated ? 8 : 0);
        append<quint16>(shared, 0x0000);  // Time and date: 1 January 1980, 00:00
        append<quint16>(shared, 0x0021);
        append<quint32>(shared, crc);
        append<quint32>(shared, zip64 ? 0xFFFFFFFFU : static_cast<quint32>(stored.size()));
        append<quint32>(shared, zip64 ? 0xFFFFFFFFU : static_cast<quint32>(size));
        append<quint16>(shared, static_cast<quint16>(entry.name.size()));
        append<quint16>(shared, static_cast<quint16>(extra.size()));

        const quint32 localHeaderOffset = static_cast<quint32>(_archive.size());
        append<quint32>(_archive, 0x04034B50);
        _archive += shared + entry.name + extra + stored;

        // An entry made on Unix (3) has its mode in the upper half of the external attributes
        append<quint32>(_directory, 0x02014B50);
        append<quint16>(_directory, (entry.unixMode != 0) ? static_cast<quint16>((3 << 8) | version) : version);
        _directory += shared;
        append<quint16>(_directory, 0);  // Comment length
        append<quint16>(_directory, 0);  // Disk
        append<quint16>(_directory, 0);  // Internal attributes
        append<quint32>(_directory, entry.unixMode << 16);
        append<quint32>(_directory, localHeaderOffset);
        _directory += entry.name + extra;
        ++_count;
    }

    /// The archive: the entries, their central directory and the record that ends it
    QByteArray finish() const
    {
        QByteArray end;
        append<quint32>(end, 0x06054B50);
        append<quint16>(end, 0);  // Disk
        append<quint16>(end, 0);  // Disk of the central directory
        append<quint16>(end, _count);
        append<quint16>(end, _count);
        append<quint32>(end, static_cast<quint32>(_directory.size()));
        append<quint32>(end, static_cast<quint32>(_archive.size()));
        append<quint16>(end, 0);  // Comment length
        return _archive + _directory + end;
    }

private:
    template <typename T>
    static void append(QByteArray& bytes, T value)
    {
        const T littleEndian = qToLittleEndian(value);
        bytes.append(reinterpret_cast<const char*>(&littleEndian), sizeof(T));
    }

    QByteArray _archive;
    QByteArray _directory;
    quint16 _count = 0;
};

/// Builds a ZIP archive, with the sizes of each entry in its local header and in the central directory
inline QByteArray makeZip(const QList<ZipEntry>& entries)
{
    ZipWriter writer;
    for (ZipEntry entry : entries) {
        // No data is stored as it is: a deflate stream is never empty
        entry.deflated = entry.deflated && !entry.data.isEmpty();
        QByteArray stored = entry.data;
        if (entry.deflated) {
            // qCompress() gives the size, a zlib header, the deflate stream and an Adler-32
            const QByteArray zlibStream = qCompress(entry.data);
            stored = zlibStream.mid(6, zlibStream.size() - 10);
        }
        const qint64 size = (entry.declaredSize >= 0) ? entry.declaredSize : entry.data.size();
        writer.add(entry, stored, crc32Of(entry.data), static_cast<quint64>(size));
    }
    return writer.finish();
}

/// Builds a ZIP archive as it is written to a stream: the sizes of an entry follow its data instead of being
/// in its local header. Without @a endRecord the central directory cannot be found, so a reader that can seek
/// cannot get the sizes from there either.
QByteArray makeStreamedZip(const QList<ZipEntry>& entries, bool endRecord);

/// An xz stream that holds @a data (1 byte to 64 KiB) in one LZMA2 chunk that is not compressed, without a check
QByteArray xzStream(const QByteArray& data);

/// Writes a gzip file of @a memberCount members, each expanding to 64 KiB of zeros
bool writeGzipOfZeros(const QString& path, int memberCount);

/// Writes a ZIP archive of one deflated file, @a entryName, of @a size zero bytes (at least one), without holding
/// them in memory: the archive is a few KiB for each MiB of them
bool writeZipOfZeros(const QString& path, const QByteArray& entryName, qint64 size);

/// Paths of everything below @a directoryPath, relative to it and sorted
QStringList entriesBelow(const QString& directoryPath);

}  // namespace ArchiveFixtures
