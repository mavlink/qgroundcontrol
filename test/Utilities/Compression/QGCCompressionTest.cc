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
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "QGCArchiveFile.h"
#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCDecompressDevice.h"

QGCCompression::Outcome QGCCompressionTest::_call(const QString& api, const Inputs& inputs, Results* results)
{
    using namespace QGCCompression;
    using EntryPoint = std::function<Outcome(const Inputs&, Results&)>;

    static const auto open = [](QFile& device, const Inputs& in) {
        device.setFileName(in.path);
        (void) device.open(QIODevice::ReadOnly);
        return &device;
    };

    static const QHash<QString, EntryPoint> entryPoints{
        {"extractArchive",
         [](const Inputs& in, Results&) {
             return extractArchive(in.path, in.output, in.format, in.progress, in.maxBytes);
         }},
        {"extractArchiveAtomic",
         [](const Inputs& in, Results&) {
             return extractArchiveAtomic(in.path, in.output, in.format, in.progress, in.maxBytes);
         }},
        {"extractArchiveFiltered",
         [](const Inputs& in, Results&) {
             return extractArchiveFiltered(in.path, in.output, in.filter, in.progress, in.maxBytes);
         }},
        {"extractFromDevice",
         [](const Inputs& in, Results&) {
             QFile device;
             return extractFromDevice(open(device, in), in.output, in.progress, in.maxBytes);
         }},
        {"extractFiles",
         [](const Inputs& in, Results&) { return extractFiles(in.path, in.names, in.output, in.format); }},
        {"extractByPattern",
         [](const Inputs& in, Results&) { return extractByPattern(in.path, in.names, in.output, nullptr, in.format); }},
        {"extractFile",
         [](const Inputs& in, Results&) {
             // The output is the path of a file, also where the name of the entry is that of a directory
             const QString name = in.names.constFirst();
             const QString fileName = name.endsWith(QLatin1Char('/')) ? name.chopped(1) : name;
             return extractFile(in.path, name, in.output + QLatin1Char('/') + fileName, in.format);
         }},
        {"extractFileData",
         [](const Inputs& in, Results& out) {
             out.data = "to be replaced";
             return extractFileData(in.path, in.names.constFirst(), out.data, in.format, in.maxBytes);
         }},
        {"extractFileDataFromDevice",
         [](const Inputs& in, Results& out) {
             QFile device;
             out.data = "to be replaced";
             return extractFileDataFromDevice(open(device, in), in.names.constFirst(), out.data, in.maxBytes);
         }},
        {"listArchive",
         [](const Inputs& in, Results& out) {
             out.names = {QStringLiteral("to be replaced")};
             return listArchive(in.path, out.names, in.format);
         }},
        {"listArchiveDetailed",
         [](const Inputs& in, Results& out) {
             out.entries = {ArchiveEntry{}};
             return listArchiveDetailed(in.path, out.entries, in.format);
         }},
        {"getArchiveStats",
         [](const Inputs& in, Results& out) {
             out.stats.totalEntries = 1;
             return getArchiveStats(in.path, out.stats, in.format);
         }},
        {"validateArchive", [](const Inputs& in, Results&) { return validateArchive(in.path, in.format); }},
        {"fileExists",
         [](const Inputs& in, Results&) { return fileExists(in.path, in.names.constFirst(), in.format); }},
        {"decompressFile",
         [](const Inputs& in, Results&) {
             return decompressFile(in.path, in.output, in.format, in.progress, in.maxBytes);
         }},
        {"decompressData",
         [](const Inputs& in, Results& out) {
             out.data = "to be replaced";
             return decompressData(TestFixtures::readFile(in.path), out.data, in.format, in.maxBytes);
         }},
        {"decompressFromDevice-file",
         [](const Inputs& in, Results&) {
             QFile device;
             return decompressFromDevice(open(device, in), in.output, in.progress, in.maxBytes);
         }},
        {"decompressFromDevice-memory",
         [](const Inputs& in, Results& out) {
             QFile device;
             out.data = "to be replaced";
             return decompressFromDevice(open(device, in), out.data, in.maxBytes);
         }},
    };

    const auto entryPoint = entryPoints.constFind(api);
    if (entryPoint == entryPoints.constEnd()) {
        QTest::qFail(qPrintable(QStringLiteral("No entry point is called %1").arg(api)), __FILE__, __LINE__);
        return {.error = Error::InternalError};
    }
    Results unused;
    return (*entryPoint)(inputs, (results != nullptr) ? *results : unused);
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

bool QGCCompressionTest::_writeGzipOfZeros(const QString& path, int memberCount, const QByteArray& firstMember)
{
    const QByteArray member = TestFixtures::gzipMember(QByteArray(64 * 1024, '\0'));

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
        out << quint32(0x08074B50) << TestFixtures::crc32(entry.data) << quint32(deflated.size())
            << quint32(entry.data.size());
        // Central directory header. An entry made on Unix (3) has its mode in the upper half of the attributes.
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
    QVERIFY(QFile::copy(":/unittest/manifest.json.gz", noExtFile));
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

void QGCCompressionTest::_testFormatParameter()
{
    using QGCCompression::Error;
    using QGCCompression::Format;
    using QGCCompression::Outcome;

    QTemporaryDir tempDir;
    const QString zip = QStringLiteral(":/unittest/manifest.json.zip");
    const QString sevenZip = QStringLiteral(":/unittest/manifest.json.7z");
    const QByteArray gzData = TestFixtures::readFile(QStringLiteral(":/unittest/manifest.json.gz"));
    const QByteArray xzData = TestFixtures::readFile(QStringLiteral(":/unittest/manifest.json.xz"));
    const QStringList manifestOnly{QStringLiteral("manifest.json")};

    // The format does not choose the decoder: one of the right kind that is not the input's reads it all the same
    const Outcome mismatched = QGCCompression::extractArchive(sevenZip, tempDir.filePath("mismatched"), Format::ZIP);
    QVERIFY(mismatched);
    QVERIFY2(mismatched.formatName.startsWith("7-Zip"), qPrintable(mismatched.formatName));
    QCOMPARE(_entriesBelow(tempDir.filePath("mismatched")), manifestOnly);
    QStringList names;
    QVERIFY(QGCCompression::listArchive(zip, names, Format::TAR_XZ));
    QCOMPARE(names, manifestOnly);
    QByteArray manifest;
    QVERIFY(QGCCompression::decompressData(gzData, manifest));
    QVERIFY(!manifest.isEmpty());
    QByteArray fromXz;
    const Outcome asGzip = QGCCompression::decompressData(xzData, fromXz, Format::GZIP);
    QCOMPARE(fromXz, manifest);
    QCOMPARE(asGzip.filterName, QStringLiteral("xz"));

    // It tells the kind of the input, and each function refuses the kind it does not handle
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Not an archive format"));
    const Outcome streamKind = QGCCompression::extractArchive(zip, tempDir.filePath("stream_kind"), Format::GZIP);
    verifyExpectedLogMessage();
    QCOMPARE(streamKind.error, Error::UnsupportedFormat);
    QVERIFY(!QFileInfo::exists(tempDir.filePath("stream_kind")));

    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Invalid decompression format"));
    QByteArray refused("stale");
    const Outcome archiveKind = QGCCompression::decompressData(gzData, refused, Format::ZIP);
    verifyExpectedLogMessage();
    QCOMPARE(archiveKind.error, Error::UnsupportedFormat);
    QVERIFY(refused.isEmpty());

    // Auto detects the kind from the name and then the content. Neither tells that this gzip stream holds a
    // TAR archive, so only the format says that it is to be extracted
    const QString unnamedArchive = tempDir.filePath("bundle");
    QVERIFY(TestFixtures::writeFile(unnamedArchive,
                                    TestFixtures::gzipMember(_makeTar({{.name = "good.txt", .data = "good"}}))));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Not an archive format"));
    const Outcome detected = QGCCompression::extractArchive(unnamedArchive, tempDir.filePath("detected"));
    verifyExpectedLogMessage();
    QCOMPARE(detected.error, Error::UnsupportedFormat);
    QVERIFY(QGCCompression::extractArchive(unnamedArchive, tempDir.filePath("told"), Format::TAR_GZ));
    QCOMPARE(TestFixtures::readFile(tempDir.filePath("told/good.txt")), QByteArray("good"));

    // For decompressFile() it also decides between extracting and decompressing, and names the default output:
    // GZIP takes the gzip layer off a .tar.gz file and leaves the archive
    const QString tarGz = tempDir.filePath("bundle.tar.gz");
    QVERIFY(QFile::copy(unnamedArchive, tarGz));
    QVERIFY(QGCCompression::decompressFile(tarGz, QString(), Format::GZIP));
    QVERIFY(QGCCompression::listArchive(tempDir.filePath("bundle.tar"), names));
    QCOMPARE(names, QStringList{QStringLiteral("good.txt")});
}

// ============================================================================
// Archive Extraction Tests (from Qt resources)
// ============================================================================
void QGCCompressionTest::_testExtractFromResource_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QGCCompression::Format>("format");

    using QGCCompression::Format;
    const QString zip = QStringLiteral(":/unittest/manifest.json.zip");

    QTest::newRow("ZIP") << "extractArchive" << zip << Format::Auto;
    QTest::newRow("7z") << "extractArchive" << QStringLiteral(":/unittest/manifest.json.7z") << Format::SEVENZ;
    QTest::newRow("ZIP, atomically") << "extractArchiveAtomic" << zip << Format::Auto;
}

void QGCCompressionTest::_testExtractFromResource()
{
    QFETCH(QString, api);
    QFETCH(QString, resource);
    QFETCH(QGCCompression::Format, format);

    QTemporaryDir tempDir;
    const QString outputPath = tempDir.filePath("output");
    const QGCCompression::Outcome outcome = _call(api, {.path = resource, .output = outputPath, .format = format});
    QVERIFY2(outcome, qPrintable(outcome.message));
    QCOMPARE(_entriesBelow(outputPath), QStringList{QStringLiteral("manifest.json")});
}

void QGCCompressionTest::_testExtractArchiveAtomicReplacesExistingDirectory()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = tempDir.path() + "/atomic_replace";
    QVERIFY(QDir().mkpath(outputPath));

    const QString staleFilePath = outputPath + "/stale.txt";
    QVERIFY(TestFixtures::writeFile(staleFilePath, "stale"));

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
    const QByteArray retainedContent("keep-me");
    QVERIFY(TestFixtures::writeFile(retainedFilePath, retainedContent));

    const QString corruptArchivePath = tempDir.path() + "/corrupt_atomic.zip";
    QVERIFY(TestFixtures::writeFile(corruptArchivePath, "not-an-archive"));

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(".*Unrecognized archive format.*"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Archive is empty or invalid.*"));
    QVERIFY(!QGCCompression::extractArchiveAtomic(corruptArchivePath, outputPath));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(TestFixtures::readFile(retainedFilePath), retainedContent);
}

void QGCCompressionTest::_testListArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QStringList entries;
    QVERIFY(QGCCompression::listArchive(zipResource, entries));
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
    QStringList autoEntries;
    QVERIFY(QGCCompression::listArchive(zipResource, autoEntries, QGCCompression::Format::Auto));
    QCOMPARE(autoEntries.size(), entries.size());
}

void QGCCompressionTest::_testListArchiveDetailed()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QList<QGCCompression::ArchiveEntry> entries;
    QVERIFY(QGCCompression::listArchiveDetailed(zipResource, entries));
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
    QVERIFY(TestFixtures::writeFile(archivePath, _makeTar({{.name = "file10.txt"},
                                                           {.name = "File2.txt"},
                                                           {.name = "file1.txt"},
                                                           {.name = "dir/file11.txt"},
                                                           {.name = "file20.txt"},
                                                           {.name = "dir/file3.txt"}})));

    // Numbers compare by their value, and an upper case letter does not come before the lower case ones
    const QStringList expected{QStringLiteral("dir/file3.txt"), QStringLiteral("dir/file11.txt"),
                               QStringLiteral("file1.txt"),     QStringLiteral("File2.txt"),
                               QStringLiteral("file10.txt"),    QStringLiteral("file20.txt")};
    QStringList names;
    QVERIFY(QGCCompression::listArchive(archivePath, names));
    QCOMPARE(names, expected);

    QList<QGCCompression::ArchiveEntry> entries;
    QVERIFY(QGCCompression::listArchiveDetailed(archivePath, entries));
    QStringList detailedNames;
    for (const QGCCompression::ArchiveEntry& entry : std::as_const(entries)) {
        detailedNames.append(entry.name);
    }
    QCOMPARE(detailedNames, expected);
}

void QGCCompressionTest::_testGetArchiveStats()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QGCCompression::ArchiveStats stats;
    QVERIFY(QGCCompression::getArchiveStats(zipResource, stats));
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
    QGCCompression::ArchiveStats stats7z;
    QVERIFY(QGCCompression::getArchiveStats(archiveResource, stats7z));
    QVERIFY(stats7z.totalEntries > 0);
    QVERIFY(stats7z.fileCount > 0);
    // Test non-existent file returns zero stats
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QGCCompression::ArchiveStats emptyStats = stats;
    QVERIFY(!QGCCompression::getArchiveStats("/nonexistent/file.zip", emptyStats));
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
    QVERIFY(TestFixtures::writeFile(corruptZip, "This is not a valid ZIP file"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    const QGCCompression::Outcome corrupt = QGCCompression::validateArchive(corruptZip);
    verifyExpectedLogMessage();
    QCOMPARE(corrupt.error, QGCCompression::Error::InvalidArchive);
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
    QCOMPARE(QGCCompression::fileExists(zipResource, "nonexistent.txt").error, QGCCompression::Error::FileNotInArchive);
    // Empty file name
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
    QVERIFY(!QGCCompression::fileExists(zipResource, ""));
    verifyExpectedLogMessage();
    // Non-existent archive
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    const QGCCompression::Outcome noArchive = QGCCompression::fileExists("/nonexistent/path/file.zip", "test.txt");
    verifyExpectedLogMessage();
    QCOMPARE(noArchive.error, QGCCompression::Error::FileNotFound);
    // A file that is not an archive cannot be said to be without the entry
    QTemporaryDir tempDir;
    const QString notAnArchive = tempDir.filePath("not_an_archive.zip");
    QVERIFY(TestFixtures::writeFile(notAnArchive, "This is not a valid ZIP file"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Unrecognized archive format"));
    const QGCCompression::Outcome unreadable = QGCCompression::fileExists(notAnArchive, "manifest.json");
    verifyExpectedLogMessage();
    QCOMPARE(unreadable.error, QGCCompression::Error::InvalidArchive);
}

void QGCCompressionTest::_testExtractArchiveFiltered()
{
    QTemporaryDir tempDir;
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Test filter that accepts all files
    const QString outputDir1 = tempDir.path() + "/filtered_all";
    int extractedCount = 0;
    QGCCompression::Outcome success = QGCCompression::extractArchiveFiltered(
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
    QVERIFY(TestFixtures::readFile(extractedPath).contains("\"name\""));
    // Test non-existent file returns false
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    const QGCCompression::Outcome notFound =
        QGCCompression::extractFile(zipResource, "nonexistent.txt", tempDir.path() + "/nope.txt");
    verifyExpectedLogMessage();
    QCOMPARE(notFound.error, QGCCompression::Error::FileNotInArchive);
}

void QGCCompressionTest::_testExtractFileData()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    QByteArray extracted;
    QVERIFY(QGCCompression::extractFileData(zipResource, "manifest.json", extracted));
    QVERIFY(!extracted.isEmpty());
    QVERIFY(extracted.contains("\"name\""));
    // Test non-existent file returns empty
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    QByteArray notFound = extracted;
    const QGCCompression::Outcome missing = QGCCompression::extractFileData(zipResource, "nonexistent.txt", notFound);
    QVERIFY(notFound.isEmpty());
    verifyExpectedLogMessage();
    QCOMPARE(missing.error, QGCCompression::Error::FileNotInArchive);
    // Test empty file name returns empty
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
    QByteArray emptyName = extracted;
    QVERIFY(!QGCCompression::extractFileData(zipResource, "", emptyName));
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
    const QGCCompression::Outcome notAll = QGCCompression::extractFiles(zipResource, badFiles, outputDir2);
    verifyExpectedLogMessage();
    QCOMPARE(notAll.error, QGCCompression::Error::FileNotInArchive);
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
        const QByteArray compressedData = TestFixtures::readFile(res.resource);
        QVERIFY2(!compressedData.isEmpty(), qPrintable(QString("Resource is empty: %1").arg(res.resource)));
        QByteArray decompressed;
        QVERIFY2(QGCCompression::decompressData(compressedData, decompressed), res.name);
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
    QVERIFY(TestFixtures::writeFile(plainFile, "plain text content"));
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

    const QByteArray compressed = TestFixtures::readFile(resource);
    QVERIFY(!compressed.isEmpty());

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(inputName);
    QVERIFY(TestFixtures::writeFile(inputPath, compressed));

    const QString expectedPath = tempDir.filePath(expectedOutputName);
    QCOMPARE(QGCCompression::decompressIfNeeded(inputPath), expectedPath);
    QCOMPARE(QGCCompression::strippedPath(inputPath), expectedPath);

    QVERIFY(TestFixtures::readFile(expectedPath).contains("\"name\""));
    QCOMPARE(TestFixtures::readFile(inputPath), compressed);

    // decompressFile() without an output path writes to the same place
    const QStringList inputAndOutput = _entriesBelow(tempDir.path());
    QVERIFY(QFile::remove(expectedPath));
    QVERIFY(QGCCompression::decompressFile(inputPath));
    QCOMPARE(_entriesBelow(tempDir.path()), inputAndOutput);
}

void QGCCompressionTest::_testDecompressEmptyStream_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<QGCCompression::Format>("format");
    QTest::addColumn<QString>("filterName");
    QTest::addColumn<QByteArray>("stream");

    using QGCCompression::Format;

    struct EmptyStream
    {
        const char* extension;
        Format format;
        const char* filterName;
        const char* hex;
    };

    // What each compressor writes for a file of no bytes
    const EmptyStream streams[] = {
        {"gz", Format::GZIP, "gzip", "1f8b080000000000000303000000000000000000"},
        {"xz", Format::XZ, "xz", "fd377a585a000004e6d6b446000000001cdf44211fb6f37d010000000004595a"},
        {"lzma", Format::XZ, "lzma", "5d00008000ffffffffffffffff0083fffbffffc0000000"},
        {"zst", Format::ZSTD, "zstd", "28b52ffd240001000099e9d851"},
#ifdef QGC_ENABLE_BZIP2
        {"bz2", Format::BZIP2, "bzip2", "425a683917724538509000000000"},
#endif
#ifdef QGC_ENABLE_LZ4
        {"lz4", Format::LZ4, "lz4", "04224d186440a700000000055dcc02"},
#endif
    };
    for (const EmptyStream& stream : streams) {
        for (const char* api :
             {"decompressFile", "decompressData", "decompressFromDevice-file", "decompressFromDevice-memory",
              "decompressIfNeeded", "QGCDecompressDevice", "QGCDecompressDevice-file", "readFile", "computeFileHash"}) {
            QTest::addRow("%s %s", stream.filterName, api)
                << api << stream.extension << stream.format << stream.filterName << QByteArray::fromHex(stream.hex);
        }
    }
}

void QGCCompressionTest::_testDecompressEmptyStream()
{
    QFETCH(QString, api);
    QFETCH(QString, extension);
    QFETCH(QGCCompression::Format, format);
    QFETCH(QString, filterName);
    QFETCH(QByteArray, stream);
    using QGCCompression::Error;
    using QGCCompression::Format;

    // An empty log or parameter file is compressed to a valid stream that holds no data
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("empty.log." + extension);
    const QString outputPath = tempDir.filePath("output.log");
    QVERIFY(TestFixtures::writeFile(inputPath, stream));
    const auto isEmptyFile = [](const QString& path) {
        const QFileInfo info(path);
        return info.isFile() && (info.size() == 0);
    };

    if (api == "decompressIfNeeded") {
        const QString defaultOutput = tempDir.filePath("empty.log");
        QCOMPARE(QGCCompression::decompressIfNeeded(inputPath), defaultOutput);
        QVERIFY(isEmptyFile(defaultOutput));
        // The signature that tells how it ended writes the file again
        QVERIFY(QFile::remove(defaultOutput));
        QGCCompression::Outcome outcome;
        QCOMPARE(QGCCompression::decompressIfNeeded(inputPath, outcome), defaultOutput);
        QVERIFY(isEmptyFile(defaultOutput));
        QCOMPARE(outcome.error, Error::None);
    } else if (api.startsWith("QGCDecompressDevice")) {
        // The device reads a file that it opens, or a device that it is given
        QBuffer source;
        source.setData(stream);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QGCDecompressDevice ofFile(inputPath);
        QGCDecompressDevice ofDevice(&source);
        QGCDecompressDevice& decompressor = api.endsWith("-file") ? ofFile : ofDevice;
        QVERIFY2(decompressor.open(QIODevice::ReadOnly), qPrintable(decompressor.errorString()));
        QCOMPARE(decompressor.filterName(), filterName);
        QVERIFY(decompressor.atEnd());
        char byte = 0;
        QCOMPARE(decompressor.read(&byte, 1), 0);
        QVERIFY(decompressor.readAll().isEmpty());
        QVERIFY2(decompressor.errorString().isEmpty(), qPrintable(decompressor.errorString()));
    } else if (api == "readFile") {
        QString readError;
        QVERIFY(QGCCompression::readFile(inputPath, &readError).isEmpty());
        QVERIFY2(readError.isEmpty(), qPrintable(readError));
    } else if (api == "computeFileHash") {
        QCOMPARE(QGCCompression::computeFileHash(inputPath),
                 QString::fromLatin1(QCryptographicHash::hash(QByteArray(), QCryptographicHash::Sha256).toHex()));
    } else {
        // A file has a name to tell its format. Data has its signature, but for the stream of LZMA, which has none
        const bool tellFormat = (api == "decompressData") && (QGCCompression::detectFormatFromData(stream) != format);
        // No data is also what a failure gives from the functions that decompress to memory, which are handed
        // an array with content
        Results results;
        const QGCCompression::Outcome outcome = _call(
            api, {.path = inputPath, .output = outputPath, .format = tellFormat ? format : Format::Auto}, &results);
        QCOMPARE(outcome.error, Error::None);
        QCOMPARE(outcome.filterName, filterName);
        // The output is a file without content, or no data
        if (api.endsWith("-memory") || (api == "decompressData")) {
            QVERIFY(results.data.isEmpty());
        } else {
            QVERIFY(isEmptyFile(outputPath));
        }
    }
}

void QGCCompressionTest::_testDecompressRejectsStreamWithoutData_data()
{
    QTest::addColumn<QByteArray>("stream");
    QTest::addColumn<QString>("reason");

    // Neither is a compressed stream of nothing: an input of no bytes, and a gzip stream of nothing that
    // has lost the eight bytes after its data. libarchive 3.8.8 has no words for the second, so the reader
    // gives its own
    QTest::newRow("no bytes") << QByteArray() << "Unrecognized archive format";
    QTest::newRow("cut gzip") << QByteArray::fromHex("1f8b08000000000000030300") << "Truncated";
}

void QGCCompressionTest::_testDecompressRejectsStreamWithoutData()
{
    QFETCH(QByteArray, stream);
    QFETCH(QString, reason);
    using QGCCompression::Error;

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("nothing.gz");
    const QString outputPath = tempDir.filePath("nothing");
    QVERIFY(TestFixtures::writeFile(inputPath, stream));
    QBuffer device;
    device.setData(stream);
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QRegularExpression failure(reason);

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, failure);
    QCOMPARE(QGCCompression::decompressFile(inputPath, outputPath).error, Error::InvalidArchive);
    verifyExpectedLogMessage();

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, failure);
    QCOMPARE(QGCCompression::decompressFromDevice(&device, outputPath).error, Error::InvalidArchive);
    verifyExpectedLogMessage();

    QVERIFY(device.seek(0));
    QByteArray data("to be replaced");
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, failure);
    QCOMPARE(QGCCompression::decompressFromDevice(&device, data).error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    if (!stream.isEmpty()) {
        // decompressData() refuses data of no bytes before it is looked at: see _testDecompressDataErrors
        data = "to be replaced";
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, failure);
        QCOMPARE(QGCCompression::decompressData(stream, data).error, Error::InvalidArchive);
        verifyExpectedLogMessage();
        QVERIFY(data.isEmpty());
    }

    // The device does not log: the reason is in its error string
    QVERIFY(device.seek(0));
    QGCDecompressDevice decompressor(&device);
    QVERIFY(!decompressor.open(QIODevice::ReadOnly));
    QVERIFY2(decompressor.errorString().contains(failure), qPrintable(decompressor.errorString()));

    QString readError;
    QVERIFY(QGCCompression::readFile(inputPath, &readError).isEmpty());
    QVERIFY(!readError.isEmpty());

    // Nothing is written for an input that is refused
    QCOMPARE(_entriesBelow(tempDir.path()), QStringList{QStringLiteral("nothing.gz")});
}

void QGCCompressionTest::_testDecompressPlainInput_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<bool>("refused");

    // A function that is asked to decompress refuses an input that is not compressed
    QTest::newRow("decompressFile") << "decompressFile" << true;
    QTest::newRow("decompressData") << "decompressData" << true;
    QTest::newRow("decompressFromDevice-file") << "decompressFromDevice-file" << true;
    QTest::newRow("decompressFromDevice-memory") << "decompressFromDevice-memory" << true;
    // One that decompresses a file if it has to hands on the file that it does not have to
    QTest::newRow("decompressIfNeeded") << "decompressIfNeeded" << false;
    QTest::newRow("decompressIfNeeded with an output") << "decompressIfNeeded-output" << false;
    // And those that read through compression read what is there
    QTest::newRow("QGCDecompressDevice") << "QGCDecompressDevice" << false;
    QTest::newRow("readFile") << "readFile" << false;
    QTest::newRow("computeFileHash") << "computeFileHash" << false;
}

void QGCCompressionTest::_testDecompressPlainInput()
{
    QFETCH(QString, api);
    QFETCH(bool, refused);
    using QGCCompression::Error;

    // Only its name says that this file is compressed, as after a download that was decompressed on the way
    const QByteArray plain("{\"name\": \"not compressed\"}\n");
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("plain.json.gz");
    const QString outputPath = tempDir.filePath("output.json");
    QVERIFY(TestFixtures::writeFile(inputPath, plain));
    const auto expectNotCompressed = [this]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Input is not compressed"));
    };

    QGCCompression::Outcome outcome;
    Results results;
    if (refused) {
        // Data has no name, and this has no signature to detect a format by, so the caller says what it is taken for
        const bool isData = (api == "decompressData");
        expectNotCompressed();
        outcome = _call(api,
                        {.path = inputPath,
                         .output = outputPath,
                         .format = isData ? QGCCompression::Format::GZIP : QGCCompression::Format::Auto},
                        &results);
        verifyExpectedLogMessage();
    } else if (api == "decompressIfNeeded") {
        expectNotCompressed();
        QCOMPARE(QGCCompression::decompressIfNeeded(inputPath), inputPath);
        verifyExpectedLogMessage();
        expectNotCompressed();
        QCOMPARE(QGCCompression::decompressIfNeeded(inputPath, outcome), inputPath);
        verifyExpectedLogMessage();
        QCOMPARE(outcome.error, Error::None);
    } else if (api == "decompressIfNeeded-output") {
        // The file is the result, so it is not removed as a compressed file is once it has been decompressed
        expectNotCompressed();
        QCOMPARE(QGCCompression::decompressIfNeeded(inputPath, outputPath, true), inputPath);
        verifyExpectedLogMessage();
        expectNotCompressed();
        QCOMPARE(QGCCompression::decompressIfNeeded(inputPath, outcome, outputPath, true), inputPath);
        verifyExpectedLogMessage();
        QCOMPARE(outcome.error, Error::None);
    } else if (api == "QGCDecompressDevice") {
        QGCDecompressDevice decompressor(inputPath);
        QVERIFY2(decompressor.open(QIODevice::ReadOnly), qPrintable(decompressor.errorString()));
        QCOMPARE(decompressor.filterName(), QStringLiteral("none"));
        QCOMPARE(decompressor.readAll(), plain);
    } else if (api == "readFile") {
        QString readError;
        QCOMPARE(QGCCompression::readFile(inputPath, &readError), plain);
        QVERIFY2(readError.isEmpty(), qPrintable(readError));
    } else if (api == "computeFileHash") {
        QCOMPARE(QGCCompression::computeFileHash(inputPath),
                 QString::fromLatin1(QCryptographicHash::hash(plain, QCryptographicHash::Sha256).toHex()));
    } else {
        QFAIL("Unknown row");
    }

    if (refused) {
        QCOMPARE(outcome.error, Error::UnsupportedFormat);
        QVERIFY2(outcome.message.endsWith(": the input is not compressed"), qPrintable(outcome.message));
        // What was read is on record: the input as it is, which no decompression filter took
        QCOMPARE(outcome.filterName, QStringLiteral("none"));
        // And it is not handed back as the data
        QVERIFY(results.isEmpty());
    }
    // A copy of the input is not a decompressed file: nothing is written, and the input stays
    QCOMPARE(_entriesBelow(tempDir.path()), QStringList{QStringLiteral("plain.json.gz")});
    QCOMPARE(TestFixtures::readFile(inputPath), plain);
}

void QGCCompressionTest::_testDecompressFileCopiesPlainInput_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("limitOffset");  // The limit is the size of the content and this; none if negative
    QTest::addColumn<bool>("cancel");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QGCCompression::Error>("expectedError");

    using QGCCompression::Error;
    constexpr int noLimit = -1;

    QTest::newRow("compressed") << "compressed" << noLimit << false << "" << Error::None;
    // What decompressFile() refuses unless it is told to copy: a file that only has the name of a compressed one
    QTest::newRow("not compressed") << "plain" << noLimit << false << "" << Error::None;
    // The copy is held to the limit and can be cancelled, as a decompression is
    QTest::newRow("not compressed, at the limit") << "plain" << 0 << false << "" << Error::None;
    QTest::newRow("not compressed, over the limit")
        << "plain" << -2 << false << "Size limit exceeded" << Error::SizeLimitExceeded;
    QTest::newRow("not compressed, cancelled") << "plain" << noLimit << true << "" << Error::Cancelled;
    // Damage is no reason to take a file as it is: it is compressed, as its name says
    QTest::newRow("compressed and truncated")
        << "truncated" << noLimit << false << "(?i)truncated gzip input" << Error::InvalidArchive;
}

void QGCCompressionTest::_testDecompressFileCopiesPlainInput()
{
    QFETCH(QString, input);
    QFETCH(int, limitOffset);
    QFETCH(bool, cancel);
    QFETCH(QString, warning);
    QFETCH(QGCCompression::Error, expectedError);
    using QGCCompression::Error;

    const QByteArray content = QByteArray("{\"name\": \"content of the file\"}\n").repeated(100);
    const QByteArray compressed = TestFixtures::gzipMember(content);
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("manifest.json.gz");
    const QString outputPath = tempDir.filePath("manifest.json");
    if (input == "plain") {
        QVERIFY(TestFixtures::writeFile(inputPath, content));
    } else if (input == "compressed") {
        QVERIFY(TestFixtures::writeFile(inputPath, compressed));
    } else {
        QVERIFY(TestFixtures::writeFile(inputPath, compressed.left(compressed.size() / 2)));
    }
    const qint64 limit = (limitOffset == -1) ? 0 : (content.size() + 1 + limitOffset);
    QGCCompression::ProgressCallback progress;
    if (cancel) {
        progress = [](qint64, qint64) { return false; };
    }

    if (!warning.isEmpty()) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(warning));
    }
    const QGCCompression::Outcome outcome = QGCCompression::decompressFile(
        inputPath, outputPath, QGCCompression::Format::Auto, progress, limit, QGCCompression::PlainInput::Copy);
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(outcome.error, expectedError);
    const bool written = (expectedError == Error::None);
    // The output holds the content, decompressed or as it was, and a failure leaves none. The input stays.
    const QStringList inputOnly{QStringLiteral("manifest.json.gz")};
    const QStringList outputAndInput{QStringLiteral("manifest.json"), QStringLiteral("manifest.json.gz")};
    QCOMPARE(_entriesBelow(tempDir.path()), written ? outputAndInput : inputOnly);
    if (written) {
        QCOMPARE(TestFixtures::readFile(outputPath), content);
        // The filter tells a copy from a decompression
        QCOMPARE(outcome.filterName, (input == "plain") ? QStringLiteral("none") : QStringLiteral("gzip"));
    }
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
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    _makeStreamedZip({{.name = "first.txt", .data = QByteArray(64 * 1024, 'a')},
                                                      {.name = "second.txt", .data = QByteArray(64 * 1024, 'b')}},
                                                     true)));
    const qint64 archiveSize = QFileInfo(archivePath).size();

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

    // extractArchive() is told the format, which the other two take no parameter for
    QVERIFY(_call(
        api, {.path = archivePath, .output = outputPath, .format = QGCCompression::Format::ZIP, .progress = progress}));
    QVERIFY2(progressValid, "Progress values were invalid");
    QVERIFY(callCount > 0);
    QCOMPARE(_entriesBelow(outputPath), (QStringList{QStringLiteral("first.txt"), QStringLiteral("second.txt")}));
}

// ============================================================================
// Format Reported by an Operation
// ============================================================================
void QGCCompressionTest::_testDetectedFormat_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("entryName");
    QTest::addColumn<bool>("succeeds");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("filter");

    const QString gz = QStringLiteral(":/unittest/manifest.json.gz");
    const QString xz = QStringLiteral(":/unittest/manifest.json.xz");
    const QString zst = QStringLiteral(":/unittest/manifest.json.zst");
    const QString zip = QStringLiteral(":/unittest/manifest.json.zip");
    const QString sevenZip = QStringLiteral(":/unittest/manifest.json.7z");
    const auto addRow = [](const char* api, const QString& resource, const char* format, const char* filter) {
        QTest::newRow(api) << api << resource << "manifest.json" << true << format << filter;
    };

    addRow("decompressFile", gz, "^raw$", "gzip");
    addRow("decompressData", xz, "^raw$", "xz");
    addRow("decompressFromDevice-file", zst, "^raw$", "zstd");
    addRow("decompressFromDevice-memory", gz, "^raw$", "gzip");
    addRow("extractArchive", zip, "^ZIP", "none");
    addRow("extractArchiveAtomic", sevenZip, "^7-Zip", "none");
    addRow("extractFromDevice", zip, "^ZIP", "none");
    addRow("extractArchiveFiltered", zip, "^ZIP", "none");
    addRow("extractFileDataFromDevice", sevenZip, "^7-Zip", "none");
    addRow("extractFile", zip, "^ZIP", "none");
    addRow("extractFileData", sevenZip, "^7-Zip", "none");
    addRow("extractFiles", zip, "^ZIP", "none");
    QTest::newRow("extractByPattern") << "extractByPattern" << sevenZip << "*.json" << true << "^7-Zip" << "none";
    addRow("listArchive", zip, "^ZIP", "none");
    addRow("listArchiveDetailed", sevenZip, "^7-Zip", "none");
    addRow("getArchiveStats", zip, "^ZIP", "none");
    addRow("validateArchive", sevenZip, "^7-Zip", "none");
    addRow("fileExists", zip, "^ZIP", "none");
    // The archive was opened and its first header read, which is what tells the format
    QTest::newRow("fileExists of an absent entry")
        << "fileExists" << sevenZip << "absent.txt" << false << "^7-Zip" << "none";
}

void QGCCompressionTest::_testDetectedFormat()
{
    QFETCH(QString, api);
    QFETCH(QString, resource);
    QFETCH(QString, entryName);
    QFETCH(bool, succeeds);
    QFETCH(QString, format);
    QFETCH(QString, filter);

    QTemporaryDir tempDir;
    Results results;
    const QGCCompression::Outcome outcome =
        _call(api, {.path = resource, .output = tempDir.filePath("output"), .names = {entryName}}, &results);

    QCOMPARE(static_cast<bool>(outcome), succeeds);
    QVERIFY2(QRegularExpression(format).match(outcome.formatName).hasMatch(), qPrintable(outcome.formatName));
    QCOMPARE(outcome.filterName, filter);
    // What an entry point gives back is what it read, and not what it was handed to fill
    if (api == "listArchive") {
        QCOMPARE(results.names, QStringList{entryName});
    } else if (api == "listArchiveDetailed") {
        QCOMPARE(results.entries.size(), 1);
        QCOMPARE(results.entries.constFirst().name, entryName);
    } else if (api == "getArchiveStats") {
        QCOMPARE(results.stats.fileCount, 1);
    } else if (api.contains("Data") || api.endsWith("-memory")) {
        QVERIFY2(results.data.contains("\"name\""), results.data.constData());
    }
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
    const bool isStream = _isStreamEntryPoint(api);
    const QByteArray data =
        isStream ? QByteArray::fromHex("1f8b08000000000000ff") : QByteArray("This is not an archive of any format");
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(isStream ? "cut.gz" : "garbage.zip");
    const QString outputPath = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(inputPath, data));

    for (int i = 0; i < reports; ++i) {
        expectLogMessage(
            "Utilities.QGClibarchive", QtWarningMsg,
            QRegularExpression("truncated gzip input|Unrecognized archive format|Archive is empty or invalid"));
    }
    Results results;
    const QGCCompression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);
    for (int i = 0; i < reports; ++i) {
        verifyExpectedLogMessage();
    }

    QVERIFY(!outcome);
    QVERIFY(results.isEmpty());
    QCOMPARE(outcome.formatName, QString());
    QCOMPARE(outcome.filterName, QString());
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================
void QGCCompressionTest::_testCorruptArchive()
{
    QTemporaryDir tempDir;
    // Create a file with garbage data pretending to be a ZIP
    const QString corruptZip = tempDir.path() + "/corrupt.zip";
    QVERIFY(TestFixtures::writeFile(corruptZip, "This is not a valid ZIP file - just garbage data!"));
    // Attempt to extract - should fail gracefully
    // extractAnyArchive opens the archive twice: once in getArchiveStats, once directly
    const QString outputPath = tempDir.path() + "/corrupt_output";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    QGCCompression::Outcome outcome = QGCCompression::extractArchive(corruptZip, outputPath);
    QVERIFY2(!outcome, "Extracting corrupt archive should fail");
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(outcome.error, QGCCompression::Error::InvalidArchive);
    // Attempt to list - should return empty or fail
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*Unrecognized archive format"));
    QStringList entries{QStringLiteral("to be replaced")};
    outcome = QGCCompression::listArchive(corruptZip, entries);
    QVERIFY2(entries.isEmpty(), "Listing corrupt archive should return empty list");
    verifyExpectedLogMessage();
    QCOMPARE(outcome.error, QGCCompression::Error::InvalidArchive);
    // Test corrupt compressed file with valid GZIP header but truncated data
    const QString corruptGz = tempDir.path() + "/corrupt.gz";
    const char corruptGzipData[] = {
        '\x1f', '\x8b',                  // GZIP magic
        '\x08',                          // Deflate method
        '\x00',                          // Flags
        '\x00', '\x00', '\x00', '\x00',  // Modification time
        '\x00',                          // Extra flags
        '\xff',                          // OS (unknown)
    };
    QVERIFY(TestFixtures::writeFile(corruptGz, QByteArray(corruptGzipData, sizeof(corruptGzipData))));
    const QString decompressedPath = tempDir.path() + "/corrupt_decompressed";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to open file.*truncated gzip input"));
    outcome = QGCCompression::decompressFile(corruptGz, decompressedPath);
    QVERIFY2(!outcome, "Decompressing truncated GZIP file should fail");
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
    const QGCCompression::Outcome noArchive = QGCCompression::extractArchive(nonExistent + ".zip", outputPath);
    verifyExpectedLogMessage();
    QCOMPARE(noArchive.error, QGCCompression::Error::FileNotFound);
    QVERIFY(!noArchive.message.isEmpty());
    // Test listArchive with non-existent file
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    QStringList entries{QStringLiteral("to be replaced")};
    QVERIFY(!QGCCompression::listArchive(nonExistent + ".zip", entries));
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
        const QString scriptPath = binPath + QLatin1Char('/') + QLatin1String(program);
        QVERIFY(TestFixtures::writeFile(
            scriptPath, "#!/bin/sh\n: > '" + QFile::encodeName(markerPath) + "'\nwhile read -r _; do :; done\n"));
        QVERIFY(QFile::setPermissions(scriptPath,
                                      QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    }
    const QByteArray originalPath = qgetenv("PATH");
    qputenv("PATH", QFile::encodeName(binPath));
    const auto restorePath = qScopeGuard([&originalPath]() { qputenv("PATH", originalPath); });

    const QByteArray padding(32, '\0');

    // lrzip is always decoded by an external program
    const QString lrzipTar = tempDir.filePath("lrzip.tar");
    QVERIFY(TestFixtures::writeFile(lrzipTar, QByteArray("LRZI\x00\x06", 6) + padding));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Unrecognized archive format"));
    QStringList lrzipEntries;
    QVERIFY(!QGCCompression::listArchive(lrzipTar, lrzipEntries));
    verifyExpectedLogMessage();
    QVERIFY(lrzipEntries.isEmpty());

#ifndef QGC_ENABLE_BZIP2
    // Each way of opening a single stream refuses one this build cannot decode
    const QByteArray bzip2Stream = QByteArray("BZh91AY&SY") + padding;
    const QString bzip2Path = tempDir.filePath("stream.bz2");
    QVERIFY(TestFixtures::writeFile(bzip2Path, bzip2Stream));
    QBuffer bzip2Device;
    bzip2Device.setData(bzip2Stream);
    QVERIFY(bzip2Device.open(QIODevice::ReadOnly));
    const auto expectBzip2Refused = [this]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("bzip2.*not supported"));
    };

    QByteArray decoded("to be replaced");
    expectBzip2Refused();
    QCOMPARE(QGCCompression::decompressData(bzip2Stream, decoded).error, QGCCompression::Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(decoded.isEmpty());

    expectBzip2Refused();
    QCOMPARE(QGCCompression::decompressFile(bzip2Path, tempDir.filePath("stream")).error,
             QGCCompression::Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(!QFile::exists(tempDir.filePath("stream")));

    decoded = "to be replaced";
    expectBzip2Refused();
    QCOMPARE(QGCCompression::decompressFromDevice(&bzip2Device, decoded).error,
             QGCCompression::Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(decoded.isEmpty());

    expectBzip2Refused();
    QCOMPARE(QGCCompression::decompressFromDevice(&bzip2Device, tempDir.filePath("stream")).error,
             QGCCompression::Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(!QFile::exists(tempDir.filePath("stream")));

    // The device does not log: the reason is in its error string
    QGCDecompressDevice bzip2Decompressor(&bzip2Device);
    QVERIFY(!bzip2Decompressor.open(QIODevice::ReadOnly));
    QCOMPARE(bzip2Decompressor.errorString(), QStringLiteral("bzip2 decompression is not supported by this build"));
#endif
#ifndef QGC_ENABLE_LZ4
    QByteArray lz4Decoded("to be replaced");
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("LZ4.*not supported"));
    QCOMPARE(QGCCompression::decompressData(QByteArray("\x04\x22\x4D\x18\x64\x40\xA7", 7) + padding, lz4Decoded).error,
             QGCCompression::Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(lz4Decoded.isEmpty());
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
    const QByteArray zip = TestFixtures::readFile(QStringLiteral(":/unittest/manifest.json.zip"));
    const QByteArray sevenZip = TestFixtures::readFile(QStringLiteral(":/unittest/manifest.json.7z"));

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
    QVERIFY(TestFixtures::writeFile(archivePath, data));

    // Each call reports the damage once, where it meets it: at an entry header or in entry data
    const QRegularExpression readFailure(
        "Failed to read archive entry|Failed to read data|Validation failed for entry");
    const auto expectReadFailure = [this, &readFailure]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, readFailure);
    };

    using QGCCompression::Error;

    expectReadFailure();
    QCOMPARE(QGCCompression::validateArchive(archivePath).error, Error::InvalidArchive);
    verifyExpectedLogMessage();

    // An archive without entries lists as empty too: the error is what tells a damaged one
    QStringList names{QStringLiteral("to be replaced")};
    expectReadFailure();
    QCOMPARE(QGCCompression::listArchive(archivePath, names).error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    QVERIFY(names.isEmpty());

    QList<QGCCompression::ArchiveEntry> entries{QGCCompression::ArchiveEntry{}};
    expectReadFailure();
    QCOMPARE(QGCCompression::listArchiveDetailed(archivePath, entries).error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    QVERIFY(entries.isEmpty());

    QGCCompression::ArchiveStats stats;
    stats.totalEntries = 1;
    expectReadFailure();
    QCOMPARE(QGCCompression::getArchiveStats(archivePath, stats).error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    QCOMPARE(stats.totalEntries, 0);

    // Once for the size pre-check and once for the extraction itself
    expectReadFailure();
    expectReadFailure();
    QCOMPARE(QGCCompression::extractArchive(archivePath, tempDir.filePath("output")).error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    const QString atomicOutput = tempDir.filePath("atomic_output");
    expectReadFailure();
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Archive is empty or invalid"));
    QCOMPARE(QGCCompression::extractArchiveAtomic(archivePath, atomicOutput).error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!QFileInfo::exists(atomicOutput));

    // A search for an entry the archive does not have reads the headers up to the damage, which is
    // then what is reported: the entry is not said to be missing
    const QString absent = QStringLiteral("absent.txt");
    const auto rejectAll = [](const QGCCompression::ArchiveEntry&) { return false; };

    struct Call
    {
        const char* api;
        int reports;
    };

    for (const Call& search :
         {Call{"extractFile", 1}, Call{"extractFileData", 1}, Call{"extractFiles", 1}, Call{"extractByPattern", 1},
          Call{"fileExists", 1},
          // Once for the size pre-check and once for the extraction itself
          Call{"extractArchiveFiltered", 2}, Call{"extractFromDevice", 1}, Call{"extractFileDataFromDevice", 1}}) {
        for (int i = 0; i < search.reports; ++i) {
            expectReadFailure();
        }
        Results results;
        QVERIFY2(!_call(search.api,
                        {.path = archivePath,
                         .output = tempDir.filePath(QLatin1String(search.api)),
                         .names = {absent},
                         .filter = rejectAll},
                        &results),
                 search.api);
        QVERIFY2(results.isEmpty(), search.api);
        for (int i = 0; i < search.reports; ++i) {
            verifyExpectedLogMessage();
        }
    }
    QVERIFY(!QFileInfo::exists(tempDir.filePath("extractFile")));

    expectReadFailure();
    QGCArchiveFile absentEntry(archivePath, absent);
    QVERIFY(!absentEntry.open(QIODevice::ReadOnly));
    verifyExpectedLogMessage();
    QVERIFY2(!absentEntry.errorString().contains("not found"), qPrintable(absentEntry.errorString()));

    // fileExists() answers no for two reasons, and the error is all that tells them apart
    expectReadFailure();
    QCOMPARE(QGCCompression::fileExists(archivePath, absent).error, Error::InvalidArchive);
    verifyExpectedLogMessage();

    if (cutEntry.isEmpty()) {
        return;
    }

    // An entry whose header comes before the damage is there
    QVERIFY(QGCCompression::fileExists(archivePath, cutEntry));

    // An entry whose data is cut is found, and fails while it is copied. Handed over alone, as a file or as
    // data, no part of it is kept
    const auto expectDataFailure = [this]() {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Read error"));
    };
    const QString cutOutput = tempDir.filePath("cut_output");

    expectDataFailure();
    QVERIFY(!QGCCompression::extractFile(archivePath, cutEntry, cutOutput));
    verifyExpectedLogMessage();
    QVERIFY(!QFileInfo::exists(cutOutput));

    QByteArray cutData("to be replaced");
    expectDataFailure();
    QVERIFY(!QGCCompression::extractFileData(archivePath, cutEntry, cutData));
    verifyExpectedLogMessage();
    QVERIFY(cutData.isEmpty());

    // The functions that select what they extract into a directory leave what extractArchive() leaves: the
    // entry, with what could be read of it
    for (const Call& selection : {Call{"extractFiles", 1}, Call{"extractByPattern", 1},
                                  // Once for the size pre-check and once for the extraction itself
                                  Call{"extractArchiveFiltered", 2}}) {
        const QString outputDir = tempDir.filePath(QStringLiteral("cut_") + QLatin1String(selection.api));
        for (int i = 0; i < selection.reports; ++i) {
            expectReadFailure();
        }
        const QGCCompression::Outcome outcome =
            _call(selection.api, {.path = archivePath, .output = outputDir, .names = {cutEntry}});
        for (int i = 0; i < selection.reports; ++i) {
            verifyExpectedLogMessage();
        }
        QVERIFY2(outcome.error == Error::InvalidArchive, selection.api);
        QVERIFY2(QFileInfo::exists(outputDir + QLatin1Char('/') + cutEntry), selection.api);
    }
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
    const QByteArray whole = TestFixtures::readFile(inputPath);
    const QByteArray cut = whole.left(whole.size() * 5 / 8);
    QVERIFY(TestFixtures::writeFile(inputPath, cut));
    QBuffer device;
    device.setData(cut);
    QVERIFY(device.open(QIODevice::ReadOnly));

    const QRegularExpression fileFailure("Decompression error.*truncated");
    const QRegularExpression memoryFailure("Read error.*truncated");
    if (api == "readFile") {
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
    } else if (api == "QGCDecompressDevice") {
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
    } else {
        // The functions that write a file and those that return data log the damage with their own words
        const bool toMemory = api.endsWith("-memory") || (api == "decompressData");
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, toMemory ? memoryFailure : fileFailure);
        Results results;
        QCOMPARE(_call(api, {.path = inputPath, .output = outputPath}, &results).error,
                 QGCCompression::Error::InvalidArchive);
        verifyExpectedLogMessage();
        // The members before the cut are not handed back as the data
        QVERIFY(results.isEmpty());
    }

    // Nothing of the damaged stream is kept
    QCOMPARE(_entriesBelow(tempDir.path()), QStringList{QStringLiteral("cut.gz")});
}

namespace {

/// Feeds bytes to a reader that cannot seek, like a network reply
class QGCCompressionTestSequentialDevice : public QIODevice
{
public:
    /// What a read gets once the bytes are used up
    enum class End
    {
        NoBytes,  ///< 0
        MinusOne  ///< -1, which is what a network reply that has finished answers
    };

    /// @param failAt Offset at which reads fail, with the bytes after it still to come (-1 = never)
    explicit QGCCompressionTestSequentialDevice(const QByteArray& data, End end = End::NoBytes, qint64 failAt = -1)
        : _data(data)
        , _end(end)
        , _failAt(failAt)
    {}

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return (_data.size() - _offset) + QIODevice::bytesAvailable(); }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        if (_offset == _failAt) {
            return -1;
        }
        const qint64 limit = (_failAt >= 0) ? _failAt : _data.size();
        const qint64 count = qMin(maxSize, limit - _offset);
        if ((count == 0) && (_end == End::MinusOne)) {
            return -1;
        }
        memcpy(data, _data.constData() + _offset, static_cast<size_t>(count));
        _offset += count;
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    const QByteArray _data;
    const End _end;
    const qint64 _failAt;
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

    QCOMPARE(TestFixtures::readFile(tempDir.filePath(QString::fromLatin1(name))), content);
}

void QGCCompressionTest::_testSequentialDeviceEnd_data()
{
    QTest::addColumn<QString>("resource");

    for (const char* extension : {"gz", "xz", "zst"}) {
        QTest::newRow(extension) << QStringLiteral(":/unittest/manifest.json.") + QLatin1String(extension);
    }
}

void QGCCompressionTest::_testSequentialDeviceEnd()
{
    QFETCH(QString, resource);
    using End = QGCCompressionTestSequentialDevice::End;
    const QByteArray stream = TestFixtures::readFile(resource);
    QByteArray content;
    QVERIFY(QGCCompression::decompressData(stream, content));
    QVERIFY(!content.isEmpty());

    // A read of -1 is the end of a device that has no more bytes to give
    QGCCompressionTestSequentialDevice finished(stream, End::MinusOne);
    QVERIFY(finished.open(QIODevice::ReadOnly));
    QByteArray fromDevice;
    QCOMPARE(QGCCompression::decompressFromDevice(&finished, fromDevice).error, QGCCompression::Error::None);
    QCOMPARE(fromDevice, content);
}

void QGCCompressionTest::_testSequentialDeviceReadError()
{
    using End = QGCCompressionTestSequentialDevice::End;
    const QByteArray tar = _makeTar({{.name = "first.txt", .data = "1"}, {.name = "second.txt", .data = "2"}});

    // A read of -1 from a device that still has bytes is an error. Taken for the end of the device, this one
    // would be read as an archive of first.txt alone: it fails where the header of second.txt starts.
    QGCCompressionTestSequentialDevice broken(tar, End::NoBytes, 2 * 512);
    QVERIFY(broken.open(QIODevice::ReadOnly));
    QTemporaryDir tempDir;
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression("Failed to read archive entry.*Truncated tar archive"));
    QCOMPARE(QGCCompression::extractFromDevice(&broken, tempDir.filePath("out")).error,
             QGCCompression::Error::InvalidArchive);
    verifyExpectedLogMessage();
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

    const QByteArray tar = _makeTar({{.name = "empty.txt"},
                                     {.name = "full.txt", .data = QByteArray(600, 'a')},
                                     {.name = "last.txt", .data = "z"},
                                     {.name = "soft.txt", .type = '2', .linkName = "full.txt"},
                                     {.name = "hard.txt", .type = '1', .linkName = "full.txt"},
                                     {.name = "pipe", .type = '6'},
                                     {.name = "folder/", .type = '5'}});
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("entries.tar");
    QVERIFY(TestFixtures::writeFile(archivePath, tar));
    // Cut inside the data of full.txt, which follows the headers of empty.txt and of full.txt itself
    const QString damagedPath = tempDir.filePath("damaged.tar");
    QVERIFY(TestFixtures::writeFile(damagedPath, tar.left(2 * 512 + 100)));

    // What the entry holds is replaced by each call
    QByteArray data;
    const auto extract = [fromDevice, &data](const QString& path, const char* entryName, qint64 limit = 0) {
        data = "to be replaced";
        if (!fromDevice) {
            return QGCCompression::extractFileData(path, entryName, data, QGCCompression::Format::Auto, limit);
        }
        QFile device(path);
        (void) device.open(QIODevice::ReadOnly);
        return QGCCompression::extractFileDataFromDevice(&device, entryName, data, limit);
    };

    // An entry without data gives nothing either, so the error is what tells it from each kind of failure
    QCOMPARE(extract(archivePath, "empty.txt").error, Error::None);
    QVERIFY(data.isEmpty());

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    QCOMPARE(extract(archivePath, "absent.txt").error, Error::FileNotInArchive);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    // The search for the entry runs into the damage: the archive is not said to be without it
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read archive entry"));
    QCOMPARE(extract(damagedPath, "absent.txt").error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    // The entry is found, and its data is cut
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Read error"));
    QCOMPARE(extract(damagedPath, "full.txt").error, Error::InvalidArchive);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    // An entry before the damage is read as from a whole archive
    QCOMPARE(extract(damagedPath, "empty.txt").error, Error::None);
    QVERIFY(data.isEmpty());

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QCOMPARE(extract(archivePath, "full.txt", 100).error, Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    // A link has no data of its own, and a special file and a directory have none: none of them is
    // extracted, and none is an entry without data
    struct SkippedEntry
    {
        const char* name;
        const char* report;
    };

    for (const SkippedEntry& skipped :
         {SkippedEntry{"soft.txt", "Skipping symlink: soft.txt -> full.txt"},
          SkippedEntry{"hard.txt", "Skipping hard link: hard.txt -> full.txt"},
          SkippedEntry{"pipe", "Skipping FIFO: pipe"}, SkippedEntry{"folder/", "Skipping directory: folder/"}}) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(skipped.report));
        QCOMPARE(extract(archivePath, skipped.name).error, Error::UnsupportedFormat);
        verifyExpectedLogMessage();
        QVERIFY(data.isEmpty());
    }

    // An entry with data, which comes after all of them in the archive
    QCOMPARE(extract(archivePath, "last.txt").error, Error::None);
    QCOMPARE(data, QByteArray("z"));
}

void QGCCompressionTest::_testExtractToFileErrors_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("archive");
    QTest::addColumn<QString>("entryName");
    QTest::addColumn<bool>("blockedOutput");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QGCCompression::Error>("expectedError");
    QTest::addColumn<bool>("entryLeft");  // Whether the entry is in the output afterwards

    using QGCCompression::Error;

    for (const char* api : {"extractFile", "extractFiles", "extractByPattern"}) {
        const bool byName = (qstrcmp(api, "extractByPattern") != 0);
        const bool oneFile = (qstrcmp(api, "extractFile") == 0);
        const auto addRow = [api](const char* name) -> QTestData& { return QTest::addRow("%s %s", api, name) << api; };

        addRow("extracted") << "whole" << "last.txt" << false << "" << Error::None << true;
        // A pattern that matches nothing is not worth a warning
        addRow("absent") << "whole" << "absent.txt" << false << (byName ? "File not found in archive" : "")
                         << Error::FileNotInArchive << false;
        addRow("damaged before the entry")
            << "damaged" << "absent.txt" << false << "Failed to read archive entry" << Error::InvalidArchive << false;
        // The file that is given a path of its own is not written. Extracted into a directory, the entry stays
        // with what could be read of it, as extractArchive() leaves it
        addRow("damaged inside the entry")
            << "damaged" << "full.txt" << false << (oneFile ? "Read error" : "Failed to read data")
            << Error::InvalidArchive << !oneFile;
        addRow("not an archive") << "garbage" << "last.txt" << false << "Unrecognized archive format"
                                 << Error::InvalidArchive << false;
        // What cannot be made is the output directory, or the file for the function that is given its path
        addRow("output cannot be written")
            << "whole" << "last.txt" << true
            << (oneFile ? "Failed to open output file" : "Failed to create output directory") << Error::IoError
            << false;
        // A link is no file to write to a path of its own. In a directory it is created, as extractArchive()
        // creates the symbolic links that stay inside it
        addRow("link") << "whole" << "soft.txt" << false << (oneFile ? "Skipping symlink: soft.txt" : "")
                       << (oneFile ? Error::UnsupportedFormat : Error::None) << !oneFile;
        // Asked for by name, the directory fails the call: it is not a file without data. A pattern passes
        // over it without a word: the files in a directory are what makes it.
        addRow("directory") << "whole" << "folder/" << false << (byName ? "Skipping directory: folder/" : "")
                            << (byName ? Error::UnsupportedFormat : Error::FileNotInArchive) << false;
    }
}

void QGCCompressionTest::_testExtractToFileErrors()
{
    QFETCH(QString, api);
    QFETCH(QString, archive);
    QFETCH(QString, entryName);
    QFETCH(bool, blockedOutput);
    QFETCH(QString, warning);
    QFETCH(QGCCompression::Error, expectedError);
    QFETCH(bool, entryLeft);
    using QGCCompression::Error;

    const QByteArray tar = _makeTar({{.name = "empty.txt"},
                                     {.name = "full.txt", .data = QByteArray(600, 'a')},
                                     {.name = "soft.txt", .type = '2', .linkName = "full.txt"},
                                     {.name = "folder/", .type = '5'},
                                     {.name = "last.txt", .data = "z"}});
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(archive + ".tar");
    if (archive == "whole") {
        QVERIFY(TestFixtures::writeFile(archivePath, tar));
    } else if (archive == "damaged") {
        // Cut inside the data of full.txt, which follows the headers of empty.txt and of full.txt itself
        QVERIFY(TestFixtures::writeFile(archivePath, tar.left(2 * 512 + 100)));
    } else {
        QVERIFY(TestFixtures::writeFile(archivePath, "This is not an archive of any format"));
    }
    // A directory cannot be made below a file
    const QString blockerPath = tempDir.filePath("blocker");
    QVERIFY(TestFixtures::writeFile(blockerPath, "in the way"));
    const QString outputDir = blockedOutput ? (blockerPath + "/output") : tempDir.filePath("output");

    if (!warning.isEmpty()) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(warning));
    }
    const QGCCompression::Outcome extracted =
        _call(api, {.path = archivePath, .output = outputDir, .names = {entryName}});
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(extracted.error, expectedError);
    // The message ends with which of the two it is that failed
    if (expectedError == Error::IoError) {
        QVERIFY2(extracted.message.endsWith(": the output cannot be written"), qPrintable(extracted.message));
    } else if (expectedError == Error::InvalidArchive) {
        QVERIFY2(extracted.message.endsWith(": the archive cannot be read or is damaged"),
                 qPrintable(extracted.message));
    }
    QCOMPARE(_entriesBelow(tempDir.filePath("output")), entryLeft ? QStringList{entryName} : QStringList());
    QCOMPARE(TestFixtures::readFile(blockerPath), QByteArray("in the way"));
}

void QGCCompressionTest::_testDecompressDataErrors()
{
    using QGCCompression::Error;

    const QByteArray compressed = TestFixtures::readFile(QStringLiteral(":/unittest/manifest.json.gz"));

    QByteArray data;
    QCOMPARE(QGCCompression::decompressData(compressed, data).error, Error::None);
    QVERIFY(!data.isEmpty());

    // The data can be given back in the array it was read from
    QByteArray inPlace = compressed;
    QVERIFY(QGCCompression::decompressData(inPlace, inPlace));
    QCOMPARE(inPlace, data);

    // Data that is refused leaves nothing in the output
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Cannot decompress empty data"));
    QCOMPARE(QGCCompression::decompressData(QByteArray(), data).error, Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    data = compressed;
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Could not detect format"));
    QCOMPARE(QGCCompression::decompressData(QByteArray("not a compressed stream"), data).error,
             Error::UnsupportedFormat);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());
}

void QGCCompressionTest::_testOutputWithoutPermission_data()
{
    QTest::addColumn<QString>("api");
    // Whether the output directory is there, or has to be made in the directory that may not be written to
    QTest::addColumn<bool>("outputExists");
    QTest::addColumn<QString>("warning");

    const char* const noFile = "Failed to open output file";
    const char* const noEntry = "Failed to write header";
    QTest::newRow("extractArchive") << "extractArchive" << true << noEntry;
    QTest::newRow("extractFromDevice") << "extractFromDevice" << true << noEntry;
    QTest::newRow("extractArchiveAtomic") << "extractArchiveAtomic" << true << "Failed to create staging directory";
    QTest::newRow("extractArchiveFiltered") << "extractArchiveFiltered" << true << noEntry;
    QTest::newRow("extractFile") << "extractFile" << true << noFile;
    QTest::newRow("extractFiles") << "extractFiles" << true << noEntry;
    QTest::newRow("extractByPattern") << "extractByPattern" << true << noEntry;
    QTest::newRow("decompressFile") << "decompressFile" << true << noFile;
    QTest::newRow("decompressFile of an empty stream")
        << "decompressFile-empty" << true << "Failed to write output file";
    QTest::newRow("decompressFromDevice") << "decompressFromDevice-file" << true << noFile;

    const char* const noDirectory = "Failed to create output directory";
    QTest::newRow("extractArchive, no directory") << "extractArchive" << false << noDirectory;
    QTest::newRow("extractFromDevice, no directory") << "extractFromDevice" << false << noDirectory;
    QTest::newRow("extractArchiveAtomic, no directory")
        << "extractArchiveAtomic" << false << "Failed to create output parent directory";
    QTest::newRow("extractArchiveFiltered, no directory") << "extractArchiveFiltered" << false << noDirectory;
    QTest::newRow("extractFiles, no directory") << "extractFiles" << false << noDirectory;
    QTest::newRow("extractByPattern, no directory") << "extractByPattern" << false << noDirectory;
    // A function that is given the path of a file fails where the file is opened
    QTest::newRow("extractFile, no directory") << "extractFile" << false << noFile;
    QTest::newRow("decompressFile, no directory") << "decompressFile" << false << noFile;
}

void QGCCompressionTest::_testOutputWithoutPermission()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a directory that the permissions of POSIX keep from being written to");
#else
    QFETCH(QString, api);
    QFETCH(bool, outputExists);
    QFETCH(QString, warning);
    using QGCCompression::Error;

    QTemporaryDir tempDir;
    const QString lockedPath = tempDir.filePath("locked");
    QVERIFY(QDir().mkdir(lockedPath));
    QVERIFY(QFile::setPermissions(lockedPath, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    const auto unlock = qScopeGuard([&lockedPath]() {
        QFile::setPermissions(lockedPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });
    if (QFileInfo(lockedPath).isWritable()) {
        QSKIP("This user may write to a directory whatever its permissions are");
    }

    const QString zipPath = QStringLiteral(":/unittest/manifest.json.zip");
    const QString gzipPath = QStringLiteral(":/unittest/manifest.json.gz");
    const QString emptyGzipPath = tempDir.filePath("empty.gz");
    QVERIFY(TestFixtures::writeFile(emptyGzipPath, QByteArray::fromHex("1f8b080000000000000303000000000000000000")));
    const QString outputDir = outputExists ? lockedPath : (lockedPath + "/made");
    const bool emptyStream = api.endsWith("-empty");
    const bool isStream = _isStreamEntryPoint(api);
    // The directory an atomic extraction replaces is made beside the one it extracts to
    const QString archiveOutput = (api == "extractArchiveAtomic") ? (outputDir + "/atomic") : outputDir;
    const QString entry = (api == "extractByPattern") ? QStringLiteral("*.json") : QStringLiteral("manifest.json");

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(warning));
    const QGCCompression::Outcome written = _call(
        emptyStream ? api.chopped(6) : api, {.path = emptyStream ? emptyGzipPath : (isStream ? gzipPath : zipPath),
                                             .output = isStream ? (outputDir + "/manifest.json") : archiveOutput,
                                             .names = {entry}});
    verifyExpectedLogMessage();

    // The input is whole and the output could be written elsewhere: the code tells what to change
    QCOMPARE(written.error, Error::PermissionDenied);
    QVERIFY2(written.message.endsWith(": no permission to write the output"), qPrintable(written.message));
    QCOMPARE(_entriesBelow(lockedPath), QStringList());
#endif
}

void QGCCompressionTest::_testInputWithoutPermission_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("fileName");

    QTest::newRow("listArchive") << "listArchive" << "archive.zip";
    QTest::newRow("extractArchive") << "extractArchive" << "archive.zip";
    QTest::newRow("extractFileData") << "extractFileData" << "archive.zip";
    QTest::newRow("decompressFile") << "decompressFile" << "stream.gz";
    // The content, which would have to tell the format, cannot be read either
    QTest::newRow("name without a format") << "extractArchive" << "archive";
}

void QGCCompressionTest::_testInputWithoutPermission()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a file that the permissions of POSIX keep from being read");
#else
    QFETCH(QString, api);
    QFETCH(QString, fileName);
    using QGCCompression::Error;

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(fileName);
    const QString outputPath = tempDir.filePath("output");
    const bool isStream = fileName.endsWith(".gz");
    QVERIFY(TestFixtures::writeFile(inputPath,
                                    TestFixtures::readFile(isStream ? QStringLiteral(":/unittest/manifest.json.gz")
                                                                    : QStringLiteral(":/unittest/manifest.json.zip"))));
    QVERIFY(QFile::setPermissions(inputPath, QFileDevice::WriteOwner));
    if (QFileInfo(inputPath).isReadable()) {
        QSKIP("This user may read a file whatever its permissions are");
    }

    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("No permission to read"));
    Results results;
    const QGCCompression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);
    verifyExpectedLogMessage();

    // The file is there and may be whole: it is not said to be damaged, or of a format that is not known
    QCOMPARE(outcome.error, Error::PermissionDenied);
    QVERIFY(results.isEmpty());
    QVERIFY(!QFileInfo::exists(outputPath));
#endif
}

void QGCCompressionTest::_testBlockedOutput_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("warning");

    const char* const noDirectory = "Failed to create output directory";
    QTest::newRow("extractArchive") << "extractArchive" << noDirectory;
    QTest::newRow("extractFromDevice") << "extractFromDevice" << noDirectory;
    QTest::newRow("extractArchiveAtomic") << "extractArchiveAtomic" << "Failed to create output parent directory";
    QTest::newRow("extractArchiveFiltered") << "extractArchiveFiltered" << noDirectory;
    QTest::newRow("decompressFile") << "decompressFile" << "Failed to open output file";
    QTest::newRow("decompressFromDevice") << "decompressFromDevice-file" << "Failed to open output file";
}

void QGCCompressionTest::_testBlockedOutput()
{
    QFETCH(QString, api);
    QFETCH(QString, warning);

    // A directory cannot be made below a file, whoever asks
    QTemporaryDir tempDir;
    const QString blockerPath = tempDir.filePath("blocker");
    QVERIFY(TestFixtures::writeFile(blockerPath, "in the way"));
    const QString outputDir = blockerPath + "/output";
    const QString zipPath = QStringLiteral(":/unittest/manifest.json.zip");
    const QString gzipPath = QStringLiteral(":/unittest/manifest.json.gz");
    const bool isStream = _isStreamEntryPoint(api);
    const QString archiveOutput = (api == "extractArchiveAtomic") ? (outputDir + "/atomic") : outputDir;

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression(warning));
    const QGCCompression::Outcome written = _call(
        api,
        {.path = isStream ? gzipPath : zipPath, .output = isStream ? (outputDir + "/manifest.json") : archiveOutput});
    verifyExpectedLogMessage();

    // The input is whole: what failed is the output, and not for lack of permission
    QCOMPARE(written.error, QGCCompression::Error::IoError);
    QCOMPARE(TestFixtures::readFile(blockerPath), QByteArray("in the way"));
}

void QGCCompressionTest::_testOutcomeBeforeInput_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QGCCompression::Error>("expectedError");

    using QGCCompression::Error;

    // These return before they look at their input
    QTest::newRow("decompressIfNeeded of a plain file") << "decompressIfNeeded" << Error::None;
    QTest::newRow("extractFiles of no files") << "extractFiles" << Error::None;
    QTest::newRow("fileExists without a name") << "fileExists" << Error::FileNotInArchive;
    QTest::newRow("extractFile without a name") << "extractFile" << Error::FileNotInArchive;
    QTest::newRow("extractFileData without a name") << "extractFileData" << Error::FileNotInArchive;
    QTest::newRow("extractByPattern without a pattern") << "extractByPattern" << Error::FileNotInArchive;
    QTest::newRow("extractArchiveFiltered without a filter") << "extractArchiveFiltered" << Error::InternalError;
}

void QGCCompressionTest::_testOutcomeBeforeInput()
{
    QFETCH(QString, api);
    QFETCH(QGCCompression::Error, expectedError);

    QTemporaryDir tempDir;
    const QString zipPath = QStringLiteral(":/unittest/manifest.json.zip");
    const QString plainPath = tempDir.filePath("plain.txt");
    QVERIFY(TestFixtures::writeFile(plainPath, "plain text content"));

    QGCCompression::Outcome outcome;
    if (api == "decompressIfNeeded") {
        QCOMPARE(QGCCompression::decompressIfNeeded(plainPath, outcome), plainPath);
    } else if (api == "extractFiles") {
        outcome = QGCCompression::extractFiles(zipPath, {}, tempDir.filePath("output"));
    } else if (api == "fileExists") {
        expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
        outcome = QGCCompression::fileExists(zipPath, QString());
        verifyExpectedLogMessage();
    } else if (api == "extractFile") {
        expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
        outcome = QGCCompression::extractFile(zipPath, QString(), tempDir.filePath("output.txt"));
        verifyExpectedLogMessage();
    } else if (api == "extractFileData") {
        QByteArray data("to be replaced");
        expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
        outcome = QGCCompression::extractFileData(zipPath, QString(), data);
        verifyExpectedLogMessage();
        QVERIFY(data.isEmpty());
    } else if (api == "extractByPattern") {
        outcome = QGCCompression::extractByPattern(zipPath, {}, tempDir.filePath("output"));
    } else if (api == "extractArchiveFiltered") {
        outcome = QGCCompression::extractArchiveFiltered(zipPath, tempDir.filePath("output"), nullptr);
    } else {
        QFAIL("Unknown row");
    }

    // No input was read, so there is no format to tell
    QCOMPARE(outcome.error, expectedError);
    QCOMPARE(outcome.formatName, QString());
    QCOMPARE(outcome.filterName, QString());
}

// ============================================================================
// QIODevice-based Operation Tests
// ============================================================================
void QGCCompressionTest::_testDecompressFromDevice()
{
    QTemporaryDir tempDir;
    // Load compressed data from Qt resource into a QBuffer
    const QByteArray compressedData = TestFixtures::readFile(":/unittest/manifest.json.gz");
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
    const QByteArray content = TestFixtures::readFile(outputPath);
    QVERIFY(content.contains("\"name\""));
    buffer.close();
    // Test decompression to memory
    buffer.setData(compressedData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QByteArray memoryResult;
    QVERIFY2(QGCCompression::decompressFromDevice(&buffer, memoryResult), "Decompress to memory from device failed");
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
    const QByteArray zipData = TestFixtures::readFile(":/unittest/manifest.json.zip");
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
    const QByteArray zipData = TestFixtures::readFile(":/unittest/manifest.json.zip");
    QVERIFY(!zipData.isEmpty());
    // Extract single file from device to memory
    QBuffer buffer;
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QByteArray extracted;
    QVERIFY2(QGCCompression::extractFileDataFromDevice(&buffer, "manifest.json", extracted),
             "Failed to extract file data from device");
    QVERIFY(extracted.contains("\"name\""));
    buffer.close();
    // Test non-existent file
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("File not found in archive"));
    QByteArray notFound = extracted;
    const QGCCompression::Outcome absent =
        QGCCompression::extractFileDataFromDevice(&buffer, "nonexistent.txt", notFound);
    QCOMPARE(absent.error, QGCCompression::Error::FileNotInArchive);
    // The archive was read to its end, which tells its format
    QVERIFY2(absent.formatName.startsWith("ZIP"), qPrintable(absent.formatName));
    QVERIFY(notFound.isEmpty());
    verifyExpectedLogMessage();
    buffer.close();
    // Test empty file name
    buffer.setData(zipData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File name cannot be empty"));
    QByteArray emptyName = extracted;
    QVERIFY(!QGCCompression::extractFileDataFromDevice(&buffer, "", emptyName));
    QVERIFY(emptyName.isEmpty());
    verifyExpectedLogMessage();
    buffer.close();
    // Test null device
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Device is null"));
    QByteArray nullDevice = extracted;
    QVERIFY(!QGCCompression::extractFileDataFromDevice(nullptr, "manifest.json", nullDevice));
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
    firstSource.setData(TestFixtures::gzipMember(firstPayload));
    QVERIFY(firstSource.open(QIODevice::ReadOnly));
    QBuffer secondSource;
    secondSource.setData(TestFixtures::gzipMember(secondPayload));
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
                                     : TestFixtures::gzipMember(payload);
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

void QGCCompressionTest::_testArchiveFileLeavesOutLinks()
{
    QBuffer source;
    source.setData(_makeTar({{.name = "target.txt", .data = "target"},
                             {.name = "soft.txt", .type = '2', .linkName = "target.txt"},
                             {.name = "hard.txt", .type = '1', .linkName = "target.txt"},
                             {.name = "pipe", .type = '6'},
                             {.name = "folder/", .type = '5'}}));
    QVERIFY(source.open(QIODevice::ReadOnly));

    // Reading one of these as a file without data would hide what it is. The device does not log: the
    // reason is in its error string.
    struct SkippedEntry
    {
        const char* name;
        const char* kind;
    };

    for (const SkippedEntry& skipped :
         {SkippedEntry{"soft.txt", "symbolic link"}, SkippedEntry{"hard.txt", "hard link"},
          SkippedEntry{"pipe", "FIFO"}, SkippedEntry{"folder/", "directory"}}) {
        QVERIFY(source.seek(0));
        QGCArchiveFile file(&source, QString::fromLatin1(skipped.name));
        QVERIFY2(!file.open(QIODevice::ReadOnly), skipped.name);
        QVERIFY(!file.entryFound());
        QVERIFY2(file.errorString().contains(QLatin1String(skipped.kind)), qPrintable(file.errorString()));
        QVERIFY2(file.errorString().contains(QLatin1String(skipped.name)), qPrintable(file.errorString()));
    }

    QVERIFY(source.seek(0));
    QGCArchiveFile target(&source, QStringLiteral("target.txt"));
    QVERIFY2(target.open(QIODevice::ReadOnly), qPrintable(target.errorString()));
    QCOMPARE(target.readAll(), QByteArray("target"));
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
    using Outcome = QGCCompression::Outcome;
    QFuture<Outcome> future = QGCCompressionJob::extractArchiveAsync(zipResource, outputDir);
    // Create a watcher to monitor progress
    QFutureWatcher<Outcome> watcher;
    QSignalSpy progressSpy(&watcher, &QFutureWatcher<Outcome>::progressValueChanged);
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<Outcome>::finished);
    watcher.setFuture(future);
    // Wait for completion
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    // Verify result
    QVERIFY(future.isFinished());
    QVERIFY(!future.isCanceled());
    QVERIFY2(future.result(), "Static async extraction failed");
    // The future carries what the worker thread found out
    QVERIFY2(future.result().formatName.startsWith("ZIP"), qPrintable(future.result().formatName));
    QVERIFY(!progressSpy.isEmpty());
    QCOMPARE(progressSpy.last().first().toInt(), 100);
    // Verify extracted file exists
    QVERIFY(QFileInfo::exists(outputDir + "/manifest.json"));
    // Test decompressFileAsync as well
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString decompressOutput = tempDir.path() + "/async_decompress.json";
    QFuture<Outcome> decompressFuture = QGCCompressionJob::decompressFileAsync(gzResource, decompressOutput);
    QFutureWatcher<Outcome> decompressWatcher;
    QSignalSpy decompressFinishedSpy(&decompressWatcher, &QFutureWatcher<Outcome>::finished);
    decompressWatcher.setFuture(decompressFuture);
    QVERIFY_SIGNAL_WAIT(decompressFinishedSpy, TestTimeout::mediumMs());
    QVERIFY(decompressFuture.isFinished());
    QVERIFY2(decompressFuture.result(), "Static async decompression failed");
    QCOMPARE(decompressFuture.result().filterName, QStringLiteral("gzip"));
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
        QVERIFY(_writeGzipOfZeros(inputPath, memberCount + 1, TestFixtures::gzipMember(header)));
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

    using Outcome = QGCCompression::Outcome;
    QFuture<Outcome> future = QGCCompressionJob::decompressFileAsync(inputPath, outputPath);
    QFutureWatcher<Outcome> watcher;
    QSignalSpy finishedSpy(&watcher, &QFutureWatcher<Outcome>::finished);
    connect(&watcher, &QFutureWatcher<Outcome>::progressValueChanged, &watcher, [&future]() { future.cancel(); });
    watcher.setFuture(future);

    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::longMs());
    QVERIFY(future.isCanceled());
    QVERIFY(!QFile::exists(outputPath));
}

void QGCCompressionTest::_testCompressionJobErrorString()
{
    QTemporaryDir tempDir;
    const QString corruptZip = tempDir.filePath("corrupt.zip");
    QVERIFY(TestFixtures::writeFile(corruptZip, "This is not a valid ZIP file"));

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

void QGCCompressionTest::_testCompressionJobPlainInput_data()
{
    QTest::addColumn<bool>("copy");

    QTest::newRow("PlainInput::Refuse") << false;
    QTest::newRow("PlainInput::Copy") << true;
}

void QGCCompressionTest::_testCompressionJobPlainInput()
{
    QFETCH(bool, copy);

    // A file that only has the name of a compressed one
    const QByteArray plain("{\"name\": \"not compressed\"}\n");
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("manifest.json.gz");
    const QString outputPath = tempDir.filePath("manifest.json");
    QVERIFY(TestFixtures::writeFile(inputPath, plain));

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    if (copy) {
        job.decompressFile(inputPath, outputPath, QGCCompression::kDefaultMaxDecompressedFileBytes,
                           QGCCompression::PlainInput::Copy);
    } else {
        // The default, which is what QGCArchiveWatcher runs
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Input is not compressed"));
        job.decompressFile(inputPath, outputPath);
    }
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    if (!copy) {
        verifyExpectedLogMessage();
    }

    // Only the job that was asked to takes the file as it is
    QCOMPARE(finishedSpy.first().first().toBool(), copy);
    if (copy) {
        QVERIFY2(job.errorString().isEmpty(), qPrintable(job.errorString()));
        QCOMPARE(TestFixtures::readFile(outputPath), plain);
    } else {
        QVERIFY2(job.errorString().endsWith(": the input is not compressed"), qPrintable(job.errorString()));
        QVERIFY(!QFileInfo::exists(outputPath));
    }
    QCOMPARE(TestFixtures::readFile(inputPath), plain);
}

void QGCCompressionTest::_testCompressionJobWork()
{
    using QGCCompression::Error;
    using QGCCompression::Outcome;

    // Work that is no function of QGCCompression: it tells how far it is and how it ended
    const QGCCompressionJob::Work work = [](QGCCompression::ProgressCallback progress) {
        const bool cancelled = !progress(50, 100);
        return Outcome{.error = cancelled ? Error::Cancelled : Error::IoError,
                       .message = QStringLiteral("the work failed")};
    };

    QGCCompressionJob job;
    QSignalSpy finishedSpy(&job, &QGCCompressionJob::finished);
    job.start(work, QStringLiteral("from"), QStringLiteral("to"));
    QVERIFY(job.isRunning());
    QCOMPARE(job.currentOperation(), QGCCompressionJob::Operation::Custom);
    QCOMPARE(job.sourcePath(), QStringLiteral("from"));
    QCOMPARE(job.outputPath(), QStringLiteral("to"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(!finishedSpy.first().first().toBool());
    QCOMPARE(job.errorString(), QStringLiteral("the work failed"));
    QCOMPARE(job.progress(), 0.5);
    QCOMPARE(job.future().result().error, Error::IoError);

    // The same work without a job
    QFuture<Outcome> future = QGCCompressionJob::run(work);
    future.waitForFinished();
    QCOMPARE(future.result().error, Error::IoError);
    QCOMPARE(future.result().message, QStringLiteral("the work failed"));
    QCOMPARE(future.progressValue(), 50);
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
    QVERIFY(
        TestFixtures::writeFile(archivePath, _makeTar({{.name = entryName.toLatin1(),
                                                        .data = firstPart + secondPart,
                                                        .type = 'S',
                                                        .sparseData = {{.offset = 2 * blockSize, .size = blockSize},
                                                                       {.offset = 8 * blockSize, .size = blockSize}},
                                                        .sparseFileSize = expected.size()}})));

    QByteArray extracted;
    if (api == "QGCArchiveFile") {
        QGCArchiveFile entry(archivePath, entryName);
        QVERIFY(entry.open(QIODevice::ReadOnly));
        extracted = entry.readAll();
    } else {
        // The limit of an extraction is that of the data: holes are not written, so they do not count
        Results results;
        QVERIFY(_call(api,
                      {.path = archivePath,
                       .output = outputPath,
                       .names = {entryName},
                       .maxBytes = (api == "extractArchive") ? (2 * blockSize) : 0},
                      &results));
        extracted = (api == "extractFileData") ? results.data
                                               : TestFixtures::readFile(outputPath + QLatin1Char('/') + entryName);
    }

    // Each part is where it belongs, with zeros in the holes
    QCOMPARE(extracted.size(), expected.size());
    QVERIFY(extracted == expected);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================
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
    QVERIFY(TestFixtures::writeFile(emptyZip, QByteArray(emptyZipData, sizeof(emptyZipData))));
    // List should return empty list (not error)
    QStringList entries{QStringLiteral("to be replaced")};
    QCOMPARE(QGCCompression::listArchive(emptyZip, entries).error, QGCCompression::Error::None);
    QVERIFY(entries.isEmpty());
    QList<QGCCompression::ArchiveEntry> detailed{QGCCompression::ArchiveEntry{}};
    QCOMPARE(QGCCompression::listArchiveDetailed(emptyZip, detailed).error, QGCCompression::Error::None);
    QVERIFY(detailed.isEmpty());
    // Stats should show zero entries
    QGCCompression::ArchiveStats stats;
    stats.totalEntries = 1;
    QCOMPARE(QGCCompression::getArchiveStats(emptyZip, stats).error, QGCCompression::Error::None);
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
    const QByteArray compressedData = TestFixtures::readFile(gzResource);
    QByteArray decompressed;
    QVERIFY(QGCCompression::decompressData(compressedData, decompressed));
    // Verify decompressed size is reasonable (small file)
    QVERIFY(decompressed.size() > 0);
    QVERIFY(decompressed.size() < 1024);  // Should be a small manifest file
    // Test boundary: decompress with exact size limit matching actual size
    QByteArray exactLimit;
    QVERIFY(
        QGCCompression::decompressData(compressedData, exactLimit, QGCCompression::Format::Auto, decompressed.size()));
    QCOMPARE(exactLimit.size(), decompressed.size());
    // Test boundary: size limit of exactly 1 byte should fail for this file
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QByteArray tooSmall = decompressed;
    // Should fail - file is bigger than 1 byte
    QVERIFY(!QGCCompression::decompressData(compressedData, tooSmall, QGCCompression::Format::Auto, 1));
    QVERIFY(tooSmall.isEmpty());
    verifyExpectedLogMessage();
}

void QGCCompressionTest::_testLargeFileDecompression()
{
    // Test decompression with size limit enforcement
    // Use the manifest.json.gz which we know is small
    const QByteArray compressedData = TestFixtures::readFile(":/unittest/manifest.json.gz");
    // First, decompress without limit to get actual size
    QByteArray fullDecompressed;
    QVERIFY(QGCCompression::decompressData(compressedData, fullDecompressed));
    QVERIFY(!fullDecompressed.isEmpty());
    const qint64 fullSize = fullDecompressed.size();
    // Now test with a limit smaller than the actual size
    if (fullSize > 10) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
        QByteArray limitedDecompressed = fullDecompressed;
        QVERIFY(!QGCCompression::decompressData(compressedData, limitedDecompressed, QGCCompression::Format::Auto, 10));
        // Should return empty due to size limit exceeded
        QVERIFY(limitedDecompressed.isEmpty());
        verifyExpectedLogMessage();
    }
    // Test with limit larger than actual size (should succeed)
    QByteArray unlimitedDecompressed;
    QVERIFY(QGCCompression::decompressData(compressedData, unlimitedDecompressed, QGCCompression::Format::Auto,
                                           fullSize * 2));
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

    using QGCCompression::Error;

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QCOMPARE(QGCCompression::decompressFile(inputPath, outputPath, QGCCompression::Format::Auto, nullptr, limit).error,
             Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Decompression failed"));
    QVERIFY(QGCCompression::decompressIfNeeded(inputPath, outputPath, false, limit).isEmpty());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);

    QGCCompression::Outcome needed;
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("Decompression failed"));
    QVERIFY(QGCCompression::decompressIfNeeded(inputPath, needed, outputPath, false, limit).isEmpty());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(needed.error, Error::SizeLimitExceeded);
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), inputOnly);

    QFile device(inputPath);
    QVERIFY(device.open(QIODevice::ReadOnly));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QCOMPARE(QGCCompression::decompressFromDevice(&device, outputPath, nullptr, limit).error, Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
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
    const QByteArray compressed = TestFixtures::readFile(":/unittest/manifest.json.gz");
    constexpr qint64 limit = 10;

    QByteArray data("to be replaced");
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QCOMPARE(QGCCompression::decompressData(compressed, data, QGCCompression::Format::Auto, limit).error,
             QGCCompression::Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    QBuffer device;
    device.setData(compressed);
    QVERIFY(device.open(QIODevice::ReadOnly));
    data = "to be replaced";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QCOMPARE(QGCCompression::decompressFromDevice(&device, data, limit).error,
             QGCCompression::Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    QJsonParseError parseError;
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded"));
    QVERIFY(QGCCompression::parseCompressedJson(compressed, &parseError, limit).isNull());
    verifyExpectedLogMessage();
    QVERIFY(parseError.error != QJsonParseError::NoError);
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

    for (const char* api : {"extractArchive", "extractFromDevice", "extractArchiveFiltered"}) {
        QTest::addRow("%s-limit", api) << api << true << false;
        QTest::addRow("%s-cancel", api) << api << true << true;
    }
    // The limit is on what is written, whatever the archive says or leaves out about the sizes
    QTest::newRow("extractArchive-limit-undeclared") << "extractArchive" << false << false;
    QTest::newRow("extractArchiveFiltered-limit-undeclared") << "extractArchiveFiltered" << false << false;
}

void QGCCompressionTest::_testExtractionStopsCleanly()
{
    QFETCH(QString, api);
    QFETCH(bool, declaredSizes);
    QFETCH(bool, cancel);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(declaredSizes ? "three.tar" : "streamed.zip");
    const QString outputDir = tempDir.filePath("output");
    if (declaredSizes) {
        QVERIFY(
            TestFixtures::writeFile(archivePath, _makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                                           {.name = "nested/second.txt", .data = QByteArray(600, 'b')},
                                                           {.name = "third.txt", .data = QByteArray(600, 'c')}})));
    } else {
        // Already the first entry is larger than the limit
        QVERIFY(TestFixtures::writeFile(
            archivePath, _makeStreamedZip({{.name = "first.txt", .data = QByteArray(64 * 1024, 'a')},
                                           {.name = "nested/second.txt", .data = QByteArray(64 * 1024, 'b')}},
                                          false)));
    }

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

    const QGCCompression::Outcome outcome =
        _call(api, {.path = archivePath, .output = outputDir, .progress = progress, .maxBytes = limit});

    if (cancel) {
        QCOMPARE(progressCalls, 2);
        QCOMPARE(outcome.error, QGCCompression::Error::Cancelled);
    } else {
        verifyExpectedLogMessage();
        QCOMPARE(outcome.error, QGCCompression::Error::SizeLimitExceeded);
    }
    // What the extraction had written before it was stopped is removed again
    QCOMPARE(_entriesBelow(outputDir), QStringList());
}

void QGCCompressionTest::_testDecompressionCancelled_data()
{
    QTest::addColumn<QString>("api");

    QTest::newRow("decompressFile") << "decompressFile";
    QTest::newRow("decompressFromDevice") << "decompressFromDevice-file";
}

void QGCCompressionTest::_testDecompressionCancelled()
{
    QFETCH(QString, api);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("zeros.gz");
    const QString outputPath = tempDir.filePath("zeros.bin");
    QVERIFY(_writeGzipOfZeros(inputPath, 4));

    int progressCalls = 0;
    const QGCCompression::ProgressCallback cancel = [&progressCalls](qint64, qint64) {
        ++progressCalls;
        return false;
    };
    const QGCCompression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath, .progress = cancel});

    // The caller that cancelled is told so, and not that the input or the output failed
    QCOMPARE(progressCalls, 1);
    QCOMPARE(outcome.error, QGCCompression::Error::Cancelled);
    QVERIFY2(outcome.message.endsWith(": cancelled"), qPrintable(outcome.message));
    QCOMPARE(_entriesBelow(tempDir.path()), QStringList{QStringLiteral("zeros.gz")});
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
    QVERIFY(TestFixtures::writeFile(archivePath, tar.left(4 * 512 + 100)));

    if (!fromDevice) {
        // The size pre-check reads the headers and finds the archive cut where the third one should be
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read archive entry"));
    }
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read data"));
    const QGCCompression::Outcome outcome =
        _call(fromDevice ? "extractFromDevice" : "extractArchive", {.path = archivePath, .output = outputDir});
    if (!fromDevice) {
        verifyExpectedLogMessage();
    }
    verifyExpectedLogMessage();
    QCOMPARE(outcome.error, QGCCompression::Error::InvalidArchive);

    // Unlike a size limit or a cancellation, damage does not undo the extraction. The entries before it stay,
    // and so does the damaged one: what could be read of it, filled up with zeros to the size it declares
    const QStringList written{QStringLiteral("first.txt"), QStringLiteral("nested"),
                              QStringLiteral("nested/second.txt")};
    QCOMPARE(_entriesBelow(outputDir), written);
    QCOMPARE(TestFixtures::readFile(outputDir + "/first.txt"), QByteArray(600, 'a'));
    QCOMPARE(TestFixtures::readFile(outputDir + "/nested/second.txt"), QByteArray(100, 'b') + QByteArray(500, '\0'));
}

void QGCCompressionTest::_testMaxEntriesArchive()
{
    // Test listing an archive with many entries
    // We'll use the existing ZIP and verify the listing mechanism
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    // Get detailed listing
    QList<QGCCompression::ArchiveEntry> entries;
    QVERIFY(QGCCompression::listArchiveDetailed(zipResource, entries));
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
    QGCCompression::ArchiveStats stats;
    QVERIFY(QGCCompression::getArchiveStats(zipResource, stats));
    QCOMPARE(stats.totalEntries, static_cast<int>(entries.size()));
}

void QGCCompressionTest::_testOverstatedEntrySize()
{
    // The header of the entry tells of 2^50 bytes, and 512 follow it
    const QString entryName = QStringLiteral("overstated.bin");
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("overstated.tar");
    QVERIFY(TestFixtures::writeFile(
        archivePath, _tarHeader({.name = entryName.toLatin1()}, Q_INT64_C(1) << 50) + QByteArray(512, 'x')));
    QFile device(archivePath);
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Without a limit, no memory is set aside on the word of the header: the entry fails where its data runs out
    const QRegularExpression dataRanOut("Read error.*Truncated");
    QByteArray data("to be replaced");
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, dataRanOut);
    QVERIFY(!QGCCompression::extractFileData(archivePath, entryName, data, QGCCompression::Format::Auto, 0));
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    data = "to be replaced";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, dataRanOut);
    QVERIFY(!QGCCompression::extractFileDataFromDevice(&device, entryName, data, 0));
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    // The default limit is far below what the header declares, so the entry is not started
    const QRegularExpression overTheLimit("Size limit exceeded");
    data = "to be replaced";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, overTheLimit);
    QCOMPARE(QGCCompression::extractFileData(archivePath, entryName, data).error,
             QGCCompression::Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    QVERIFY(device.seek(0));
    data = "to be replaced";
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, overTheLimit);
    QCOMPARE(QGCCompression::extractFileDataFromDevice(&device, entryName, data).error,
             QGCCompression::Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());
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
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    declaredSize ? _makeTar({{.name = entryName, .data = content}})
                                                 : _makeStreamedZip({{.name = entryName, .data = content}}, false)));

    QByteArray data;
    const auto extract = [&](const QString& path, qint64 limit) {
        data = "to be replaced";
        if (!fromDevice) {
            return QGCCompression::extractFileData(path, entryName, data, QGCCompression::Format::Auto, limit);
        }
        QFile device(path);
        (void) device.open(QIODevice::ReadOnly);
        return QGCCompression::extractFileDataFromDevice(&device, entryName, data, limit);
    };

    // One byte short of the entry
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Size limit exceeded: 2999 bytes"));
    QCOMPARE(extract(archivePath, content.size() - 1).error, QGCCompression::Error::SizeLimitExceeded);
    verifyExpectedLogMessage();
    QVERIFY(data.isEmpty());

    QVERIFY(extract(archivePath, content.size()));
    QCOMPARE(data, content);
    QVERIFY(extract(archivePath, 0));
    QCOMPARE(data, content);

    if (declaredSize) {
        // An entry that says it does not fit is not started: its data, cut short here, is never read
        const QString overstatedPath = tempDir.filePath("overstated.tar");
        QVERIFY(TestFixtures::writeFile(overstatedPath, _tarHeader({.name = entryName}, 5000) + QByteArray(512, 'x')));
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                         QRegularExpression("Size limit exceeded: 4000 bytes"));
        QCOMPARE(extract(overstatedPath, 4000).error, QGCCompression::Error::SizeLimitExceeded);
        verifyExpectedLogMessage();
        QVERIFY(data.isEmpty());
    }
}

void QGCCompressionTest::_testUncompressDataSizeLimit_data()
{
    QTest::addColumn<qint64>("limit");
    QTest::addColumn<bool>("accepted");

    constexpr qint64 fourGiB = Q_INT64_C(1) << 32;
    QTest::newRow("no limit") << qint64(0) << true;
    QTest::newRow("one byte less than the data") << qint64(999) << false;
    QTest::newRow("the size of the data") << qint64(1000) << true;
    // The size the data declares has 32 bits, the limit has more
    QTest::newRow("4 GiB") << fourGiB << true;
    QTest::newRow("4 GiB and 16 bytes") << (fourGiB + 16) << true;
}

void QGCCompressionTest::_testUncompressDataSizeLimit()
{
    QFETCH(qint64, limit);
    QFETCH(bool, accepted);

    const QByteArray data(1000, 'x');
    const QByteArray framed = QGCCompression::compressData(data);
    QVERIFY(QGCCompression::isDataCompressed(framed));

    if (!accepted) {
        expectLogMessage("Utilities.QGCCompression", QtWarningMsg,
                         QRegularExpression("declared size 1000 exceeds limit"));
    }
    const QByteArray result = QGCCompression::uncompressData(framed, limit);
    if (!accepted) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(result, accepted ? data : QByteArray());
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
    QStringList entries;
    QVERIFY(QGCCompression::listArchive(zipResource, entries));
    for (const QString& entry : std::as_const(entries)) {
        // Entry names should not contain backslashes after processing
        // (or if they do, they should be handled correctly)
        // Just verify no crashes or issues accessing these entries
        const QGCCompression::Outcome exists = QGCCompression::fileExists(zipResource, entry);
        Q_UNUSED(exists);  // Just testing it doesn't crash
    }
    // Test path joining with mixed separators
    // QGCFileHelper::joinPath should normalize these
    // Create a test file with normal path
    const QString testDir = tempDir.path() + "/path_test";
    QVERIFY(QDir().mkpath(testDir));
    QVERIFY(TestFixtures::writeFile(testDir + "/test.txt", "test"));
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
    QVERIFY(
        TestFixtures::writeFile(archivePath, _makeTar({{.name = "../escape.txt", .data = "escaped"},
                                                       {.name = QFile::encodeName(absolutePath), .data = "absolute"},
                                                       {.name = "good.txt", .data = "good"}})));

    // Asked for by name, so refusing the entry fails the call. The others leave it out and go on
    const bool byName = (api == "extractFiles");
    const QStringList names = byName ? QStringList{"../escape.txt", "good.txt"} : QStringList{"*.txt"};
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("ath traversal attempt.*escape"));
    const QGCCompression::Outcome outcome = _call(api, {.path = archivePath, .output = outputDir, .names = names});
    verifyExpectedLogMessage();
    QCOMPARE(outcome.error, byName ? QGCCompression::Error::UnsupportedFormat : QGCCompression::Error::None);

    QVERIFY(!QFile::exists(tempDir.filePath("escape.txt")));
    QVERIFY(!QFile::exists(absolutePath));
    const QStringList archiveAndOutput{QStringLiteral("hostile.tar"), QStringLiteral("output")};
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name),
             archiveAndOutput);
    if (!byName) {
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
    QVERIFY(TestFixtures::writeFile(victimPath, "original"));

    QVERIFY(TestFixtures::writeFile(
        archivePath,
        _makeTar({
            // "climb" only leaves the output directory through "inside", which a check of the names alone misses
            {.name = "inside", .type = '2', .linkName = "."},
            {.name = "climb", .type = '2', .linkName = "inside/.."},
            {.name = "absolute", .type = '2', .linkName = "/"},
            {.name = "parent", .type = '2', .linkName = ".."},
            {.name = "hard.txt", .type = '1', .linkName = "victim.txt"},
            {.name = "good.txt", .data = "good"},
            {.name = "climb/made_dir/inner.txt", .data = "inner"},
        })));

    // libarchive resolves a hard link target against the working directory
    const QString originalDir = QDir::currentPath();
    QVERIFY(QDir::setCurrent(tempDir.path()));
    const auto restoreDir = qScopeGuard([&originalDir]() { QDir::setCurrent(originalDir); });

    for (const char* skipped : {"climb", "absolute", "parent"}) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Skipping symlink.*%1").arg(QLatin1String(skipped))));
    }
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Skipping hard link.*hard.txt"));
    const QGCCompression::Outcome extracted = QGCCompression::extractArchive(archivePath, outputDir);
    for (int i = 0; i < 4; ++i) {
        verifyExpectedLogMessage();
    }

    QVERIFY(!QFileInfo::exists(outputDir + "/hard.txt"));
    QCOMPARE(TestFixtures::readFile(victimPath), QByteArray("original"));
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

void QGCCompressionTest::_testExtractLinks_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api :
         {"extractArchive", "extractArchiveFiltered", "extractByPattern", "extractFiles", "extractFile"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testExtractLinks()
{
    QFETCH(QString, api);

    // Both links stay inside the output directory
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("links.tar");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(
        TestFixtures::writeFile(archivePath, _makeTar({{.name = "target.txt", .data = "target"},
                                                       {.name = "soft.txt", .type = '2', .linkName = "target.txt"},
                                                       {.name = "hard.txt", .type = '1', .linkName = "target.txt"}})));
    const auto expectSkipped = [this](const char* kind, const char* name) {
        expectLogMessage(
            "Utilities.QGClibarchive", QtWarningMsg,
            QRegularExpression(
                QStringLiteral("Skipping %1.*%2 -> target.txt").arg(QLatin1String(kind), QLatin1String(name))));
    };

    if (api == "extractFile") {
        // The function that writes one file to a path of its own cannot make a link, and a link is not an empty file
        QVERIFY(QGCCompression::extractFile(archivePath, "target.txt", outputDir + "/target.txt"));
        expectSkipped("symlink", "soft.txt");
        QVERIFY(!QGCCompression::extractFile(archivePath, "soft.txt", outputDir + "/soft.txt"));
        verifyExpectedLogMessage();
        expectSkipped("hard link", "hard.txt");
        QVERIFY(!QGCCompression::extractFile(archivePath, "hard.txt", outputDir + "/hard.txt"));
        verifyExpectedLogMessage();
        QCOMPARE(_entriesBelow(outputDir), QStringList{QStringLiteral("target.txt")});
        QCOMPARE(TestFixtures::readFile(outputDir + "/target.txt"), QByteArray("target"));
        return;
    }

#ifndef Q_OS_UNIX
    QSKIP("The archive holds POSIX links");
#else
    // The functions that write into a directory create the symbolic link, whichever way their entries are
    // chosen, and leave out the hard link: libarchive would resolve its target outside the directory
    const QStringList targetAndLink{QStringLiteral("soft.txt"), QStringLiteral("target.txt")};
    expectSkipped("hard link", "hard.txt");
    if (api == "extractByPattern") {
        QStringList extractedFiles;
        QVERIFY(QGCCompression::extractByPattern(archivePath, {"*.txt"}, outputDir, &extractedFiles));
        extractedFiles.sort();
        QCOMPARE(extractedFiles, targetAndLink);
    } else if (api == "extractFiles") {
        // Asked for by name, so the entry that is left out fails the call
        QCOMPARE(_call(api, {.path = archivePath, .output = outputDir, .names = {"target.txt", "soft.txt", "hard.txt"}})
                     .error,
                 QGCCompression::Error::UnsupportedFormat);
    } else {
        QVERIFY(_call(api, {.path = archivePath, .output = outputDir}));
    }
    verifyExpectedLogMessage();

    QCOMPARE(_entriesBelow(outputDir), targetAndLink);
    QVERIFY(QFileInfo(outputDir + "/soft.txt").isSymLink());
    QCOMPARE(TestFixtures::readFile(outputDir + "/soft.txt"), QByteArray("target"));
#endif
}

void QGCCompressionTest::_testExtractRestoresModificationTime_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"extractArchive", "extractArchiveFiltered", "extractByPattern", "extractFiles"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testExtractRestoresModificationTime()
{
    QFETCH(QString, api);

    // Every function that writes into a directory gives a file the time the archive has for it
    constexpr qint64 modified = 1000000000;
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("dated.tar");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(
        TestFixtures::writeFile(archivePath, _makeTar({{.name = "dated.txt", .data = "dated", .modified = modified}})));

    const QString entry = (api == "extractByPattern") ? QStringLiteral("*.txt") : QStringLiteral("dated.txt");
    QVERIFY(_call(api, {.path = archivePath, .output = outputDir, .names = {entry}}));

    QCOMPARE(QFileInfo(outputDir + "/dated.txt").lastModified().toSecsSinceEpoch(), modified);
}

void QGCCompressionTest::_testExtractLeavesOutSpecialFiles_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"extractArchive", "extractArchiveAtomic", "extractFromDevice", "extractArchiveFiltered",
                            "extractByPattern", "extractFiles", "extractFile"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testExtractLeavesOutSpecialFiles()
{
    QFETCH(QString, api);

    // A tar archive can hold a FIFO and device nodes. A ZIP archive made on Unix can say that an entry is a
    // socket, which a reader that can seek learns from the central directory.
    QTemporaryDir tempDir;
    const QString tarPath = tempDir.filePath("special.tar");
    QVERIFY(TestFixtures::writeFile(tarPath, _makeTar({{.name = "before.txt", .data = "before"},
                                                       {.name = "pipe", .type = '6'},
                                                       {.name = "tty", .type = '3'},
                                                       {.name = "disk", .type = '4'},
                                                       {.name = "after.txt", .data = "after"}})));
    const QString zipPath = tempDir.filePath("socket.zip");
    QVERIFY(TestFixtures::writeFile(
        zipPath,
        _makeStreamedZip({{.name = "sock", .unixMode = 0140644}, {.name = "file.txt", .data = "file"}}, true)));
    const QString tarOutput = tempDir.filePath("from-tar");
    const QString zipOutput = tempDir.filePath("from-zip");
    const auto expectSkipped = [this](const char* kind, const char* name) {
        expectLogMessage(
            "Utilities.QGClibarchive", QtWarningMsg,
            QRegularExpression(QStringLiteral("Skipping %1: %2").arg(QLatin1String(kind), QLatin1String(name))));
    };

    if ((api == "extractFiles") || (api == "extractFile")) {
        // Asked for by name, so leaving the entry out fails the call
        struct Skipped
        {
            const char* kind;
            const char* name;
        };

        for (const Skipped& skipped :
             {Skipped{"FIFO", "pipe"}, Skipped{"character device", "tty"}, Skipped{"block device", "disk"}}) {
            expectSkipped(skipped.kind, skipped.name);
            QVERIFY(!_call(api, {.path = tarPath, .output = tarOutput, .names = {skipped.name}}));
            verifyExpectedLogMessage();
        }
        expectSkipped("socket", "sock");
        QVERIFY(!_call(api, {.path = zipPath, .output = zipOutput, .names = {"sock"}}));
        verifyExpectedLogMessage();
        QCOMPARE(_entriesBelow(tarOutput), QStringList());
        QCOMPARE(_entriesBelow(zipOutput), QStringList());
        if (api == "extractFiles") {
            // A file that is asked for along with such an entry is extracted, and stays
            expectSkipped("FIFO", "pipe");
            QVERIFY(!_call(api, {.path = tarPath, .output = tarOutput, .names = {"before.txt", "pipe"}}));
            verifyExpectedLogMessage();
            QCOMPARE(_entriesBelow(tarOutput), QStringList{QStringLiteral("before.txt")});
        }
        return;
    }

    // The functions that walk the archive go on with the entries that follow
    expectSkipped("FIFO", "pipe");
    expectSkipped("character device", "tty");
    expectSkipped("block device", "disk");
    QVERIFY(_call(api, {.path = tarPath, .output = tarOutput, .names = {"*"}}));
    for (int i = 0; i < 3; ++i) {
        verifyExpectedLogMessage();
    }
    const QStringList regularFiles{QStringLiteral("after.txt"), QStringLiteral("before.txt")};
    QCOMPARE(_entriesBelow(tarOutput), regularFiles);
    QCOMPARE(TestFixtures::readFile(tarOutput + "/after.txt"), QByteArray("after"));

    expectSkipped("socket", "sock");
    QVERIFY(_call(api, {.path = zipPath, .output = zipOutput, .names = {"*"}}));
    verifyExpectedLogMessage();
    QCOMPARE(_entriesBelow(zipOutput), QStringList{QStringLiteral("file.txt")});
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
    QVERIFY(TestFixtures::readFile(unicodeDir + "/manifest.json").contains("\"name\""));
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
void QGCCompressionTest::_benchmarkExtract_data()
{
    QTest::addColumn<QString>("resource");

    QTest::newRow("ZIP") << QStringLiteral(":/unittest/manifest.json.zip");
    QTest::newRow("7z") << QStringLiteral(":/unittest/manifest.json.7z");
}

void QGCCompressionTest::_benchmarkExtract()
{
    QFETCH(QString, resource);

    int iteration = 0;
    QBENCHMARK
    {
        QTemporaryDir tempDir;
        const QString outputDir = tempDir.path() + QString("/bench_%1").arg(iteration++);
        (void) QGCCompression::extractArchive(resource, outputDir);
    }
}

void QGCCompressionTest::_benchmarkDecompress_data()
{
    QTest::addColumn<QString>("resource");

    QTest::newRow("gzip") << QStringLiteral(":/unittest/manifest.json.gz");
    QTest::newRow("xz") << QStringLiteral(":/unittest/manifest.json.xz");
}

void QGCCompressionTest::_benchmarkDecompress()
{
    QFETCH(QString, resource);

    // Load compressed data once
    const QByteArray compressedData = TestFixtures::readFile(resource);
    QByteArray decompressed;
    QBENCHMARK
    {
        (void) QGCCompression::decompressData(compressedData, decompressed);
    }
}

void QGCCompressionTest::_benchmarkConcurrentExtraction()
{
    constexpr int kNumExtractions = 4;
    int baseIteration = 0;
    QBENCHMARK
    {
        QTemporaryDir tempDir;
        QList<QFuture<QGCCompression::Outcome>> futures;
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
    QList<QGCCompression::ArchiveEntry> entries;
    QBENCHMARK
    {
        (void) QGCCompression::listArchiveDetailed(zipResource, entries);
    }
}

#include <QtCore/QTemporaryDir>

#include "UnitTest.h"

UT_REGISTER_TEST_LIGHTWEIGHT(QGCCompressionTest, TestLabel::Unit, TestLabel::Utilities)
