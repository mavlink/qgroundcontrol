#include "QGCCompressionTest.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QBuffer>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>
#include <QtTest/QSignalSpy>

#include "QGCArchiveFile.h"
#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCDecompressDevice.h"

bool QGCCompressionTest::_compareFiles(const QString& file1, const QString& file2)
{
    QFile f1(file1);
    QFile f2(file2);
    if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
        return false;
    }
    QCryptographicHash hash1(QCryptographicHash::Sha256);
    QCryptographicHash hash2(QCryptographicHash::Sha256);
    hash1.addData(&f1);
    hash2.addData(&f2);
    return hash1.result() == hash2.result();
}

quint32 QGCCompressionTest::_crc32(const QByteArray& data)
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

bool QGCCompressionTest::_writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && (file.write(data) == data.size());
}

QByteArray QGCCompressionTest::_readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QStringList QGCCompressionTest::_entriesBelow(const QString& directoryPath)
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

QByteArray QGCCompressionTest::_gzipMember(const QByteArray& payload)
{
    // qCompress() output: 4-byte length, 2-byte zlib header, deflate stream, 4-byte Adler-32
    const QByteArray zlibStream = qCompress(payload);
    QByteArray member = QByteArray::fromHex("1f8b08000000000000ff");
    member.append(zlibStream.mid(6, zlibStream.size() - 10));
    for (const quint32 field : {_crc32(payload), static_cast<quint32>(payload.size())}) {
        for (int shift = 0; shift < 32; shift += 8) {
            member.append(static_cast<char>((field >> shift) & 0xFFU));
        }
    }
    return member;
}

bool QGCCompressionTest::_writeGzipOfZeros(const QString& path, int memberCount, const QByteArray& firstMember)
{
    const QByteArray member = _gzipMember(QByteArray(64 * 1024, '\0'));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || (file.write(firstMember) != firstMember.size())) {
        return false;
    }
    for (int i = 0; i < memberCount; ++i) {
        if (file.write(member) != member.size()) {
            return false;
        }
    }
    return true;
}

QByteArray QGCCompressionTest::_makeStreamedZip(const QList<ZipEntry>& entries, bool endRecord)
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
        const QByteArray zlibStream = qCompress(entry.data);
        const QByteArray deflated = zlibStream.mid(6, zlibStream.size() - 10);
        const quint32 localHeaderOffset = zip.size();
        // Local file header, data, data descriptor
        out << quint32(0x04034B50) << quint16(20) << lengthAtEnd << deflate << quint32(0) << quint32(0) << quint32(0)
            << quint32(0) << quint16(entry.name.size()) << quint16(0);
        out.writeRawData(entry.name.constData(), entry.name.size());
        out.writeRawData(deflated.constData(), deflated.size());
        out << quint32(0x08074B50) << _crc32(entry.data) << quint32(deflated.size()) << quint32(entry.data.size());
        // Central directory header
        directory << quint32(0x02014B50) << quint16(20) << quint16(20) << lengthAtEnd << deflate << quint32(0)
                  << _crc32(entry.data) << quint32(deflated.size()) << quint32(entry.data.size())
                  << quint16(entry.name.size()) << quint16(0) << quint16(0) << quint16(0) << quint16(0) << quint32(0)
                  << localHeaderOffset;
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

QByteArray QGCCompressionTest::_tarHeader(const TarEntry& entry, qint64 dataSize)
{
    QByteArray header(512, '\0');
    const auto setField = [&header](qsizetype offset, const QByteArray& value) {
        header.replace(offset, value.size(), value);
    };
    const auto octal = [](qint64 value) { return QByteArray::number(value, 8).rightJustified(11, '0'); };
    // A name longer than its field continues in the ustar prefix field
    const qsizetype prefixSize = (entry.name.size() > 100) ? entry.name.indexOf('/', entry.name.size() - 101) : -1;
    setField(0, entry.name.mid(prefixSize + 1).left(100));
    setField(345, entry.name.left(qMax<qsizetype>(prefixSize, 0)));
    setField(100, "0000644");
    setField(108, "0000000");
    setField(116, "0000000");
    if (dataSize < (Q_INT64_C(1) << 33)) {
        setField(124, octal(dataSize));
    } else {
        // A size that does not fit in eleven octal digits is stored as a binary number below a set top bit
        QByteArray binary(12, '\0');
        binary[0] = static_cast<char>(0x80);
        for (int i = 0; i < 8; ++i) {
            binary[11 - i] = static_cast<char>((dataSize >> (8 * i)) & 0xFF);
        }
        setField(124, binary);
    }
    setField(136, "00000000000");
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
        setField(483, octal(entry.sparseFileSize));
    }

    quint32 checksum = 0;
    for (const char byte : std::as_const(header)) {
        checksum += static_cast<quint8>(byte);
    }
    setField(148, QByteArray::number(checksum, 8).rightJustified(6, '0'));
    header[154] = '\0';
    return header;
}

QByteArray QGCCompressionTest::_makeTar(const QList<TarEntry>& entries)
{
    constexpr qsizetype blockSize = 512;
    QByteArray archive;
    for (const TarEntry& entry : entries) {
        archive.append(_tarHeader(entry, entry.data.size()));
        archive.append(entry.data);
        archive.append(QByteArray((blockSize - (entry.data.size() % blockSize)) % blockSize, '\0'));
    }
    archive.append(QByteArray(2 * blockSize, '\0'));
    return archive;
}

// ============================================================================
// Format Detection Tests
// ============================================================================
void QGCCompressionTest::_testFormatDetection_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("contentFallback");
    QTest::addColumn<int>("expected");

    const auto addRow = [](const char* tag, const QString& path, bool fallback, QGCCompression::Format fmt) {
        QTest::newRow(tag) << path << fallback << static_cast<int>(fmt);
    };

    addRow("zip", "file.zip", true, QGCCompression::Format::ZIP);
    addRow("7z", "file.7z", true, QGCCompression::Format::SEVENZ);
    addRow("gz", "file.gz", true, QGCCompression::Format::GZIP);
    addRow("gzip", "file.gzip", true, QGCCompression::Format::GZIP);
    addRow("xz", "file.xz", true, QGCCompression::Format::XZ);
    addRow("lzma", "file.lzma", true, QGCCompression::Format::XZ);
    addRow("zst", "file.zst", true, QGCCompression::Format::ZSTD);
    addRow("zstd", "file.zstd", true, QGCCompression::Format::ZSTD);
    addRow("tar", "file.tar", true, QGCCompression::Format::TAR);
    addRow("tar.gz", "file.tar.gz", true, QGCCompression::Format::TAR_GZ);
    addRow("tgz", "file.tgz", true, QGCCompression::Format::TAR_GZ);
    addRow("tar.xz", "file.tar.xz", true, QGCCompression::Format::TAR_XZ);
    addRow("txz", "file.txz", true, QGCCompression::Format::TAR_XZ);
    addRow("unknown-ext", "file.unknown", true, QGCCompression::Format::Auto);
    addRow("txt", "file.txt", true, QGCCompression::Format::Auto);
    addRow("uppercase-zip", "FILE.ZIP", true, QGCCompression::Format::ZIP);
    addRow("mixedcase-gz", "File.Gz", true, QGCCompression::Format::GZIP);
    addRow("unknown-no-fallback", "file.unknown", false, QGCCompression::Format::Auto);
}

void QGCCompressionTest::_testFormatDetection()
{
    QFETCH(QString, path);
    QFETCH(bool, contentFallback);
    QFETCH(int, expected);

    QCOMPARE(static_cast<int>(QGCCompression::detectFormat(path, contentFallback)), expected);
}

void QGCCompressionTest::_testDetectFormatFromFile_data()
{
    QTest::addColumn<QString>("resource");
    QTest::addColumn<int>("expected");

    QTest::newRow("zip") << QStringLiteral(":/unittest/manifest.json.zip")
                         << static_cast<int>(QGCCompression::Format::ZIP);
    QTest::newRow("gz") << QStringLiteral(":/unittest/manifest.json.gz")
                        << static_cast<int>(QGCCompression::Format::GZIP);
    QTest::newRow("xz") << QStringLiteral(":/unittest/manifest.json.xz")
                        << static_cast<int>(QGCCompression::Format::XZ);
    QTest::newRow("zst") << QStringLiteral(":/unittest/manifest.json.zst")
                         << static_cast<int>(QGCCompression::Format::ZSTD);
#ifdef QGC_ENABLE_BZIP2
    QTest::newRow("bz2") << QStringLiteral(":/unittest/manifest.json.bz2")
                         << static_cast<int>(QGCCompression::Format::BZIP2);
#endif
    QTest::newRow("7z") << QStringLiteral(":/unittest/manifest.json.7z")
                        << static_cast<int>(QGCCompression::Format::SEVENZ);
}

void QGCCompressionTest::_testDetectFormatFromFile()
{
    QFETCH(QString, resource);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(QGCCompression::detectFormatFromFile(resource)), expected);
}

void QGCCompressionTest::_testDetectFormatFromMagicBytes_data()
{
    QTest::addColumn<QByteArray>("magic");
    QTest::addColumn<int>("expected");

    const auto padded = [](std::initializer_list<int> bytes) {
        QByteArray data;
        for (int b : bytes) {
            data.append(static_cast<char>(b));
        }
        data.append(QByteArray(10, '\0'));
        return data;
    };

    QTest::newRow("gzip") << padded({0x1F, 0x8B}) << static_cast<int>(QGCCompression::Format::GZIP);
    QTest::newRow("zip") << padded({0x50, 0x4B, 0x03, 0x04}) << static_cast<int>(QGCCompression::Format::ZIP);
    QTest::newRow("xz") << padded({0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00}) << static_cast<int>(QGCCompression::Format::XZ);
    QTest::newRow("zstd") << padded({0x28, 0xB5, 0x2F, 0xFD}) << static_cast<int>(QGCCompression::Format::ZSTD);
}

void QGCCompressionTest::_testDetectFormatFromMagicBytes()
{
    QFETCH(QByteArray, magic);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(QGCCompression::detectFormatFromData(magic)), expected);
}

void QGCCompressionTest::_testFormatDetectionFromContent()
{
    QTemporaryDir tempDir;
    // Test content fallback in detectFormat()
    // Copy a .gz file to a file without extension and verify detection
    const QString noExtFile = tempDir.filePath("compressed_no_ext");
    {
        QFile source(":/unittest/manifest.json.gz");
        QVERIFY(source.open(QIODevice::ReadOnly));
        QFile dest(noExtFile);
        QVERIFY(dest.open(QIODevice::WriteOnly));
        dest.write(source.readAll());
    }
    // With content fallback enabled, should detect as GZIP
    QCOMPARE(QGCCompression::detectFormat(noExtFile, true), QGCCompression::Format::GZIP);
    // Without content fallback, should return Auto
    QCOMPARE(QGCCompression::detectFormat(noExtFile, false), QGCCompression::Format::Auto);
}

void QGCCompressionTest::_testFormatHelpers()
{
    // isArchiveFormat
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::ZIP));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::SEVENZ));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::TAR));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::TAR_GZ));
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::TAR_XZ));
    QVERIFY(!QGCCompression::isArchiveFormat(QGCCompression::Format::GZIP));
    QVERIFY(!QGCCompression::isArchiveFormat(QGCCompression::Format::XZ));
    QVERIFY(!QGCCompression::isArchiveFormat(QGCCompression::Format::ZSTD));
    // isCompressionFormat
    QVERIFY(QGCCompression::isCompressionFormat(QGCCompression::Format::GZIP));
    QVERIFY(QGCCompression::isCompressionFormat(QGCCompression::Format::XZ));
    QVERIFY(QGCCompression::isCompressionFormat(QGCCompression::Format::ZSTD));
    QVERIFY(!QGCCompression::isCompressionFormat(QGCCompression::Format::ZIP));
    QVERIFY(!QGCCompression::isCompressionFormat(QGCCompression::Format::TAR));
    // isCompressedFile / isArchiveFile
    QVERIFY(QGCCompression::isCompressedFile("file.gz"));
    QVERIFY(QGCCompression::isCompressedFile("file.xz"));
    QVERIFY(!QGCCompression::isCompressedFile("file.zip"));
    QVERIFY(!QGCCompression::isCompressedFile("file.txt"));
    QVERIFY(QGCCompression::isArchiveFile("file.zip"));
    QVERIFY(QGCCompression::isArchiveFile("file.7z"));
    QVERIFY(QGCCompression::isArchiveFile("file.tar.gz"));
    QVERIFY(!QGCCompression::isArchiveFile("file.gz"));
    // strippedPath
    QCOMPARE(QGCCompression::strippedPath("file.gz"), QString("file"));
    QCOMPARE(QGCCompression::strippedPath("file.xz"), QString("file"));
    QCOMPARE(QGCCompression::strippedPath("file.zst"), QString("file"));
    QCOMPARE(QGCCompression::strippedPath("file.txt"), QString("file.txt"));
    QCOMPARE(QGCCompression::strippedPath("path/to/file.gz"), QString("path/to/file"));
    // formatExtension
    QCOMPARE(QGCCompression::formatExtension(QGCCompression::Format::ZIP), QString(".zip"));
    QCOMPARE(QGCCompression::formatExtension(QGCCompression::Format::GZIP), QString(".gz"));
    QCOMPARE(QGCCompression::formatExtension(QGCCompression::Format::TAR_GZ), QString(".tar.gz"));
    // formatName
    QVERIFY(!QGCCompression::formatName(QGCCompression::Format::ZIP).isEmpty());
    QVERIFY(!QGCCompression::formatName(QGCCompression::Format::GZIP).isEmpty());
}

// ============================================================================
// Archive Extraction Tests (from Qt resources)
// ============================================================================
void QGCCompressionTest::_testZipFromResource()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir.path() + "/resource_output";
    QVERIFY2(QGCCompression::extractArchive(zipResource, outputPath), "Failed to extract ZIP from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");
    QDir outputDir(outputPath);
    QStringList files = outputDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from ZIP");
}

void QGCCompressionTest::_test7zFromResource()
{
    QTemporaryDir tempDir;
    const QString archiveResource = QStringLiteral(":/unittest/manifest.json.7z");
    const QString outputPath = tempDir.path() + "/7z_output";
    QVERIFY2(QGCCompression::extractArchive(archiveResource, outputPath, QGCCompression::Format::SEVENZ),
             "Failed to extract 7z from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");
    QDir outputDir(outputPath);
    QStringList files = outputDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from 7z");
    QVERIFY2(QFile::exists(outputPath + "/manifest.json"), "manifest.json not extracted");
}

void QGCCompressionTest::_testExtractArchiveAtomic()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir.path() + "/atomic_output";

    QVERIFY2(QGCCompression::extractArchiveAtomic(zipResource, outputPath),
             "Failed to atomically extract ZIP from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");
    QVERIFY2(QFile::exists(outputPath + "/manifest.json"), "manifest.json not extracted");
}

void QGCCompressionTest::_testExtractArchiveAtomicReplacesExistingDirectory()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir.path() + "/atomic_replace";
    QVERIFY(QDir().mkpath(outputPath));

    const QString staleFilePath = outputPath + "/stale.txt";
    QFile staleFile(staleFilePath);
    QVERIFY(staleFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    staleFile.write("stale");
    staleFile.close();
    QVERIFY(QFile::exists(staleFilePath));

    QVERIFY2(QGCCompression::extractArchiveAtomic(zipResource, outputPath),
             "Atomic extraction into existing directory failed");
    QVERIFY(QFile::exists(outputPath + "/manifest.json"));
    QVERIFY(!QFile::exists(staleFilePath));
}

void QGCCompressionTest::_testExtractArchiveAtomicFailureKeepsExistingDirectory()
{
    QTemporaryDir tempDir;
    const QString outputPath = tempDir.path() + "/atomic_rollback";
    QVERIFY(QDir().mkpath(outputPath));

    const QString retainedFilePath = outputPath + "/retained.txt";
    QFile retainedFile(retainedFilePath);
    QVERIFY(retainedFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray retainedContent("keep-me");
    retainedFile.write(retainedContent);
    retainedFile.close();

    const QString corruptArchivePath = tempDir.path() + "/corrupt_atomic.zip";
    QFile corrupt(corruptArchivePath);
    QVERIFY(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
    corrupt.write("not-an-archive");
    corrupt.close();

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(".*Unrecognized archive format.*"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Archive is empty or invalid.*"));
    QVERIFY(!QGCCompression::extractArchiveAtomic(corruptArchivePath, outputPath));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QFile retainedAfter(retainedFilePath);
    QVERIFY(retainedAfter.open(QIODevice::ReadOnly));
    QCOMPARE(retainedAfter.readAll(), retainedContent);
    retainedAfter.close();
}

void QGCCompressionTest::_testListArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QStringList entries = QGCCompression::listArchive(zipResource);
    QVERIFY2(!entries.isEmpty(), "listArchive returned empty list");
    bool foundManifest = false;
    for (const QString& entry : entries) {
        if (entry.contains("manifest.json")) {
            foundManifest = true;
            break;
        }
    }
    QVERIFY2(foundManifest, "manifest.json not found in archive listing");
    // Test auto-detection
    QStringList autoEntries = QGCCompression::listArchive(zipResource, QGCCompression::Format::Auto);
    QCOMPARE(autoEntries.size(), entries.size());
}

void QGCCompressionTest::_testListArchiveDetailed()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QList<QGCCompression::ArchiveEntry> entries = QGCCompression::listArchiveDetailed(zipResource);
    QVERIFY(!entries.isEmpty());
    bool foundManifest = false;
    for (const auto& entry : entries) {
        if (entry.name == "manifest.json") {
            foundManifest = true;
            QVERIFY(entry.size > 0);
            QVERIFY(!entry.isDirectory);
            break;
        }
    }
    QVERIFY2(foundManifest, "manifest.json not found in detailed listing");
}

void QGCCompressionTest::_testListArchiveNaturalSort()
{
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("natural.tar");
    // Stored in an order which neither the archive's own nor that of the character codes would keep
    QVERIFY(_writeFile(archivePath, _makeTar({{.name = "file10.txt"},
                                              {.name = "File2.txt"},
                                              {.name = "file1.txt"},
                                              {.name = "dir/file11.txt"},
                                              {.name = "file20.txt"},
                                              {.name = "dir/file3.txt"}})));

    // Numbers compare by their value, and an upper case letter does not come before the lower case ones
    const QStringList expected{QStringLiteral("dir/file3.txt"), QStringLiteral("dir/file11.txt"),
                               QStringLiteral("file1.txt"),     QStringLiteral("File2.txt"),
                               QStringLiteral("file10.txt"),    QStringLiteral("file20.txt")};
    QCOMPARE(QGCCompression::listArchive(archivePath), expected);

    QStringList detailedNames;
    for (const QGCCompression::ArchiveEntry& entry : QGCCompression::listArchiveDetailed(archivePath)) {
        detailedNames.append(entry.name);
    }
    QCOMPARE(detailedNames, expected);
}

void QGCCompressionTest::_testGetArchiveStats()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QGCCompression::ArchiveStats stats = QGCCompression::getArchiveStats(zipResource);
    // Should have at least one file
    QVERIFY(stats.totalEntries > 0);
    QVERIFY(stats.fileCount > 0);
    QVERIFY(stats.totalUncompressedSize > 0);
    QVERIFY(stats.largestFileSize > 0);
    QVERIFY(!stats.largestFileName.isEmpty());
    // File count + directory count should equal total entries
    QCOMPARE(stats.totalEntries, stats.fileCount + stats.directoryCount);
    // Largest file size should not exceed total size
    QVERIFY(stats.largestFileSize <= stats.totalUncompressedSize);
    // Test with 7z archive
    const QString archiveResource = QStringLiteral(":/unittest/manifest.json.7z");
    const QGCCompression::ArchiveStats stats7z = QGCCompression::getArchiveStats(archiveResource);
    QVERIFY(stats7z.totalEntries > 0);
    QVERIFY(stats7z.fileCount > 0);
    // Test non-existent file returns zero stats
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    const QGCCompression::ArchiveStats emptyStats = QGCCompression::getArchiveStats("/nonexistent/file.zip");
    verifyExpectedLogMessage();
    QCOMPARE(emptyStats.totalEntries, 0);
    QCOMPARE(emptyStats.fileCount, 0);
    QCOMPARE(emptyStats.totalUncompressedSize, 0);
}

void QGCCompressionTest::_testValidateArchive()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QVERIFY(QGCCompression::validateArchive(zipResource));
    // Test corrupt archive fails validation
    const QString corruptZip = tempDir.path() + "/corrupt_validate.zip";
    QFile corrupt(corruptZip);
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("This is not a valid ZIP file");
    corrupt.close();
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    QVERIFY(!QGCCompression::validateArchive(corruptZip));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::InvalidArchive);
    // Test non-existent file
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY(!QGCCompression::validateArchive("/nonexistent/path/file.zip"));
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testFileExists()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // File that exists
    QVERIFY(QGCCompression::fileExists(zipResource, "manifest.json"));
    // File that doesn't exist
    QVERIFY(!QGCCompression::fileExists(zipResource, "nonexistent.txt"));
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::FileNotInArchive);
    // Empty file name
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
    QVERIFY(!QGCCompression::fileExists(zipResource, ""));
    verifyExpectedLogMessage();
    // Non-existent archive
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY(!QGCCompression::fileExists("/nonexistent/path/file.zip", "test.txt"));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::FileNotFound);
    // A file that is not an archive cannot be said to be without the entry
    QTemporaryDir tempDir;
    const QString notAnArchive = tempDir.filePath("not_an_archive.zip");
    QVERIFY(_writeFile(notAnArchive, "This is not a valid ZIP file"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Unrecognized archive format"));
    QVERIFY(!QGCCompression::fileExists(notAnArchive, "manifest.json"));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::InvalidArchive);
}

void QGCCompressionTest::_testExtractArchiveFiltered()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Test filter that accepts all files
    const QString outputDir1 = tempDir.path() + "/filtered_all";
    int extractedCount = 0;
    bool success = QGCCompression::extractArchiveFiltered(
        zipResource, outputDir1, [&extractedCount](const QGCCompression::ArchiveEntry& entry) -> bool {
            Q_UNUSED(entry);
            extractedCount++;
            return true;  // Accept all
        });
    QVERIFY2(success, "extractArchiveFiltered with accept-all filter failed");
    QVERIFY(extractedCount > 0);
    QVERIFY(QFile::exists(outputDir1 + "/manifest.json"));
    // Test filter that rejects all files
    const QString outputDir2 = tempDir.path() + "/filtered_none";
    success = QGCCompression::extractArchiveFiltered(zipResource, outputDir2,
                                                     [](const QGCCompression::ArchiveEntry&) -> bool {
                                                         return false;  // Reject all
                                                     });
    QVERIFY2(success, "extractArchiveFiltered with reject-all filter should still succeed");
    // Directory should exist but be empty (or not created at all)
    QDir dir2(outputDir2);
    if (dir2.exists()) {
        QStringList files = dir2.entryList(QDir::Files | QDir::NoDotAndDotDot);
        QVERIFY2(files.isEmpty(), "No files should be extracted when filter rejects all");
    }
    // Test filter by extension (only .json files)
    const QString outputDir3 = tempDir.path() + "/filtered_json";
    QStringList extractedNames;
    success = QGCCompression::extractArchiveFiltered(
        zipResource, outputDir3, [&extractedNames](const QGCCompression::ArchiveEntry& entry) -> bool {
            if (entry.name.endsWith(".json")) {
                extractedNames.append(entry.name);
                return true;
            }
            return false;
        });
    QVERIFY(success);
    QVERIFY(extractedNames.contains("manifest.json"));
    // Test filter receives correct metadata
    bool metadataCorrect = true;
    success =
        QGCCompression::extractArchiveFiltered(zipResource, tempDir.path() + "/filtered_meta",
                                               [&metadataCorrect](const QGCCompression::ArchiveEntry& entry) -> bool {
                                                   if (entry.name == "manifest.json") {
                                                       // Verify metadata is populated
                                                       if (entry.size <= 0)
                                                           metadataCorrect = false;
                                                       if (entry.isDirectory)
                                                           metadataCorrect = false;
                                                   }
                                                   return true;
                                               });
    QVERIFY(success);
    QVERIFY2(metadataCorrect, "Filter did not receive correct metadata");
    // Test with non-existent file
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    success = QGCCompression::extractArchiveFiltered("/nonexistent/file.zip", outputDir1,
                                                     [](const QGCCompression::ArchiveEntry&) { return true; });
    QVERIFY(!success);
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testExtractSingleFile()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString extractedPath = tempDir.path() + "/extracted_manifest.json";
    QVERIFY(QGCCompression::extractFile(zipResource, "manifest.json", extractedPath));
    QVERIFY(QFile::exists(extractedPath));
    QVERIFY(QFileInfo(extractedPath).size() > 0);
    // Verify content contains expected JSON structure
    QFile extracted(extractedPath);
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    const QByteArray content = extracted.readAll();
    QVERIFY(content.contains("\"name\""));
    extracted.close();
    // Test non-existent file returns false
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    QVERIFY(!QGCCompression::extractFile(zipResource, "nonexistent.txt", tempDir.path() + "/nope.txt"));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::FileNotInArchive);
}

void QGCCompressionTest::_testExtractFileData()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QByteArray extracted = QGCCompression::extractFileData(zipResource, "manifest.json");
    QVERIFY(!extracted.isEmpty());
    QVERIFY(extracted.contains("\"name\""));
    // Test non-existent file returns empty
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    const QByteArray notFound = QGCCompression::extractFileData(zipResource, "nonexistent.txt");
    QVERIFY(notFound.isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::FileNotInArchive);
    // Test empty file name returns empty
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
    const QByteArray emptyName = QGCCompression::extractFileData(zipResource, "");
    QVERIFY(emptyName.isEmpty());
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testExtractMultipleFiles()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Extract available file(s)
    const QString outputDir = tempDir.path() + "/multi_extract";
    QStringList filesToExtract = {"manifest.json"};
    QVERIFY(QGCCompression::extractFiles(zipResource, filesToExtract, outputDir));
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Test with non-existent file in list - should fail
    const QString outputDir2 = tempDir.path() + "/multi_extract2";
    QStringList badFiles = {"manifest.json", "nonexistent.txt"};
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    QVERIFY(!QGCCompression::extractFiles(zipResource, badFiles, outputDir2));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::FileNotInArchive);
    // Test empty list succeeds (no-op)
    QVERIFY(QGCCompression::extractFiles(zipResource, {}, outputDir));
}

void QGCCompressionTest::_testExtractByPattern()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Test wildcard pattern matching
    const QString outputDir = tempDir.path() + "/pattern_extract";
    QStringList extractedFiles;
    QVERIFY(QGCCompression::extractByPattern(zipResource, {"*.json"}, outputDir, &extractedFiles));
    QVERIFY(!extractedFiles.isEmpty());
    QVERIFY(extractedFiles.contains("manifest.json"));
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Test exact filename as pattern (should work like extractFiles)
    const QString outputDir2 = tempDir.path() + "/pattern_exact";
    QStringList exactFiles;
    QVERIFY(QGCCompression::extractByPattern(zipResource, {"manifest.json"}, outputDir2, &exactFiles));
    QCOMPARE(exactFiles.size(), 1);
    QCOMPARE(exactFiles.first(), QStringLiteral("manifest.json"));
    // Test no matches returns false
    const QString outputDir3 = tempDir.path() + "/pattern_nomatch";
    QVERIFY(!QGCCompression::extractByPattern(zipResource, {"*.xyz"}, outputDir3));
    // Test empty patterns returns false
    const QString outputDir4 = tempDir.path() + "/pattern_empty";
    QVERIFY(!QGCCompression::extractByPattern(zipResource, {}, outputDir4));
}

// ============================================================================
// Single-File Decompression Tests (from Qt resources)
// ============================================================================
void QGCCompressionTest::_testDecompressFromResource()
{
    QTemporaryDir tempDir;

    struct ResourceTest
    {
        const char* resource;
        const char* outputName;
        const char* name;
    };

    std::vector<ResourceTest> resources = {
        {":/unittest/manifest.json.gz", "manifest_gz.json", "GZIP"},
        {":/unittest/manifest.json.xz", "manifest_xz.json", "XZ"},
        {":/unittest/manifest.json.zst", "manifest_zst.json", "ZSTD"},
#ifdef QGC_ENABLE_BZIP2
        {":/unittest/manifest.json.bz2", "manifest_bz2.json", "BZip2"},
#endif
#ifdef QGC_ENABLE_LZ4
        {":/unittest/manifest.json.lz4", "manifest_lz4.json", "LZ4"},
#endif
    };
    for (const auto& res : resources) {
        const QString outputFile = tempDir.path() + "/" + res.outputName;
        QVERIFY2(QGCCompression::decompressFile(res.resource, outputFile),
                 qPrintable(QString("Failed to decompress %1 file").arg(res.name)));
        QVERIFY(QFile::exists(outputFile));
        QVERIFY(QFileInfo(outputFile).size() > 0);
    }
}

void QGCCompressionTest::_testDecompressData()
{
    // Read compressed data from resources and decompress in memory
    struct ResourceTest
    {
        const char* resource;
        const char* name;
    };

    std::vector<ResourceTest> resources = {
        {":/unittest/manifest.json.gz", "GZIP"},   {":/unittest/manifest.json.xz", "XZ"},
        {":/unittest/manifest.json.zst", "ZSTD"},
#ifdef QGC_ENABLE_BZIP2
        {":/unittest/manifest.json.bz2", "BZip2"},
#endif
#ifdef QGC_ENABLE_LZ4
        {":/unittest/manifest.json.lz4", "LZ4"},
#endif
    };
    for (const auto& res : resources) {
        QFile resourceFile(res.resource);
        QVERIFY2(resourceFile.open(QIODevice::ReadOnly),
                 qPrintable(QString("Failed to open resource: %1").arg(res.resource)));
        const QByteArray compressedData = resourceFile.readAll();
        resourceFile.close();
        QVERIFY2(!compressedData.isEmpty(), qPrintable(QString("Resource is empty: %1").arg(res.resource)));
        const QByteArray decompressed = QGCCompression::decompressData(compressedData);
        QVERIFY2(!decompressed.isEmpty(), qPrintable(QString("%1 decompression returned empty data").arg(res.name)));
        QVERIFY2(decompressed.contains("\"name\""),
                 qPrintable(QString("%1 decompressed content invalid").arg(res.name)));
    }
}

void QGCCompressionTest::_testDecompressIfNeeded()
{
    QTemporaryDir tempDir;
    // Test with compressed file
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString outputPath = tempDir.path() + "/decompress_if_needed.json";
    QString result = QGCCompression::decompressIfNeeded(gzResource, outputPath);
    QVERIFY2(!result.isEmpty(), "decompressIfNeeded should return output path");
    QCOMPARE(result, outputPath);
    QVERIFY(QFile::exists(outputPath));
    QVERIFY(QFileInfo(outputPath).size() > 0);
    // Test with non-compressed file (should return original path)
    const QString plainFile = tempDir.path() + "/plain.txt";
    QFile plain(plainFile);
    QVERIFY(plain.open(QIODevice::WriteOnly));
    plain.write("plain text content");
    plain.close();
    result = QGCCompression::decompressIfNeeded(plainFile);
    QCOMPARE(result, plainFile);
    QVERIFY(QFile::exists(plainFile));
    // Test with non-existent file
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Decompression failed.*"));
    result = QGCCompression::decompressIfNeeded("/nonexistent/file.gz");
    QVERIFY(result.isEmpty());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testDecompressIfNeededDefaultOutput_data()
{
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("inputName");
    QTest::addColumn<QString>("expectedOutputName");

    const QString gz = QStringLiteral(":/unittest/manifest.json.gz");
    const QString xz = QStringLiteral(":/unittest/manifest.json.xz");
    const QString zst = QStringLiteral(":/unittest/manifest.json.zst");

    QTest::newRow("gz") << gz << "manifest.json.gz" << "manifest.json";
    QTest::newRow("gzip") << gz << "manifest.json.gzip" << "manifest.json";
    QTest::newRow("xz") << xz << "manifest.json.xz" << "manifest.json";
    QTest::newRow("lzma") << xz << "manifest.json.lzma" << "manifest.json";
    QTest::newRow("zst") << zst << "manifest.json.zst" << "manifest.json";
    QTest::newRow("zstd") << zst << "manifest.json.zstd" << "manifest.json";
#ifdef QGC_ENABLE_BZIP2
    const QString bz2 = QStringLiteral(":/unittest/manifest.json.bz2");
    QTest::newRow("bz2") << bz2 << "manifest.json.bz2" << "manifest.json";
    QTest::newRow("bzip2") << bz2 << "manifest.json.bzip2" << "manifest.json";
#endif
#ifdef QGC_ENABLE_LZ4
    QTest::newRow("lz4") << QStringLiteral(":/unittest/manifest.json.lz4") << "manifest.json.lz4" << "manifest.json";
#endif
    QTest::newRow("uppercase") << gz << "MANIFEST.JSON.GZIP" << "MANIFEST.JSON";
    QTest::newRow("no-extension") << gz << "manifest" << "manifest.decompressed";
    QTest::newRow("unrecognised-extension") << gz << "manifest.bin" << "manifest.bin.decompressed";
}

void QGCCompressionTest::_testDecompressIfNeededDefaultOutput()
{
    QFETCH(QString, resource);
    QFETCH(QString, inputName);
    QFETCH(QString, expectedOutputName);

    QFile resourceFile(resource);
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressed = resourceFile.readAll();

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(inputName);
    QFile input(inputPath);
    QVERIFY(input.open(QIODevice::WriteOnly));
    QCOMPARE(input.write(compressed), compressed.size());
    input.close();

    const QString expectedPath = tempDir.filePath(expectedOutputName);
    QCOMPARE(QGCCompression::decompressIfNeeded(inputPath), expectedPath);
    QCOMPARE(QGCCompression::strippedPath(inputPath), expectedPath);

    QFile output(expectedPath);
    QVERIFY(output.open(QIODevice::ReadOnly));
    QVERIFY(output.readAll().contains("\"name\""));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QCOMPARE(input.readAll(), compressed);
    output.close();

    // decompressFile() without an output path writes to the same place
    const QStringList inputAndOutput = _entriesBelow(tempDir.path());
    QVERIFY(output.remove());
    QVERIFY(QGCCompression::decompressFile(inputPath));
    QCOMPARE(_entriesBelow(tempDir.path()), inputAndOutput);
}

// ============================================================================
// Progress Callback Tests
// ============================================================================
void QGCCompressionTest::_testProgressCallbackExtract_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"extractArchive", "extractFromDevice", "extractArchiveFiltered"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testProgressCallbackExtract()
{
    QFETCH(QString, api);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("compressible.zip");
    const QString outputPath = tempDir.filePath("output");
    // Each entry is many times larger than the archive
    QVERIFY(_writeFile(archivePath, _makeStreamedZip({{.name = "first.txt", .data = QByteArray(64 * 1024, 'a')},
                                                      {.name = "second.txt", .data = QByteArray(64 * 1024, 'b')}},
                                                     true)));
    const qint64 archiveSize = QFileInfo(archivePath).size();
    QFile device(archivePath);
    QVERIFY(device.open(QIODevice::ReadOnly));

    qint64 lastProgress = 0;
    int callCount = 0;
    bool progressValid = true;
    const QGCCompression::ProgressCallback progress = [&](qint64 bytesProcessed, qint64 totalBytes) -> bool {
        // Both count bytes of the archive, not of its entries
        if ((bytesProcessed < lastProgress) || (bytesProcessed <= 0) || (bytesProcessed > totalBytes) ||
            (totalBytes != archiveSize)) {
            progressValid = false;
        }
        lastProgress = bytesProcessed;
        callCount++;
        return true;  // Continue
    };

    if (api == "extractArchive") {
        QVERIFY(QGCCompression::extractArchive(archivePath, outputPath, QGCCompression::Format::ZIP, progress));
    } else if (api == "extractFromDevice") {
        QVERIFY(QGCCompression::extractFromDevice(&device, outputPath, progress));
    } else {
        QVERIFY(QGCCompression::extractArchiveFiltered(
            archivePath, outputPath, [](const QGCCompression::ArchiveEntry&) { return true; }, progress));
    }
    QVERIFY2(progressValid, "Progress values were invalid");
    QVERIFY(callCount > 0);
    QCOMPARE(_entriesBelow(outputPath), (QStringList{QStringLiteral("first.txt"), QStringLiteral("second.txt")}));
}

// ============================================================================
// Format Reported for the Last Operation
// ============================================================================
void QGCCompressionTest::_testDetectedFormat_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("filter");

    const QString gz = QStringLiteral(":/unittest/manifest.json.gz");
    const QString xz = QStringLiteral(":/unittest/manifest.json.xz");
    const QString zst = QStringLiteral(":/unittest/manifest.json.zst");
    const QString zip = QStringLiteral(":/unittest/manifest.json.zip");
    const QString sevenZip = QStringLiteral(":/unittest/manifest.json.7z");

    QTest::newRow("decompressFile") << "decompressFile" << gz << "^raw$" << "gzip";
    QTest::newRow("decompressData") << "decompressData" << xz << "^raw$" << "xz";
    QTest::newRow("decompressFromDevice-file") << "decompressFromDevice-file" << zst << "^raw$" << "zstd";
    QTest::newRow("decompressFromDevice-memory") << "decompressFromDevice-memory" << gz << "^raw$" << "gzip";
    QTest::newRow("extractArchive") << "extractArchive" << zip << "^ZIP" << "none";
    QTest::newRow("extractArchiveAtomic") << "extractArchiveAtomic" << sevenZip << "^7-Zip" << "none";
    QTest::newRow("extractFromDevice") << "extractFromDevice" << zip << "^ZIP" << "none";
    QTest::newRow("extractArchiveFiltered") << "extractArchiveFiltered" << zip << "^ZIP" << "none";
    QTest::newRow("extractFileDataFromDevice") << "extractFileDataFromDevice" << sevenZip << "^7-Zip" << "none";
}

void QGCCompressionTest::_testDetectedFormat()
{
    QFETCH(QString, api);
    QFETCH(QString, resource);
    QFETCH(QString, format);
    QFETCH(QString, filter);

    QTemporaryDir tempDir;
    const QString outputPath = tempDir.filePath("output");
    const QByteArray data = _readFile(resource);
    QBuffer device;
    device.setData(data);
    QVERIFY(device.open(QIODevice::ReadOnly));

    // An operation of the other kind first: what it detected must not be what is reported afterwards
    const bool isArchive = QGCCompression::isArchiveFile(resource);
    if (isArchive) {
        QVERIFY(!QGCCompression::decompressData(_readFile(QStringLiteral(":/unittest/manifest.json.xz"))).isEmpty());
        QCOMPARE(QGCCompression::detectedFilterName(), QStringLiteral("xz"));
    } else {
        QVERIFY(
            QGCCompression::extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), tempDir.filePath("other")));
        QVERIFY(QGCCompression::detectedFormatName().startsWith("ZIP"));
    }

    if (api == "decompressFile") {
        QVERIFY(QGCCompression::decompressFile(resource, outputPath));
    } else if (api == "decompressData") {
        QVERIFY(!QGCCompression::decompressData(data).isEmpty());
    } else if (api == "decompressFromDevice-file") {
        QVERIFY(QGCCompression::decompressFromDevice(&device, outputPath));
    } else if (api == "decompressFromDevice-memory") {
        QVERIFY(!QGCCompression::decompressFromDevice(&device).isEmpty());
    } else if (api == "extractArchive") {
        QVERIFY(QGCCompression::extractArchive(resource, outputPath));
    } else if (api == "extractArchiveAtomic") {
        QVERIFY(QGCCompression::extractArchiveAtomic(resource, outputPath));
    } else if (api == "extractFromDevice") {
        QVERIFY(QGCCompression::extractFromDevice(&device, outputPath));
    } else if (api == "extractArchiveFiltered") {
        QVERIFY(QGCCompression::extractArchiveFiltered(resource, outputPath,
                                                       [](const QGCCompression::ArchiveEntry&) { return true; }));
    } else if (api == "extractFileDataFromDevice") {
        QVERIFY(!QGCCompression::extractFileDataFromDevice(&device, QStringLiteral("manifest.json")).isEmpty());
    } else {
        QFAIL("Unknown row");
    }

    const QString detectedFormat = QGCCompression::detectedFormatName();
    QVERIFY2(QRegularExpression(format).match(detectedFormat).hasMatch(), qPrintable(detectedFormat));
    QCOMPARE(QGCCompression::detectedFilterName(), filter);
}

void QGCCompressionTest::_testDetectedFormatAfterFailure_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<int>("reports");  // Warnings the failure is logged with

    QTest::newRow("decompressFile") << "decompressFile" << 1;
    QTest::newRow("decompressData") << "decompressData" << 1;
    QTest::newRow("decompressFromDevice-file") << "decompressFromDevice-file" << 1;
    QTest::newRow("decompressFromDevice-memory") << "decompressFromDevice-memory" << 1;
    // The archive is opened twice: for the size pre-check and for the extraction
    QTest::newRow("extractArchive") << "extractArchive" << 2;
    QTest::newRow("extractArchiveAtomic") << "extractArchiveAtomic" << 2;
    QTest::newRow("extractArchiveFiltered") << "extractArchiveFiltered" << 2;
    QTest::newRow("extractFromDevice") << "extractFromDevice" << 1;
    QTest::newRow("extractFileDataFromDevice") << "extractFileDataFromDevice" << 1;
}

void QGCCompressionTest::_testDetectedFormatAfterFailure()
{
    QFETCH(QString, api);
    QFETCH(int, reports);

    // Neither input gets as far as the header that tells its format: a gzip stream that ends with its own
    // header, and bytes that are no archive
    const bool isStream = api.startsWith("decompress");
    const QByteArray data =
        isStream ? QByteArray::fromHex("1f8b08000000000000ff") : QByteArray("This is not an archive of any format");
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(isStream ? "cut.gz" : "garbage.zip");
    const QString outputPath = tempDir.filePath("output");
    QVERIFY(_writeFile(inputPath, data));
    QBuffer device;
    device.setData(data);
    QVERIFY(device.open(QIODevice::ReadOnly));

    // What the operation before it detected must not be reported for the one that fails
    QVERIFY(QGCCompression::extractArchive(QStringLiteral(":/unittest/manifest.json.zip"), tempDir.filePath("other")));
    QVERIFY(QGCCompression::detectedFormatName().startsWith("ZIP"));
    QCOMPARE(QGCCompression::detectedFilterName(), QStringLiteral("none"));

    for (int i = 0; i < reports; ++i) {
        expectLogMessage(
            "Utilities.QGClibarchive", QtWarningMsg,
            QRegularExpression("truncated gzip input|Unrecognized archive format|Archive is empty or invalid"));
    }
    if (api == "decompressFile") {
        QVERIFY(!QGCCompression::decompressFile(inputPath, outputPath));
    } else if (api == "decompressData") {
        QVERIFY(QGCCompression::decompressData(data).isEmpty());
    } else if (api == "decompressFromDevice-file") {
        QVERIFY(!QGCCompression::decompressFromDevice(&device, outputPath));
    } else if (api == "decompressFromDevice-memory") {
        QVERIFY(QGCCompression::decompressFromDevice(&device).isEmpty());
    } else if (api == "extractArchive") {
        QVERIFY(!QGCCompression::extractArchive(inputPath, outputPath));
    } else if (api == "extractArchiveAtomic") {
        QVERIFY(!QGCCompression::extractArchiveAtomic(inputPath, outputPath));
    } else if (api == "extractArchiveFiltered") {
        QVERIFY(!QGCCompression::extractArchiveFiltered(inputPath, outputPath,
                                                        [](const QGCCompression::ArchiveEntry&) { return true; }));
    } else if (api == "extractFromDevice") {
        QVERIFY(!QGCCompression::extractFromDevice(&device, outputPath));
    } else if (api == "extractFileDataFromDevice") {
        QVERIFY(QGCCompression::extractFileDataFromDevice(&device, QStringLiteral("manifest.json")).isEmpty());
    } else {
        QFAIL("Unknown row");
    }
    for (int i = 0; i < reports; ++i) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(QGCCompression::detectedFormatName(), QString());
    QCOMPARE(QGCCompression::detectedFilterName(), QString());
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================
void QGCCompressionTest::_testCorruptArchive()
{
    QTemporaryDir tempDir;
    // Create a file with garbage data pretending to be a ZIP
    const QString corruptZip = tempDir.path() + "/corrupt.zip";
    QFile file(corruptZip);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("This is not a valid ZIP file - just garbage data!");
    file.close();
    // Attempt to extract - should fail gracefully
    // extractAnyArchive opens the archive twice: once in getArchiveStats, once directly
    const QString outputPath = tempDir.path() + "/corrupt_output";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    bool success = QGCCompression::extractArchive(corruptZip, outputPath);
    QVERIFY2(!success, "Extracting corrupt archive should fail");
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    // Attempt to list - should return empty or fail
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    QStringList entries = QGCCompression::listArchive(corruptZip);
    QVERIFY2(entries.isEmpty(), "Listing corrupt archive should return empty list");
    verifyExpectedLogMessage();
    // Test corrupt compressed file with valid GZIP header but truncated data
    const QString corruptGz = tempDir.path() + "/corrupt.gz";
    QFile gzFile(corruptGz);
    QVERIFY(gzFile.open(QIODevice::WriteOnly));
    const char corruptGzipData[] = {
        '\x1f', '\x8b',                  // GZIP magic
        '\x08',                          // Deflate method
        '\x00',                          // Flags
        '\x00', '\x00', '\x00', '\x00',  // Modification time
        '\x00',                          // Extra flags
        '\xff',                          // OS (unknown)
    };
    gzFile.write(corruptGzipData, sizeof(corruptGzipData));
    gzFile.close();
    const QString decompressedPath = tempDir.path() + "/corrupt_decompressed";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*truncated gzip input"));
    success = QGCCompression::decompressFile(corruptGz, decompressedPath);
    QVERIFY2(!success, "Decompressing truncated GZIP file should fail");
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testNonExistentInput()
{
    QTemporaryDir tempDir;
    const QString nonExistent = "/path/that/does/not/exist/file.txt";
    const QString outputPath = tempDir.path() + "/output";
    // Test decompressFile with non-existent input
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY2(!QGCCompression::decompressFile(nonExistent + ".gz", outputPath),
             "decompressFile should fail for non-existent input");
    verifyExpectedLogMessage();
    // Test extractArchive with non-existent archive
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QVERIFY2(!QGCCompression::extractArchive(nonExistent + ".zip", outputPath),
             "extractArchive should fail for non-existent archive");
    verifyExpectedLogMessage();
    // Test listArchive with non-existent file
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QStringList entries = QGCCompression::listArchive(nonExistent + ".zip");
    QVERIFY2(entries.isEmpty(), "listArchive should return empty for non-existent file");
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testNoExternalDecompressors()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a POSIX shell to stand in for the external decompressors");
#else
    QTemporaryDir tempDir;
    const QString markerPath = tempDir.filePath("spawned");
    const QString binPath = tempDir.filePath("bin");
    QVERIFY(QDir().mkpath(binPath));
    // Stand-ins for the programs libarchive runs when it has no built-in decoder for a filter
    for (const char* program : {"bzip2", "lz4", "lrzip"}) {
        QFile script(binPath + QLatin1Char('/') + QLatin1String(program));
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\n: > '" + QFile::encodeName(markerPath) + "'\nwhile read -r _; do :; done\n");
        script.close();
        QVERIFY(script.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    }
    const QByteArray originalPath = qgetenv("PATH");
    qputenv("PATH", QFile::encodeName(binPath));
    const auto restorePath = qScopeGuard([&originalPath]() { qputenv("PATH", originalPath); });

    const QByteArray padding(32, '\0');

    // lrzip is always decoded by an external program
    const QString lrzipTar = tempDir.filePath("lrzip.tar");
    QFile lrzipFile(lrzipTar);
    QVERIFY(lrzipFile.open(QIODevice::WriteOnly));
    lrzipFile.write(QByteArray("LRZI\x00\x06", 6) + padding);
    lrzipFile.close();
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Unrecognized archive format"));
    QVERIFY(QGCCompression::listArchive(lrzipTar).isEmpty());
    verifyExpectedLogMessage();

#ifndef QGC_ENABLE_BZIP2
    // Each way of opening a single stream refuses one this build cannot decode
    const QByteArray bzip2Stream = QByteArray("BZh91AY&SY") + padding;
    const QString bzip2Path = tempDir.filePath("stream.bz2");
    QVERIFY(_writeFile(bzip2Path, bzip2Stream));
    QBuffer bzip2Device;
    bzip2Device.setData(bzip2Stream);
    QVERIFY(bzip2Device.open(QIODevice::ReadOnly));
    const auto expectBzip2Refused = [this]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("bzip2.*not supported"));
    };

    expectBzip2Refused();
    QVERIFY(QGCCompression::decompressData(bzip2Stream).isEmpty());
    verifyExpectedLogMessage();

    expectBzip2Refused();
    QVERIFY(!QGCCompression::decompressFile(bzip2Path, tempDir.filePath("stream")));
    verifyExpectedLogMessage();
    QVERIFY(!QFile::exists(tempDir.filePath("stream")));

    expectBzip2Refused();
    QVERIFY(QGCCompression::decompressFromDevice(&bzip2Device).isEmpty());
    verifyExpectedLogMessage();

    QGCDecompressDevice bzip2Decompressor(&bzip2Device);
    expectBzip2Refused();
    QVERIFY(!bzip2Decompressor.open(QIODevice::ReadOnly));
    verifyExpectedLogMessage();
    QCOMPARE(bzip2Decompressor.errorString(), QStringLiteral("Compression format is not supported by this build"));
#endif
#ifndef QGC_ENABLE_LZ4
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("LZ4.*not supported"));
    QVERIFY(QGCCompression::decompressData(QByteArray("\x04\x22\x4D\x18\x64\x40\xA7", 7) + padding).isEmpty());
    verifyExpectedLogMessage();
#endif

    QVERIFY2(!QFile::exists(markerPath), "An external decompression program was run");
#endif
}

void QGCCompressionTest::_testTruncatedArchive_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("data");
    // Entry whose header is whole and whose data the cut falls in
    QTest::addColumn<QString>("cutEntry");

    const QByteArray tar = _makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                     {.name = "second.txt", .data = QByteArray(600, 'b')},
                                     {.name = "third.txt", .data = "c"}});
    const QByteArray zip = _readFile(QStringLiteral(":/unittest/manifest.json.zip"));
    const QByteArray sevenZip = _readFile(QStringLiteral(":/unittest/manifest.json.7z"));

    // first.txt takes three 512-byte blocks: its header and two of data
    QTest::newRow("tar-inside-header") << "cut.tar" << tar.left(3 * 512 + 256) << QString();
    QTest::newRow("tar-inside-data") << "cut.tar" << tar.left(4 * 512 + 100) << "second.txt";
    QTest::newRow("zip-inside-header") << "cut.zip" << zip.left(10) << QString();
    QTest::newRow("zip-inside-data") << "cut.zip" << zip.left(zip.size() / 2) << "manifest.json";
    QTest::newRow("7z") << "cut.7z" << sevenZip.left(sevenZip.size() / 2) << QString();
}

void QGCCompressionTest::_testTruncatedArchive()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, data);
    QFETCH(QString, cutEntry);
    QVERIFY(!data.isEmpty());

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(fileName);
    QVERIFY(_writeFile(archivePath, data));

    // Each call reports the damage once, where it meets it: at an entry header or in entry data
    const QRegularExpression readFailure(
        "Failed to read archive entry|Failed to read data|Validation failed for entry");
    const auto expectReadFailure = [this, &readFailure]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, readFailure);
    };

    expectReadFailure();
    QVERIFY(!QGCCompression::validateArchive(archivePath));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::InvalidArchive);

    expectReadFailure();
    QVERIFY(QGCCompression::listArchive(archivePath).isEmpty());
    verifyExpectedLogMessage();

    expectReadFailure();
    QVERIFY(QGCCompression::listArchiveDetailed(archivePath).isEmpty());
    verifyExpectedLogMessage();

    expectReadFailure();
    QCOMPARE(QGCCompression::getArchiveStats(archivePath).totalEntries, 0);
    verifyExpectedLogMessage();

    // Once for the size pre-check and once for the extraction itself
    expectReadFailure();
    expectReadFailure();
    QVERIFY(!QGCCompression::extractArchive(archivePath, tempDir.filePath("output")));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    const QString atomicOutput = tempDir.filePath("atomic_output");
    expectReadFailure();
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Archive is empty or invalid"));
    QVERIFY(!QGCCompression::extractArchiveAtomic(archivePath, atomicOutput));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!QFileInfo::exists(atomicOutput));

    // A search for an entry the archive does not have reads the headers up to the damage, which is
    // then what is reported: the entry is not said to be missing
    const QString absent = QStringLiteral("absent.txt");
    const QString absentOutput = tempDir.filePath("absent_output");
    const QString partialDir = tempDir.filePath("partial");
    const auto rejectAll = [](const QGCCompression::ArchiveEntry&) { return false; };
    QFile archiveFile(archivePath);
    QVERIFY(archiveFile.open(QIODevice::ReadOnly));

    struct Search
    {
        const char* name;
        int reports;
        std::function<bool()> failed;
    };

    const QList<Search> searches{
        {"extractFile", 1, [&]() { return !QGCCompression::extractFile(archivePath, absent, absentOutput); }},
        {"extractFileData", 1, [&]() { return QGCCompression::extractFileData(archivePath, absent).isEmpty(); }},
        {"extractFiles", 1, [&]() { return !QGCCompression::extractFiles(archivePath, {absent}, partialDir); }},
        {"extractByPattern", 1, [&]() { return !QGCCompression::extractByPattern(archivePath, {absent}, partialDir); }},
        {"fileExists", 1, [&]() { return !QGCCompression::fileExists(archivePath, absent); }},
        // Once for the size pre-check and once for the extraction itself
        {"extractArchiveFiltered", 2,
         [&]() { return !QGCCompression::extractArchiveFiltered(archivePath, partialDir, rejectAll); }},
        {"extractFromDevice", 1,
         [&]() { return !QGCCompression::extractFromDevice(&archiveFile, tempDir.filePath("device_output")); }},
        {"extractFileDataFromDevice", 1,
         [&]() {
             return archiveFile.seek(0) && QGCCompression::extractFileDataFromDevice(&archiveFile, absent).isEmpty();
         }},
        {"QGCArchiveFile", 1,
         [&]() {
             QGCArchiveFile entry(archivePath, absent);
             return !entry.open(QIODevice::ReadOnly) && !entry.errorString().contains("not found");
         }},
    };
    for (const Search& search : searches) {
        for (int i = 0; i < search.reports; ++i) {
            expectReadFailure();
        }
        QVERIFY2(search.failed(), search.name);
        for (int i = 0; i < search.reports; ++i) {
            verifyExpectedLogMessage();
        }
    }
    QVERIFY(!QFileInfo::exists(absentOutput));

    // fileExists() answers no for two reasons, and the error is all that tells them apart
    expectReadFailure();
    QVERIFY(!QGCCompression::fileExists(archivePath, absent));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::InvalidArchive);

    if (cutEntry.isEmpty()) {
        return;
    }

    // An entry whose header comes before the damage is there
    QVERIFY(QGCCompression::fileExists(archivePath, cutEntry));

    // An entry whose data is cut is found, and fails while it is copied: no part of it is kept
    const auto expectDataFailure = [this]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Read error"));
    };
    const QString cutOutput = tempDir.filePath("cut_output");
    const QString cutDir = tempDir.filePath("cut");
    const auto acceptAll = [](const QGCCompression::ArchiveEntry&) { return true; };

    expectDataFailure();
    QVERIFY(!QGCCompression::extractFile(archivePath, cutEntry, cutOutput));
    verifyExpectedLogMessage();
    QVERIFY(!QFileInfo::exists(cutOutput));

    expectDataFailure();
    QVERIFY(QGCCompression::extractFileData(archivePath, cutEntry).isEmpty());
    verifyExpectedLogMessage();

    expectDataFailure();
    QVERIFY(!QGCCompression::extractFiles(archivePath, {cutEntry}, cutDir));
    verifyExpectedLogMessage();

    expectDataFailure();
    QVERIFY(!QGCCompression::extractByPattern(archivePath, {cutEntry}, cutDir));
    verifyExpectedLogMessage();

    expectReadFailure();
    expectDataFailure();
    QVERIFY(!QGCCompression::extractArchiveFiltered(archivePath, cutDir, acceptAll));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY(!QFileInfo::exists(cutDir + QLatin1Char('/') + cutEntry));
}

void QGCCompressionTest::_testTruncatedStream_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api :
         {"decompressFile", "decompressData", "decompressFromDevice-file", "decompressFromDevice-memory",
          "QGCDecompressDevice", "QGCDecompressDevice-readAll", "readFile", "computeFileHash"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testTruncatedStream()
{
    QFETCH(QString, api);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("cut.gz");
    const QString outputPath = tempDir.filePath("cut.bin");
    // Four members of 64 KiB each, cut inside the third: the damage is met after data has been decompressed
    QVERIFY(_writeGzipOfZeros(inputPath, 4));
    const QByteArray whole = _readFile(inputPath);
    const QByteArray cut = whole.left(whole.size() * 5 / 8);
    QVERIFY(_writeFile(inputPath, cut));
    QBuffer device;
    device.setData(cut);
    QVERIFY(device.open(QIODevice::ReadOnly));

    const QRegularExpression fileFailure("Decompression error.*truncated");
    const QRegularExpression memoryFailure("Read error.*truncated");
    if (api == "decompressFile") {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, fileFailure);
        QVERIFY(!QGCCompression::decompressFile(inputPath, outputPath));
        verifyExpectedLogMessage();
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::IoError);
    } else if (api == "decompressData") {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, memoryFailure);
        QVERIFY(QGCCompression::decompressData(cut).isEmpty());
        verifyExpectedLogMessage();
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::IoError);
    } else if (api == "decompressFromDevice-file") {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, fileFailure);
        QVERIFY(!QGCCompression::decompressFromDevice(&device, outputPath));
        verifyExpectedLogMessage();
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::IoError);
    } else if (api == "decompressFromDevice-memory") {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, memoryFailure);
        QVERIFY(QGCCompression::decompressFromDevice(&device).isEmpty());
        verifyExpectedLogMessage();
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::IoError);
    } else if (api == "readFile") {
        QString errorString;
        expectLogMessage("Utilities.QGCArchiveDeviceBase", QtWarningMsg, memoryFailure);
        QVERIFY(QGCCompression::readFile(inputPath, &errorString).isEmpty());
        verifyExpectedLogMessage();
        QVERIFY2(errorString.contains("truncated"), qPrintable(errorString));
    } else if (api == "computeFileHash") {
        expectLogMessage("Utilities.QGCArchiveDeviceBase", QtWarningMsg, memoryFailure);
        expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("read error"));
        QVERIFY(QGCCompression::computeFileHash(inputPath).isEmpty());
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    } else if (api == "QGCDecompressDevice-readAll") {
        QGCDecompressDevice decompressor(&device);
        QVERIFY(decompressor.open(QIODevice::ReadOnly));
        expectLogMessage("Utilities.QGCArchiveDeviceBase", QtWarningMsg, memoryFailure);
        const QByteArray readable = decompressor.readAll();
        verifyExpectedLogMessage();
        // readAll() cannot fail: it returns the members before the cut, and the device tells of the error
        QVERIFY(!readable.isEmpty());
        QVERIFY(readable.size() < 4 * 64 * 1024);
        // QIODevice::errorString() is not virtual: this is what a caller holding a QIODevice sees
        const QIODevice& asDevice = decompressor;
        QVERIFY2(asDevice.errorString().contains("truncated"), qPrintable(asDevice.errorString()));
        QVERIFY2(decompressor.errorString().contains("truncated"), qPrintable(decompressor.errorString()));
        char byte = 0;
        QCOMPARE(decompressor.read(&byte, 1), -1);
    } else {
        QGCDecompressDevice decompressor(&device);
        QVERIFY(decompressor.open(QIODevice::ReadOnly));
        QByteArray buffer(64 * 1024, Qt::Uninitialized);
        qint64 bytesRead = 0;
        expectLogMessage("Utilities.QGCArchiveDeviceBase", QtWarningMsg, memoryFailure);
        while ((bytesRead = decompressor.read(buffer.data(), buffer.size())) > 0) {
        }
        verifyExpectedLogMessage();
        // The stream does not just end
        QCOMPARE(bytesRead, -1);
        QVERIFY2(decompressor.errorString().contains("truncated"), qPrintable(decompressor.errorString()));
    }

    // Nothing of the damaged stream is kept
    QCOMPARE(_entriesBelow(tempDir.path()), QStringList{QStringLiteral("cut.gz")});
}

namespace {

/// Feeds bytes to a reader that cannot seek, like a network reply
class QGCCompressionTestSequentialDevice : public QIODevice
{
public:
    explicit QGCCompressionTestSequentialDevice(const QByteArray& data)
        : _data(data)
    {}

    bool isSequential() const override { return true; }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 count = qMin(maxSize, _data.size() - _offset);
        memcpy(data, _data.constData() + _offset, static_cast<size_t>(count));
        _offset += count;
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    const QByteArray _data;
    qint64 _offset = 0;
};

}  // namespace

void QGCCompressionTest::_testExtractStreamedZipEntry()
{
    // A reader that cannot seek to the central directory does not know the size of a streamed entry
    const QByteArray name("streamed.txt");
    const QByteArray content("content of an entry whose size is only known after its data\n");

    QGCCompressionTestSequentialDevice device(_makeStreamedZip({{.name = name, .data = content}}, true));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QTemporaryDir tempDir;
    QVERIFY(QGCCompression::extractFromDevice(&device, tempDir.path()));

    QFile extracted(tempDir.filePath(QString::fromLatin1(name)));
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    QCOMPARE(extracted.readAll(), content);
}

void QGCCompressionTest::_testExtractFileDataErrors_data()
{
    QTest::addColumn<bool>("fromDevice");

    QTest::newRow("file") << false;
    QTest::newRow("device") << true;
}

void QGCCompressionTest::_testExtractFileDataErrors()
{
    QFETCH(bool, fromDevice);
    using QGCCompression::Error;

    const QByteArray tar = _makeTar(
        {{.name = "empty.txt"}, {.name = "full.txt", .data = QByteArray(600, 'a')}, {.name = "last.txt", .data = "z"}});
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("entries.tar");
    QVERIFY(_writeFile(archivePath, tar));
    // Cut inside the data of full.txt, which follows the headers of empty.txt and of full.txt itself
    const QString damagedPath = tempDir.filePath("damaged.tar");
    QVERIFY(_writeFile(damagedPath, tar.left(2 * 512 + 100)));

    const auto extract = [fromDevice](const QString& path, const char* entryName, qint64 limit = 0) {
        if (!fromDevice) {
            return QGCCompression::extractFileData(path, entryName, QGCCompression::Format::Auto, limit);
        }
        QFile device(path);
        if (!device.open(QIODevice::ReadOnly)) {
            return QByteArray("could not open the archive");
        }
        return QGCCompression::extractFileDataFromDevice(&device, entryName, limit);
    };

    // An entry without data gives nothing either, so the error is what tells it from each kind of failure
    QVERIFY(extract(archivePath, "empty.txt").isEmpty());
    QCOMPARE(QGCCompression::lastError(), Error::None);

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    QVERIFY(extract(archivePath, "absent.txt").isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), Error::FileNotInArchive);

    // The search for the entry runs into the damage: the archive is not said to be without it
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read archive entry"));
    QVERIFY(extract(damagedPath, "absent.txt").isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), Error::InvalidArchive);

    // The entry is found, and its data is cut
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Read error"));
    QVERIFY(extract(damagedPath, "full.txt").isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), Error::InvalidArchive);

    // An entry before the damage is read as from a whole archive
    QVERIFY(extract(damagedPath, "empty.txt").isEmpty());
    QCOMPARE(QGCCompression::lastError(), Error::None);

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(extract(archivePath, "full.txt", 100).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), Error::SizeLimitExceeded);

    // The error of a failure is gone after the next extraction
    QCOMPARE(extract(archivePath, "last.txt"), QByteArray("z"));
    QCOMPARE(QGCCompression::lastError(), Error::None);
}

void QGCCompressionTest::_testDecompressDataErrors()
{
    using QGCCompression::Error;

    const QByteArray compressed = _readFile(QStringLiteral(":/unittest/manifest.json.gz"));
    const auto leaveAnErrorBehind = [this]() {
        expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
        const bool exists = QGCCompression::fileExists(QStringLiteral("/nonexistent/archive.zip"), QStringLiteral("a"));
        verifyExpectedLogMessage();
        return !exists && (QGCCompression::lastError() == Error::FileNotFound);
    };

    // Decompressed data is never empty, but lastError() is only of use if each call sets it
    QVERIFY(leaveAnErrorBehind());
    QVERIFY(!QGCCompression::decompressData(compressed).isEmpty());
    QCOMPARE(QGCCompression::lastError(), Error::None);

    QVERIFY(leaveAnErrorBehind());
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Cannot decompress empty data"));
    QVERIFY(QGCCompression::decompressData(QByteArray()).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), Error::UnsupportedFormat);

    QVERIFY(leaveAnErrorBehind());
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Could not detect format"));
    QVERIFY(QGCCompression::decompressData(QByteArray("not a compressed stream")).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), Error::UnsupportedFormat);
}

// ============================================================================
// QIODevice-based Operation Tests
// ============================================================================
void QGCCompressionTest::_testDecompressFromDevice()
{
    QTemporaryDir tempDir;
    // Load compressed data from Qt resource into a QBuffer
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    QVERIFY(!compressedData.isEmpty());
    // Test decompression to file using QBuffer as device
    QBuffer buffer;
    buffer.setData(compressedData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QString outputPath = tempDir.path() + "/device_decompressed.json";
    QVERIFY2(QGCCompression::decompressFromDevice(&buffer, outputPath), "Failed to decompress from QBuffer device");
    QVERIFY(QFile::exists(outputPath));
    QVERIFY(QFileInfo(outputPath).size() > 0);
    // Verify content
    QFile decompressed(outputPath);
    QVERIFY(decompressed.open(QIODevice::ReadOnly));
    const QByteArray content = decompressed.readAll();
    QVERIFY(content.contains("\"name\""));
    decompressed.close();
    buffer.close();
    // Test decompression to memory
    buffer.setData(compressedData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QByteArray memoryResult = QGCCompression::decompressFromDevice(&buffer);
    QVERIFY2(!memoryResult.isEmpty(), "Decompress to memory from device failed");
    QVERIFY(memoryResult.contains("\"name\""));
    QCOMPARE(memoryResult, content);
    buffer.close();
    // Test with null device
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Device is null"));
    QVERIFY(!QGCCompression::decompressFromDevice(nullptr, outputPath));
    verifyExpectedLogMessage();
    // Test with closed device
    QBuffer closedBuffer;
    closedBuffer.setData(compressedData);
    // Don't open it
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg,
                     QRegularExpression("Device is null, not open, or not readable"));
    QVERIFY(!QGCCompression::decompressFromDevice(&closedBuffer, outputPath));
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testExtractFromDevice()
{
    QTemporaryDir tempDir;
    // Load ZIP from Qt resource into a QBuffer
    QFile resourceFile(":/unittest/manifest.json.zip");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray zipData = resourceFile.readAll();
    resourceFile.close();
    QVERIFY(!zipData.isEmpty());
    // Extract using QBuffer as device
    QBuffer buffer;
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QString outputDir = tempDir.path() + "/device_extract";
    QVERIFY2(QGCCompression::extractFromDevice(&buffer, outputDir), "Failed to extract archive from QBuffer device");
    QVERIFY(QDir(outputDir).exists());
    // Verify extracted content
    QDir extractedDir(outputDir);
    QStringList files = extractedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from device");
    buffer.close();
    // Test with null device
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Device is null"));
    QVERIFY(!QGCCompression::extractFromDevice(nullptr, outputDir));
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testExtractFileDataFromDevice()
{
    // Load ZIP from Qt resource into a QBuffer
    QFile resourceFile(":/unittest/manifest.json.zip");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray zipData = resourceFile.readAll();
    resourceFile.close();
    QVERIFY(!zipData.isEmpty());
    // Extract single file from device to memory
    QBuffer buffer;
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const QByteArray extracted = QGCCompression::extractFileDataFromDevice(&buffer, "manifest.json");
    QVERIFY2(!extracted.isEmpty(), "Failed to extract file data from device");
    QVERIFY(extracted.contains("\"name\""));
    buffer.close();
    // Test non-existent file
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    const QByteArray notFound = QGCCompression::extractFileDataFromDevice(&buffer, "nonexistent.txt");
    QVERIFY(notFound.isEmpty());
    verifyExpectedLogMessage();
    buffer.close();
    // Test empty file name
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
    const QByteArray emptyName = QGCCompression::extractFileDataFromDevice(&buffer, "");
    QVERIFY(emptyName.isEmpty());
    verifyExpectedLogMessage();
    buffer.close();
    // Test null device
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Device is null"));
    const QByteArray nullDevice = QGCCompression::extractFileDataFromDevice(nullptr, "manifest.json");
    QVERIFY(nullDevice.isEmpty());
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testInterleavedDeviceReaders()
{
    // Each stream expands to several of the blocks it is decompressed in, so it is partly read
    // when the other one is read from
    const auto payload = [](char first) {
        QByteArray data(400000, Qt::Uninitialized);
        for (qsizetype i = 0; i < data.size(); ++i) {
            data[i] = static_cast<char>(first + ((i * 7) + (i / 13)) % 23);
        }
        return data;
    };
    const QByteArray firstPayload = payload('a');
    const QByteArray secondPayload = payload('A');
    QBuffer firstSource;
    firstSource.setData(_gzipMember(firstPayload));
    QVERIFY(firstSource.open(QIODevice::ReadOnly));
    QBuffer secondSource;
    secondSource.setData(_gzipMember(secondPayload));
    QVERIFY(secondSource.open(QIODevice::ReadOnly));

    QGCDecompressDevice first(&firstSource);
    QGCDecompressDevice second(&secondSource);
    QVERIFY(first.open(QIODevice::ReadOnly));
    QVERIFY(second.open(QIODevice::ReadOnly));

    QByteArray firstData;
    QByteArray secondData;
    QByteArray chunk(4096, Qt::Uninitialized);
    qint64 firstRead = 0;
    qint64 secondRead = 0;
    do {
        firstRead = first.read(chunk.data(), chunk.size());
        firstData.append(chunk.constData(), qMax<qint64>(firstRead, 0));
        secondRead = second.read(chunk.data(), chunk.size());
        secondData.append(chunk.constData(), qMax<qint64>(secondRead, 0));
    } while ((firstRead > 0) || (secondRead > 0));

    QCOMPARE(firstRead, 0);
    QCOMPARE(secondRead, 0);
    QCOMPARE(firstData.size(), firstPayload.size());
    QVERIFY(firstData == firstPayload);
    QCOMPARE(secondData.size(), secondPayload.size());
    QVERIFY(secondData == secondPayload);
}

void QGCCompressionTest::_testDeviceAtEnd_data()
{
    QTest::addColumn<bool>("archive");

    QTest::newRow("QGCDecompressDevice") << false;
    QTest::newRow("QGCArchiveFile") << true;
}

void QGCCompressionTest::_testDeviceAtEnd()
{
    QFETCH(bool, archive);

    // Several of the blocks the devices decompress in, and not a whole number of them
    QByteArray payload(300000, Qt::Uninitialized);
    for (qsizetype i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<char>('a' + ((i * 7) + (i / 13)) % 23);
    }
    const QByteArray input = archive ? _makeTar({{.name = "first.txt", .data = "first"},
                                                 {.name = "payload.bin", .data = payload},
                                                 {.name = "empty.txt"}})
                                     : _gzipMember(payload);
    const auto makeDevice = [archive](QIODevice* source,
                                      const char* entryName) -> std::unique_ptr<QGCArchiveDeviceBase> {
        if (archive) {
            return std::make_unique<QGCArchiveFile>(source, QString::fromLatin1(entryName));
        }
        return std::make_unique<QGCDecompressDevice>(source);
    };

    // A read of the devices' block of 128 KiB leaves nothing buffered while there is more to decompress, as
    // there is nothing buffered before the first read
    for (const qint64 chunkSize : {qint64(1000), qint64(128 * 1024)}) {
        QBuffer source;
        source.setData(input);
        QVERIFY(source.open(QIODevice::ReadOnly));
        const std::unique_ptr<QGCArchiveDeviceBase> device = makeDevice(&source, "payload.bin");
        QVERIFY(device->atEnd());
        QVERIFY(device->open(QIODevice::ReadOnly));

        QByteArray data;
        while (!device->atEnd()) {
            const QByteArray chunk = device->read(chunkSize);
            QVERIFY2(!chunk.isEmpty(), "atEnd() was false with nothing left to read");
            data.append(chunk);
        }
        QCOMPARE(data.size(), payload.size());
        QVERIFY(data == payload);
        QVERIFY2(device->errorString().isEmpty(), qPrintable(device->errorString()));
    }

    // A stream that is cut ends the loop as well, and the device tells why
    QBuffer cutSource;
    cutSource.setData(input.left(input.size() / 2));
    QVERIFY(cutSource.open(QIODevice::ReadOnly));
    const std::unique_ptr<QGCArchiveDeviceBase> cutDevice = makeDevice(&cutSource, "payload.bin");
    QVERIFY(cutDevice->open(QIODevice::ReadOnly));
    qint64 readable = 0;
    expectLogMessage("Utilities.QGCArchiveDeviceBase", QtWarningMsg, QRegularExpression("Read error"));
    while (!cutDevice->atEnd()) {
        const QByteArray chunk = cutDevice->read(1000);
        QVERIFY2(!chunk.isEmpty(), "atEnd() was false with nothing left to read");
        readable += chunk.size();
    }
    verifyExpectedLogMessage();
    QVERIFY(readable > 0);
    QVERIFY(readable < payload.size());
    QVERIFY(!cutDevice->errorString().isEmpty());

    if (archive) {
        // An entry without data is at its end before anything is read
        QBuffer source;
        source.setData(input);
        QVERIFY(source.open(QIODevice::ReadOnly));
        const std::unique_ptr<QGCArchiveDeviceBase> emptyDevice = makeDevice(&source, "empty.txt");
        QVERIFY(emptyDevice->open(QIODevice::ReadOnly));
        QVERIFY(emptyDevice->atEnd());
        QVERIFY(emptyDevice->readAll().isEmpty());
        QVERIFY2(emptyDevice->errorString().isEmpty(), qPrintable(emptyDevice->errorString()));
    }
}

// ============================================================================
// QGCCompressionJob Tests (Async Operations)
// ============================================================================
void QGCCompressionTest::_testCompressionJobExtract()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputDir = tempDir.path() + "/job_extract";
    QGCCompressionJob job;
    // Set up signal spies
    QSignalSpy progressSpy(&job, &QGCCompressionJob::progressChanged);
    QSignalSpy runningSpy(&job, &QGCCompressionJob::runningChanged);
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    QVERIFY(progressSpy.isValid());
    QVERIFY(runningSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    // Initial state
    QVERIFY(!job.isRunning());
    QCOMPARE(job.progress(), 0.0);
    QVERIFY(job.errorString().isEmpty());
    // Start extraction
    job.extractArchive(zipResource, outputDir);
    // Should be running immediately
    QVERIFY(job.isRunning());
    // Wait for completion (with timeout)
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    // Verify completion
    QVERIFY(!job.isRunning());
    QCOMPARE(finishedSpy.count(), 1);
    // Check result
    QList<QVariant> finishedArgs = finishedSpy.takeFirst();
    bool success = finishedArgs.at(0).toBool();
    QVERIFY2(success, qPrintable(QString("Extraction failed: %1").arg(job.errorString())));
    // Verify extracted files
    QVERIFY(QDir(outputDir).exists());
    QVERIFY(QFile::exists(outputDir + "/manifest.json"));
    // Progress should have changed (at least start and end)
    QVERIFY_SIGNAL_COUNT_WAIT(runningSpy, 2, TestTimeout::mediumMs());  // Started and stopped
    // Final progress should be 1.0 on success
    QCOMPARE(job.progress(), 1.0);
}

void QGCCompressionTest::_testCompressionJobReentrantStartFromFinished()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString firstOutputDir = tempDir.path() + "/job_reentrant_extract";
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString secondOutputFile = tempDir.path() + "/job_reentrant_decompress.json";

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    QVERIFY(finishedSpy.isValid());

    bool secondStartAttempted = false;
    bool secondStartAccepted = false;

    connect(&job, &QGCCompressionJob::finished, this, [&](bool success) {
        if (!secondStartAttempted && success) {
            secondStartAttempted = true;
            job.decompressFile(gzResource, secondOutputFile);
            secondStartAccepted = job.isRunning();
        }
    });

    job.extractArchive(zipResource, firstOutputDir);
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QCOMPARE(finishedSpy.count(), 2);
    QVERIFY(secondStartAttempted);
    QVERIFY(secondStartAccepted);
    QVERIFY(finishedSpy.at(0).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QVERIFY(QFile::exists(firstOutputDir + "/manifest.json"));
    QVERIFY(QFile::exists(secondOutputFile));
    QVERIFY(!job.isRunning());
}

void QGCCompressionTest::_testCompressionJobAsyncStatic()
{
    QTemporaryDir tempDir;
    // Test the static async method that returns QFuture directly
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputDir = tempDir.path() + "/async_static";
    // Use the static method
    QFuture<bool> future = QGCCompressionJob::extractArchiveAsync(zipResource, outputDir);
    // Create a watcher to monitor progress
    QFutureWatcher<bool> watcher;
    QSignalSpy progressSpy(&watcher, &QFutureWatcher<bool>::progressValueChanged);
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<bool>::finished);
    watcher.setFuture(future);
    // Wait for completion
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    // Verify result
    QVERIFY(future.isFinished());
    QVERIFY(!future.isCanceled());
    QVERIFY2(future.result(), "Static async extraction failed");
    QVERIFY(!progressSpy.isEmpty());
    QCOMPARE(progressSpy.last().first().toInt(), 100);
    // Verify extracted file exists
    QVERIFY(QFileInfo::exists(outputDir + "/manifest.json"));
    // Test decompressFileAsync as well
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString decompressOutput = tempDir.path() + "/async_decompress.json";
    QFuture<bool> decompressFuture = QGCCompressionJob::decompressFileAsync(gzResource, decompressOutput);
    QFutureWatcher<bool> decompressWatcher;
    QSignalSpy decompressFinishedSpy(&decompressWatcher, &QFutureWatcher<bool>::finished);
    decompressWatcher.setFuture(decompressFuture);
    QVERIFY_SIGNAL_WAIT(decompressFinishedSpy, TestTimeout::mediumMs());
    QVERIFY(decompressFuture.isFinished());
    QVERIFY2(decompressFuture.result(), "Static async decompression failed");
    QVERIFY(QFileInfo::exists(decompressOutput));
}

void QGCCompressionTest::_testCompressionJobCancel_data()
{
    QTest::addColumn<QString>("operation");

    QTest::newRow("decompressFile") << "decompressFile";
    QTest::newRow("extractArchive") << "extractArchive";
}

void QGCCompressionTest::_testCompressionJobCancel()
{
    QFETCH(QString, operation);
    const bool extract = (operation == "extractArchive");

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(extract ? "zeros.tar.gz" : "zeros.gz");
    const QString outputPath = tempDir.filePath("output");
    // Expands to 512 MiB so the worker is still busy when its first progress report is handled here
    constexpr int memberCount = 8192;
    if (extract) {
        // One entry of that size: its header, its data and a last member with the zero blocks which end a tar archive
        const QByteArray header = _tarHeader({.name = "zeros.bin"}, static_cast<qint64>(memberCount) * 64 * 1024);
        QVERIFY(_writeGzipOfZeros(inputPath, memberCount + 1, _gzipMember(header)));
    } else {
        QVERIFY(_writeGzipOfZeros(inputPath, memberCount));
    }

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    qreal firstProgress = 0.0;
    connect(&job, &QGCCompressionJob::progressChanged, this, [&job, &firstProgress](qreal progress) {
        if (qFuzzyIsNull(firstProgress)) {
            firstProgress = progress;
            job.cancel();
        }
    });

    if (extract) {
        job.extractArchive(inputPath, outputPath);
    } else {
        job.decompressFile(inputPath, outputPath);
    }
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::longMs());
    QVERIFY(firstProgress > 0.0);
    QVERIFY(firstProgress < 1.0);
    // The report arrived while the worker was running, so the cancellation stopped it
    QVERIFY(!finishedSpy.first().first().toBool());
    QCOMPARE(job.errorString(), QStringLiteral("Operation cancelled"));
    QVERIFY(!job.isRunning());
    // Nothing of the interrupted output is kept: no file, and an output directory without entries
    QCOMPARE(_entriesBelow(outputPath), QStringList());
    QVERIFY(!QFileInfo(outputPath).isFile());

    // Cancelling a job which is not running does nothing
    job.cancel();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(job.errorString(), QStringLiteral("Operation cancelled"));
}

void QGCCompressionTest::_testCompressionJobCancelFuture()
{
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("cancel.gz");
    const QString outputPath = tempDir.filePath("cancel.bin");
    QVERIFY(_writeGzipOfZeros(inputPath, 8192));

    QFuture<bool> future = QGCCompressionJob::decompressFileAsync(inputPath, outputPath);
    QFutureWatcher<bool> watcher;
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<bool>::finished);
    connect(&watcher, &QFutureWatcher<bool>::progressValueChanged, &watcher, [&future]() { future.cancel(); });
    watcher.setFuture(future);

    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::longMs());
    QVERIFY(future.isCanceled());
    QVERIFY(!QFile::exists(outputPath));
}

void QGCCompressionTest::_testCompressionJobErrorString()
{
    QTemporaryDir tempDir;
    const QString corruptZip = tempDir.filePath("corrupt.zip");
    QFile file(corruptZip);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("This is not a valid ZIP file");
    file.close();

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    // The archive is opened twice: for the size pre-check and for the extraction
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Unrecognized archive format"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Unrecognized archive format"));
    job.extractArchive(corruptZip, tempDir.filePath("corrupt_output"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY(!finishedSpy.first().first().toBool());
    QCOMPARE(job.errorString(), QStringLiteral("Failed to extract archive: ") + corruptZip);
}

// ============================================================================
// Sparse File Handling Tests
// ============================================================================
void QGCCompressionTest::_testSparseFileExtraction_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"extractArchive", "extractFile", "extractFileData", "QGCArchiveFile"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testSparseFileExtraction()
{
    QFETCH(QString, api);

    // The archive stores the two parts of the file which are not holes, one after the other
    constexpr qsizetype blockSize = 512;
    const QString entryName = QStringLiteral("sparse.bin");
    const QByteArray firstPart(blockSize, 'A');
    const QByteArray secondPart(blockSize, 'B');
    QByteArray expected(9 * blockSize, '\0');
    expected.replace(2 * blockSize, blockSize, firstPart);
    expected.replace(8 * blockSize, blockSize, secondPart);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("sparse.tar");
    const QString outputPath = tempDir.filePath("output");
    QVERIFY(_writeFile(archivePath, _makeTar({{.name = entryName.toLatin1(),
                                               .data = firstPart + secondPart,
                                               .type = 'S',
                                               .sparseData = {{.offset = 2 * blockSize, .size = blockSize},
                                                              {.offset = 8 * blockSize, .size = blockSize}},
                                               .sparseFileSize = expected.size()}})));

    QByteArray extracted;
    if (api == "extractArchive") {
        // The limit is that of the data: holes are not written, so they do not count
        QVERIFY(QGCCompression::extractArchive(archivePath, outputPath, QGCCompression::Format::Auto, nullptr,
                                               2 * blockSize));
        extracted = _readFile(outputPath + QLatin1Char('/') + entryName);
    } else if (api == "extractFile") {
        QVERIFY(QGCCompression::extractFile(archivePath, entryName, outputPath));
        extracted = _readFile(outputPath);
    } else if (api == "extractFileData") {
        extracted = QGCCompression::extractFileData(archivePath, entryName);
    } else {
        QGCArchiveFile entry(archivePath, entryName);
        QVERIFY(entry.open(QIODevice::ReadOnly));
        extracted = entry.readAll();
    }

    // Each part is where it belongs, with zeros in the holes
    QCOMPARE(extracted.size(), expected.size());
    QVERIFY(extracted == expected);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================
void QGCCompressionTest::_testThreadLocalState()
{
    QTemporaryDir tempDir;
    // Test that thread-local error state is isolated between threads
    // Each thread should have its own lastError() and lastErrorString()
    QMutex syncMutex;
    QWaitCondition thread1Ready;
    bool thread1HasErrorState = false;
    std::atomic<bool> thread1Done{false};
    std::atomic<bool> thread2Done{false};
    std::atomic<bool> thread2ObservedThread1{false};
    QGCCompression::Error thread1Error = QGCCompression::Error::None;
    QGCCompression::Error thread2Error = QGCCompression::Error::None;
    QString thread1ErrorString;
    QString thread2ErrorString;
    // Thread 1: Trigger FileNotFound error
    QThread* t1 = QThread::create([&]() {
        // This should fail and set error state
        QGCCompression::extractArchive("/nonexistent/path/file.zip", tempDir.path() + "/thread1_output");
        thread1Error = QGCCompression::lastError();
        thread1ErrorString = QGCCompression::lastErrorString();
        {
            QMutexLocker locker(&syncMutex);
            thread1HasErrorState = true;
            thread1Ready.wakeOne();
        }
        thread1Done = true;
    });
    // Thread 2: Successful operation (should have no error)
    QThread* t2 = QThread::create([&tempDir, &syncMutex, &thread1Ready, &thread1HasErrorState, &thread2ObservedThread1,
                                   &thread2Error, &thread2ErrorString, &thread2Done]() {
        {
            QMutexLocker locker(&syncMutex);
            if (!thread1HasErrorState) {
                thread2ObservedThread1 = thread1Ready.wait(&syncMutex, TestTimeout::mediumMs());
            } else {
                thread2ObservedThread1 = true;
            }
        }
        if (!thread2ObservedThread1.load()) {
            thread2Done = true;
            return;
        }

        // This should succeed
        const QString outputDir = tempDir.path() + "/thread2_output";
        QGCCompression::extractArchive(":/unittest/manifest.json.zip", outputDir);
        thread2Error = QGCCompression::lastError();
        thread2ErrorString = QGCCompression::lastErrorString();
        thread2Done = true;
    });
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist.*"));
    t1->start();
    t2->start();
    // Wait for both threads
    QVERIFY(t1->wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    QVERIFY(t2->wait(TestTimeout::mediumMs()));
    delete t1;
    delete t2;
    // Verify thread isolation
    QVERIFY(thread1Done.load());
    QVERIFY(thread2Done.load());
    QVERIFY(thread2ObservedThread1.load());
    QCOMPARE(thread1Error, QGCCompression::Error::FileNotFound);
    QVERIFY(!thread1ErrorString.isEmpty());
    // Thread 2 should have its own clean state (success = no error)
    QCOMPARE(thread2Error, QGCCompression::Error::None);
}

void QGCCompressionTest::_testConcurrentExtractions()
{
    QTemporaryDir tempDir;
    // Test multiple concurrent extractions don't interfere with each other
    constexpr int kNumThreads = 4;
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
    QList<QFuture<void>> futures;
    for (int i = 0; i < kNumThreads; ++i) {
        futures.append(QtConcurrent::run([i, &tempDir, &successCount, &failureCount]() {
            const QString outputDir = tempDir.path() + QString("/concurrent_%1").arg(i);
            // Alternate between different archive types
            QString archivePath;
            if (i % 2 == 0) {
                archivePath = QStringLiteral(":/unittest/manifest.json.zip");
            } else {
                archivePath = QStringLiteral(":/unittest/manifest.json.7z");
            }
            if (QGCCompression::extractArchive(archivePath, outputDir)) {
                if (QFile::exists(outputDir + "/manifest.json")) {
                    successCount++;
                } else {
                    failureCount++;
                }
            } else {
                failureCount++;
            }
        }));
    }
    // Wait for all futures
    for (auto& future : futures) {
        future.waitForFinished();
    }
    QCOMPARE(successCount.load(), kNumThreads);
    QCOMPARE(failureCount.load(), 0);
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================
void QGCCompressionTest::_testEmptyArchive()
{
    QTemporaryDir tempDir;
    // Create an empty ZIP file (valid but contains no entries)
    // ZIP empty archive signature: PK\x05\x06 followed by 18 zero bytes
    const QString emptyZip = tempDir.path() + "/empty.zip";
    QFile file(emptyZip);
    QVERIFY(file.open(QIODevice::WriteOnly));
    // Minimal empty ZIP (End of Central Directory record only)
    const char emptyZipData[] = {
        '\x50', '\x4b', '\x05', '\x06',  // End of central dir signature
        '\x00', '\x00',                  // Number of this disk
        '\x00', '\x00',                  // Disk where central directory starts
        '\x00', '\x00',                  // Number of central directory records on this disk
        '\x00', '\x00',                  // Total number of central directory records
        '\x00', '\x00', '\x00', '\x00',  // Size of central directory
        '\x00', '\x00', '\x00', '\x00',  // Offset of central directory
        '\x00', '\x00'                   // Comment length
    };
    file.write(emptyZipData, sizeof(emptyZipData));
    file.close();
    // List should return empty list (not error)
    const QStringList entries = QGCCompression::listArchive(emptyZip);
    QVERIFY(entries.isEmpty());
    // Stats should show zero entries
    const QGCCompression::ArchiveStats stats = QGCCompression::getArchiveStats(emptyZip);
    QCOMPARE(stats.totalEntries, 0);
    QCOMPARE(stats.fileCount, 0);
    // Extract should succeed (nothing to extract)
    const QString outputDir = tempDir.path() + "/empty_extract";
    QVERIFY(QGCCompression::extractArchive(emptyZip, outputDir));
}

void QGCCompressionTest::_testSingleByteFile()
{
    // Test that small files decompress correctly
    // Use the existing .gz resource and verify size is small but non-zero
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    // Decompress to memory
    QFile resourceFile(gzResource);
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    const QByteArray decompressed = QGCCompression::decompressData(compressedData);
    QVERIFY(!decompressed.isEmpty());
    // Verify decompressed size is reasonable (small file)
    QVERIFY(decompressed.size() > 0);
    QVERIFY(decompressed.size() < 1024);  // Should be a small manifest file
    // Test boundary: decompress with exact size limit matching actual size
    const QByteArray exactLimit =
        QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, decompressed.size());
    QCOMPARE(exactLimit.size(), decompressed.size());
    // Test boundary: size limit of exactly 1 byte should fail for this file
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    const QByteArray tooSmall = QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, 1);
    QVERIFY(tooSmall.isEmpty());  // Should fail - file is bigger than 1 byte
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testLargeFileDecompression()
{
    // Test decompression with size limit enforcement
    // Use the manifest.json.gz which we know is small
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    // First, decompress without limit to get actual size
    const QByteArray fullDecompressed = QGCCompression::decompressData(compressedData);
    QVERIFY(!fullDecompressed.isEmpty());
    const qint64 fullSize = fullDecompressed.size();
    // Now test with a limit smaller than the actual size
    if (fullSize > 10) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
        const QByteArray limitedDecompressed =
            QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, 10);
        // Should return empty due to size limit exceeded
        QVERIFY(limitedDecompressed.isEmpty());
        verifyExpectedLogMessage();
    }
    // Test with limit larger than actual size (should succeed)
    const QByteArray unlimitedDecompressed =
        QGCCompression::decompressData(compressedData, QGCCompression::Format::Auto, fullSize * 2);
    QCOMPARE(unlimitedDecompressed.size(), fullSize);
}

void QGCCompressionTest::_testDecompressToFileSizeLimit()
{
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("zeros.gz");
    const QString outputPath = tempDir.filePath("zeros.bin");
    QVERIFY(_writeGzipOfZeros(inputPath, 1));
    constexpr qint64 limit = 1024;
    const QStringList inputOnly{QStringLiteral("zeros.gz")};

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(!QGCCompression::decompressFile(inputPath, outputPath, QGCCompression::Format::Auto, nullptr, limit));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Decompression failed"));
    QVERIFY(QGCCompression::decompressIfNeeded(inputPath, outputPath, false, limit).isEmpty());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);

    QFile device(inputPath);
    QVERIFY(device.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(!QGCCompression::decompressFromDevice(&device, outputPath, nullptr, limit));
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);
    device.close();

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    job.decompressFile(inputPath, outputPath, limit);
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!finishedSpy.first().first().toBool());
    QVERIFY2(job.errorString().contains("size limit exceeded"), qPrintable(job.errorString()));
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);
}

void QGCCompressionTest::_testDecompressToMemorySizeLimit()
{
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressed = resourceFile.readAll();
    constexpr qint64 limit = 10;

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(QGCCompression::decompressData(compressed, QGCCompression::Format::Auto, limit).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);

    QBuffer device;
    device.setData(compressed);
    QVERIFY(device.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(QGCCompression::decompressFromDevice(&device, limit).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);

    QJsonParseError parseError;
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(QGCCompression::parseCompressedJson(compressed, &parseError, limit).isNull());
    verifyExpectedLogMessage();
    QVERIFY(parseError.error != QJsonParseError::NoError);
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
    QVERIFY(QGCCompression::parseCompressedJson(compressed, &parseError).isObject());

    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    QString errorString;
    QVERIFY(QGCCompression::readFile(gzResource, &errorString, 0, limit).isEmpty());
    QVERIFY2(errorString.contains("exceeds the limit"), qPrintable(errorString));
    // A read that stops within the limit is not an error
    errorString.clear();
    QCOMPARE(QGCCompression::readFile(gzResource, &errorString, limit, limit).size(), limit);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
}

void QGCCompressionTest::_testExtractionStopsCleanly_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<bool>("declaredSizes");  // Whether the archive tells the size of an entry before its data
    QTest::addColumn<bool>("cancel");         // Otherwise the size limit stops the extraction
    QTest::addColumn<QStringList>("remaining");

    const QStringList nothing;
    QTest::newRow("extractArchive-limit") << "extractArchive" << true << false << nothing;
    QTest::newRow("extractArchive-cancel") << "extractArchive" << true << true << nothing;
    QTest::newRow("extractFromDevice-limit") << "extractFromDevice" << true << false << nothing;
    QTest::newRow("extractFromDevice-cancel") << "extractFromDevice" << true << true << nothing;
    // Filtered extraction keeps the entries it wrote before it was stopped
    QTest::newRow("extractArchiveFiltered-limit")
        << "extractArchiveFiltered" << true << false << QStringList{QStringLiteral("first.txt")};
    // The entry that is being written when the callback cancels is not kept, its directory is
    QTest::newRow("extractArchiveFiltered-cancel")
        << "extractArchiveFiltered" << true << true
        << QStringList{QStringLiteral("first.txt"), QStringLiteral("nested")};
    // The limit is on what is written, whatever the archive says or leaves out about the sizes
    QTest::newRow("extractArchive-limit-undeclared") << "extractArchive" << false << false << nothing;
    QTest::newRow("extractArchiveFiltered-limit-undeclared") << "extractArchiveFiltered" << false << false << nothing;
}

void QGCCompressionTest::_testExtractionStopsCleanly()
{
    QFETCH(QString, api);
    QFETCH(bool, declaredSizes);
    QFETCH(bool, cancel);
    QFETCH(QStringList, remaining);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(declaredSizes ? "three.tar" : "streamed.zip");
    const QString outputDir = tempDir.filePath("output");
    if (declaredSizes) {
        QVERIFY(_writeFile(archivePath, _makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                                  {.name = "nested/second.txt", .data = QByteArray(600, 'b')},
                                                  {.name = "third.txt", .data = QByteArray(600, 'c')}})));
    } else {
        // Already the first entry is larger than the limit
        QVERIFY(_writeFile(archivePath,
                           _makeStreamedZip({{.name = "first.txt", .data = QByteArray(64 * 1024, 'a')},
                                             {.name = "nested/second.txt", .data = QByteArray(64 * 1024, 'b')}},
                                            false)));
    }
    QFile device(archivePath);
    QVERIFY(device.open(QIODevice::ReadOnly));

    // In the TAR archive the second entry is the one that does not fit, and the one during which the
    // extraction is cancelled
    const qint64 limit = cancel ? 0 : 1000;
    int progressCalls = 0;
    QGCCompression::ProgressCallback progress;
    if (cancel) {
        progress = [&progressCalls](qint64, qint64) { return ++progressCalls < 2; };
    } else {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    }

    if (api == "extractArchive") {
        QVERIFY(!QGCCompression::extractArchive(archivePath, outputDir, QGCCompression::Format::Auto, progress, limit));
    } else if (api == "extractFromDevice") {
        QVERIFY(!QGCCompression::extractFromDevice(&device, outputDir, progress, limit));
    } else {
        QVERIFY(!QGCCompression::extractArchiveFiltered(
            archivePath, outputDir, [](const QGCCompression::ArchiveEntry&) { return true; }, progress, limit));
    }

    if (cancel) {
        QCOMPARE(progressCalls, 2);
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::IoError);
    } else {
        verifyExpectedLogMessage();
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
    }
    QCOMPARE(_entriesBelow(outputDir), remaining);
}

void QGCCompressionTest::_testExtractArchiveKeepsEntriesBeforeDamage_data()
{
    QTest::addColumn<bool>("fromDevice");

    QTest::newRow("extractArchive") << false;
    QTest::newRow("extractFromDevice") << true;
}

void QGCCompressionTest::_testExtractArchiveKeepsEntriesBeforeDamage()
{
    QFETCH(bool, fromDevice);

    const QByteArray tar = _makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                     {.name = "nested/second.txt", .data = QByteArray(600, 'b')},
                                     {.name = "third.txt", .data = QByteArray(600, 'c')}});
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("cut.tar");
    const QString outputDir = tempDir.filePath("output");
    // first.txt takes three 512-byte blocks: the cut is 100 bytes into the data of the second entry
    QVERIFY(_writeFile(archivePath, tar.left(4 * 512 + 100)));
    QFile device(archivePath);
    QVERIFY(device.open(QIODevice::ReadOnly));

    if (!fromDevice) {
        // The size pre-check reads the headers and finds the archive cut where the third one should be
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read archive entry"));
    }
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read data"));
    if (fromDevice) {
        QVERIFY(!QGCCompression::extractFromDevice(&device, outputDir));
    } else {
        QVERIFY(!QGCCompression::extractArchive(archivePath, outputDir));
        verifyExpectedLogMessage();
    }
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::IoError);

    // Unlike a size limit or a cancellation, damage does not undo the extraction. The entries before it stay,
    // and so does the damaged one: what could be read of it, filled up with zeros to the size it declares
    const QStringList written{QStringLiteral("first.txt"), QStringLiteral("nested"),
                              QStringLiteral("nested/second.txt")};
    QCOMPARE(_entriesBelow(outputDir), written);
    QCOMPARE(_readFile(outputDir + "/first.txt"), QByteArray(600, 'a'));
    QCOMPARE(_readFile(outputDir + "/nested/second.txt"), QByteArray(100, 'b') + QByteArray(500, '\0'));
}

void QGCCompressionTest::_testMaxEntriesArchive()
{
    // Test listing an archive with many entries
    // We'll use the existing ZIP and verify the listing mechanism
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Get detailed listing
    const QList<QGCCompression::ArchiveEntry> entries = QGCCompression::listArchiveDetailed(zipResource);
    // Verify each entry has valid data
    for (const auto& entry : entries) {
        QVERIFY(!entry.name.isEmpty());
        if (!entry.isDirectory) {
            QVERIFY(entry.size >= 0);
        }
        // Permissions should be reasonable (not zero for files)
        if (!entry.isDirectory && entry.size > 0) {
            QVERIFY(entry.permissions > 0);
        }
    }
    // Verify stats match entry count
    const QGCCompression::ArchiveStats stats = QGCCompression::getArchiveStats(zipResource);
    QCOMPARE(stats.totalEntries, static_cast<int>(entries.size()));
}

void QGCCompressionTest::_testOverstatedEntrySize()
{
    // The header of the entry tells of 2^50 bytes, and 512 follow it
    const QString entryName = QStringLiteral("overstated.bin");
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("overstated.tar");
    QVERIFY(
        _writeFile(archivePath, _tarHeader({.name = entryName.toLatin1()}, Q_INT64_C(1) << 50) + QByteArray(512, 'x')));
    QFile device(archivePath);
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Without a limit, no memory is set aside on the word of the header: the entry fails where its data runs out
    const QRegularExpression dataRanOut("Read error.*Truncated");
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, dataRanOut);
    QVERIFY(QGCCompression::extractFileData(archivePath, entryName, QGCCompression::Format::Auto, 0).isEmpty());
    verifyExpectedLogMessage();

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, dataRanOut);
    QVERIFY(QGCCompression::extractFileDataFromDevice(&device, entryName, 0).isEmpty());
    verifyExpectedLogMessage();

    // The default limit is far below what the header declares, so the entry is not started
    const QRegularExpression overTheLimit("Size limit exceeded");
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, overTheLimit);
    QVERIFY(QGCCompression::extractFileData(archivePath, entryName).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);

    QVERIFY(device.seek(0));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, overTheLimit);
    QVERIFY(QGCCompression::extractFileDataFromDevice(&device, entryName).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
}

void QGCCompressionTest::_testExtractFileDataSizeLimit_data()
{
    QTest::addColumn<bool>("declaredSize");
    QTest::addColumn<bool>("fromDevice");

    // A TAR header declares the size of its entry; an entry of a streamed ZIP shows its size only as it is read
    QTest::newRow("declared size, file") << true << false;
    QTest::newRow("declared size, device") << true << true;
    QTest::newRow("undeclared size, file") << false << false;
    QTest::newRow("undeclared size, device") << false << true;
}

void QGCCompressionTest::_testExtractFileDataSizeLimit()
{
    QFETCH(bool, declaredSize);
    QFETCH(bool, fromDevice);

    const QByteArray entryName("entry.bin");
    const QByteArray content(3000, 'a');
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(declaredSize ? "entry.tar" : "entry.zip");
    QVERIFY(_writeFile(archivePath, declaredSize ? _makeTar({{.name = entryName, .data = content}})
                                                 : _makeStreamedZip({{.name = entryName, .data = content}}, false)));

    const auto extract = [&](const QString& path, qint64 limit) {
        if (!fromDevice) {
            return QGCCompression::extractFileData(path, entryName, QGCCompression::Format::Auto, limit);
        }
        QFile device(path);
        if (!device.open(QIODevice::ReadOnly)) {
            return QByteArray();
        }
        return QGCCompression::extractFileDataFromDevice(&device, entryName, limit);
    };

    // One byte short of the entry
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded: 2999 bytes"));
    QVERIFY(extract(archivePath, content.size() - 1).isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);

    QCOMPARE(extract(archivePath, content.size()), content);
    QCOMPARE(extract(archivePath, 0), content);

    if (declaredSize) {
        // An entry that says it does not fit is not started: its data, cut short here, is never read
        const QString overstatedPath = tempDir.filePath("overstated.tar");
        QVERIFY(_writeFile(overstatedPath, _tarHeader({.name = entryName}, 5000) + QByteArray(512, 'x')));
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                         QRegularExpression("Size limit exceeded: 4000 bytes"));
        QVERIFY(extract(overstatedPath, 4000).isEmpty());
        verifyExpectedLogMessage();
        QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
    }
}

// ============================================================================
// Cross-Platform Path Handling Tests
// ============================================================================
void QGCCompressionTest::_testWindowsPathSeparators()
{
    QTemporaryDir tempDir;
    // Test that paths with Windows-style separators are handled correctly
    // libarchive archives may contain entries with backslashes
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Get entries and verify none have issues
    const QStringList entries = QGCCompression::listArchive(zipResource);
    for (const QString& entry : entries) {
        // Entry names should not contain backslashes after processing
        // (or if they do, they should be handled correctly)
        // Just verify no crashes or issues accessing these entries
        const bool exists = QGCCompression::fileExists(zipResource, entry);
        Q_UNUSED(exists);  // Just testing it doesn't crash
    }
    // Test path joining with mixed separators
    // QGCFileHelper::joinPath should normalize these
    // Create a test file with normal path
    const QString testDir = tempDir.path() + "/path_test";
    QVERIFY(QDir().mkpath(testDir));
    QFile testFile(testDir + "/test.txt");
    QVERIFY(testFile.open(QIODevice::WriteOnly));
    testFile.write("test");
    testFile.close();
    // Verify the file exists regardless of separator style used in creation
    QVERIFY(QFile::exists(testDir + "/test.txt"));
}

void QGCCompressionTest::_testExtractArchiveToSymlinkedOutputPath()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString realOutputDir = tempDir.path() + "/symlink_real_output";
    const QString symlinkOutputDir = tempDir.path() + "/symlink_alias_output";
    QVERIFY(QDir().mkpath(realOutputDir));
    QFile::remove(symlinkOutputDir);
    if (!QFile::link(realOutputDir, symlinkOutputDir)) {
        QSKIP(qPrintable(QStringLiteral("Directory symlinks are not supported in this environment")));
    }

    QFileInfo symlinkInfo(symlinkOutputDir);
    QVERIFY(symlinkInfo.isSymLink());
    QVERIFY(QGCCompression::extractArchive(zipResource, symlinkOutputDir));
    QVERIFY(QFile::exists(realOutputDir + "/manifest.json"));
    QVERIFY(QFile::exists(symlinkOutputDir + "/manifest.json"));
}

void QGCCompressionTest::_testExtractRejectsEscapingNames_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"extractArchive", "extractArchiveFiltered", "extractByPattern", "extractFiles"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testExtractRejectsEscapingNames()
{
    QFETCH(QString, api);

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    const QString absolutePath = tempDir.filePath("absolute.txt");
    const QString archivePath = tempDir.filePath("hostile.tar");
    QFile archive(archivePath);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    archive.write(_makeTar({{.name = "../escape.txt", .data = "escaped"},
                            {.name = QFile::encodeName(absolutePath), .data = "absolute"},
                            {.name = "good.txt", .data = "good"}}));
    archive.close();

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("ath traversal attempt.*escape"));
    if (api == "extractArchive") {
        QVERIFY(QGCCompression::extractArchive(archivePath, outputDir));
    } else if (api == "extractArchiveFiltered") {
        QVERIFY(QGCCompression::extractArchiveFiltered(archivePath, outputDir,
                                                       [](const QGCCompression::ArchiveEntry&) { return true; }));
    } else if (api == "extractByPattern") {
        QVERIFY(QGCCompression::extractByPattern(archivePath, {"*.txt"}, outputDir));
    } else {
        // Asked for by name, so refusing the entry fails the call
        QVERIFY(!QGCCompression::extractFiles(archivePath, {"../escape.txt", "good.txt"}, outputDir));
    }
    verifyExpectedLogMessage();

    QVERIFY(!QFile::exists(tempDir.filePath("escape.txt")));
    QVERIFY(!QFile::exists(absolutePath));
    const QStringList archiveAndOutput{QStringLiteral("hostile.tar"), QStringLiteral("output")};
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name),
             archiveAndOutput);
    if (api != "extractFiles") {
        QVERIFY(QFile::exists(outputDir + "/good.txt"));
        // The absolute name is kept, below the output directory
        QVERIFY(QFile::exists(outputDir + absolutePath));
    }
}

void QGCCompressionTest::_testExtractRejectsEscapingLinks()
{
#ifndef Q_OS_UNIX
    QSKIP("The hostile archive is made of POSIX links");
#else
    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    const QString archivePath = tempDir.filePath("hostile.tar");
    const QString victimPath = tempDir.filePath("victim.txt");
    QFile victim(victimPath);
    QVERIFY(victim.open(QIODevice::WriteOnly));
    victim.write("original");
    victim.close();

    QFile archive(archivePath);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    archive.write(_makeTar({
        // "climb" only leaves the output directory through "inside", which a check of the names alone misses
        {.name = "inside", .type = '2', .linkName = "."},
        {.name = "climb", .type = '2', .linkName = "inside/.."},
        {.name = "absolute", .type = '2', .linkName = "/"},
        {.name = "parent", .type = '2', .linkName = ".."},
        {.name = "hard.txt", .type = '1', .linkName = "victim.txt"},
        {.name = "good.txt", .data = "good"},
        {.name = "climb/made_dir/inner.txt", .data = "inner"},
    }));
    archive.close();

    // libarchive resolves a hard link target against the working directory
    const QString originalDir = QDir::currentPath();
    QVERIFY(QDir::setCurrent(tempDir.path()));
    const auto restoreDir = qScopeGuard([&originalDir]() { QDir::setCurrent(originalDir); });

    for (const char* skipped : {"climb", "absolute", "parent"}) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Skipping symlink.*%1").arg(QLatin1String(skipped))));
    }
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Skipping hard link.*hard.txt"));
    const bool extracted = QGCCompression::extractArchive(archivePath, outputDir);
    for (int i = 0; i < 4; ++i) {
        verifyExpectedLogMessage();
    }

    QVERIFY(!QFileInfo::exists(outputDir + "/hard.txt"));
    QVERIFY(victim.open(QIODevice::ReadOnly));
    QCOMPARE(victim.readAll(), QByteArray("original"));
    for (const char* link : {"climb", "absolute", "parent"}) {
        QVERIFY2(!QFileInfo(outputDir + QLatin1Char('/') + QLatin1String(link)).isSymLink(), link);
    }
    const QStringList untouched{QStringLiteral("hostile.tar"), QStringLiteral("output"), QStringLiteral("victim.txt")};
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name),
             untouched);

    // Entries that stay inside are still extracted
    QVERIFY(extracted);
    QVERIFY(QFile::exists(outputDir + "/good.txt"));
    QVERIFY(QFile::exists(outputDir + "/climb/made_dir/inner.txt"));
#endif
}

void QGCCompressionTest::_testExtractLeavesOutLinks_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"extractArchiveFiltered", "extractByPattern", "extractFiles", "extractFile"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testExtractLeavesOutLinks()
{
    QFETCH(QString, api);

    // Both links stay inside the output directory. These functions write the data of regular files and
    // cannot make a link, and a link is not an empty file.
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("links.tar");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(_writeFile(archivePath, _makeTar({{.name = "target.txt", .data = "target"},
                                              {.name = "soft.txt", .type = '2', .linkName = "target.txt"},
                                              {.name = "hard.txt", .type = '1', .linkName = "target.txt"}})));
    const QStringList targetOnly{QStringLiteral("target.txt")};
    const auto expectSkipped = [this](const char* kind, const char* name) {
        expectLogMessage(
            "Utilities.QGClibarchive", QtWarningMsg,
            QRegularExpression(
                QStringLiteral("Skipping %1.*%2 -> target.txt").arg(QLatin1String(kind), QLatin1String(name))));
    };

    if (api == "extractArchiveFiltered") {
        expectSkipped("symlink", "soft.txt");
        expectSkipped("hard link", "hard.txt");
        QVERIFY(QGCCompression::extractArchiveFiltered(archivePath, outputDir,
                                                       [](const QGCCompression::ArchiveEntry&) { return true; }));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    } else if (api == "extractByPattern") {
        QStringList extractedFiles;
        expectSkipped("symlink", "soft.txt");
        expectSkipped("hard link", "hard.txt");
        QVERIFY(QGCCompression::extractByPattern(archivePath, {"*.txt"}, outputDir, &extractedFiles));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        QCOMPARE(extractedFiles, targetOnly);
    } else if (api == "extractFiles") {
        // Asked for by name, so leaving the entry out fails the call
        expectSkipped("symlink", "soft.txt");
        QVERIFY(!QGCCompression::extractFiles(archivePath, {"target.txt", "soft.txt"}, outputDir));
        verifyExpectedLogMessage();
        expectSkipped("hard link", "hard.txt");
        QVERIFY(!QGCCompression::extractFiles(archivePath, {"hard.txt"}, outputDir));
        verifyExpectedLogMessage();
    } else {
        QVERIFY(QGCCompression::extractFile(archivePath, "target.txt", outputDir + "/target.txt"));
        expectSkipped("symlink", "soft.txt");
        QVERIFY(!QGCCompression::extractFile(archivePath, "soft.txt", outputDir + "/soft.txt"));
        verifyExpectedLogMessage();
        expectSkipped("hard link", "hard.txt");
        QVERIFY(!QGCCompression::extractFile(archivePath, "hard.txt", outputDir + "/hard.txt"));
        verifyExpectedLogMessage();
    }

    QCOMPARE(_entriesBelow(outputDir), targetOnly);
    QCOMPARE(_readFile(outputDir + "/target.txt"), QByteArray("target"));
}

void QGCCompressionTest::_testSpecialCharactersInPath()
{
    QTemporaryDir tempDir;
    // Test paths with special characters (spaces, unicode, etc.)
    // Create directory with space in name
    const QString dirWithSpace = tempDir.path() + "/dir with space";
    QVERIFY(QDir().mkpath(dirWithSpace));
    // Extract to directory with space
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QVERIFY(QGCCompression::extractArchive(zipResource, dirWithSpace));
    QVERIFY(QFile::exists(dirWithSpace + "/manifest.json"));
    // Create directory with special chars (parentheses, brackets)
    const QString dirWithSpecial = tempDir.path() + "/dir(1)[test]";
    QVERIFY(QDir().mkpath(dirWithSpecial));
    // Extract to directory with special chars
    QVERIFY(QGCCompression::extractArchive(zipResource, dirWithSpecial));
    QVERIFY(QFile::exists(dirWithSpecial + "/manifest.json"));
    // Test with ampersand and equals (common in URLs that get saved as files)
    const QString dirWithAmp = tempDir.path() + "/dir&param=value";
    QVERIFY(QDir().mkpath(dirWithAmp));
    QVERIFY(QGCCompression::extractArchive(zipResource, dirWithAmp));
    QVERIFY(QFile::exists(dirWithAmp + "/manifest.json"));
}

void QGCCompressionTest::_testUnicodePaths()
{
    QTemporaryDir tempDir;
    // Test paths with Unicode characters
    // Create directory with Unicode name (Japanese, Chinese, emoji-like)
    const QString unicodeDir = tempDir.path() + QString::fromUtf8("/日本語_中文_αβγ");
    QVERIFY(QDir().mkpath(unicodeDir));
    // Extract to Unicode directory
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QVERIFY2(QGCCompression::extractArchive(zipResource, unicodeDir), "Failed to extract to Unicode path");
    QVERIFY(QFile::exists(unicodeDir + "/manifest.json"));
    // Verify content is intact
    QFile extracted(unicodeDir + "/manifest.json");
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    const QByteArray content = extracted.readAll();
    QVERIFY(content.contains("\"name\""));
    extracted.close();
    // Test with accented Latin characters
    const QString accentedDir = tempDir.path() + QString::fromUtf8("/café_naïve_résumé");
    QVERIFY(QDir().mkpath(accentedDir));
    QVERIFY(QGCCompression::extractArchive(zipResource, accentedDir));
    QVERIFY(QFile::exists(accentedDir + "/manifest.json"));
    // Test with Cyrillic
    const QString cyrillicDir = tempDir.path() + QString::fromUtf8("/Привет_мир");
    QVERIFY(QDir().mkpath(cyrillicDir));
    QVERIFY(QGCCompression::extractArchive(zipResource, cyrillicDir));
    QVERIFY(QFile::exists(cyrillicDir + "/manifest.json"));
}

// ============================================================================
// Benchmarks
// ============================================================================
void QGCCompressionTest::_benchmarkExtractZip()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    int iteration = 0;
    QBENCHMARK
    {
        QTemporaryDir tempDir;
        const QString outputDir = tempDir.path() + QString("/bench_zip_%1").arg(iteration++);
        QGCCompression::extractArchive(zipResource, outputDir);
    }
}

void QGCCompressionTest::_benchmarkExtract7z()
{
    const QString archive = QStringLiteral(":/unittest/manifest.json.7z");
    int iteration = 0;
    QBENCHMARK
    {
        QTemporaryDir tempDir;
        const QString outputDir = tempDir.path() + QString("/bench_7z_%1").arg(iteration++);
        QGCCompression::extractArchive(archive, outputDir);
    }
}

void QGCCompressionTest::_benchmarkDecompressGzip()
{
    // Load compressed data once
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    QBENCHMARK
    {
        QGCCompression::decompressData(compressedData);
    }
}

void QGCCompressionTest::_benchmarkDecompressXz()
{
    // Load compressed data once
    QFile resourceFile(":/unittest/manifest.json.xz");
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    const QByteArray compressedData = resourceFile.readAll();
    resourceFile.close();
    QBENCHMARK
    {
        QGCCompression::decompressData(compressedData);
    }
}

void QGCCompressionTest::_benchmarkConcurrentExtraction()
{
    constexpr int kNumExtractions = 4;
    int baseIteration = 0;
    QBENCHMARK
    {
        QTemporaryDir tempDir;
        QList<QFuture<bool>> futures;
        for (int i = 0; i < kNumExtractions; ++i) {
            const int iteration = baseIteration * kNumExtractions + i;
            futures.append(QtConcurrent::run([iteration, &tempDir]() {
                const QString outputDir = tempDir.path() + QString("/bench_concurrent_%1").arg(iteration);
                return QGCCompression::extractArchive(":/unittest/manifest.json.zip", outputDir);
            }));
        }
        for (auto& future : futures) {
            future.waitForFinished();
        }
        baseIteration++;
    }
}

void QGCCompressionTest::_benchmarkListArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QBENCHMARK
    {
        QGCCompression::listArchiveDetailed(zipResource);
    }
}

#include <QtCore/QTemporaryDir>

#include "UnitTest.h"

UT_REGISTER_TEST_LIGHTWEIGHT(QGCCompressionTest, TestLabel::Unit, TestLabel::Utilities)
