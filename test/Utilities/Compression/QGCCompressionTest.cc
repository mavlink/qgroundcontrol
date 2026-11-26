#include "QGCCompressionTest.h"

#include <initializer_list>
#include <limits>
#include <utility>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTranslator>
#include <QtCore/QtEndian>

#include "ArchiveFixtures.h"
#include "BidAgreement.h"
#include "Compression.h"
#include "FileFixtures.h"
#include "ProcessFixtures.h"

#ifdef Q_OS_UNIX
#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#endif

namespace {
/// Limit of the reads whose limit is not what a test checks: that of data held in memory
constexpr qint64 kMemoryLimit = qint64{256} * 1024 * 1024;

/// The entry points that decompress a single stream, as _call() names them
constexpr const char* kStreamEntryPoints[] = {"decompressFile", "decompressData", "readFile", "readFileChunks"};
}  // namespace

Compression::Outcome QGCCompressionTest::_call(const QString& api, const Inputs& inputs, Results* results)
{
    using namespace Compression;

    Results unused;
    Results& out = (results != nullptr) ? *results : unused;
    if (api == "extractArchive") {
        return extractArchive(inputs.path, inputs.output);
    }
    if (api == "decompressFile") {
        if (inputs.output.isEmpty()) {
            QTest::qFail("decompressFile needs an output path", __FILE__, __LINE__);
            return {.error = Error::InternalError};
        }
        const Outcome outcome = decompressFile(inputs.path, inputs.output);
        out.wroteFile = QFileInfo::exists(inputs.output);
        out.data = out.wroteFile ? TestFixtures::readFile(inputs.output) : QByteArray();
        return outcome;
    }
    if (api == "decompressData") {
        out.data = "to be replaced";
        return decompressData(TestFixtures::readFile(inputs.path), out.data, kMemoryLimit);
    }
    if (api == "readFile") {
        out.data = "to be replaced";
        return readFile(inputs.path, out.data, kMemoryLimit);
    }
    if (api == "readFileChunks") {
        out.data = "to be replaced";
        QByteArray chunks;
        const Outcome outcome = readFileChunks(inputs.path, kMemoryLimit, [&chunks](QByteArrayView chunk) {
            chunks.append(chunk);
            return true;
        });
        // As its caller must: what the chunks gave is discarded after a failure
        out.data = outcome ? chunks : QByteArray();
        return outcome;
    }
    QTest::qFail(qPrintable(QStringLiteral("No entry point is called %1").arg(api)), __FILE__, __LINE__);
    return {.error = Error::InternalError};
}

// ============================================================================
// Kind of a File
// ============================================================================
void QGCCompressionTest::_testKindFromName_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("compressed");
    QTest::addColumn<bool>("archive");
    QTest::addColumn<QString>("stripped");

    const auto stream = [](const char* path, const char* stripped) {
        QTest::newRow(path) << path << true << false << stripped;
    };
    const auto archive = [](const char* path) { QTest::newRow(path) << path << false << true << path; };
    const auto neither = [](const char* path) { QTest::newRow(path) << path << false << false << path; };

    stream("file.gz", "file");
    stream("file.gzip", "file");
    stream("file.xz", "file");
    stream("file.lzma", "file");
    stream("file.zst", "file");
    stream("file.zstd", "file");
    stream("file.bz2", "file");
    stream("file.bzip2", "file");
    stream("file.lz4", "file");
    stream("File.Gz", "File");
    stream("path/to/file.json.gz", "path/to/file.json");
    archive("file.zip");
    archive("FILE.ZIP");
    archive("file.7z");
    archive("file.tar");
    // A compressed archive is an archive, although its name ends with the suffix of a compression
    archive("file.tar.gz");
    archive("file.tgz");
    archive("file.tar.xz");
    archive("file.txz");
    archive("file.tar.zst");
    archive("file.tar.bz2");
    archive("file.tar.lz4");
    // An unrecognized name leaves the decision to the content, and these files do not exist
    neither("file.unknown");
    neither("file.txt");
    neither("gz");
}

void QGCCompressionTest::_testKindFromName()
{
    QFETCH(QString, path);
    QFETCH(bool, compressed);
    QFETCH(bool, archive);
    QFETCH(QString, stripped);

    QCOMPARE(Compression::isCompressedFile(path), compressed);
    QCOMPARE(Compression::isArchiveFile(path), archive);
    QCOMPARE(Compression::strippedPath(path), stripped);
}

void QGCCompressionTest::_testKindFromContent_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<bool>("compressed");
    QTest::addColumn<bool>("archive");

    const auto stream = [](const char* tag, const QByteArray& content) {
        QTest::newRow(tag) << content << true << false;
    };
    const auto archive = [](const char* tag, const QByteArray& content) {
        QTest::newRow(tag) << content << false << true;
    };
    const QByteArray padding(10, '\0');

    stream("gz", TestFixtures::readFile(ArchiveFixtures::gzResource));
    stream("xz", TestFixtures::readFile(ArchiveFixtures::xzResource));
    // The signature identifies the stream, whether or not this build can decode it
    stream("zstd signature", QByteArray::fromHex("28b52ffd") + padding);
    stream("bzip2 signature", QByteArray("BZh91AY&SY") + padding);
    stream("LZ4 signature", QByteArray("\x04\x22\x4D\x18\x64\x40\xA7", 7) + padding);
    stream("legacy LZ4", QByteArray("\x02\x21\x4C\x18", 4) + padding);
    archive("zip", TestFixtures::readFile(ArchiveFixtures::zipResource));
    archive("empty zip", QByteArray("PK\x05\x06", 4) + QByteArray(18, '\0'));
    archive("7z signature", QByteArray::fromHex("377abcaf271c0004") + padding);
    // The signature of TAR is not at the start of the file
    archive("tar", ArchiveFixtures::makeTar({{.name = "good.txt", .data = "good"}}));
    QTest::newRow("text") << QByteArray("neither compressed nor an archive") << false << false;
    // Too short to identify: the first two bytes of a gzip stream are not enough
    QTest::newRow("short") << QByteArray("\x1F\x8B\x08", 3) << false << false;
    QTest::newRow("no bytes") << QByteArray() << false << false;
}

void QGCCompressionTest::_testKindFromContent()
{
    QFETCH(QByteArray, content);
    QFETCH(bool, compressed);
    QFETCH(bool, archive);

    // An unrecognized name leaves the decision to the content
    QTemporaryDir tempDir;
    const QString unnamed = tempDir.filePath("no_extension");
    QVERIFY(TestFixtures::writeFile(unnamed, content));
    QCOMPARE(Compression::isCompressedFile(unnamed), compressed);
    QCOMPARE(Compression::isArchiveFile(unnamed), archive);
    // Such a name has no extension to strip, and the output path must differ from the input
    QCOMPARE(Compression::strippedPath(unnamed), compressed ? (unnamed + ".decompressed") : unnamed);

    // A recognized name wins over the content
    const QString misnamedArchive = tempDir.filePath("misnamed.tar");
    QVERIFY(TestFixtures::writeFile(misnamedArchive, content));
    QVERIFY(Compression::isArchiveFile(misnamedArchive));
    const QString misnamedStream = tempDir.filePath("misnamed.gz");
    QVERIFY(TestFixtures::writeFile(misnamedStream, content));
    QVERIFY(Compression::isCompressedFile(misnamedStream));
    QVERIFY(!Compression::isArchiveFile(misnamedStream));
}

void QGCCompressionTest::_testNameDoesNotChooseTheReader()
{
    using Compression::Error;
    using Compression::Outcome;

    QTemporaryDir tempDir;
    const QByteArray tar = ArchiveFixtures::makeTar({{.name = "good.txt", .data = "good"}});
    const QByteArray tarGz = TestFixtures::gzipMember(tar);
    const QStringList goodOnly{QStringLiteral("good.txt")};

    // The function called decides how the file is read, and the content decides the format. An archive is
    // extracted whether its name is unrecognized, that of a compressed file, or that of another archive format,
    // also one that this build does not extract
    QList<std::pair<QByteArray, const char*>> archives{
        {ArchiveFixtures::makeZip({{.name = "good.txt", .data = "good"}}), "none"}};
#ifdef QGC_ENABLE_TAR
    archives.append({tarGz, "gzip"});
#endif
    for (const auto& [archive, filterName] : std::as_const(archives)) {
        for (const char* name : {"bundle", "bundle.gz", "bundle.zip", "bundle.tar", "bundle.tar.gz", "bundle.7z"}) {
            const QString archivePath = tempDir.filePath(name);
            QVERIFY(TestFixtures::writeFile(archivePath, archive));
            const QString outputDir = tempDir.filePath(QStringLiteral("from_") + QLatin1String(name));
            const Outcome extracted = Compression::extractArchive(archivePath, outputDir);
            QVERIFY2(extracted, name);
            QCOMPARE(extracted.filterName, QLatin1String(filterName));
            QCOMPARE(ArchiveFixtures::entriesBelow(outputDir), goodOnly);
            QVERIFY(QDir(outputDir).removeRecursively());
        }
    }

    // A compressed file is decompressed whether its name is that of another compression or unrecognized
    const QByteArray xzData = TestFixtures::readFile(ArchiveFixtures::xzResource);
    for (const char* name : {"manifest.json.gz", "stream.bin"}) {
        QVERIFY(TestFixtures::writeFile(tempDir.filePath(name), xzData));
        QByteArray read;
        const Outcome renamed = Compression::readFile(tempDir.filePath(name), read, kMemoryLimit);
        QVERIFY2(renamed, qPrintable(renamed.description()));
        QCOMPARE(renamed.filterName, QStringLiteral("xz"));
        QVERIFY(read.contains("\"name\""));
    }

    // Decompressing a compressed archive removes the compression and gives the archive, also in a build that does
    // not extract it
    const QString tarGzPath = tempDir.filePath("whole.tar.gz");
    QVERIFY(TestFixtures::writeFile(tarGzPath, tarGz));
    QByteArray untarredGz;
    QVERIFY(Compression::readFile(tarGzPath, untarredGz, kMemoryLimit));
    QCOMPARE(untarredGz, tar);
#ifdef QGC_ENABLE_TAR
    QVERIFY(TestFixtures::writeFile(tempDir.filePath("whole.tar"), untarredGz));
    QVERIFY(Compression::extractArchive(tempDir.filePath("whole.tar"), tempDir.filePath("untarred")));
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.filePath("untarred")), goodOnly);
#endif

    // A compressed file that holds no archive is not extracted
    const Outcome noArchive = Compression::extractArchive(ArchiveFixtures::gzResource, tempDir.filePath("no_archive"));
    QCOMPARE(noArchive.error, Error::InvalidArchive);
    QVERIFY2(noArchive.detail.contains("Unrecognized archive format"), qPrintable(noArchive.detail));
    QVERIFY(!QFileInfo::exists(tempDir.filePath("no_archive")));

    // A ZIP archive is not a compressed stream, so readFile() gives it unchanged
    QByteArray zip;
    const Outcome notCompressed = Compression::readFile(ArchiveFixtures::zipResource, zip, kMemoryLimit);
    QVERIFY2(notCompressed, qPrintable(notCompressed.description()));
    QCOMPARE(notCompressed.filterName, QStringLiteral("none"));
    QCOMPARE(zip, TestFixtures::readFile(ArchiveFixtures::zipResource));
}

// ============================================================================
// Single-File Decompression
// ============================================================================
void QGCCompressionTest::_testDecompressEveryFormat_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("resource");

    QStringList extensions{QStringLiteral("gz"), QStringLiteral("xz")};
#ifdef QGC_ENABLE_ZSTD
    extensions.append(QStringLiteral("zst"));
#endif
#ifdef QGC_ENABLE_BZIP2
    extensions.append(QStringLiteral("bz2"));
#endif
#ifdef QGC_ENABLE_LZ4
    extensions.append(QStringLiteral("lz4"));
#endif
    for (const QString& extension : std::as_const(extensions)) {
        for (const char* api : kStreamEntryPoints) {
            QTest::addRow("%s %s", qPrintable(extension), api)
                << api << (QStringLiteral(":/unittest/manifest.json.") + extension);
        }
    }
}

void QGCCompressionTest::_testDecompressEveryFormat()
{
    QFETCH(QString, api);
    QFETCH(QString, resource);

    QTemporaryDir tempDir;
    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = resource, .output = tempDir.filePath("manifest.json")}, &results);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QVERIFY(results.data.contains("\"name\""));
}

void QGCCompressionTest::_testDecompressEmptyStream_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<QString>("filterName");
    QTest::addColumn<QByteArray>("stream");

    struct EmptyStream
    {
        const char* extension;
        const char* filterName;
        const char* hex;
    };

    // What each compressor writes for an empty file
    const EmptyStream streams[] = {
        {"gz", "gzip", "1f8b080000000000000303000000000000000000"},
        {"xz", "xz", "fd377a585a000004e6d6b446000000001cdf44211fb6f37d010000000004595a"},
#ifdef QGC_ENABLE_LZMA
        // An LZMA stream has no signature, but libarchive detects it, also as data without a file name
        {"lzma", "lzma", "5d00008000ffffffffffffffff0083fffbffffc0000000"},
#endif
#ifdef QGC_ENABLE_ZSTD
        {"zst", "zstd", "28b52ffd240001000099e9d851"},
#endif
#ifdef QGC_ENABLE_BZIP2
        {"bz2", "bzip2", "425a683917724538509000000000"},
#endif
#ifdef QGC_ENABLE_LZ4
        {"lz4", "lz4", "04224d186440a700000000055dcc02"},
#endif
    };
    for (const EmptyStream& stream : streams) {
        for (const char* api : kStreamEntryPoints) {
            QTest::addRow("%s %s", stream.filterName, api)
                << api << stream.extension << stream.filterName << QByteArray::fromHex(stream.hex);
        }
    }
}

void QGCCompressionTest::_testDecompressEmptyStream()
{
    QFETCH(QString, api);
    QFETCH(QString, extension);
    QFETCH(QString, filterName);
    QFETCH(QByteArray, stream);
    using Compression::Error;

    // An empty log or parameter file compresses to a valid stream that holds no data
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("empty.log." + extension);
    QVERIFY(TestFixtures::writeFile(inputPath, stream));

    // Empty data alone would not show success: a failure also empties the array that _call() fills beforehand
    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = inputPath, .output = tempDir.filePath("output.log")}, &results);
    QCOMPARE(outcome.error, Error::None);
    QCOMPARE(outcome.filterName, filterName);
    // The output is an empty file, or empty data
    QVERIFY(results.data.isEmpty());
    QCOMPARE(results.wroteFile, api == "decompressFile");
}

void QGCCompressionTest::_testDecompressRejectsCutEmptyStream()
{
    using Compression::Error;

    // A gzip stream of an empty file that lacks its eight-byte trailer is truncated, not empty
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("nothing.gz");
    QVERIFY(TestFixtures::writeFile(inputPath, QByteArray::fromHex("1f8b08000000000000030300")));

    for (const char* api : kStreamEntryPoints) {
        Results results;
        const Compression::Outcome outcome =
            _call(api, {.path = inputPath, .output = tempDir.filePath("nothing")}, &results);
        QVERIFY2(outcome.error == Error::InvalidArchive, api);
        QVERIFY2(outcome.detail.contains("Truncated"), qPrintable(outcome.detail));
        QVERIFY2(results.isEmpty(), api);
    }

    // Nothing is written for a refused input
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("nothing.gz")});
}

void QGCCompressionTest::_testDecompressPlainInput_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QByteArray>("content");

    for (const char* api : kStreamEntryPoints) {
        QTest::addRow("%s, text", api) << api << QByteArray("{\"name\": \"not compressed\"}\n");
        // Shorter than the signature of a stream, or with a byte of it changed: data, not a stream
        QTest::addRow("%s, first byte of the gzip signature", api) << api << QByteArray("\x1F", 1);
        QTest::addRow("%s, five bytes of the xz signature", api) << api << QByteArray::fromHex("fd377a585a");
        QTest::addRow("%s, xz signature with its last byte changed", api)
            << api << QByteArray::fromHex("fd377a585a0100000000");
        QTest::addRow("%s, no bytes", api) << api << QByteArray();
    }
}

void QGCCompressionTest::_testDecompressPlainInput()
{
    QFETCH(QString, api);
    QFETCH(QByteArray, content);

    // Only the name says that this file is compressed, as after a download that was decompressed in transit
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("plain.json.gz");
    QVERIFY(TestFixtures::writeFile(inputPath, content));

    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = inputPath, .output = tempDir.filePath("output.json")}, &results);

    // It is taken unchanged, and the filter name shows that nothing was decompressed
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QCOMPARE(outcome.filterName, QStringLiteral("none"));
    QCOMPARE(results.data, content);
    QCOMPARE(results.wroteFile, api == "decompressFile");
    QCOMPARE(TestFixtures::readFile(inputPath), content);
}

namespace {

/// Value of the limitOffset column of _testDecompressFileCopiesPlainInput for a row without a limit
constexpr int kNoLimitOffset = std::numeric_limits<int>::min();

}  // namespace

void QGCCompressionTest::_testDecompressFileCopiesPlainInput_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("limitOffset");  // Limit: content size + this, or none
    QTest::addColumn<bool>("cancel");
    QTest::addColumn<Compression::Error>("expectedError");

    using Compression::Error;
    constexpr int noLimit = kNoLimitOffset;

    QTest::newRow("compressed") << "compressed" << noLimit << false << Error::None;
    // A file that only has the name of a compressed file
    QTest::newRow("not compressed") << "plain" << noLimit << false << Error::None;
    // The copy is held to the limit and can be cancelled, like a decompression
    QTest::newRow("not compressed, at the limit") << "plain" << 0 << false << Error::None;
    QTest::newRow("not compressed, over the limit") << "plain" << -1 << false << Error::SizeLimitExceeded;
    QTest::newRow("not compressed, cancelled") << "plain" << noLimit << true << Error::Cancelled;
    // A damaged file is not taken unchanged: it is compressed, as its name says
    QTest::newRow("compressed and truncated") << "truncated" << noLimit << false << Error::InvalidArchive;
}

void QGCCompressionTest::_testDecompressFileCopiesPlainInput()
{
    QFETCH(QString, input);
    QFETCH(int, limitOffset);
    QFETCH(bool, cancel);
    QFETCH(Compression::Error, expectedError);
    using Compression::Error;

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
    const qint64 limit = (limitOffset == kNoLimitOffset) ? 0 : (content.size() + limitOffset);
    Compression::ProgressCallback progress;
    if (cancel) {
        progress = [](qint64, qint64) { return false; };
    }

    const Compression::Outcome outcome = Compression::decompressFile(inputPath, outputPath, progress, limit);

    QCOMPARE(outcome.error, expectedError);
    const bool written = (expectedError == Error::None);
    // The output holds the content, decompressed or copied, and a failure leaves no output. The input stays.
    const QStringList inputOnly{QStringLiteral("manifest.json.gz")};
    const QStringList outputAndInput{QStringLiteral("manifest.json"), QStringLiteral("manifest.json.gz")};
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), written ? outputAndInput : inputOnly);
    if (written) {
        QCOMPARE(TestFixtures::readFile(outputPath), content);
        // The filter name distinguishes a copy from a decompression
        QCOMPARE(outcome.filterName, (input == "plain") ? QStringLiteral("none") : QStringLiteral("gzip"));
    } else if (expectedError == Error::InvalidArchive) {
        QVERIFY2(outcome.detail.contains(QRegularExpression("(?i)truncated gzip input")), qPrintable(outcome.detail));
    }
}

void QGCCompressionTest::_testDecompressFileDefaultOutput_data()
{
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("inputName");
    QTest::addColumn<QString>("expectedOutputName");

    QTest::newRow("gz") << ArchiveFixtures::gzResource << "manifest.json.gz" << "manifest.json";
    QTest::newRow("gzip") << ArchiveFixtures::gzResource << "manifest.json.gzip" << "manifest.json";
    QTest::newRow("xz") << ArchiveFixtures::xzResource << "manifest.json.xz" << "manifest.json";
    QTest::newRow("lzma") << ArchiveFixtures::xzResource << "manifest.json.lzma" << "manifest.json";
#ifdef QGC_ENABLE_ZSTD
    const QString zst = QStringLiteral(":/unittest/manifest.json.zst");
    QTest::newRow("zst") << zst << "manifest.json.zst" << "manifest.json";
    QTest::newRow("zstd") << zst << "manifest.json.zstd" << "manifest.json";
#endif
#ifdef QGC_ENABLE_BZIP2
    const QString bz2 = QStringLiteral(":/unittest/manifest.json.bz2");
    QTest::newRow("bz2") << bz2 << "manifest.json.bz2" << "manifest.json";
    QTest::newRow("bzip2") << bz2 << "manifest.json.bzip2" << "manifest.json";
#endif
#ifdef QGC_ENABLE_LZ4
    QTest::newRow("lz4") << QStringLiteral(":/unittest/manifest.json.lz4") << "manifest.json.lz4" << "manifest.json";
#endif
    QTest::newRow("uppercase") << ArchiveFixtures::gzResource << "MANIFEST.JSON.GZIP" << "MANIFEST.JSON";
    // A compressed archive loses the extension of its compression
    QTest::newRow("compressed archive") << ArchiveFixtures::gzResource << "manifest.tar.gz" << "manifest.tar";
    QTest::newRow("no-extension") << ArchiveFixtures::gzResource << "manifest" << "manifest.decompressed";
    QTest::newRow("only-extension") << ArchiveFixtures::gzResource << ".gz" << ".gz.decompressed";
    // Without the extension, the name is the directory or the one above it
    QTest::newRow("dot and extension") << ArchiveFixtures::gzResource << "..gz" << "..gz.decompressed";
    QTest::newRow("dots and extension") << ArchiveFixtures::gzResource << "...gz" << "...gz.decompressed";
    QTest::newRow("unrecognised-extension")
        << ArchiveFixtures::gzResource << "manifest.bin" << "manifest.bin.decompressed";
}

void QGCCompressionTest::_testDecompressFileDefaultOutput()
{
    QFETCH(QString, resource);
    QFETCH(QString, inputName);
    QFETCH(QString, expectedOutputName);

    const QByteArray compressed = TestFixtures::readFile(resource);
    QVERIFY(!compressed.isEmpty());

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(inputName);
    QVERIFY(TestFixtures::writeFile(inputPath, compressed));

    // Without an output path, the output is written beside the input, which stays
    const Compression::Outcome outcome = Compression::decompressFile(inputPath);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QStringList expectedEntries{inputName, expectedOutputName};
    expectedEntries.sort();
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), expectedEntries);
    QVERIFY(TestFixtures::readFile(tempDir.filePath(expectedOutputName)).contains("\"name\""));
    QCOMPARE(TestFixtures::readFile(inputPath), compressed);
}

// ============================================================================
// Compression Reported by an Operation
// ============================================================================
void QGCCompressionTest::_testDetectedFilter_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("filter");

    QTest::newRow("decompressFile") << "decompressFile" << ArchiveFixtures::gzResource << "gzip";
    QTest::newRow("decompressData") << "decompressData" << ArchiveFixtures::xzResource << "xz";
    QTest::newRow("readFile, gzip") << "readFile" << ArchiveFixtures::gzResource << "gzip";
    QTest::newRow("readFile, xz") << "readFile" << ArchiveFixtures::xzResource << "xz";
    QTest::newRow("extractArchive") << "extractArchive" << ArchiveFixtures::zipResource << "none";
}

void QGCCompressionTest::_testDetectedFilter()
{
    QFETCH(QString, api);
    QFETCH(QString, resource);
    QFETCH(QString, filter);

    QTemporaryDir tempDir;
    Results results;
    const Compression::Outcome outcome = _call(api, {.path = resource, .output = tempDir.filePath("output")}, &results);

    QVERIFY2(outcome, qPrintable(outcome.message));
    QCOMPARE(outcome.description(), QString());
    QCOMPARE(outcome.filterName, filter);
    // The data returned is what was read, not what _call() put in the array beforehand
    if (_isStreamEntryPoint(api)) {
        QVERIFY2(results.data.contains("\"name\""), results.data.constData());
    }
}

void QGCCompressionTest::_testDetectedFilterAfterFailure_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : {"decompressFile", "readFile", "decompressData", "extractArchive"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testDetectedFilterAfterFailure()
{
    QFETCH(QString, api);

    // Neither input reaches the header that reveals its compression: a gzip stream that ends after its own
    // header, and bytes that are not an archive
    const bool isStream = _isStreamEntryPoint(api);
    const QByteArray data =
        isStream ? QByteArray::fromHex("1f8b08000000000000ff") : QByteArray("This is not an archive of any format");
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(isStream ? "cut.gz" : "garbage.zip");
    const QString outputPath = tempDir.filePath("output");
    QVERIFY(TestFixtures::writeFile(inputPath, data));

    // The detail says what could not be opened and why: the stream is truncated, the text is not an archive
    const QString source = (api == "decompressData") ? QStringLiteral("compressed data") : QStringLiteral("file");
    const QRegularExpression report(QStringLiteral("Failed to open %1.*%2")
                                        .arg(source, isStream ? QStringLiteral("truncated gzip input")
                                                              : QStringLiteral("Unrecognized archive format")));
    Results results;
    const Compression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);

    QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
    QVERIFY(results.isEmpty());
    QCOMPARE(outcome.filterName, QString());
    QVERIFY2(outcome.detail.contains(report), qPrintable(outcome.detail));
    QVERIFY(outcome.description().startsWith(outcome.message));
    QVERIFY(outcome.description().endsWith(QLatin1Char('(') + outcome.detail + QLatin1Char(')')));
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================
void QGCCompressionTest::_testMissingInput_data()
{
    QTest::addColumn<QString>("api");

    // readFile() is tested in _testReadFile
    for (const char* api : {"decompressFile", "extractArchive"}) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testMissingInput()
{
    QFETCH(QString, api);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath((api == "extractArchive") ? "absent.zip" : "absent.gz");

    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = inputPath, .output = tempDir.filePath("output")}, &results);

    // The file is refused before anything is read or written
    QCOMPARE(outcome.error, Compression::Error::FileNotFound);
    QCOMPARE(outcome.message, QStringLiteral("File does not exist: ") + inputPath);
    QVERIFY(results.isEmpty());
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList());
}

void QGCCompressionTest::_testInputThatCannotBeRead_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<bool>("directory");

    for (const char* api : {"decompressFile", "readFile", "extractArchive"}) {
        QTest::addRow("%s, a directory", api) << QString::fromLatin1(api) << true;
#ifdef Q_OS_LINUX
        // /proc/self/mem can be opened, and reading its first byte fails
        QTest::addRow("%s, a file whose content cannot be read", api) << QString::fromLatin1(api) << false;
#endif
    }
}

void QGCCompressionTest::_testInputThatCannotBeRead()
{
    QFETCH(QString, api);
    QFETCH(bool, directory);

    // The input exists and may be read, so it is refused only when it is opened or read. A failure to read is
    // not taken for the end of an empty file
    QTemporaryDir tempDir;
    const QString inputPath = directory ? tempDir.filePath("folder.gz") : QStringLiteral("/proc/self/mem");
    if (directory) {
        QVERIFY(QDir().mkdir(inputPath));
    }
    const QStringList before = ArchiveFixtures::entriesBelow(tempDir.path());

    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = inputPath, .output = tempDir.filePath("output")}, &results);

    QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
    QVERIFY2(outcome.detail.contains("Failed to open file"), qPrintable(outcome.detail));
    QVERIFY(results.isEmpty());
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), before);
}

#ifdef Q_OS_UNIX
namespace {

/// A named pipe that a thread writes bytes into once a reader has opened it: input that cannot seek and that has
/// no size
class CompressionTestPipe
{
    Q_DISABLE_COPY_MOVE(CompressionTestPipe)

public:
    CompressionTestPipe(const QString& path, const QByteArray& bytes)
        : _path(QFile::encodeName(path))
    {
        if (::mkfifo(_path.constData(), 0600) != 0) {
            return;
        }
        _writer = std::thread([pipePath = _path, bytes]() {
            // A reader that gives up closes the pipe. Writing then fails, instead of ending the process with
            // SIGPIPE
            sigset_t brokenPipe;
            sigemptyset(&brokenPipe);
            sigaddset(&brokenPipe, SIGPIPE);
            pthread_sigmask(SIG_BLOCK, &brokenPipe, nullptr);

            // Returns once a reader has opened the pipe
            const int fd = ::open(pipePath.constData(), O_WRONLY);
            if (fd < 0) {
                return;
            }
            qsizetype written = 0;
            while (written < bytes.size()) {
                const ssize_t count =
                    ::write(fd, bytes.constData() + written, static_cast<size_t>(bytes.size() - written));
                if (count <= 0) {
                    break;
                }
                written += count;
            }
            ::close(fd);
        });
    }

    ~CompressionTestPipe()
    {
        if (!_writer.joinable()) {
            return;
        }
        // Lets the writer finish, also if nothing has opened the pipe or has read all of it
        const int fd = ::open(_path.constData(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            (void) ::fcntl(fd, F_SETFL, 0);
            char discarded[4096];
            while (::read(fd, discarded, sizeof(discarded)) > 0) {
            }
            ::close(fd);
        }
        _writer.join();
    }

    bool isValid() const { return _writer.joinable(); }

private:
    QByteArray _path;
    std::thread _writer;
};

}  // namespace
#endif

void QGCCompressionTest::_testInputThatCannotSeek_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<bool>("archive");
    QTest::addColumn<bool>("readable");

    QTest::newRow("compressed stream") << TestFixtures::gzipMember(QByteArray(200000, 's')) << false << true;
    QTest::newRow("ZIP") << TestFixtures::readFile(QStringLiteral(":/unittest/tree.zip")) << true << true;
#ifdef QGC_ENABLE_TAR
    // The entry before the one that is extracted is larger than the pipe holds
    QTest::newRow("TAR") << ArchiveFixtures::makeTar({{.name = "before.bin", .data = QByteArray(200000, 'b')},
                                                      {.name = "readme.txt", .data = "read me"}})
                         << true << true;
#endif
#ifdef QGC_ENABLE_7ZIP
    // A 7z archive is read from its end
    QTest::newRow("7z") << TestFixtures::readFile(QStringLiteral(":/unittest/tree.7z")) << true << false;
#endif
}

void QGCCompressionTest::_testInputThatCannotSeek()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a named pipe");
#else
    QFETCH(QByteArray, input);
    QFETCH(bool, archive);
    QFETCH(bool, readable);

    QTemporaryDir tempDir;
    const QString pipePath = tempDir.filePath("pipe");
    const QString outputPath = tempDir.filePath("output");
    Compression::Outcome outcome;
    QByteArray data;
    {
        const CompressionTestPipe pipe(pipePath, input);
        QVERIFY(pipe.isValid());
        // A pipe is not read to tell its kind: the bytes would be lost to the operation
        QVERIFY(!Compression::isArchiveFile(pipePath));
        // The entries of an archive that are not extracted are passed over without seeking
        outcome =
            archive ? Compression::extractArchive(
                          pipePath, outputPath,
                          {.filter = [](const Compression::ArchiveEntry& entry) { return entry.name == "readme.txt"; }})
                    : Compression::readFile(pipePath, data, kMemoryLimit);
    }

    if (!readable) {
        QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
        QVERIFY2(outcome.detail.startsWith("Failed to read archive entry"), qPrintable(outcome.detail));
        QVERIFY(!QFileInfo::exists(outputPath));
        return;
    }
    QVERIFY2(outcome, qPrintable(outcome.description()));
    if (archive) {
        QCOMPARE(ArchiveFixtures::entriesBelow(outputPath), QStringList{QStringLiteral("readme.txt")});
    } else {
        QCOMPARE(outcome.filterName, QStringLiteral("gzip"));
        QCOMPARE(data, QByteArray(200000, 's'));
    }
#endif
}

void QGCCompressionTest::_testErrorNames()
{
    using Compression::Error;

    // Every code has a name of its own
    QStringList names;
    for (const Error error :
         {Error::None, Error::FileNotFound, Error::PermissionDenied, Error::InvalidArchive, Error::UnsupportedFormat,
          Error::SizeLimitExceeded, Error::Cancelled, Error::IoError, Error::InternalError, Error::DiskSpaceExceeded}) {
        const QString name = Compression::errorName(error);
        QVERIFY(!name.isEmpty());
        QVERIFY2(!names.contains(name), qPrintable(name));
        names.append(name);
    }
    // And a value that is not a code has none of them
    QVERIFY(!names.contains(Compression::errorName(static_cast<Error>(-1))));
}

namespace {

/// Translates the strings of the Compression context by putting them in brackets
class QGCCompressionTestTranslator : public QTranslator
{
public:
    QString translate(const char* context, const char* sourceText, const char*, int) const override
    {
        const bool asked = (qstrcmp(context, "Compression") == 0);
        return asked ? QStringLiteral("[%1]").arg(QLatin1String(sourceText)) : QString();
    }

    bool isEmpty() const override { return false; }
};

}  // namespace

void QGCCompressionTest::_testMessagesAreTranslated()
{
    QGCCompressionTestTranslator translator;
    QVERIFY(QCoreApplication::installTranslator(&translator));
    const auto removeTranslator = qScopeGuard([&translator]() { QCoreApplication::removeTranslator(&translator); });

    // The message of an outcome is shown to the user, so what failed and why are both translated
    QTemporaryDir tempDir;
    const Compression::Outcome outcome =
        Compression::extractArchive(ArchiveFixtures::zipResource, tempDir.filePath("output"), {.maxBytes = 1});
    QCOMPARE(outcome.error, Compression::Error::SizeLimitExceeded);
    QCOMPARE(
        outcome.message,
        QStringLiteral("[Failed to extract archive: %1][: size limit exceeded]").arg(ArchiveFixtures::zipResource));
    QCOMPARE(Compression::errorName(outcome.error), QStringLiteral("[Size limit exceeded]"));
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
    // Stand-ins for the programs that libarchive runs when it has no built-in decoder for a filter
    for (const char* program : {"bzip2", "lz4", "zstd", "lrzip"}) {
        QVERIFY(TestFixtures::writeExecutable(
            binPath + QLatin1Char('/') + QLatin1String(program),
            "#!/bin/sh\n: > '" + QFile::encodeName(markerPath) + "'\nwhile read -r _; do :; done\n"));
    }
    const TestFixtures::EnvVarFixture restorePath("PATH");
    qputenv("PATH", QFile::encodeName(binPath));

    const QByteArray padding(32, '\0');

    // libarchive always decodes lrzip with an external program
    const QString lrzipTar = tempDir.filePath("lrzip.tar");
    QVERIFY(TestFixtures::writeFile(lrzipTar, QByteArray("LRZI\x00\x06", 6) + padding));
    const Compression::Outcome lrzip = Compression::extractArchive(lrzipTar, tempDir.filePath("lrzip"));
    QVERIFY(!lrzip);
    QVERIFY2(lrzip.detail.contains("Unrecognized archive format"), qPrintable(lrzip.detail));
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.filePath("lrzip")), QStringList());

#ifndef QGC_ENABLE_BZIP2
    // Both ways of opening a single stream refuse a stream that this build cannot decode
    const QByteArray bzip2Stream = QByteArray("BZh91AY&SY") + padding;
    const QString bzip2Path = tempDir.filePath("stream.bz2");
    QVERIFY(TestFixtures::writeFile(bzip2Path, bzip2Stream));

    for (const char* api : kStreamEntryPoints) {
        Results results;
        const Compression::Outcome outcome =
            _call(api, {.path = bzip2Path, .output = tempDir.filePath("stream")}, &results);
        QVERIFY2(outcome.error == Compression::Error::UnsupportedFormat, api);
        QVERIFY2(outcome.detail.contains(QRegularExpression("bzip2.*not supported")), qPrintable(outcome.detail));
        QVERIFY2(results.isEmpty(), api);
    }
#endif
    // Neither is a Zstandard or an LZ4 stream returned as if it were not compressed. Either can start with
    // skippable frames, here one with four bytes of data, and both formats give those the same magic number
    struct Undecodable
    {
        QByteArray stream;
        const char* detail;
    };

    QList<Undecodable> undecodable;
    const QByteArray skippableFrame = QByteArray::fromHex("502a4d1804000000") + QByteArray(4, 's');
#ifndef QGC_ENABLE_LZ4
    const QByteArray lz4Frame = QByteArray("\x04\x22\x4D\x18\x64\x40\xA7", 7) + padding;
    undecodable.append({lz4Frame, "^LZ4.*not supported"});
    undecodable.append({skippableFrame + lz4Frame, "^LZ4.*not supported"});
    // The frame that LZ4 wrote before its version 1.4
    undecodable.append({QByteArray("\x02\x21\x4C\x18", 4) + padding, "^LZ4.*not supported"});
#endif
#ifndef QGC_ENABLE_ZSTD
    const QByteArray zstdFrame = QByteArray::fromHex("28b52ffd240001000099e9d851");
    undecodable.append({zstdFrame, "^Zstandard.*not supported"});
    undecodable.append({skippableFrame + zstdFrame, "^Zstandard.*not supported"});
#endif
#if !defined(QGC_ENABLE_LZ4) || !defined(QGC_ENABLE_ZSTD)
    // No frame after the skippable one tells which of the two formats it is
    undecodable.append({skippableFrame, "Zstandard or LZ4.*not supported"});
    // Nor does a second skippable frame that is cut before its size
    undecodable.append({skippableFrame + skippableFrame.first(5), "Zstandard or LZ4.*not supported"});
#endif
    for (const Undecodable& input : std::as_const(undecodable)) {
        QByteArray decoded("to be replaced");
        const Compression::Outcome outcome = Compression::decompressData(input.stream, decoded, kMemoryLimit);
        QCOMPARE(outcome.error, Compression::Error::UnsupportedFormat);
        QVERIFY2(outcome.detail.contains(QRegularExpression(input.detail)), qPrintable(outcome.detail));
        QVERIFY(decoded.isEmpty());
    }

    QVERIFY2(!QFile::exists(markerPath), "An external decompression program was run");
#endif
}

void QGCCompressionTest::_testLegacyLzmaStream_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QByteArray>("stream");

    // What the lzma program of XZ Utils writes for the same content with two dictionary sizes: a power of two, and
    // 3 MiB, which libarchive takes only with the usual first byte and an unknown size
    const QByteArray data = QByteArray::fromHex("003d8889c65436c3174fe29e63242ecb7bfc7f5aa5a6fffffc6a3000");
    const QByteArray streams[] = {QByteArray::fromHex("5d00008000ffffffffffffffff") + data,
                                  QByteArray::fromHex("5d00003000ffffffffffffffff") + data};
    for (const QByteArray& stream : streams) {
        for (const char* api : kStreamEntryPoints) {
            const int dictionaryKiB = static_cast<int>(qFromLittleEndian<quint32>(stream.constData() + 1) / 1024);
            QTest::addRow("%s, dictionary of %d KiB", api, dictionaryKiB) << api << stream;
        }
    }
}

void QGCCompressionTest::_testLegacyLzmaStream()
{
    QFETCH(QString, api);
    QFETCH(QByteArray, stream);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("manifest.json.lzma");
    const Inputs inputs{.path = inputPath, .output = tempDir.filePath("manifest.json")};
    QVERIFY(TestFixtures::writeFile(inputPath, stream));

#ifdef QGC_ENABLE_LZMA
    Results results;
    const Compression::Outcome outcome = _call(api, inputs, &results);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QCOMPARE(outcome.filterName, QStringLiteral("lzma"));
    QCOMPARE(results.data, QByteArray("{\"name\": \"lzma\"}\n"));
#else
    // Not taken for data that is not compressed, as the raw format of libarchive would take it
    Results results;
    const Compression::Outcome outcome = _call(api, inputs, &results);
    QCOMPARE(outcome.error, Compression::Error::UnsupportedFormat);
    QVERIFY2(outcome.message.endsWith(": the compression is not supported by this build"), qPrintable(outcome.message));
    QCOMPARE(outcome.detail, QStringLiteral("LZMA decompression is not supported by this build"));
    QVERIFY(results.isEmpty());
#endif
}

void QGCCompressionTest::_testDataThatIsNotLzma_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("JSON") << QByteArray("{\n    \"version\": 1,\n    \"parameters\": []\n}\n");
    QTest::newRow("XML") << QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<mavlinkcamera/>\n");
    QTest::newRow("ZIP") << ArchiveFixtures::makeZip({{.name = "manifest.json", .data = "{}"}});
    QTest::newRow("text") << QByteArray("Plain text, which no decoder takes\n");
    // The start of a legacy lzma stream that libarchive does not take: a dictionary of 3 MiB and a byte, and one
    // of 3 MiB before a known size
    QTest::newRow("dictionary of an odd size") << QByteArray::fromHex("5d01003000ffffffffffffffff00000000000000000000");
    QTest::newRow("dictionary of 3 MiB, known size")
        << QByteArray::fromHex("5d00003000110000000000000000000000000000000000");
}

void QGCCompressionTest::_testDataThatIsNotLzma()
{
    QFETCH(QByteArray, data);

    // A legacy lzma stream has no magic number. The data is not taken for one in any build: it comes back as it is
    QByteArray decompressed;
    const Compression::Outcome outcome = Compression::decompressData(data, decompressed, kMemoryLimit);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QCOMPARE(outcome.filterName, QStringLiteral("none"));
    QCOMPARE(decompressed, data);
}

void QGCCompressionTest::_testStreamInsideStream_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QByteArray>("stream");
    QTest::addColumn<QByteArray>("content");
    // The filter the outcome names: the inner one if the build decodes it, the outer one otherwise
    QTest::addColumn<QString>("filterName");
    // The detail of the refusal of a stream that this build cannot decode, or empty if it decodes it
    QTest::addColumn<QString>("refusal");

    struct Inner
    {
        const char* filterName;  // nullptr: no stream
        QByteArray stream;
        QByteArray content;
        const char* refusal;
        const char* label = filterName;  // Of the rows
    };

    // What a build without the decoder of each format reports
#ifdef QGC_ENABLE_LZMA
    constexpr const char* lzmaRefusal = "";
#else
    constexpr const char* lzmaRefusal = "LZMA decompression is not supported by this build";
#endif
#ifdef QGC_ENABLE_ZSTD
    constexpr const char* zstdRefusal = "";
#else
    constexpr const char* zstdRefusal = "Zstandard decompression is not supported by this build";
#endif
#ifdef QGC_ENABLE_BZIP2
    constexpr const char* bzip2Refusal = "";
#else
    constexpr const char* bzip2Refusal = "bzip2 decompression is not supported by this build";
#endif
#ifdef QGC_ENABLE_LZ4
    constexpr const char* lz4Refusal = "";
#else
    constexpr const char* lz4Refusal = "LZ4 decompression is not supported by this build";
#endif

    const Inner inners[] = {
        {"lzma",
         QByteArray::fromHex("5d00008000ffffffffffffffff003d8889c65436c3174fe29e63242ecb7bfc7f5aa5a6fffffc6a3000"),
         "{\"name\": \"lzma\"}\n", lzmaRefusal},
        {"zstd", QByteArray::fromHex("28b52ffd00588900007b226e616d65223a20227a737464227d0a"), "{\"name\": \"zstd\"}\n",
         zstdRefusal},
        {"bzip2",
         QByteArray::fromHex("425a6839314159265359299378be00000859800010500010103223401a2000229a62341ed50a600009a834a93"
                             "23ce03fc5dc914e14240a64de2f80"),
         "{\"name\": \"bzip2\"}\n", bzip2Refusal},
        // One block that is not compressed
        {"lz4", QByteArray::fromHex("04224d18604082100000807b226e616d65223a20226c7a34227d0a00000000"),
         "{\"name\": \"lz4\"}\n", lz4Refusal},
        // Data that is no stream is what the outer stream gives in every build
        {nullptr, "{\"name\": \"plain\"}\n", "{\"name\": \"plain\"}\n", "", "plain"},
        // Also text that starts with the signature of bzip2: libarchive takes no such data for a stream
        {nullptr, "BZhello, this is text\n", "BZhello, this is text\n", "", "text like bzip2"},
    };
    const std::pair<const char*, QByteArray (*)(const QByteArray&)> outers[] = {
        {"gzip", &TestFixtures::gzipMember},
        {"xz", &ArchiveFixtures::xzStream},
    };
    for (const Inner& inner : inners) {
        for (const auto& [outerName, wrap] : outers) {
            const bool decoded = (inner.filterName != nullptr) && (*inner.refusal == '\0');
            for (const char* api : kStreamEntryPoints) {
                QTest::addRow("%s in %s, %s", inner.label, outerName, api)
                    << api << wrap(inner.stream) << inner.content
                    << QString::fromLatin1(decoded ? inner.filterName : outerName)
                    << QString::fromLatin1(inner.refusal);
            }
        }
    }
}

void QGCCompressionTest::_testStreamInsideStream()
{
    QFETCH(QString, api);
    QFETCH(QByteArray, stream);
    QFETCH(QByteArray, content);
    QFETCH(QString, filterName);
    QFETCH(QString, refusal);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("manifest.json.inner.outer");
    QVERIFY(TestFixtures::writeFile(inputPath, stream));

    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = inputPath, .output = tempDir.filePath("manifest.json")}, &results);
    QCOMPARE(outcome.filterName, filterName);
    if (refusal.isEmpty()) {
        QVERIFY2(outcome, qPrintable(outcome.description()));
        QCOMPARE(results.data, content);
    } else {
        // Not the inner stream as if it were the content, as libarchive would give it
        QCOMPARE(outcome.error, Compression::Error::UnsupportedFormat);
        QVERIFY2(outcome.message.endsWith(": the compression is not supported by this build"),
                 qPrintable(outcome.message));
        QCOMPARE(outcome.detail, refusal);
        QVERIFY(results.isEmpty());
    }
}

void QGCCompressionTest::_testTruncatedStream_data()
{
    QTest::addColumn<QString>("api");

    for (const char* api : kStreamEntryPoints) {
        QTest::newRow(api) << QString::fromLatin1(api);
    }
}

void QGCCompressionTest::_testRefusalRulesAgreeWithLibarchive()
{
    if (BidAgreement::comparableFilters().isEmpty()) {
        QSKIP("Needs a build with the legacy lzma, bzip2, LZ4 or Zstandard decoder, whose bidder libarchive runs");
    }

    QList<QByteArray> inputs = BidAgreement::edgeInputs();
    for (const char* resource : {"manifest.json.gz", "manifest.json.xz", "manifest.json.zip", "manifest.json.zst",
                                 "manifest.json.bz2", "manifest.json.lz4", "manifest.json.7z", "tree.zip"}) {
        QFile file(QStringLiteral(":/unittest/") + QLatin1String(resource));
        if (file.open(QIODevice::ReadOnly)) {
            inputs.append(file.readAll());
        }
    }
    inputs.append(ArchiveFixtures::xzStream("{\"name\": \"xz\"}\n"));
    inputs.append(QByteArray("{\n    \"version\": 1,\n    \"parameters\": []\n}\n"));

    for (const QByteArray& input : std::as_const(inputs)) {
        const QString disagreement = BidAgreement::disagreement(input);
        QVERIFY2(disagreement.isEmpty(), qPrintable(disagreement));
    }
}

void QGCCompressionTest::_testTruncatedStream()
{
    QFETCH(QString, api);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("cut.gz");
    // Four members of 64 KiB each, cut inside the third: the damage is found after data was decompressed
    QVERIFY(ArchiveFixtures::writeGzipOfZeros(inputPath, 4));
    const QByteArray whole = TestFixtures::readFile(inputPath);
    const QByteArray cut = whole.left(whole.size() * 5 / 8);
    QVERIFY(TestFixtures::writeFile(inputPath, cut));

    Results results;
    const Compression::Outcome outcome =
        _call(api, {.path = inputPath, .output = tempDir.filePath("cut.bin")}, &results);
    QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
    // What libarchive said about the damage is passed on, after words that tell a file from data
    const QRegularExpression damage((api == "decompressFile") ? QStringLiteral("Decompression error.*truncated")
                                                              : QStringLiteral("Read error.*truncated"));
    QVERIFY2(outcome.detail.contains(damage), qPrintable(outcome.detail));
    // The members before the cut are not returned as data, and nothing of them is kept on disk
    QVERIFY(results.isEmpty());
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("cut.gz")});
}

namespace {

/// A gzip member as gzip writes it for a file, with the name of the file in its header
QByteArray gzipMemberWithName(const QByteArray& payload)
{
    const QByteArray member = TestFixtures::gzipMember(payload);
    return QByteArray::fromHex("1f8b0808000000000003") + QByteArray("t.json", 7) + member.mid(10);
}

/// @a bytes with the byte at @a index set to @a value
QByteArray withByte(QByteArray bytes, qsizetype index, quint8 value)
{
    bytes[index] = static_cast<char>(value);
    return bytes;
}

}  // namespace

void QGCCompressionTest::_testEveryPrefixOfAStream_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QByteArray>("stream");
    QTest::addColumn<int>("signatureSize");
    // Whether the build has the decoder: without it, a prefix may also be refused as UnsupportedFormat
    QTest::addColumn<bool>("decoded");
    // Size of a whole first member that the stream starts with, which a prefix of it and one more byte decodes to
    // firstContent (0: none)
    QTest::addColumn<int>("firstSize");
    QTest::addColumn<QByteArray>("firstContent");

    struct Sample
    {
        const char* label;
        QByteArray stream;
        int signatureSize;
        bool decoded = true;
        int firstSize = 0;
        QByteArray firstContent = {};
    };

    const QByteArray content("{\"a\":1}\n");
    const QByteArray member = TestFixtures::gzipMember(content);
#ifdef QGC_ENABLE_ZSTD
    constexpr bool zstd = true;
#else
    constexpr bool zstd = false;
#endif
#ifdef QGC_ENABLE_BZIP2
    constexpr bool bzip2 = true;
#else
    constexpr bool bzip2 = false;
#endif
#ifdef QGC_ENABLE_LZ4
    constexpr bool lz4 = true;
#else
    constexpr bool lz4 = false;
#endif
    const Sample samples[] = {
        {"gzip", member, 2},
        {"gzip with a file name", gzipMemberWithName(content), 2},
        {"gzip of two members", member + gzipMemberWithName(content), 2, true, static_cast<int>(member.size()),
         content},
        {"xz", ArchiveFixtures::xzStream(content), 6},
        // The streams of _testStreamInsideStream, which hold {"name": "zstd"} and so on
        {"zstd", QByteArray::fromHex("28b52ffd00588900007b226e616d65223a20227a737464227d0a"), 4, zstd},
        {"bzip2",
         QByteArray::fromHex("425a6839314159265359299378be00000859800010500010103223401a2000229a62341ed50a600009a834a93"
                             "23ce03fc5dc914e14240a64de2f80"),
         4, bzip2},
        {"lz4", QByteArray::fromHex("04224d18604082100000807b226e616d65223a20226c7a34227d0a00000000"), 4, lz4},
    };
    for (const Sample& sample : samples) {
        for (const char* api : kStreamEntryPoints) {
            QTest::addRow("%s, %s", sample.label, api)
                << QString::fromLatin1(api) << sample.stream << sample.signatureSize << sample.decoded
                << sample.firstSize << sample.firstContent;
        }
    }
}

void QGCCompressionTest::_testEveryPrefixOfAStream()
{
    QFETCH(QString, api);
    QFETCH(QByteArray, stream);
    QFETCH(int, signatureSize);
    QFETCH(bool, decoded);
    QFETCH(int, firstSize);
    QFETCH(QByteArray, firstContent);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("prefix.gz");
    const QString outputPath = tempDir.filePath("prefix");
    for (qsizetype size = 1; size < stream.size(); ++size) {
        const QByteArray prefix = stream.first(size);
        QVERIFY(TestFixtures::writeFile(inputPath, prefix));
        Results results;
        const Compression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);
        (void) QFile::remove(outputPath);
        const QByteArray what = QByteArray::number(size) + " bytes: " + outcome.description().toUtf8();

        if (size < signatureSize) {
            // Too short to be told from data
            QVERIFY2(outcome && (outcome.filterName == QLatin1String("none")), what.constData());
            QVERIFY2(results.data == prefix, what.constData());
        } else if ((firstSize > 0) && ((size == firstSize) || (size == (firstSize + 1)))) {
            // A whole member, and then nothing or less than a signature
            QVERIFY2(outcome, what.constData());
            QVERIFY2(results.data == firstContent, what.constData());
        } else {
            // Truncated: not the bytes as if they were not compressed, and not the part before the cut
            QVERIFY2((outcome.error == Compression::Error::InvalidArchive) ||
                         (!decoded && (outcome.error == Compression::Error::UnsupportedFormat)),
                     what.constData());
            QVERIFY2(results.isEmpty(), what.constData());
        }
    }
}

void QGCCompressionTest::_testDamagedStreamHeader_data()
{
    QTest::addColumn<QByteArray>("stream");
    // What the detail says, if the module and not libarchive finds the damage
    QTest::addColumn<QString>("detail");

    const QByteArray content("{\"a\":1}\n");
    const QByteArray member = TestFixtures::gzipMember(content);
    const QByteArray xz = ArchiveFixtures::xzStream(content);
    const QString damagedGzip =
        QStringLiteral("The input starts with the signature of a gzip stream, but its header is truncated or damaged");

    // A gzip member: the method (always 8, deflate), and the flags, whose top three bits are reserved
    QTest::newRow("gzip, method 7") << withByte(member, 2, 7) << damagedGzip;
    QTest::newRow("gzip, method 9") << withByte(member, 2, 9) << damagedGzip;
    for (const quint8 flags : {0x20, 0x40, 0x80, 0xE0}) {
        QTest::addRow("gzip, flags %02x", flags) << withByte(member, 3, flags) << damagedGzip;
    }
    // The same damage to a second member, which libarchive takes for the end of the stream
    QTest::newRow("gzip, method 9 in the second member") << (member + withByte(member, 2, 9)) << QString();
    QTest::newRow("gzip, flags e0 in the second member") << (member + withByte(member, 3, 0xE0)) << QString();
    // An xz stream: its flags, and the checksum of its header
    QTest::newRow("xz, flags") << withByte(xz, 6, 1) << QString();
    QTest::newRow("xz, header checksum") << withByte(xz, 8, static_cast<quint8>(xz.at(8) ^ 0x01)) << QString();
    // An LZ4 frame: the version in its descriptor, which libarchive's bid checks
    QTest::newRow("lz4, version") << QByteArray::fromHex(
                                         "04224d18204082100000807b226e616d65223a20226c7a34227d0a00000000")
                                  << QString();
    // A stream inside a stream: its header is damaged or cut where the outer stream ends
    const QString damagedInner = QStringLiteral(
        "The decompressed data starts with the signature of a gzip stream, but its header is truncated or damaged");
    QTest::newRow("gzip, method 9, in gzip") << TestFixtures::gzipMember(withByte(member, 2, 9)) << damagedInner;
    QTest::newRow("gzip signature in xz") << ArchiveFixtures::xzStream(member.first(5)) << damagedInner;
}

void QGCCompressionTest::_testDamagedStreamHeader()
{
    QFETCH(QByteArray, stream);
    QFETCH(QString, detail);

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("damaged.gz");
    QVERIFY(TestFixtures::writeFile(inputPath, stream));

    const QString outputPath = tempDir.filePath("damaged");
    for (const char* api : kStreamEntryPoints) {
        Results results;
        const Compression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);
        // Not the damaged bytes as if they were not compressed, nor what came before the damage
        QVERIFY2((outcome.error == Compression::Error::InvalidArchive) ||
                     (outcome.error == Compression::Error::UnsupportedFormat),
                 qPrintable(QStringLiteral("%1: %2").arg(QLatin1String(api), outcome.description())));
        QVERIFY2(results.isEmpty(), api);
        if (!detail.isEmpty()) {
            QCOMPARE(outcome.error, Compression::Error::InvalidArchive);
            QCOMPARE(outcome.detail, detail);
        }
    }
}

void QGCCompressionTest::_testGzipChecksums_data()
{
    QTest::addColumn<QByteArray>("stream");
    // What the stream decompresses to, or nothing if it is damaged
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<bool>("sound");

    // A member ends with the checksum of its data and the length of its data, four bytes each
    const QByteArray first(3000, 'a');
    const QByteArray second(2000, 'b');
    const QByteArray member = TestFixtures::gzipMember(first);
    const auto changed = [](QByteArray bytes, qsizetype index) {
        bytes[index] = static_cast<char>(bytes.at(index) ^ 0x01);
        return bytes;
    };
    // The data of a member whose deflate block is stored, not compressed, can be changed and still inflates:
    // nothing but the checksum tells
    const QByteArray stored = QByteArray::fromHex("1f8b08000000000000ff") +
                              qCompress(first, 0).mid(6, qCompress(first, 0).size() - 10) + member.last(8);

    QTest::newRow("one member") << member << first << true;
    QTest::newRow("two members") << (member + TestFixtures::gzipMember(second)) << (first + second) << true;
    QTest::newRow("stored data") << stored << first << true;
    // What follows the last member is not gzip data, and is left alone
    QTest::newRow("data after the stream") << (member + QByteArray("not a member")) << first << true;

    QTest::newRow("checksum") << changed(member, member.size() - 8) << QByteArray() << false;
    QTest::newRow("length") << changed(member, member.size() - 1) << QByteArray() << false;
    QTest::newRow("checksum of the second member")
        << (member + changed(TestFixtures::gzipMember(second), TestFixtures::gzipMember(second).size() - 8))
        << QByteArray() << false;
    QTest::newRow("stored data, changed") << changed(stored, 100) << QByteArray() << false;
    // The stream a compressor writes for an empty file, whose checksum is zero
    QTest::newRow("checksum of an empty stream")
        << QByteArray::fromHex("1f8b080000000000000303000100000000000000") << QByteArray() << false;
}

void QGCCompressionTest::_testGzipChecksums()
{
    QFETCH(QByteArray, stream);
    QFETCH(QByteArray, content);
    QFETCH(bool, sound);

    // libarchive does not compare the checksum and the length that end a gzip member, so Compression does
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("stream.gz");
    QVERIFY(TestFixtures::writeFile(inputPath, stream));

    const QString outputPath = tempDir.filePath("stream");
    for (const char* api : kStreamEntryPoints) {
        Results results;
        const Compression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);
        (void) QFile::remove(outputPath);
        if (sound) {
            QVERIFY2(outcome, qPrintable(outcome.description()));
            QVERIFY2(results.data == content, api);
        } else {
            QVERIFY2(outcome.error == Compression::Error::InvalidArchive, api);
            QCOMPARE(outcome.detail, QStringLiteral("The gzip data does not match its checksum"));
            QVERIFY2(results.isEmpty(), api);
        }
    }
}

void QGCCompressionTest::_testReadFileChunks()
{
    using Compression::Error;

    // Four gzip members of 64 KiB of zeros each, which arrive in several chunks
    QTemporaryDir tempDir;
    const QString wholePath = tempDir.filePath("whole.gz");
    QVERIFY(ArchiveFixtures::writeGzipOfZeros(wholePath, 4));
    const QByteArray whole = TestFixtures::readFile(wholePath);
    const QByteArray content(4 * 64 * 1024, '\0');

    struct Read
    {
        Compression::Outcome outcome;
        int chunks = 0;
        QByteArray data;
    };

    const auto read = [](const QString& path, qint64 limit, int cancelAt = -1) {
        Read result;
        result.outcome = Compression::readFileChunks(path, limit, [&result, cancelAt](QByteArrayView chunk) {
            result.data.append(chunk);
            return ++result.chunks != cancelAt;
        });
        return result;
    };

    const Read sound = read(wholePath, kMemoryLimit);
    QVERIFY2(sound.outcome, qPrintable(sound.outcome.description()));
    QCOMPARE(sound.outcome.filterName, QStringLiteral("gzip"));
    QCOMPARE_GT(sound.chunks, 1);
    QCOMPARE(sound.data, content);

    // Damage that is found after chunks were given fails all the same: the caller has to discard them
    const QString cutPath = tempDir.filePath("cut.gz");
    QVERIFY(TestFixtures::writeFile(cutPath, whole.left(whole.size() * 5 / 8)));
    const Read cut = read(cutPath, kMemoryLimit);
    QCOMPARE(cut.outcome.error, Error::InvalidArchive);
    QCOMPARE_GT(cut.data.size(), 64 * 1024);

    QByteArray changed = whole;
    changed[changed.size() - 8] = static_cast<char>(changed.at(changed.size() - 8) ^ 0x01);
    const QString changedPath = tempDir.filePath("changed.gz");
    QVERIFY(TestFixtures::writeFile(changedPath, changed));
    const Read damaged = read(changedPath, kMemoryLimit);
    QCOMPARE(damaged.outcome.error, Error::InvalidArchive);
    QCOMPARE(damaged.outcome.detail, QStringLiteral("The gzip data does not match its checksum"));
    QCOMPARE(damaged.data, content);

    const Read limited = read(wholePath, 100 * 1024);
    QCOMPARE(limited.outcome.error, Error::SizeLimitExceeded);
    QCOMPARE_GT(limited.chunks, 0);
    QCOMPARE_LE(limited.data.size(), 100 * 1024);

    // The sink cancels the read: no chunk follows
    const Read cancelled = read(wholePath, kMemoryLimit, 1);
    QCOMPARE(cancelled.outcome.error, Error::Cancelled);
    QCOMPARE(cancelled.chunks, 1);

    const Read missing = read(tempDir.filePath("missing.gz"), kMemoryLimit);
    QCOMPARE(missing.outcome.error, Error::FileNotFound);
    QCOMPARE(missing.chunks, 0);
}

void QGCCompressionTest::_testPlainDataIsShared()
{
    const QByteArray plain("{\"name\": \"not compressed\"}\n");
    QByteArray decompressed;
    const Compression::Outcome outcome = Compression::decompressData(plain, decompressed, kMemoryLimit);
    QVERIFY2(outcome, qPrintable(outcome.description()));
    QCOMPARE(outcome.filterName, QStringLiteral("none"));
    // The same bytes, not a copy of them
    QVERIFY(decompressed.isSharedWith(plain));

    // Bytes that the array does not own are copied: the caller may free them once the call returns
    static const char raw[] = "{\"name\": \"raw\"}\n";
    const QByteArray rawInput = QByteArray::fromRawData(raw, sizeof(raw) - 1);
    QByteArray fromRaw;
    QVERIFY(Compression::decompressData(rawInput, fromRaw, kMemoryLimit));
    QCOMPARE(fromRaw, rawInput);
    QVERIFY(fromRaw.constData() != raw);

    // Still held to the limit
    QByteArray limited("stale");
    const Compression::Outcome tooLarge = Compression::decompressData(plain, limited, plain.size() - 1);
    QCOMPARE(tooLarge.error, Compression::Error::SizeLimitExceeded);
    QVERIFY(limited.isEmpty());
}

void QGCCompressionTest::_testDecompressDataInPlace()
{
    const QByteArray compressed = TestFixtures::readFile(ArchiveFixtures::gzResource);
    QByteArray data;
    QVERIFY(Compression::decompressData(compressed, data, kMemoryLimit));
    QVERIFY(!data.isEmpty());

    // The output array can be the input array, for compressed and uncompressed data
    QByteArray inPlace = compressed;
    QVERIFY(Compression::decompressData(inPlace, inPlace, kMemoryLimit));
    QCOMPARE(inPlace, data);
    QByteArray plain("not compressed");
    QVERIFY(Compression::decompressData(plain, plain, kMemoryLimit));
    QCOMPARE(plain, QByteArray("not compressed"));
}

void QGCCompressionTest::_testReadFile()
{
    using Compression::Error;

    // The content decides whether a file is compressed, whatever its name is
    QTemporaryDir tempDir;
    const QString renamedPath = tempDir.filePath("manifest.tar.gz");
    QVERIFY(TestFixtures::writeFile(renamedPath, TestFixtures::readFile(ArchiveFixtures::xzResource)));
    QByteArray data("to be replaced");
    const Compression::Outcome decompressed = Compression::readFile(renamedPath, data, kMemoryLimit);
    QVERIFY2(decompressed, qPrintable(decompressed.description()));
    QCOMPARE(decompressed.filterName, QStringLiteral("xz"));
    QVERIFY(data.contains("\"name\""));

    // A file that is not compressed is held to the limit like a compressed file
    const QString plainPath = tempDir.filePath("plain.json");
    const QByteArray content(100, 'x');
    QVERIFY(TestFixtures::writeFile(plainPath, content));
    const Compression::Outcome plain = Compression::readFile(plainPath, data, content.size());
    QVERIFY2(plain, qPrintable(plain.description()));
    QCOMPARE(plain.filterName, QStringLiteral("none"));
    QCOMPARE(data, content);
    QCOMPARE(Compression::readFile(plainPath, data, content.size() - 1).error, Error::SizeLimitExceeded);
    QVERIFY(data.isEmpty());

    for (const char* name : {"absent.json", "absent.json.gz"}) {
        data = "to be replaced";
        const Compression::Outcome missing = Compression::readFile(tempDir.filePath(name), data, kMemoryLimit);
        QCOMPARE(missing.error, Error::FileNotFound);
        QVERIFY2(missing.message.endsWith(QLatin1String(name)), qPrintable(missing.message));
        QVERIFY(data.isEmpty());
    }
}

void QGCCompressionTest::_testUnwritableOutput_data()
{
    QTest::addColumn<QString>("api");
    // What prevents writing the output: the output directory is "locked" (not writable), it must be created
    // "in locked", or it must be created "below a file"
    QTest::addColumn<QString>("obstacle");
    QTest::addColumn<QString>("detail");

    const char* const noFile = "Failed to open output file";
    QTest::newRow("extractArchive, locked") << "extractArchive" << "locked" << noFile;
    QTest::newRow("decompressFile, locked") << "decompressFile" << "locked" << noFile;
    QTest::newRow("decompressFile of an empty stream, locked")
        << "decompressFile-empty" << "locked" << "Failed to write output file";
    for (const char* obstacle : {"in locked", "below a file"}) {
        // A directory can be created neither where that is not permitted nor below a file
        QTest::addRow("extractArchive, %s", obstacle)
            << "extractArchive" << obstacle << "Failed to create output directory";
        // A function that is given a file path fails when it opens the file
        QTest::addRow("decompressFile, %s", obstacle) << "decompressFile" << obstacle << noFile;
    }
}

void QGCCompressionTest::_testUnwritableOutput()
{
    QFETCH(QString, api);
    QFETCH(QString, obstacle);
    QFETCH(QString, detail);
    using Compression::Error;

    QTemporaryDir tempDir;
    const QString obstaclePath = tempDir.filePath("obstacle");
    const bool belowFile = (obstacle == "below a file");
    const auto unlock = qScopeGuard([&obstaclePath, belowFile]() {
        if (!belowFile) {
            QFile::setPermissions(obstaclePath,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        }
    });
    if (belowFile) {
        // Nobody can create a directory below a file
        QVERIFY(TestFixtures::writeFile(obstaclePath, "in the way"));
    } else {
#ifndef Q_OS_UNIX
        QSKIP("Needs a directory that the permissions of POSIX keep from being written to");
#else
        QVERIFY(QDir().mkdir(obstaclePath));
        QVERIFY(QFile::setPermissions(obstaclePath, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
        if (QFileInfo(obstaclePath).isWritable()) {
            QSKIP("This user may write to a directory whatever its permissions are");
        }
#endif
    }

    const QString emptyGzipPath = tempDir.filePath("empty.gz");
    QVERIFY(TestFixtures::writeFile(emptyGzipPath, QByteArray::fromHex("1f8b080000000000000303000000000000000000")));
    const QString outputDir = (obstacle == "locked") ? obstaclePath : (obstaclePath + "/made");
    const bool emptyStream = api.endsWith("-empty");
    const bool isStream = _isStreamEntryPoint(api);

    const Compression::Outcome written = _call(
        emptyStream ? api.chopped(6) : api,
        {.path = emptyStream ? emptyGzipPath : (isStream ? ArchiveFixtures::gzResource : ArchiveFixtures::zipResource),
         .output = isStream ? (outputDir + "/manifest.json") : outputDir});
    // The outcome says which file or directory could not be created
    QVERIFY2(written.detail.startsWith(detail), qPrintable(written.detail));

    if (belowFile) {
        // The input is intact: the output failed, and not for lack of permission
        QCOMPARE(written.error, Error::IoError);
        QCOMPARE(TestFixtures::readFile(obstaclePath), QByteArray("in the way"));
    } else {
        // The input is intact and the output could be written elsewhere: the code tells the user what to change
        QCOMPARE(written.error, Error::PermissionDenied);
        QVERIFY2(written.message.endsWith(": no permission to write the output"), qPrintable(written.message));
        QCOMPARE(ArchiveFixtures::entriesBelow(obstaclePath), QStringList());
    }
}

void QGCCompressionTest::_testOutputIsTheInput_data()
{
    QTest::addColumn<QString>("output");

    QTest::newRow("same path") << "stream.gz";
    QTest::newRow("another spelling") << "other/../stream.gz";
    QTest::newRow("symbolic link") << "link";
}

void QGCCompressionTest::_testOutputIsTheInput()
{
    QFETCH(QString, output);

    QTemporaryDir tempDir;
    const QByteArray compressed = TestFixtures::readFile(ArchiveFixtures::gzResource);
    const QString inputPath = tempDir.filePath("stream.gz");
    const QString outputPath = tempDir.filePath(output);
    QVERIFY(TestFixtures::writeFile(inputPath, compressed));
    QVERIFY(QDir(tempDir.path()).mkdir("other"));
    if (output == "link") {
        const bool linked = TestFixtures::createSymbolicLink(inputPath, outputPath);
#ifdef Q_OS_WIN
        if (!linked) {
            QSKIP("This user may not create a symbolic link");
        }
#endif
        QVERIFY(linked);
    }

    const Compression::Outcome outcome = Compression::decompressFile(inputPath, outputPath);

    QCOMPARE(outcome.error, Compression::Error::IoError);
    QCOMPARE(outcome.message, QStringLiteral("Cannot decompress a file onto itself: ") + inputPath);
    // Without the check, the decompressed content would have replaced the compressed file
    QCOMPARE(TestFixtures::readFile(inputPath), compressed);
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()).size(), (output == "link") ? 3 : 2);
}

void QGCCompressionTest::_testInputWithoutPermission_data()
{
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("fileName");

    QTest::newRow("extractArchive") << "extractArchive" << "archive.zip";
    QTest::newRow("decompressFile") << "decompressFile" << "stream.gz";
    // The content, which would have to identify the format, cannot be read either
    QTest::newRow("name without a format") << "extractArchive" << "archive";
}

void QGCCompressionTest::_testInputWithoutPermission()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a file that the permissions of POSIX keep from being read");
#else
    QFETCH(QString, api);
    QFETCH(QString, fileName);
    using Compression::Error;

    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath(fileName);
    const QString outputPath = tempDir.filePath("output");
    const bool isStream = fileName.endsWith(".gz");
    QVERIFY(TestFixtures::writeFile(
        inputPath, TestFixtures::readFile(isStream ? ArchiveFixtures::gzResource : ArchiveFixtures::zipResource)));
    QVERIFY(QFile::setPermissions(inputPath, QFileDevice::WriteOwner));
    if (QFileInfo(inputPath).isReadable()) {
        QSKIP("This user may read a file whatever its permissions are");
    }

    Results results;
    const Compression::Outcome outcome = _call(api, {.path = inputPath, .output = outputPath}, &results);

    // The file exists and may be intact: it is not reported as damaged or of an unknown format
    QCOMPARE(outcome.error, Error::PermissionDenied);
    QCOMPARE(outcome.message, QStringLiteral("No permission to read: ") + inputPath);
    QVERIFY(results.isEmpty());
    QVERIFY(!QFileInfo::exists(outputPath));
#endif
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================
void QGCCompressionTest::_testDecompressToFileSizeLimit()
{
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("zeros.gz");
    const QString outputPath = tempDir.filePath("zeros.bin");
    QVERIFY(ArchiveFixtures::writeGzipOfZeros(inputPath, 1));
    constexpr qint64 wholeSize = 64 * 1024;

    // Over the limit nothing is written, not even a temporary file
    const Compression::Outcome overLimit = Compression::decompressFile(inputPath, outputPath, nullptr, wholeSize - 1);
    QCOMPARE(overLimit.error, Compression::Error::SizeLimitExceeded);
    QVERIFY2(overLimit.message.endsWith(": size limit exceeded"), qPrintable(overLimit.message));
    QCOMPARE(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{QStringLiteral("zeros.gz")});

    // The limit is on the output: a file that decompresses to exactly the limit is written whole
    QVERIFY(Compression::decompressFile(inputPath, outputPath, nullptr, wholeSize));
    QCOMPARE(TestFixtures::readFile(outputPath), QByteArray(wholeSize, '\0'));
}

void QGCCompressionTest::_testDecompressToMemorySizeLimit()
{
    const QByteArray compressed = TestFixtures::readFile(ArchiveFixtures::gzResource);
    constexpr qint64 limit = 10;

    QByteArray refused("to be replaced");
    QCOMPARE(Compression::decompressData(compressed, refused, limit).error, Compression::Error::SizeLimitExceeded);
    QVERIFY(refused.isEmpty());

    // A limit equal to the size of the data lets it through, and so does a larger one
    QByteArray whole;
    QVERIFY(Compression::decompressData(compressed, whole, kMemoryLimit));
    QVERIFY(whole.size() > limit);
    for (const qint64 enough : {qint64(whole.size()), qint64(2 * whole.size())}) {
        QByteArray limited;
        QVERIFY(Compression::decompressData(compressed, limited, enough));
        QCOMPARE(limited, whole);
    }

    // Data that is not compressed is held to the limit too
    QByteArray plain("to be replaced");
    QCOMPARE(Compression::decompressData(QByteArray(100, 'x'), plain, limit).error,
             Compression::Error::SizeLimitExceeded);
    QVERIFY(plain.isEmpty());

    QByteArray read("to be replaced");
    const Compression::Outcome overLimit = Compression::readFile(ArchiveFixtures::gzResource, read, limit);
    QCOMPARE(overLimit.error, Compression::Error::SizeLimitExceeded);
    QVERIFY2(overLimit.message.endsWith(": size limit exceeded"), qPrintable(overLimit.message));
    QVERIFY(read.isEmpty());
    // The limit is on the output: a file that decompresses to exactly the limit is read whole
    QVERIFY(Compression::readFile(ArchiveFixtures::gzResource, read, whole.size()));
    QCOMPARE(read, whole);
}

void QGCCompressionTest::_testDecompressionCancelled()
{
    QTemporaryDir tempDir;
    const QString inputPath = tempDir.filePath("zeros.gz");
    QVERIFY(ArchiveFixtures::writeGzipOfZeros(inputPath, 4));

    int progressCalls = 0;
    const Compression::ProgressCallback cancel = [&progressCalls](qint64, qint64) {
        ++progressCalls;
        return false;
    };
    const Compression::Outcome outcome = Compression::decompressFile(inputPath, tempDir.filePath("zeros.bin"), cancel);

    // The outcome reports the cancellation, not a failure of the input or the output
    QCOMPARE(progressCalls, 1);
    QCOMPARE(outcome.error, Compression::Error::Cancelled);
    QVERIFY2(outcome.message.endsWith(": cancelled"), qPrintable(outcome.message));
    QCOMPARE(ArchiveFixtures::entriesBelow(tempDir.path()), QStringList{QStringLiteral("zeros.gz")});
}

QGC_REGISTER_PORTABLE_TEST(QGCCompressionTest, TestLabel::Unit, TestLabel::Utilities)
