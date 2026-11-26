#include "QGCArchiveExtractionTest.h"

#include <atomic>
#include <climits>
#include <filesystem>
#include <stdexcept>
#include <system_error>

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "ArchiveFixtures.h"
#include "Compression.h"
#include "EntryNames.h"
#include "FileFixtures.h"

#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <sys/resource.h>
#endif

namespace {

/// Whether a failed extraction left its output directory as it was before: gone if the extraction created it,
/// otherwise empty
bool isOutputAsBefore(const QString& outputDir, bool existed)
{
    const QFileInfo output(outputDir);
    if (!existed) {
        return !output.exists() && !output.isSymLink();
    }
    return output.isDir() && ArchiveFixtures::entriesBelow(outputDir).isEmpty();
}

}  // namespace

void QGCArchiveExtractionTest::_testExtractFromResource_data()
{
    QTest::addColumn<QString>("resource");

    QTest::newRow("ZIP") << ArchiveFixtures::zipResource;
#ifdef QGC_ENABLE_7ZIP
    QTest::newRow("7z") << ArchiveFixtures::sevenZipResource;
#endif
}

void QGCArchiveExtractionTest::_testExtractFromResource()
{
    QFETCH(QString, resource);

    QTemporaryDir tempDir;
    const QString outputPath = tempDir.filePath("output");
    const Compression::Outcome outcome = Compression::extractArchive(resource, outputPath);
    QVERIFY2(outcome, qPrintable(outcome.message));
    QCOMPARE(ArchiveFixtures::entriesBelow(outputPath), QStringList{QStringLiteral("manifest.json")});
}

void QGCArchiveExtractionTest::_testArchiveOfManyEntries_data()
{
    QTest::addColumn<QString>("resource");

    QTest::newRow("zip") << QStringLiteral(":/unittest/tree.zip");
#ifdef QGC_ENABLE_7ZIP
    QTest::newRow("7z") << QStringLiteral(":/unittest/tree.7z");
#endif
}

void QGCArchiveExtractionTest::_testArchiveOfManyEntries()
{
    QFETCH(QString, resource);

    // Written by an archiver: a directory, files in it and next to it, one of them empty, one with a non-ASCII
    // name and, in the ZIP archive, one with ZIP64 fields
    const QString beyondAscii = QStringLiteral("data/\u00fcbersicht.txt");
    QStringList files{QStringLiteral("data/empty.bin"), QStringLiteral("data/large-format.txt"),
                      QStringLiteral("data/values.json"), beyondAscii, QStringLiteral("readme.txt")};
    files.sort();

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(Compression::extractArchive(resource, outputDir));
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList{QStringLiteral("data")} + files);
    QCOMPARE(TestFixtures::readFile(outputDir + QLatin1Char('/') + beyondAscii), QByteArray("\xC3\xBC"
                                                                                            "bersicht\n"));
    QCOMPARE(TestFixtures::readFile(outputDir + "/data/large-format.txt"), QByteArray("written with ZIP64 fields\n"));
    QCOMPARE(QFileInfo(outputDir + "/data/empty.bin").size(), 0);
}

void QGCArchiveExtractionTest::_testEncryptedArchive_data()
{
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("refusal");

    QTest::newRow("zip") << QStringLiteral(":/unittest/encrypted.zip") << "Passphrase required";
#ifdef QGC_ENABLE_7ZIP
    QTest::newRow("7z") << QStringLiteral(":/unittest/encrypted.7z") << "content is encrypted";
#endif
}

void QGCArchiveExtractionTest::_testEncryptedArchive()
{
    QFETCH(QString, resource);
    QFETCH(QString, refusal);

    // No function takes a passphrase: the data is not read, and nothing is left behind
    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    const Compression::Outcome outcome = Compression::extractArchive(resource, outputDir);
    QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
    // The outcome says why the archive, although intact, cannot be read
    QVERIFY2(outcome.detail.contains(refusal), qPrintable(outcome.detail));
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList());
}

void QGCArchiveExtractionTest::_testZipCompressionMethods_data()
{
    QTest::addColumn<int>("method");
    QTest::addColumn<QByteArray>("stored");
    // Whether the entry filter accepts the entry
    QTest::addColumn<bool>("accepted");
    // libarchive's name of the method if the extraction refuses it, or empty if it extracts the entry
    QTest::addColumn<QString>("refusedMethod");

    const QByteArray content("{\"name\": \"tiles\"}\n");
    // Data no decoder takes: an entry that is refused is not decoded
    const QByteArray undecodable(64, 'x');

    QTest::newRow("stored") << 0 << content << true << QString();
    QTest::newRow("deflated") << 8 << TestFixtures::deflate(content) << true << QString();
    // An xz entry that libarchive would decode, if the extraction let it
    QTest::newRow("xz") << 95 << ArchiveFixtures::xzStream(content) << true << QStringLiteral("xz");
    QTest::newRow("xz, refused by the filter") << 95 << ArchiveFixtures::xzStream(content) << false << QString();
    QTest::newRow("lzma") << 14 << undecodable << true << QStringLiteral("lzma");
    QTest::newRow("bzip2") << 12 << undecodable << true << QStringLiteral("bzip");
    QTest::newRow("zstd") << 93 << undecodable << true << QStringLiteral("zstd");
    QTest::newRow("ppmd") << 98 << undecodable << true << QStringLiteral("ppmd-1");
    // Windows Explorer writes it for files larger than about 2 GB
    QTest::newRow("deflate64") << 9 << undecodable << true << QStringLiteral("deflation-64-bit");
    QTest::newRow("unknown method") << 200 << undecodable << true << QStringLiteral("an unknown method");
}

void QGCArchiveExtractionTest::_testZipCompressionMethods()
{
    QFETCH(int, method);
    QFETCH(QByteArray, stored);
    QFETCH(bool, accepted);
    QFETCH(QString, refusedMethod);

    // A stored entry before the one of the method: what it wrote is removed when the extraction fails
    const QByteArray content("{\"name\": \"tiles\"}\n");
    ArchiveFixtures::ZipWriter writer;
    writer.add({.name = "first.txt", .deflated = false}, "first", ArchiveFixtures::crc32Of("first"), 5);
    writer.add({.name = "tiles.db", .method = method}, stored, ArchiveFixtures::crc32Of(content),
               static_cast<quint64>(content.size()));

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("methods.zip");
    QVERIFY(TestFixtures::writeFile(archivePath, writer.finish()));
    const QString outputDir = tempDir.filePath("output");
    const Compression::Outcome outcome = Compression::extractArchive(
        archivePath, outputDir, {.filter = [accepted](const Compression::ArchiveEntry& entry) {
            return accepted || (entry.name != QLatin1String("tiles.db"));
        }});

    if (refusedMethod.isEmpty()) {
        QVERIFY2(outcome, qPrintable(outcome.description()));
        QCOMPARE(TestFixtures::readFile(QDir(outputDir).filePath("first.txt")), QByteArray("first"));
        QCOMPARE(QFileInfo::exists(QDir(outputDir).filePath("tiles.db")), accepted);
        if (accepted) {
            QCOMPARE(TestFixtures::readFile(QDir(outputDir).filePath("tiles.db")), content);
        }
        return;
    }

    // The message tells the user why the archive cannot be imported, and the detail which entry and method
    QCOMPARE(outcome.error, Compression::Error::UnsupportedFormat);
    QVERIFY2(
        outcome.message.endsWith(": an entry is compressed with a method other than deflate, which is not extracted"),
        qPrintable(outcome.message));
    QCOMPARE(outcome.detail, QStringLiteral("Entry tiles.db is compressed with %1").arg(refusedMethod));
    QVERIFY(isOutputAsBefore(outputDir, false));
}

void QGCArchiveExtractionTest::_testZipEntryWithBadCrc_data()
{
    QTest::addColumn<bool>("deflated");

    QTest::newRow("stored, a byte of the data changed") << false;
    QTest::newRow("deflated, the CRC changed") << true;
}

void QGCArchiveExtractionTest::_testZipEntryWithBadCrc()
{
    QFETCH(bool, deflated);

    // libarchive compares the CRC-32 of a ZIP entry with its data, unless told to ignore it: a stored entry whose
    // data was changed, and a deflated one whose CRC was, are damaged
    const QByteArray content("{\"name\": \"tiles\"}\n");
    QByteArray stored = deflated ? TestFixtures::deflate(content) : content;
    quint32 crc = ArchiveFixtures::crc32Of(content);
    if (deflated) {
        crc ^= 1U;
    } else {
        stored[2] = 'N';
    }
    ArchiveFixtures::ZipWriter writer;
    writer.add({.name = "tiles.db", .deflated = deflated}, stored, crc, static_cast<quint64>(content.size()));

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("crc.zip");
    QVERIFY(TestFixtures::writeFile(archivePath, writer.finish()));
    const QString outputDir = tempDir.filePath("output");
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
    QVERIFY2(outcome.detail.contains(QStringLiteral("ZIP bad CRC")), qPrintable(outcome.detail));
    QVERIFY(isOutputAsBefore(outputDir, false));
}

void QGCArchiveExtractionTest::_testExtractArchiveFiltered()
{
    QTemporaryDir tempDir;

    // The filter sees each entry as the archive describes it, and the entries it accepts are extracted
    const QString acceptedDir = tempDir.filePath("accepted");
    QList<Compression::ArchiveEntry> asked;
    QVERIFY(Compression::extractArchive(ArchiveFixtures::zipResource, acceptedDir,
                                        {.filter = [&asked](const Compression::ArchiveEntry& entry) {
                                            asked.append(entry);
                                            return true;
                                        }}));
    QCOMPARE(asked.size(), 1);
    QCOMPARE(asked.constFirst().name, QStringLiteral("manifest.json"));
    QVERIFY(!asked.constFirst().isDirectory);
    QCOMPARE(ArchiveFixtures::entriesBelow(acceptedDir), QStringList{QStringLiteral("manifest.json")});

    // The entries it refuses are skipped: the extraction still succeeds and writes nothing
    const QString refusedDir = tempDir.filePath("refused");
    QVERIFY(Compression::extractArchive(ArchiveFixtures::zipResource, refusedDir,
                                        {.filter = [](const Compression::ArchiveEntry&) { return false; }}));
    QCOMPARE(ArchiveFixtures::entriesBelow(refusedDir), QStringList());
}

void QGCArchiveExtractionTest::_testProgressCallbackExtract()
{
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("compressible.zip");
    const QString outputPath = tempDir.filePath("output");
    // Each entry is many times larger than the archive
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeStreamedZip({{.name = "first.txt", .data = QByteArray(64 * 1024, 'a')},
                                                       {.name = "second.txt", .data = QByteArray(64 * 1024, 'b')}},
                                                      true)));
    const qint64 archiveSize = QFileInfo(archivePath).size();

    qint64 lastProgress = 0;
    int callCount = 0;
    bool progressValid = true;
    const Compression::ProgressCallback progress = [&](qint64 bytesProcessed, qint64 totalBytes) -> bool {
        // Both values count bytes of the archive, not of its entries
        if ((bytesProcessed < lastProgress) || (bytesProcessed <= 0) || (bytesProcessed > totalBytes) ||
            (totalBytes != archiveSize)) {
            progressValid = false;
        }
        lastProgress = bytesProcessed;
        callCount++;
        return true;
    };

    QVERIFY(Compression::extractArchive(archivePath, outputPath, {.progress = progress}));
    QVERIFY2(progressValid, "Progress values were invalid");
    QVERIFY(callCount > 0);
    QCOMPARE(ArchiveFixtures::entriesBelow(outputPath),
             (QStringList{QStringLiteral("first.txt"), QStringLiteral("second.txt")}));
}

void QGCArchiveExtractionTest::_testTruncatedArchive_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("data");

    // Cut where the second entry is: inside its local header, which starts 30 bytes before its name, and inside its
    // data. The central directory at the end is gone, so the archive is read from its start
    const QByteArray threeFiles =
        ArchiveFixtures::makeZip({{.name = "first.txt", .data = QByteArray(600, 'a'), .deflated = false},
                                  {.name = "second.txt", .data = QByteArray(600, 'b'), .deflated = false},
                                  {.name = "third.txt", .data = "c"}});
    const QByteArray zip = TestFixtures::readFile(ArchiveFixtures::zipResource);

    QTest::newRow("zip-inside-second-header") << "cut.zip" << threeFiles.left(threeFiles.indexOf("second.txt") - 10);
    QTest::newRow("zip-inside-second-data") << "cut.zip" << threeFiles.left(threeFiles.indexOf("bbbb") + 100);
    QTest::newRow("zip-inside-header") << "cut.zip" << zip.left(10);
    QTest::newRow("zip-inside-data") << "cut.zip" << zip.left(zip.size() / 2);
#ifdef QGC_ENABLE_TAR
    const QByteArray tar = ArchiveFixtures::makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                                     {.name = "second.txt", .data = QByteArray(600, 'b')},
                                                     {.name = "third.txt", .data = "c"}});
    // first.txt takes three 512-byte blocks: its header and two of data
    QTest::newRow("tar-inside-header") << "cut.tar" << tar.left(3 * 512 + 256);
    QTest::newRow("tar-inside-data") << "cut.tar" << tar.left(4 * 512 + 100);
#endif
#ifdef QGC_ENABLE_7ZIP
    const QByteArray sevenZip = TestFixtures::readFile(ArchiveFixtures::sevenZipResource);
    QTest::newRow("7z") << "cut.7z" << sevenZip.left(sevenZip.size() / 2);
#endif
}

void QGCArchiveExtractionTest::_testTruncatedArchive()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, data);
    QVERIFY(!data.isEmpty());

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(fileName);
    QVERIFY(TestFixtures::writeFile(archivePath, data));

    // The damage is reported once, where it is found: in an entry header or in entry data. The error tells a
    // damaged archive from one without entries, also when the filter refuses every entry, because the headers up
    // to the damage are still read
    const QRegularExpression readFailure("Failed to read archive entry|Failed to read data");
    for (const bool accepted : {true, false}) {
        const QString outputDir = tempDir.filePath(accepted ? "accepted" : "refused");
        const Compression::Outcome outcome = Compression::extractArchive(
            archivePath, outputDir, {.filter = [accepted](const Compression::ArchiveEntry&) { return accepted; }});
        QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
        QVERIFY2(outcome.detail.contains(readFailure), qPrintable(outcome.detail));
        // Nothing of a damaged archive is kept, not even the output directory created for it
        QVERIFY(!QFileInfo::exists(outputDir));
    }
}

void QGCArchiveExtractionTest::_testGzipChecksumOfArchive_data()
{
#ifndef QGC_ENABLE_TAR
    QSKIP("This build does not extract TAR archives");
#endif
    // What the gzip stream holds after the end of the TAR archive
    QTest::addColumn<QByteArray>("afterArchive");
    QTest::addColumn<bool>("checked");

    // Bytes that do not compress, so that the stream is longer than what libarchive reads of it
    QList<quint32> words(256 * 1024);
    QRandomGenerator(0x51474343).fillRange(words.data(), words.size());
    const QByteArray noise(reinterpret_cast<const char*>(words.constData()), words.size() * qsizetype{sizeof(quint32)});

    QTest::newRow("archive only") << QByteArray() << true;
    // libarchive stops reading where the archive ends. The rest of the stream is read for its checksum
    QTest::newRow("data after the archive") << noise << true;
    // No more than 16 MiB of it, so that a small file cannot hide a long time of inflating behind its archive
    QTest::newRow("more data than is checked") << (noise + QByteArray(24 * 1024 * 1024, '\0')) << false;
}

void QGCArchiveExtractionTest::_testGzipChecksumOfArchive()
{
    QFETCH(QByteArray, afterArchive);
    QFETCH(bool, checked);

    // A TAR archive that is compressed as a whole. libarchive does not compare the checksum that ends the gzip
    // stream, so the extraction does
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("files.tar.gz");
    const QString outputDir = tempDir.filePath("output");
    const QByteArray archive =
        TestFixtures::gzipMember(ArchiveFixtures::makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                                           {.name = "nested/second.txt", .data = "second"}}) +
                                 afterArchive);

    QVERIFY(TestFixtures::writeFile(archivePath, archive));
    const Compression::Outcome intact = Compression::extractArchive(archivePath, outputDir);
    QVERIFY2(intact, qPrintable(intact.description()));
    QCOMPARE(intact.filterName, QStringLiteral("gzip"));
    QVERIFY(QDir(outputDir).removeRecursively());

    QByteArray damaged = archive;
    damaged[damaged.size() - 8] = static_cast<char>(damaged.at(damaged.size() - 8) ^ 0x01);
    QVERIFY(TestFixtures::writeFile(archivePath, damaged));
    if (!checked) {
        const Compression::Outcome unchecked = Compression::extractArchive(archivePath, outputDir);
        QVERIFY2(unchecked, qPrintable(unchecked.description()));
        return;
    }
    // With one bit of the checksum changed, every entry still reads to its end. The failure is found at the end
    // of the archive, and what was written is removed
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
    QCOMPARE(outcome.detail, QStringLiteral("The gzip data does not match its checksum"));
    QVERIFY(!QFileInfo::exists(outputDir));
}

void QGCArchiveExtractionTest::_testSparseEntrySkipped_data()
{
#ifndef QGC_ENABLE_TAR
    QSKIP("Only a TAR archive holds sparse files, and this build does not extract TAR archives");
#else
    QTest::addColumn<QByteArray>("archive");

    // The archive stores the two parts of the file that are not holes, one after the other. The file begins and
    // ends with a hole
    constexpr qsizetype blockSize = 512;
    const QByteArray parts = QByteArray(blockSize, 'A') + QByteArray(blockSize, 'B');
    const ArchiveFixtures::TarEntry after{.name = "after.txt", .data = "after"};
    // A pax header gives its records to the entry that follows it. A record starts with its own length
    const auto paxHeader = [](const QList<QByteArray>& records) {
        QByteArray data;
        for (const QByteArray& record : records) {
            qsizetype length = record.size() + 3;
            while ((QByteArray::number(length).size() + record.size() + 2) != length) {
                ++length;
            }
            data += QByteArray::number(length) + ' ' + record + '\n';
        }
        return ArchiveFixtures::TarEntry{.name = "pax-header", .data = data, .type = 'x'};
    };

    // The parts are listed in the header of the entry
    QTest::newRow("old GNU") << ArchiveFixtures::makeTar(
        {{.name = "sparse.bin",
          .data = parts,
          .type = 'S',
          .sparseData = {{.offset = 2 * blockSize, .size = blockSize}, {.offset = 8 * blockSize, .size = blockSize}},
          .sparseFileSize = 11 * blockSize},
         after});
    // The parts are listed in the pax header, one record for each number
    QTest::newRow("pax 0.0") << ArchiveFixtures::makeTar(
        {paxHeader({"GNU.sparse.size=5632", "GNU.sparse.numblocks=2", "GNU.sparse.offset=1024",
                    "GNU.sparse.numbytes=512", "GNU.sparse.offset=4096", "GNU.sparse.numbytes=512"}),
         {.name = "sparse.bin", .data = parts},
         after});
    // The parts are listed in one record of the pax header
    QTest::newRow("pax 0.1") << ArchiveFixtures::makeTar(
        {paxHeader({"GNU.sparse.size=5632", "GNU.sparse.numblocks=2", "GNU.sparse.map=1024,512,4096,512",
                    "GNU.sparse.name=sparse.bin"}),
         {.name = "GNUSparseFile.0/sparse.bin", .data = parts},
         after});
    // The parts are listed in a block before the data of the entry
    QByteArray map("2\n1024\n512\n4096\n512\n");
    map.append(QByteArray(blockSize - map.size(), '\0'));
    QTest::newRow("pax 1.0") << ArchiveFixtures::makeTar(
        {paxHeader(
             {"GNU.sparse.major=1", "GNU.sparse.minor=0", "GNU.sparse.name=sparse.bin", "GNU.sparse.realsize=5632"}),
         {.name = "GNUSparseFile.0/sparse.bin", .data = map + parts},
         after});
#endif
}

void QGCArchiveExtractionTest::_testSparseEntrySkipped()
{
    QFETCH(QByteArray, archive);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("sparse.tar");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath, archive));

    // libarchive tells a sparse file by its header, so it is skipped before any of its data is read, and the
    // extraction continues
    expectLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression("Skipping sparse file: sparse.bin$"));
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    verifyExpectedLogMessage();
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList{QStringLiteral("after.txt")});
}

void QGCArchiveExtractionTest::_testConcurrentExtractions()
{
    QTemporaryDir tempDir;
    // Concurrent extractions do not interfere with each other
    constexpr int kNumThreads = 4;
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
    QList<QFuture<void>> futures;
    for (int i = 0; i < kNumThreads; ++i) {
        futures.append(QtConcurrent::run([i, &tempDir, &successCount, &failureCount]() {
            const QString outputDir = tempDir.path() + QString("/concurrent_%1").arg(i);
            QString archivePath = ArchiveFixtures::zipResource;
#ifdef QGC_ENABLE_7ZIP
            // Alternate between two archive formats
            if (i % 2 != 0) {
                archivePath = ArchiveFixtures::sevenZipResource;
            }
#endif
            if (Compression::extractArchive(archivePath, outputDir)) {
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
    for (auto& future : futures) {
        future.waitForFinished();
    }
    QCOMPARE(successCount.load(), kNumThreads);
    QCOMPARE(failureCount.load(), 0);
}

void QGCArchiveExtractionTest::_testEmptyArchive()
{
    QTemporaryDir tempDir;
    const QString emptyZip = tempDir.path() + "/empty.zip";
    // A valid ZIP archive without entries: only the End of Central Directory record
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
    // An archive without entries is not an error: the extraction succeeds and writes nothing
    const QString outputDir = tempDir.path() + "/empty_extract";
    QVERIFY(Compression::extractArchive(emptyZip, outputDir));
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList());
}

void QGCArchiveExtractionTest::_testOutputDirectoryInUse_data()
{
    QTest::addColumn<QString>("obstacle");  // What is at the path of the output directory
    QTest::addColumn<QString>("reason");    // End of the message, with %1 for the path

    const QString notEmpty = QStringLiteral(": the output directory %1 is not empty");
    const QString notDirectory = QStringLiteral(": the output path %1 is not a directory");
    QTest::newRow("a directory with a file") << "file in directory" << notEmpty;
    QTest::newRow("a directory with a hidden file") << "hidden file in directory" << notEmpty;
    QTest::newRow("a directory with an empty directory") << "directory in directory" << notEmpty;
    // Until "missing" exists, the system finds no file at this path, which names the directory with the file
    QTest::newRow("a directory with a file, by way of a missing one")
        << "file in directory, by way of a missing one" << notEmpty;
    QTest::newRow("a file") << "file" << notDirectory;
    QTest::newRow("a link that leads nowhere") << "dangling link" << notDirectory;
#ifdef Q_OS_UNIX
    QTest::newRow("a directory that cannot be read")
        << "unreadable directory" << QStringLiteral(": the output directory %1 cannot be read");
#endif
}

void QGCArchiveExtractionTest::_testOutputDirectoryInUse()
{
    QFETCH(QString, obstacle);
    QFETCH(QString, reason);

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    QString filePath;
    if (obstacle == "file") {
        filePath = outputDir;
    } else if (obstacle == "dangling link") {
        if (!TestFixtures::createSymbolicLink(tempDir.filePath("absent"), outputDir)) {
            QSKIP("Symbolic links are not supported in this environment");
        }
    } else {
        QVERIFY(QDir().mkdir(outputDir));
        if (obstacle.startsWith("file in directory")) {
            filePath = outputDir + "/there-before.txt";
        } else if (obstacle == "hidden file in directory") {
            filePath = outputDir + "/.hidden";
        } else if (obstacle == "directory in directory") {
            QVERIFY(QDir().mkdir(outputDir + "/folder"));
        }
    }
    if (!filePath.isEmpty()) {
        QVERIFY(TestFixtures::writeFile(filePath, "there before"));
    }
    const bool unreadable = (obstacle == "unreadable directory");
    const auto unlock = qScopeGuard([&outputDir, unreadable]() {
        if (unreadable) {
            QFile::setPermissions(outputDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        }
    });
    if (unreadable) {
        QVERIFY(QFile::setPermissions(outputDir, QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        if (QFileInfo(outputDir).isReadable()) {
            QSKIP("This user may read a directory whatever its permissions are");
        }
    }
    const QStringList before = ArchiveFixtures::entriesBelow(tempDir.path());

    const QString givenPath = obstacle.endsWith("missing one") ? (outputDir + "/missing/..") : outputDir;
    const Compression::Outcome outcome = Compression::extractArchive(ArchiveFixtures::zipResource, givenPath);

    // Refused before the archive is opened, so no compression filter is known, and nothing is changed
    QCOMPARE(outcome.error, Compression::Error::IoError);
    QCOMPARE(outcome.message,
             QStringLiteral("Failed to extract archive: ") + ArchiveFixtures::zipResource + reason.arg(outputDir));
    QVERIFY(outcome.filterName.isEmpty());
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), before);
    if (!filePath.isEmpty()) {
        QCOMPARE(TestFixtures::readFile(filePath), QByteArray("there before"));
    }
}

// The output path is resolved by name before any step looks at it. Until "missing" exists, the system finds no
// file at "output/missing/..": a failure would remove the directory as one that the extraction made
void QGCArchiveExtractionTest::_testOutputPathIsResolvedByName()
{
    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(QDir().mkdir(outputDir));
    const QString archivePath = tempDir.filePath("damaged.zip");
    QVERIFY(TestFixtures::writeFile(archivePath, "This is not a valid ZIP file"));

    const Compression::Outcome failed = Compression::extractArchive(archivePath, outputDir + "/missing/..");
    QCOMPARE(failed.error, Compression::Error::InvalidArchive);
    QVERIFY2(failed.detail.contains("Unrecognized archive format"), qPrintable(failed.detail));
    // The directory that was there is kept, and "missing" was not made
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()),
             (QStringList{QStringLiteral("damaged.zip"), QStringLiteral("output")}));

    const Compression::Outcome extracted =
        Compression::extractArchive(ArchiveFixtures::zipResource, outputDir + "/missing/..");
    QVERIFY2(extracted, qPrintable(extracted.description()));
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList{QStringLiteral("manifest.json")});
}

void QGCArchiveExtractionTest::_testFailedExtractionIsUndone_data()
{
    QTest::addColumn<bool>("declaredSizes");   // Whether the archive declares the size of an entry before its data
    QTest::addColumn<QString>("failure");      // What ends the extraction: the size "limit", a "cancel", or "damage"
    QTest::addColumn<bool>("existingOutput");  // Whether the output directory exists, empty, before the extraction

    for (const bool existingOutput : {false, true}) {
        const char* const output = existingOutput ? "empty directory" : "new directory";
        QTest::addRow("limit, %s", output) << true << "limit" << existingOutput;
        QTest::addRow("cancel, %s", output) << true << "cancel" << existingOutput;
        QTest::addRow("damage, %s", output) << true << "damage" << existingOutput;
        // The limit applies to what is written, whether or not the archive declares the sizes
        QTest::addRow("limit, sizes undeclared, %s", output) << false << "limit" << existingOutput;
    }
}

void QGCArchiveExtractionTest::_testFailedExtractionIsUndone()
{
    QFETCH(bool, declaredSizes);
    QFETCH(QString, failure);
    QFETCH(bool, existingOutput);
    using Compression::Error;

    QTemporaryDir tempDir;
    const QString archiveName = declaredSizes ? QStringLiteral("three.zip") : QStringLiteral("streamed.zip");
    const QString archivePath = tempDir.filePath(archiveName);
    // A new output directory is created together with the directory above it
    const QString outputDir = tempDir.filePath(existingOutput ? "output" : "missing/output");
    if (existingOutput) {
        QVERIFY(QDir().mkdir(outputDir));
    }
    if (declaredSizes) {
        // The directory is an entry of its own, written before the extraction fails
        const QByteArray zip =
            ArchiveFixtures::makeZip({{.name = "empty/"},
                                      {.name = "first.txt", .data = QByteArray(600, 'a'), .deflated = false},
                                      {.name = "nested/second.txt", .data = QByteArray(600, 'b'), .deflated = false},
                                      {.name = "third.txt", .data = QByteArray(600, 'c'), .deflated = false}});
        // Cut 100 bytes into the data of the second file
        QVERIFY(
            TestFixtures::writeFile(archivePath, (failure == "damage") ? zip.left(zip.indexOf("bbbb") + 100) : zip));
    } else {
        // The second entry alone is larger than the limit
        QVERIFY(TestFixtures::writeFile(
            archivePath,
            ArchiveFixtures::makeStreamedZip({{.name = "small.txt", .data = "small"},
                                              {.name = "first.txt", .data = QByteArray(64 * 1024, 'a')},
                                              {.name = "nested/second.txt", .data = QByteArray(64 * 1024, 'b')}},
                                             false)));
    }

    // In the archive that declares the sizes, the second file is the one that does not fit, the one that is cut, and
    // the one at which the extraction is cancelled: the callback is called at each of the first three entries and
    // after the data of the first file, so the fourth call cancels
    int progressCalls = 0;
    Compression::ProgressCallback progress;
    Error expected = Error::None;
    if (failure == "cancel") {
        progress = [&progressCalls](qint64, qint64) { return ++progressCalls < 4; };
        expected = Error::Cancelled;
    } else if (failure == "limit") {
        expected = Error::SizeLimitExceeded;
    } else {
        expected = Error::InvalidArchive;
    }

    const Compression::Outcome outcome = Compression::extractArchive(
        archivePath, outputDir, {.maxBytes = (failure == "limit") ? 1000 : 0, .progress = progress});

    if (failure == "cancel") {
        QCOMPARE(progressCalls, 4);
    }
    QCOMPARE(outcome.error, expected);
    if (failure == "damage") {
        QVERIFY2(outcome.detail.startsWith("Failed to read data:"), qPrintable(outcome.detail));
    }
    // What the extraction wrote before it failed is removed: the output directory if the extraction created it,
    // otherwise what is in it. A directory that the extraction created above it stays
    QVERIFY(isOutputAsBefore(outputDir, existingOutput));
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()),
             (QStringList{existingOutput ? QStringLiteral("output") : QStringLiteral("missing"), archiveName}));
}

void QGCArchiveExtractionTest::_testThrowingCallbackUndoesExtraction_data()
{
    QTest::addColumn<bool>("filterThrows");    // Otherwise the progress callback does
    QTest::addColumn<bool>("existingOutput");  // Whether the output directory exists, empty, before the extraction

    for (const bool existingOutput : {false, true}) {
        const char* const output = existingOutput ? "empty directory" : "new directory";
        QTest::addRow("filter, %s", output) << true << existingOutput;
        QTest::addRow("progress callback, %s", output) << false << existingOutput;
    }
}

void QGCArchiveExtractionTest::_testThrowingCallbackUndoesExtraction()
{
    QFETCH(bool, filterThrows);
    QFETCH(bool, existingOutput);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("three.zip");
    const QString outputDir = tempDir.filePath("output");
    if (existingOutput) {
        QVERIFY(QDir().mkdir(outputDir));
    }
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = "first.txt", .data = QByteArray(600, 'a')},
                                               {.name = "nested/second.txt", .data = QByteArray(600, 'b')},
                                               {.name = "third.txt", .data = QByteArray(600, 'c')}})));

    // Thrown at the second file, after the first one has been written
    Compression::ExtractOptions options;
    if (filterThrows) {
        options.filter = [](const Compression::ArchiveEntry& entry) -> bool {
            if (entry.name.startsWith(QLatin1String("nested"))) {
                throw std::runtime_error("filter");
            }
            return true;
        };
    } else {
        // Called at the first entry, after its data, and at the second entry
        options.progress = [calls = 0](qint64, qint64) mutable -> bool {
            if (++calls == 3) {
                throw std::runtime_error("progress");
            }
            return true;
        };
    }

    // The exception reaches the caller, and the extraction has removed what it wrote
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, (void) Compression::extractArchive(archivePath, outputDir, options));
    QVERIFY(isOutputAsBefore(outputDir, existingOutput));
}

void QGCArchiveExtractionTest::_testEntriesOfTheSameName_data()
{
    QTest::addColumn<bool>("existingOutput");  // Whether the output directory exists, empty, before the extraction

    QTest::newRow("new directory") << false;
    QTest::newRow("empty directory") << true;
}

void QGCArchiveExtractionTest::_testEntriesOfTheSameName()
{
    QFETCH(bool, existingOutput);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("names.zip");
    const QString outputDir = tempDir.filePath("output");
    if (existingOutput) {
        QVERIFY(QDir().mkdir(outputDir));
    }
    QVERIFY(
        TestFixtures::writeFile(archivePath, ArchiveFixtures::makeZip({{.name = "first.txt", .data = "earlier entry"},
                                                                       {.name = "nested/second.txt", .data = "second"},
                                                                       {.name = "first.txt", .data = "later entry"}})));

    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QVERIFY2(outcome, qPrintable(outcome.description()));

    // The later entry replaces the file of the earlier one, and the directory holds the entries and nothing else
    const QStringList extracted{QStringLiteral("first.txt"), QStringLiteral("nested"),
                                QStringLiteral("nested/second.txt")};
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), extracted);
    QCOMPARE(TestFixtures::readFile(outputDir + "/first.txt"), QByteArray("later entry"));
    QCOMPARE(TestFixtures::readFile(outputDir + "/nested/second.txt"), QByteArray("second"));
}

void QGCArchiveExtractionTest::_testExtractionNeedsRoomOnDisk_data()
{
    QTest::addColumn<bool>("existingOutput");  // Whether the output directory exists, empty, before the extraction

    QTest::newRow("new directory") << false;
    QTest::newRow("empty directory") << true;
}

void QGCArchiveExtractionTest::_testExtractionNeedsRoomOnDisk()
{
    QFETCH(bool, existingOutput);

    // The second entry declares 2^50 bytes, more than any test disk has room for, in a ZIP64 field. What the archive
    // holds of it is a few bytes
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("vast.zip");
    const QString outputDir = tempDir.filePath("output");
    if (existingOutput) {
        QVERIFY(QDir().mkdir(outputDir));
    }
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip(
                         {{.name = "first.txt", .data = "first"},
                          {.name = "vast.bin", .data = QByteArray(512, 'x'), .declaredSize = Q_INT64_C(1) << 50}})));

    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QCOMPARE(outcome.error, Compression::Error::DiskSpaceExceeded);
    QVERIFY2(outcome.message.endsWith(": not enough disk space"), qPrintable(outcome.message));

    // The entry is refused before any of it is written, and the file written before it is removed
    QVERIFY(isOutputAsBefore(outputDir, existingOutput));
}

void QGCArchiveExtractionTest::_testOutputThatFailsWhileWritten_data()
{
    // Largest file the system lets the process write (0 = no limit: a directory takes the place of the file)
    QTest::addColumn<qint64>("fileSizeLimit");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<bool>("existingOutput");  // Whether the output directory exists, empty, before the extraction

    for (const bool existingOutput : {false, true}) {
        const char* const output = existingOutput ? "empty directory" : "new directory";
#ifdef Q_OS_UNIX
        // The system writes the part that fits, which Qt does not report as an error: the outcome has the reason
        // from the system
        const QString tooLarge = QRegularExpression::escape(qt_error_string(EFBIG));
        QTest::addRow("writing a block, %s", output)
            << Q_INT64_C(100000) << QStringLiteral("Failed to write output: .*large.bin %1").arg(tooLarge)
            << existingOutput;
        // A file smaller than the buffer it is written through reaches the disk when it is complete
        QTest::addRow("writing buffered data, %s", output)
            << Q_INT64_C(1000) << QStringLiteral("Failed to write output: .*medium.bin %1").arg(tooLarge)
            << existingOutput;
#endif
        QTest::addRow("opening the file, %s", output)
            << Q_INT64_C(0) << QStringLiteral("Failed to open output file: .*large.bin") << existingOutput;
    }
}

void QGCArchiveExtractionTest::_testOutputThatFailsWhileWritten()
{
    QFETCH(qint64, fileSizeLimit);
    QFETCH(QString, detail);
    QFETCH(bool, existingOutput);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("three.zip");
    const QString outputDir = tempDir.filePath("output");
    if (existingOutput) {
        QVERIFY(QDir().mkdir(outputDir));
    }
    QVERIFY(TestFixtures::writeFile(
        archivePath,
        ArchiveFixtures::makeZip({{.name = "small.txt", .data = "small", .deflated = false},
                                  {.name = "medium.bin", .data = QByteArray(3000, 'm'), .deflated = false},
                                  {.name = "large.bin", .data = QByteArray(300000, 'l'), .deflated = false}})));

    // Without a limit, a directory appears where the large file goes before it is written: at the first call of the
    // callback once the medium file is complete, which is the one at the entry of the large file
    bool directoryInTheWay = false;
    Compression::ProgressCallback progress;
    if (fileSizeLimit == 0) {
        progress = [&](qint64, qint64) {
            if (!directoryInTheWay && (QFileInfo(outputDir + "/medium.bin").size() == 3000)) {
                directoryInTheWay = QDir().mkdir(outputDir + "/large.bin");
            }
            return true;
        };
    }

    Compression::Outcome outcome;
    {
#ifdef Q_OS_UNIX
        struct rlimit unlimited = {};
        QCOMPARE(getrlimit(RLIMIT_FSIZE, &unlimited), 0);
        // A write beyond the limit raises SIGXFSZ, which ends the process unless it is ignored
        const auto previousHandler = std::signal(SIGXFSZ, SIG_IGN);
        const auto restore = qScopeGuard([&unlimited, previousHandler]() {
            setrlimit(RLIMIT_FSIZE, &unlimited);
            std::signal(SIGXFSZ, previousHandler);
        });
        if (fileSizeLimit > 0) {
            struct rlimit limited = unlimited;
            limited.rlim_cur = static_cast<rlim_t>(fileSizeLimit);
            QCOMPARE(setrlimit(RLIMIT_FSIZE, &limited), 0);
        }
#endif
        outcome = Compression::extractArchive(archivePath, outputDir, {.progress = progress});
    }

    // The output failed, and not for lack of permission or of room on the disk
    QCOMPARE(outcome.error, Compression::Error::IoError);
    QVERIFY2(outcome.detail.contains(QRegularExpression(detail)), qPrintable(outcome.detail));
    // The files written before are removed, and whatever else is in the output directory: also the directory that
    // took the place of the large file
    QVERIFY(isOutputAsBefore(outputDir, existingOutput));
}

void QGCArchiveExtractionTest::_testEntryIsWrittenInPlace()
{
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("large.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(
        archivePath,
        ArchiveFixtures::makeZip({{.name = "large.bin", .data = QByteArray(300000, 'l'), .deflated = false}})));

    // While its data is written, the file is at its own path, beside no temporary file
    QStringList whileWritten;
    const Compression::Outcome outcome =
        Compression::extractArchive(archivePath, outputDir, {.progress = [&](qint64, qint64) {
                                        if (whileWritten.isEmpty()) {
                                            whileWritten = ArchiveFixtures::entriesBelow(outputDir);
                                        }
                                        return true;
                                    }});
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QCOMPARE(whileWritten, QStringList{QStringLiteral("large.bin")});
    QCOMPARE(TestFixtures::readFile(outputDir + "/large.bin"), QByteArray(300000, 'l'));
}

void QGCArchiveExtractionTest::_testCancelledWhereNoDataIsCopied_data()
{
    QTest::addColumn<bool>("passedOver");

    // No block of data is copied for a directory, an empty file, or an entry the filter refuses
    QTest::newRow("entries without data") << false;
    QTest::newRow("entries passed over") << true;
}

void QGCArchiveExtractionTest::_testCancelledWhereNoDataIsCopied()
{
    QFETCH(bool, passedOver);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("no-data.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(
        archivePath,
        ArchiveFixtures::makeZip({{.name = "directory/"},
                                  {.name = "other.txt", .data = passedOver ? QByteArray("refused") : QByteArray()},
                                  {.name = "empty.txt"}})));

    int progressCalls = 0;
    const Compression::ProgressCallback cancel = [&progressCalls](qint64, qint64) {
        ++progressCalls;
        return false;
    };
    Compression::ExtractOptions options{.progress = cancel};
    if (passedOver) {
        options.filter = [](const Compression::ArchiveEntry& entry) { return entry.name == "empty.txt"; };
    }
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir, options);

    QCOMPARE(progressCalls, 1);
    QCOMPARE(outcome.error, Compression::Error::Cancelled);
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList());
}

void QGCArchiveExtractionTest::_testExtractArchiveToSymlinkedOutputPath()
{
    QTemporaryDir tempDir;
    const QString realOutputDir = tempDir.path() + "/symlink_real_output";
    const QString symlinkOutputDir = tempDir.path() + "/symlink_alias_output";
    QVERIFY(QDir().mkpath(realOutputDir));
    QFile::remove(symlinkOutputDir);
    if (!TestFixtures::createSymbolicLink(realOutputDir, symlinkOutputDir)) {
        QSKIP(qPrintable(QStringLiteral("Directory symlinks are not supported in this environment")));
    }
    // On Windows QTemporaryDir cannot remove a link to a directory, and then leaves its directory behind
    const auto removeLink = qScopeGuard([&symlinkOutputDir]() {
        std::error_code error;
        std::filesystem::remove(QFileInfo(symlinkOutputDir).filesystemAbsoluteFilePath(), error);
    });

    QFileInfo symlinkInfo(symlinkOutputDir);
    QVERIFY(symlinkInfo.isSymLink());
    QVERIFY(Compression::extractArchive(ArchiveFixtures::zipResource, symlinkOutputDir));
    QVERIFY(QFile::exists(realOutputDir + "/manifest.json"));
    QVERIFY(QFile::exists(symlinkOutputDir + "/manifest.json"));
}

void QGCArchiveExtractionTest::_testExtractRejectsEscapingNames_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("warning");

    QTest::newRow("above the output directory") << "../escape.txt" << "path traversal attempt.*escape";
    // A file that would replace the output directory
    QTest::newRow("the output directory") << "." << "names the output directory";
}

void QGCArchiveExtractionTest::_testExtractRejectsEscapingNames()
{
    QFETCH(QString, name);
    QFETCH(QString, warning);

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    QString absolutePath = tempDir.filePath("absolute.txt");
#ifdef Q_OS_WIN
    // Without its drive letter: Windows allows no colon in a file name
    absolutePath = absolutePath.mid(2);
#endif
    const QString archivePath = tempDir.filePath("hostile.zip");
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = name.toLatin1(), .data = "escaped"},
                                               {.name = QFile::encodeName(absolutePath), .data = "absolute"},
                                               {.name = "good.txt", .data = "good"}})));

    // The entry is skipped, and the extraction continues
    expectLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(Compression::extractArchive(archivePath, outputDir));
    verifyExpectedLogMessage();

    QVERIFY(!QFile::exists(tempDir.filePath("escape.txt")));
    QVERIFY(!QFile::exists(absolutePath));
    const QStringList archiveAndOutput{QStringLiteral("hostile.zip"), QStringLiteral("output")};
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name),
             archiveAndOutput);
    QVERIFY(QFileInfo(outputDir).isDir());
    QVERIFY(QFile::exists(outputDir + "/good.txt"));
    // The entry with an absolute name is extracted below the output directory
    QVERIFY(QFile::exists(outputDir + absolutePath));
}

void QGCArchiveExtractionTest::_testExtractSkipsNamesThatAreNotPortable_data()
{
    QTest::addColumn<QByteArray>("name");
    // What the warning says is in the name, or empty for a name that is extracted
    QTest::addColumn<QString>("reason");

    const QString device = QStringLiteral("a device name of Windows");
    const QString character = QStringLiteral("a character that Windows does not allow");
    const QString ending = QStringLiteral("a part that ends with a dot or a space");
    QTest::newRow("device") << QByteArray("CON") << device;
    QTest::newRow("device in lower case, with an extension") << QByteArray("nul.txt") << device;
    QTest::newRow("serial port, with the extension of a tile set") << QByteArray("COM1.db") << device;
    QTest::newRow("serial port with a superscript digit") << QByteArray("lpt\xC2\xB9") << device;
    QTest::newRow("console input") << QByteArray("CONIN$") << device;
    QTest::newRow("device before a space and an extension") << QByteArray("aux .log") << device;
    QTest::newRow("device as a directory") << QByteArray("prn/inner.txt") << device;
    QTest::newRow("alternate data stream") << QByteArray("file.txt:stream") << character;
    QTest::newRow("drive-relative") << QByteArray("C:file.txt") << character;
    QTest::newRow("wildcard") << QByteArray("what?.txt") << character;
    QTest::newRow("control character") << QByteArray("bell\x07.txt") << character;
    QTest::newRow("trailing dot") << QByteArray("file.") << ending;
    QTest::newRow("trailing space") << QByteArray("file ") << ending;
    QTest::newRow("three dots") << QByteArray("dir/.../file.txt") << ending;
    QTest::newRow("dots and a space") << QByteArray("dir/.. /file.txt") << ending;
    // Longer than any system takes, also as a path below the output directory, but each part within the limit
    QByteArray overlong;
    while (overlong.size() <= 4096) {
        overlong += QByteArray(100, 'd') + '/';
    }
    QTest::newRow("name of more than 4096 bytes") << (overlong + "file.txt") << QStringLiteral("more than 4096 bytes");
    QTest::newRow("part of 256 bytes") << QByteArray(256, 'a') << QStringLiteral("a part longer than 255 bytes");
    QTest::newRow("part of 258 bytes in UTF-8")
        << QString(86, QChar(0x65E5)).toUtf8() << QStringLiteral("a part longer than 255 bytes");

    QTest::newRow("device name inside a word") << QByteArray("console.txt") << QString();
    QTest::newRow("serial port of two digits") << QByteArray("COM10") << QString();
    QTest::newRow("hidden file") << QByteArray(".hidden") << QString();
    QTest::newRow("part of 255 bytes in UTF-8") << QString(85, QChar(0x65E5)).toUtf8() << QString();
}

void QGCArchiveExtractionTest::_testExtractSkipsNamesThatAreNotPortable()
{
    QFETCH(QByteArray, name);
    QFETCH(QString, reason);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("names.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath, ArchiveFixtures::makeZip({{.name = name, .data = "entry"},
                                                                           {.name = "good.txt", .data = "good"}})));

    if (!reason.isEmpty()) {
        expectLogMessage(
            "Utilities.LibArchive", QtWarningMsg,
            QRegularExpression(
                QStringLiteral("Skipping entry whose name has %1:").arg(QRegularExpression::escape(reason))));
    }
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    if (!reason.isEmpty()) {
        verifyExpectedLogMessage();
    }

    // The same on every system: the entry is skipped, and the extraction goes on
    QStringList extracted{QStringLiteral("good.txt")};
    if (reason.isEmpty()) {
        extracted.append(QString::fromUtf8(name));
        extracted.sort();
    }
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), extracted);
}

void QGCArchiveExtractionTest::_testExtractSkipsSpellingsThatClash_data()
{
    QTest::addColumn<QByteArray>("first");
    QTest::addColumn<QByteArray>("second");
    QTest::addColumn<QString>("clash");   // The spelling the second entry clashes with, or empty if it does not
    QTest::addColumn<bool>("utf8Names");  // Whether the entries declare their names UTF-8

    QTest::newRow("file in another case")
        << QByteArray("README.txt") << QByteArray("readme.txt") << "README.txt" << true;
    QTest::newRow("directory in another case")
        << QByteArray("Data/a.txt") << QByteArray("data/b.txt") << "Data" << true;
#ifndef Q_OS_WIN
    // "ü" as one code point (NFC), and as "u" and a combining diaeresis (NFD), as macOS writes names. libarchive
    // makes a name that is declared UTF-8 NFC itself (NFD on macOS), and so the same name as the other, but not a
    // name in the charset of the system, which Windows does not read: there libarchive is told to read every name as
    // UTF-8
    QTest::newRow("file in another normalisation") << QByteArray(
                                                          "\xC3\xBC"
                                                          "bersicht.txt")
                                                   << QByteArray(
                                                          "u\xCC\x88"
                                                          "bersicht.txt")
                                                   << QString::fromUtf8(
                                                          "\xC3\xBC"
                                                          "bersicht.txt")
                                                   << false;
    QTest::newRow("directory in another normalisation and case") << QByteArray(
                                                                        "M\xC3\xBC"
                                                                        "nchen/a.txt")
                                                                 << QByteArray(
                                                                        "MU\xCC\x88"
                                                                        "NCHEN/b.txt")
                                                                 << QString::fromUtf8(
                                                                        "M\xC3\xBC"
                                                                        "nchen")
                                                                 << false;
#endif
    QTest::newRow("different names") << QByteArray("one.txt") << QByteArray("two.txt") << QString() << true;
    QTest::newRow("file below the same directory")
        << QByteArray("dir/one.txt") << QByteArray("dir/two.txt") << QString() << true;
}

void QGCArchiveExtractionTest::_testExtractSkipsSpellingsThatClash()
{
    QFETCH(QByteArray, first);
    QFETCH(QByteArray, second);
    QFETCH(QString, clash);
    QFETCH(bool, utf8Names);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("names.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = first, .data = "first", .utf8Name = utf8Names},
                                               {.name = second, .data = "second", .utf8Name = utf8Names}})));

    if (!clash.isEmpty()) {
        expectLogMessage(
            "Utilities.LibArchive", QtWarningMsg,
            QRegularExpression(
                QStringLiteral("Skipping entry whose name clashes with %1").arg(QRegularExpression::escape(clash))));
    }
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    if (!clash.isEmpty()) {
        verifyExpectedLogMessage();
    }

    // The first entry is extracted as it is spelled, also on a system that ignores case or normalisation
    QCOMPARE(TestFixtures::readFile(outputDir + QLatin1Char('/') + QString::fromUtf8(first)), QByteArray("first"));
    const QStringList files = ArchiveFixtures::entriesBelow(outputDir).filter(QStringLiteral(".txt"));
    QCOMPARE(files.size(), clash.isEmpty() ? 2 : 1);
    if (clash.isEmpty()) {
        QCOMPARE(TestFixtures::readFile(outputDir + QLatin1Char('/') + QString::fromUtf8(second)),
                 QByteArray("second"));
    }
}

void QGCArchiveExtractionTest::_testSpellingsClashInAnotherNormalisation()
{
    // Whatever libarchive makes of the names: NFD after NFC, and NFC after NFD, in a file and in a directory
    const QString composed = QString::fromUtf8("\xC3\xBC");
    const QString decomposed = QString::fromUtf8("u\xCC\x88");
    LibArchive::EntrySpellings spellings;
    QVERIFY(spellings.claim(composed + QStringLiteral(".db")).isEmpty());
    QCOMPARE(spellings.claim(decomposed + QStringLiteral(".db")), composed + QStringLiteral(".db"));
    QVERIFY(spellings.claim(QStringLiteral("dir/") + decomposed + QStringLiteral("/a.db")).isEmpty());
    QCOMPARE(spellings.claim(QStringLiteral("dir/") + composed + QStringLiteral("/b.db")),
             QStringLiteral("dir/") + decomposed);
}

void QGCArchiveExtractionTest::_testSpellingsGrowWithNewComponentsOnly()
{
    // Paths of 2,000 directories: what is held grows with the parts of a path that no earlier path has, not with the
    // length of every directory above each part
    const QString deep = QStringLiteral("a/").repeated(2000);
    LibArchive::EntrySpellings spellings;
    QVERIFY(spellings.claim(deep + QStringLiteral("x.db")).isEmpty());
    const qsizetype afterFirst = spellings.storedCharacters();
    QCOMPARE_LE(afterFirst, 2 * (deep.size() + 4));

    // A file beside it adds its own name only, and paths that differ at the top add their own length each
    QVERIFY(spellings.claim(deep + QStringLiteral("y.db")).isEmpty());
    QCOMPARE(spellings.storedCharacters(), afterFirst + 8);
    qsizetype claimed = 2 * deep.size() + 8;
    for (int i = 0; i < 200; ++i) {
        const QString path = QString::number(i) + QLatin1Char('/') + deep + QStringLiteral("x.db");
        QVERIFY(spellings.claim(path).isEmpty());
        claimed += path.size();
    }
    QCOMPARE_LE(spellings.storedCharacters(), 2 * claimed);

    // A path in another spelling claims nothing
    QVERIFY(spellings.claim(QStringLiteral("b/x.db")).isEmpty());
    const qsizetype beforeClash = spellings.storedCharacters();
    QCOMPARE(spellings.claim(QStringLiteral("A/b.db")), QStringLiteral("a"));
    QCOMPARE(spellings.claim(QStringLiteral("b/X.DB")), QStringLiteral("b/x.db"));
    QCOMPARE(spellings.storedCharacters(), beforeClash);
}

void QGCArchiveExtractionTest::_testExtractTakesBackslashForSeparator_data()
{
    QTest::addColumn<QByteArray>("archive");
    // Where the entry is written, or empty if it leads outside the output directory
    QTest::addColumn<QString>("written");

    const auto zip = [](const QByteArray& name, quint32 unixMode) {
        return ArchiveFixtures::makeZip({{.name = name, .data = "tiles", .unixMode = unixMode}});
    };
    // libarchive itself turns the backslashes of a ZIP entry name without a slash into slashes, as Windows
    // PowerShell writes them, when the header says the archive was made on Windows: these fixtures say so
    QTest::newRow("ZIP, backslashes only") << zip("Tiles\\offline.db", 0) << QStringLiteral("Tiles/offline.db");
    QTest::newRow("ZIP, both separators")
        << zip("Tiles/sets\\offline.db", 0) << QStringLiteral("Tiles/sets/offline.db");
    QTest::newRow("ZIP, above the output directory") << zip("..\\escape.db", 0100644) << QString();
    QTest::newRow("ZIP, above, after a directory") << zip("Tiles/..\\..\\escape.db", 0) << QString();
#ifdef QGC_ENABLE_TAR
    // libarchive leaves the name of a TAR entry as it is
    QTest::newRow("TAR") << ArchiveFixtures::makeTar({{.name = "Tiles\\offline.db", .data = "tiles"}})
                         << QStringLiteral("Tiles/offline.db");
    QTest::newRow("TAR, above the output directory")
        << ArchiveFixtures::makeTar({{.name = "..\\escape.db", .data = "tiles"}}) << QString();
#endif
}

void QGCArchiveExtractionTest::_testExtractTakesBackslashForSeparator()
{
    QFETCH(QByteArray, archive);
    QFETCH(QString, written);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("windows.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath, archive));

    if (written.isEmpty()) {
        expectLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression("path traversal attempt"));
    }
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QVERIFY2(outcome, qPrintable(outcome.description()));

    // The same on every system
    if (written.isEmpty()) {
        verifyExpectedLogMessage();
        QCOMPARE(outcome.skippedEntries, 1);
        QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList());
        const QStringList outputAndArchive{QStringLiteral("output"), QStringLiteral("windows.zip")};
        QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), outputAndArchive);
    } else {
        QCOMPARE(outcome.skippedEntries, 0);
        QCOMPARE(TestFixtures::readFile(outputDir + QLatin1Char('/') + written), QByteArray("tiles"));
    }
}

void QGCArchiveExtractionTest::_testExtractCountsSkippedEntries()
{
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("skipped.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    ArchiveFixtures::makeZip({{.name = "good.txt", .data = "good"},
                                                              {.name = "nul.txt", .data = "device"},
                                                              {.name = "../escape.txt", .data = "escape"},
                                                              {.name = "link", .data = "good.txt", .unixMode = 0120777},
                                                              {.name = "GOOD.TXT", .data = "clash"},
                                                              {.name = "refused.txt", .data = "refused"}})));

    ignoreLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression("Skipping"));
    const Compression::Outcome outcome = Compression::extractArchive(
        archivePath, outputDir,
        {.filter = [](const Compression::ArchiveEntry& entry) { return entry.name != QStringLiteral("refused.txt"); }});
    QVERIFY2(outcome, qPrintable(outcome.description()));

    // An entry the filter refuses is not skipped for what it is
    QCOMPARE(outcome.skippedEntries, 4);
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList{QStringLiteral("good.txt")});
}

void QGCArchiveExtractionTest::_testExtractNameThatIsNotUtf8_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<bool>("declaredUtf8");
    QTest::addColumn<QString>("warning");  // Empty: none

    // An entry without the flag that declares its name UTF-8. With the flag, libarchive gives no name at all
    QTest::newRow("ZIP, charset not declared") << "zip" << false << QString();
#ifdef QGC_ENABLE_TAR
    // The header of a TAR entry does not say which charset its name is in
    QTest::newRow("TAR, charset not declared") << "tar" << false << QString();
    // A pax record is UTF-8 by definition
    QTest::newRow("TAR, declared as UTF-8 by a pax header") << "tar" << true << "Archive entry read with a warning";
#endif
}

void QGCArchiveExtractionTest::_testExtractNameThatIsNotUtf8()
{
#ifdef Q_OS_WIN
    QSKIP("On Windows libarchive gives no name for such an entry, and the entry is skipped");
#endif
    QFETCH(QString, format);
    QFETCH(bool, declaredUtf8);
    QFETCH(QString, warning);

    // "café.txt" in Latin-1: the byte of its last letter is not valid UTF-8
    const QByteArray name("caf\xE9.txt");
    QByteArray archive;
    if (format == "zip") {
        archive = ArchiveFixtures::makeZip(
            {{.name = name, .data = "latin", .utf8Name = declaredUtf8}, {.name = "good.txt", .data = "good"}});
    } else {
        QList<ArchiveFixtures::TarEntry> entries{{.name = name, .data = "latin"}, {.name = "good.txt", .data = "good"}};
        if (declaredUtf8) {
            // A pax header gives the name of the entry that follows it, in a record that starts with its own length
            const QByteArray record = " path=" + name + '\n';
            entries.first().name = "short.txt";
            entries.prepend(
                {.name = "pax-header", .data = QByteArray::number(record.size() + 2) + record, .type = 'x'});
        }
        archive = ArchiveFixtures::makeTar(entries);
    }

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("latin." + format);
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath, archive));
    if (!warning.isEmpty()) {
        expectLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression(warning));
    }
    QVERIFY(Compression::extractArchive(archivePath, outputDir));
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }

    // The entry is extracted, with the byte that cannot be read replaced in its name
    const QString replaced = QStringLiteral("caf\uFFFD.txt");
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), (QStringList{replaced, QStringLiteral("good.txt")}));
    QCOMPARE(TestFixtures::readFile(outputDir + QLatin1Char('/') + replaced), QByteArray("latin"));
}

void QGCArchiveExtractionTest::_testExtractLeavesOutOverlongPath()
{
#ifdef PATH_MAX
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("deep.zip");
    const QString outputDir = tempDir.filePath("output");

    // Each directory name here is within the system's limit, and the name within the 4096 bytes of every system, but
    // the path in the output directory is longer than PATH_MAX. The name is sized for that, whatever the length of
    // the path of the output directory is
    const qsizetype nameSize = PATH_MAX - QFile::encodeName(outputDir).size();
    QVERIFY(nameSize > 200);
    QByteArray longName;
    while ((nameSize - longName.size()) > 200) {
        longName += QByteArray(100, 'd') + '/';
    }
    longName += QByteArray(nameSize - longName.size() - 4, 'd') + ".txt";
    QCOMPARE(longName.size(), nameSize);
    QVERIFY(longName.size() <= 4096);
    QVERIFY(TestFixtures::writeFile(archivePath, ArchiveFixtures::makeZip({{.name = longName, .data = "deep"},
                                                                           {.name = "good.txt", .data = "good"}})));

    expectLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression("path longer than the system takes"));
    QVERIFY(Compression::extractArchive(archivePath, outputDir));
    verifyExpectedLogMessage();

    // No part of the path is created
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), QStringList{QStringLiteral("good.txt")});
#else
    QSKIP("The system sets no limit to the length of a path");
#endif
}

void QGCArchiveExtractionTest::_testExtractLeavesOutLinks_data()
{
    QTest::addColumn<QString>("format");

    // A ZIP archive marks a symbolic link with its Unix mode, and has no hard links
    QTest::newRow("ZIP") << "zip";
#ifdef QGC_ENABLE_TAR
    QTest::newRow("TAR") << "tar";
#endif
}

void QGCArchiveExtractionTest::_testExtractLeavesOutLinks()
{
    QFETCH(QString, format);
    const bool tar = (format == "tar");

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath("output");
    const QString archiveName = QStringLiteral("links.") + format;
    const QString archivePath = tempDir.filePath(archiveName);
    const QString victimPath = tempDir.filePath("victim.txt");
    QVERIFY(TestFixtures::writeFile(victimPath, "original"));

    // No link is created, whether it points inside the output directory or out of it
    const QList<std::pair<QByteArray, QByteArray>> symbolicLinks{
        {"soft.txt", "target.txt"}, {"inside", "."}, {"absolute", "/"}, {"parent", ".."}, {"nowhere", {}}};
    // An entry below a skipped link goes into a real directory of that name
    const QList<std::pair<QByteArray, QByteArray>> filesAfter{{"inside/made_dir/inner.txt", "inner"},
                                                              {"good.txt", "good"}};
    if (tar) {
        QList<ArchiveFixtures::TarEntry> entries{{.name = "target.txt", .data = "target"}};
        for (const auto& [name, target] : symbolicLinks) {
            entries.append({.name = name, .type = '2', .linkName = target});
        }
        entries.append({.name = "hard.txt", .type = '1', .linkName = "victim.txt"});
        for (const auto& [name, data] : filesAfter) {
            entries.append({.name = name, .data = data});
        }
        QVERIFY(TestFixtures::writeFile(archivePath, ArchiveFixtures::makeTar(entries)));
    } else {
        QList<ArchiveFixtures::ZipEntry> entries{{.name = "target.txt", .data = "target"}};
        for (const auto& [name, target] : symbolicLinks) {
            // Stored: libarchive cannot inflate the empty target of a deflated link
            entries.append({.name = name, .data = target, .unixMode = 0120777, .deflated = false});
        }
        for (const auto& [name, data] : filesAfter) {
            entries.append({.name = name, .data = data});
        }
        QVERIFY(TestFixtures::writeFile(archivePath, ArchiveFixtures::makeZip(entries)));
    }

    // Make the working directory the one a relative hard link target would be resolved against
    const QString originalDir = QDir::currentPath();
    QVERIFY(QDir::setCurrent(tempDir.path()));
    const auto restoreDir = qScopeGuard([&originalDir]() { QDir::setCurrent(originalDir); });

    for (const char* skipped :
         {"soft.txt -> target.txt", "inside -> \\.", "absolute -> /", "parent -> \\.\\.", "nowhere"}) {
        expectLogMessage("Utilities.LibArchive", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Skipping symbolic link: %1$").arg(QLatin1String(skipped))));
    }
    if (tar) {
        expectLogMessage("Utilities.LibArchive", QtWarningMsg,
                         QRegularExpression("Skipping hard link: hard.txt -> victim.txt"));
    }
    const Compression::Outcome extracted = Compression::extractArchive(archivePath, outputDir);
    for (int i = 0; i < (tar ? 6 : 5); ++i) {
        verifyExpectedLogMessage();
    }

    // The links are skipped, and the extraction continues with the entries after them
    QVERIFY2(extracted, qPrintable(extracted.description()));
    for (const char* link : {"soft.txt", "inside", "absolute", "parent", "nowhere", "hard.txt"}) {
        QVERIFY2(!QFileInfo(outputDir + QLatin1Char('/') + QLatin1String(link)).isSymLink(), link);
    }
    const QStringList extractedFiles{QStringLiteral("good.txt"), QStringLiteral("inside"),
                                     QStringLiteral("inside/made_dir"), QStringLiteral("inside/made_dir/inner.txt"),
                                     QStringLiteral("target.txt")};
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), extractedFiles);
    QCOMPARE(TestFixtures::readFile(outputDir + "/target.txt"), QByteArray("target"));
    QCOMPARE(TestFixtures::readFile(outputDir + "/good.txt"), QByteArray("good"));
    QCOMPARE(TestFixtures::readFile(outputDir + "/inside/made_dir/inner.txt"), QByteArray("inner"));
    // Nothing outside the output directory is changed
    QCOMPARE(TestFixtures::readFile(victimPath), QByteArray("original"));
    const QStringList untouched{archiveName, QStringLiteral("output"), QStringLiteral("victim.txt")};
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name),
             untouched);
}

void QGCArchiveExtractionTest::_testExtractDirectoryEntries()
{
    // Each directory is an entry of the archive, whose header gives it the permissions of a file (0644)
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("folders.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    ArchiveFixtures::makeZip({{.name = "empty/", .unixMode = 040644},
                                                              {.name = "folder/", .unixMode = 040644},
                                                              {.name = "folder/inner.txt", .data = "inner"}})));

    QVERIFY(Compression::extractArchive(archivePath, outputDir));

    // A directory entry is created, even if no file is written into it
    const QStringList extracted{QStringLiteral("empty"), QStringLiteral("folder"), QStringLiteral("folder/inner.txt")};
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), extracted);
    QVERIFY(QFileInfo(outputDir + "/empty").isDir());
    QCOMPARE(TestFixtures::readFile(outputDir + "/folder/inner.txt"), QByteArray("inner"));
#ifdef Q_OS_UNIX
    // And it can be entered, whatever permissions its header gives
    QVERIFY(QFileInfo(outputDir + "/empty")
                .permissions()
                .testFlags(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
#endif
}

void QGCArchiveExtractionTest::_testEntriesThatClash_data()
{
    QTest::addColumn<QByteArray>("archive");
    QTest::addColumn<QString>("detail");

    // An entry of a later kind needs the place of a file or a directory that an earlier entry wrote
    QTest::newRow("directory where a file is")
        << ArchiveFixtures::makeZip(
               {{.name = "first.txt", .data = "first"}, {.name = "folder", .data = "file"}, {.name = "folder/"}})
        << "Failed to create directory: .*folder$";
    QTest::newRow("file where a directory is")
        << ArchiveFixtures::makeZip(
               {{.name = "first.txt", .data = "first"}, {.name = "folder/"}, {.name = "folder", .data = "file"}})
        << "Failed to open output file: .*folder ";
    QTest::newRow("file below a file") << ArchiveFixtures::makeZip({{.name = "first.txt", .data = "first"},
                                                                    {.name = "folder", .data = "file"},
                                                                    {.name = "folder/inner.txt", .data = "inner"}})
                                       << "Failed to open output file: .*folder/inner.txt ";
}

void QGCArchiveExtractionTest::_testEntriesThatClash()
{
    QFETCH(QByteArray, archive);
    QFETCH(QString, detail);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("clash.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath, archive));

    // The extraction fails, and not for lack of permission, and leaves nothing
    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);
    QCOMPARE(outcome.error, Compression::Error::IoError);
    QVERIFY2(outcome.detail.contains(QRegularExpression(detail)), qPrintable(outcome.detail));
    QVERIFY(!QFileInfo::exists(outputDir));
}

void QGCArchiveExtractionTest::_testExtractRestoresModificationTime()
{
    // A file gets the modification time stored in the archive
    constexpr qint64 modified = 1000000000;
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("dated.zip");
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = "dated.txt", .data = "dated", .modified = modified}})));

    QVERIFY(Compression::extractArchive(archivePath, outputDir));

    QCOMPARE(QFileInfo(outputDir + "/dated.txt").lastModified().toSecsSinceEpoch(), modified);
}

void QGCArchiveExtractionTest::_testExtractLeavesOutSpecialFiles_data()
{
    QTest::addColumn<QString>("format");

    // A ZIP archive made on Unix marks a special file with its mode in the central directory, which only a reader
    // that can seek sees. libarchive reads a FIFO in a ZIP archive as a regular file, which Info-ZIP wrote for a
    // pipe it read from
    QTest::newRow("ZIP") << "zip";
#ifdef QGC_ENABLE_TAR
    // A TAR archive has a type of entry for each but a socket
    QTest::newRow("TAR") << "tar";
#endif
}

void QGCArchiveExtractionTest::_testExtractLeavesOutSpecialFiles()
{
    QFETCH(QString, format);
    const bool tar = (format == "tar");

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("special.") + format);
    const QString outputDir = tempDir.filePath("output");
    if (tar) {
        QVERIFY(
            TestFixtures::writeFile(archivePath, ArchiveFixtures::makeTar({{.name = "before.txt", .data = "before"},
                                                                           {.name = "pipe", .type = '6'},
                                                                           {.name = "tty", .type = '3'},
                                                                           {.name = "disk", .type = '4'},
                                                                           {.name = "after.txt", .data = "after"}})));
    } else {
        QVERIFY(
            TestFixtures::writeFile(archivePath, ArchiveFixtures::makeZip({{.name = "before.txt", .data = "before"},
                                                                           {.name = "tty", .unixMode = 020644},
                                                                           {.name = "disk", .unixMode = 060644},
                                                                           {.name = "sock", .unixMode = 0140644},
                                                                           {.name = "after.txt", .data = "after"}})));
    }
    const auto expectSkipped = [this](const char* kind, const char* name) {
        expectLogMessage(
            "Utilities.LibArchive", QtWarningMsg,
            QRegularExpression(QStringLiteral("Skipping %1: %2").arg(QLatin1String(kind), QLatin1String(name))));
    };

    // The extraction continues with the entries that follow
    if (tar) {
        expectSkipped("FIFO", "pipe");
    }
    expectSkipped("character device", "tty");
    expectSkipped("block device", "disk");
    if (!tar) {
        expectSkipped("socket", "sock");
    }
    QVERIFY(Compression::extractArchive(archivePath, outputDir));
    for (int i = 0; i < 3; ++i) {
        verifyExpectedLogMessage();
    }
    const QStringList regularFiles{QStringLiteral("after.txt"), QStringLiteral("before.txt")};
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), regularFiles);
    QCOMPARE(TestFixtures::readFile(outputDir + "/after.txt"), QByteArray("after"));
}

void QGCArchiveExtractionTest::_testExtractToUnusualPath_data()
{
    QTest::addColumn<QString>("directoryName");

    QTest::newRow("space") << "dir with space";
    QTest::newRow("parentheses and brackets") << "dir(1)[test]";
    // Common in URLs that get saved as files
    QTest::newRow("ampersand and equals") << "dir&param=value";
    QTest::newRow("Japanese, Chinese and Greek") << QString::fromUtf8("日本語_中文_αβγ");
    QTest::newRow("accented Latin") << QString::fromUtf8("café_naïve_résumé");
    QTest::newRow("Cyrillic") << QString::fromUtf8("Привет_мир");
}

void QGCArchiveExtractionTest::_testExtractToUnusualPath()
{
    QFETCH(QString, directoryName);

    QTemporaryDir tempDir;
    const QString outputDir = tempDir.filePath(directoryName);
    QVERIFY(QDir().mkpath(outputDir));
    QVERIFY(Compression::extractArchive(ArchiveFixtures::zipResource, outputDir));
    QVERIFY(TestFixtures::readFile(outputDir + "/manifest.json").contains("\"name\""));
}

void QGCArchiveExtractionTest::_testFormatNotInThisBuild_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("archive");
    QTest::addColumn<bool>("isArchive");  // By its name or its content

    // libarchive recognizes no archive in these files, and their name or their signature shows the format
#ifndef QGC_ENABLE_7ZIP
    const QByteArray sevenZip = QByteArray::fromHex("377abcaf271c0004") + QByteArray(56, '\x01');
    QTest::newRow("7z") << "archive.7z" << sevenZip << true;
    QTest::newRow("7z, named as ZIP") << "archive.zip" << sevenZip << true;
#endif
#ifndef QGC_ENABLE_TAR
    const QByteArray tar = ArchiveFixtures::makeTar({{.name = "tiles.qgctiledb", .data = "tiles"}});
    QTest::newRow("TAR") << "archive.tar" << tar << true;
    QTest::newRow("TAR, without a name that tells") << "archive" << tar << true;
    QTest::newRow("TAR compressed with gzip") << "archive.tar.gz" << TestFixtures::gzipMember(tar) << true;
#endif
    // An archive compressed as a whole, in a compression that the build does not read. Its name or its content
    // then shows a compressed file
#ifndef QGC_ENABLE_LZMA
    QTest::newRow("compressed with lzma, by its name") << "archive.tar.lzma" << QByteArray(64, '\x01') << false;
#endif
#ifndef QGC_ENABLE_ZSTD
    QTest::newRow("compressed with Zstandard, by its content")
        << "archive.bin" << (QByteArray::fromHex("28b52ffd") + QByteArray(60, '\x01')) << false;
#endif
#if defined(QGC_ENABLE_7ZIP) && defined(QGC_ENABLE_TAR) && defined(QGC_ENABLE_LZMA) && defined(QGC_ENABLE_ZSTD)
    QSKIP("This build reads every format");
#endif
}

void QGCArchiveExtractionTest::_testFormatNotInThisBuild()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, archive);
    QFETCH(bool, isArchive);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(fileName);
    const QString outputDir = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(archivePath, archive));
    QVERIFY(QDir().mkdir(outputDir));
    // The name and the content still say whether the file is an archive
    QCOMPARE(Compression::isArchiveFile(archivePath), isArchive);

    const Compression::Outcome outcome = Compression::extractArchive(archivePath, outputDir);

    // Not an invalid archive: one that another build extracts. The output directory is as it was
    QCOMPARE(outcome.error, Compression::Error::UnsupportedFormat);
    QCOMPARE(outcome.message, QStringLiteral("Failed to extract archive: %1: the archive format is not supported by "
                                             "this build")
                                  .arg(archivePath));
    QVERIFY2(outcome.detail.contains("Unrecognized archive format"), qPrintable(outcome.detail));
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), (QStringList{fileName, QStringLiteral("output")}));
}

QGC_REGISTER_PORTABLE_TEST(QGCArchiveExtractionTest, TestLabel::Unit, TestLabel::Utilities)
