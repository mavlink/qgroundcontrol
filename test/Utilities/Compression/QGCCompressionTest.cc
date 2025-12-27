#include "QGCCompressionTest.h"
#include "QGCCompression.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <QtCore/QCryptographicHash>
#include <QtTest/QTest>

UT_REGISTER_TEST(QGCCompressionTest)

void QGCCompressionTest::init()
{
    UnitTest::init();

    _testDataDir = new QTemporaryDir();
    QVERIFY(_testDataDir->isValid());

    _tempOutputDir = new QTemporaryDir();
    QVERIFY(_tempOutputDir->isValid());

    _createTestData();
}

void QGCCompressionTest::cleanup()
{
    delete _testDataDir;
    _testDataDir = nullptr;

    delete _tempOutputDir;
    _tempOutputDir = nullptr;

    UnitTest::cleanup();
}

void QGCCompressionTest::_createTestData()
{
    // Text file with repeated content (compresses well)
    _testTextFile = _testDataDir->path() + "/test.txt";
    QFile textFile(_testTextFile);
    QVERIFY(textFile.open(QIODevice::WriteOnly));
    QByteArray textContent;
    for (int i = 0; i < 100; ++i) {
        textContent.append(QString("Line %1: The quick brown fox jumps over the lazy dog.\n").arg(i).toUtf8());
    }
    textFile.write(textContent);
    textFile.close();

    // Binary file with sequential bytes
    _testBinaryFile = _testDataDir->path() + "/test.bin";
    QFile binaryFile(_testBinaryFile);
    QVERIFY(binaryFile.open(QIODevice::WriteOnly));
    QByteArray binaryContent;
    binaryContent.resize(4096);
    for (int i = 0; i < binaryContent.size(); ++i) {
        binaryContent[i] = static_cast<char>(i % 256);
    }
    binaryFile.write(binaryContent);
    binaryFile.close();

    // Nested directory structure
    _testNestedDir = _testDataDir->path() + "/nested/subdir";
    QVERIFY(QDir().mkpath(_testNestedDir));

    _testNestedFile = _testNestedDir + "/nested.txt";
    QFile nestedFile(_testNestedFile);
    QVERIFY(nestedFile.open(QIODevice::WriteOnly));
    nestedFile.write("Nested file content");
    nestedFile.close();

    // Empty file (edge case)
    QFile emptyFile(_testDataDir->path() + "/empty.dat");
    QVERIFY(emptyFile.open(QIODevice::WriteOnly));
    emptyFile.close();
}

bool QGCCompressionTest::_compareDirectories(const QString &dir1, const QString &dir2)
{
    QDir d1(dir1);
    QDir d2(dir2);

    if (!d1.exists() || !d2.exists()) {
        qWarning() << "Directory does not exist:" << (!d1.exists() ? dir1 : dir2);
        return false;
    }

    QStringList files1;
    QDirIterator it1(dir1, QDir::Files, QDirIterator::Subdirectories);
    while (it1.hasNext()) {
        it1.next();
        files1.append(d1.relativeFilePath(it1.filePath()));
    }

    QStringList files2;
    QDirIterator it2(dir2, QDir::Files, QDirIterator::Subdirectories);
    while (it2.hasNext()) {
        it2.next();
        files2.append(d2.relativeFilePath(it2.filePath()));
    }

    files1.sort();
    files2.sort();

    if (files1 != files2) {
        qWarning() << "File lists differ - dir1:" << files1 << "dir2:" << files2;
        return false;
    }

    for (const QString &relativePath : files1) {
        if (!_compareFiles(dir1 + "/" + relativePath, dir2 + "/" + relativePath)) {
            qWarning() << "Content mismatch:" << relativePath;
            return false;
        }
    }

    return true;
}

bool QGCCompressionTest::_compareFiles(const QString &file1, const QString &file2)
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

// ============================================================================
// Format Detection Tests
// ============================================================================

void QGCCompressionTest::_testFormatDetection()
{
    // Extension-based detection
    QCOMPARE(QGCCompression::detectFormat("file.zip"), QGCCompression::Format::ZIP);
    QCOMPARE(QGCCompression::detectFormat("file.gz"), QGCCompression::Format::GZIP);
    QCOMPARE(QGCCompression::detectFormat("file.gzip"), QGCCompression::Format::GZIP);
    QCOMPARE(QGCCompression::detectFormat("file.xz"), QGCCompression::Format::XZ);
    QCOMPARE(QGCCompression::detectFormat("file.lzma"), QGCCompression::Format::XZ);
    QCOMPARE(QGCCompression::detectFormat("file.zst"), QGCCompression::Format::ZSTD);
    QCOMPARE(QGCCompression::detectFormat("file.zstd"), QGCCompression::Format::ZSTD);
    QCOMPARE(QGCCompression::detectFormat("file.tar"), QGCCompression::Format::TAR);
    QCOMPARE(QGCCompression::detectFormat("file.tar.gz"), QGCCompression::Format::TAR_GZ);
    QCOMPARE(QGCCompression::detectFormat("file.tgz"), QGCCompression::Format::TAR_GZ);
    QCOMPARE(QGCCompression::detectFormat("file.tar.xz"), QGCCompression::Format::TAR_XZ);
    QCOMPARE(QGCCompression::detectFormat("file.txz"), QGCCompression::Format::TAR_XZ);
    QCOMPARE(QGCCompression::detectFormat("file.unknown"), QGCCompression::Format::Auto);
    QCOMPARE(QGCCompression::detectFormat("file.txt"), QGCCompression::Format::Auto);

    // Case insensitive
    QCOMPARE(QGCCompression::detectFormat("FILE.ZIP"), QGCCompression::Format::ZIP);
    QCOMPARE(QGCCompression::detectFormat("File.Gz"), QGCCompression::Format::GZIP);
}

void QGCCompressionTest::_testFormatHelpers()
{
    // isArchiveFormat
    QVERIFY(QGCCompression::isArchiveFormat(QGCCompression::Format::ZIP));
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
// ZIP Archive Tests
// ============================================================================

void QGCCompressionTest::_testZipRoundtrip()
{
    const QString zipPath = _tempOutputDir->path() + "/roundtrip.zip";
    const QString outputPath = _tempOutputDir->path() + "/roundtrip_output";

    QVERIFY2(QGCCompression::zipDirectory(_testDataDir->path(), zipPath),
             "Failed to create ZIP archive");
    QVERIFY(QFile::exists(zipPath));
    QVERIFY(QFileInfo(zipPath).size() > 0);

    QVERIFY2(QGCCompression::unzipFile(zipPath, outputPath),
             "Failed to extract ZIP archive");
    QVERIFY(QDir(outputPath).exists());

    QVERIFY2(_compareDirectories(_testDataDir->path(), outputPath),
             "Extracted contents don't match original");
}

void QGCCompressionTest::_testZipNestedDirectories()
{
    const QString zipPath = _tempOutputDir->path() + "/nested.zip";
    const QString outputPath = _tempOutputDir->path() + "/nested_output";

    QVERIFY(QGCCompression::zipDirectory(_testDataDir->path(), zipPath));
    QVERIFY(QGCCompression::unzipFile(zipPath, outputPath));

    const QString extractedNested = outputPath + "/nested/subdir/nested.txt";
    QVERIFY2(QFile::exists(extractedNested),
             qPrintable(QString("Nested file not found: %1").arg(extractedNested)));
    QVERIFY(_compareFiles(_testNestedFile, extractedNested));
}

void QGCCompressionTest::_testZipEmptyFiles()
{
    const QString zipPath = _tempOutputDir->path() + "/empty.zip";
    const QString outputPath = _tempOutputDir->path() + "/empty_output";

    QVERIFY(QGCCompression::zipDirectory(_testDataDir->path(), zipPath));
    QVERIFY(QGCCompression::unzipFile(zipPath, outputPath));

    const QString extractedEmpty = outputPath + "/empty.dat";
    QVERIFY2(QFile::exists(extractedEmpty), "Empty file not extracted");
    QCOMPARE(QFileInfo(extractedEmpty).size(), 0);
}

void QGCCompressionTest::_testZipFromResource()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    const QString outputPath = _tempOutputDir->path() + "/resource_output";

    QVERIFY2(QGCCompression::unzipFile(zipResource, outputPath),
             "Failed to extract ZIP from Qt resource");
    QVERIFY2(QDir(outputPath).exists(), "Output directory not created");

    QDir outputDir(outputPath);
    QStringList files = outputDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QVERIFY2(!files.isEmpty(), "No files extracted from ZIP");
}

// ============================================================================
// Single-File Decompression Tests
// ============================================================================

void QGCCompressionTest::_testDecompressGzip()
{
    const QString gzResource = QStringLiteral(":/unittest/manifest.json.gz");
    const QString outputFile = _tempOutputDir->path() + "/manifest_gz.json";

    QVERIFY2(QGCCompression::decompressFile(gzResource, outputFile),
             "Failed to decompress GZIP file");
    QVERIFY(QFile::exists(outputFile));
    QVERIFY(QFileInfo(outputFile).size() > 0);
}

void QGCCompressionTest::_testDecompressLZMA()
{
    const QString xzResource = QStringLiteral(":/unittest/manifest.json.xz");
    const QString outputFile = _tempOutputDir->path() + "/manifest_xz.json";

    QVERIFY2(QGCCompression::decompressFile(xzResource, outputFile),
             "Failed to decompress XZ/LZMA file");
    QVERIFY(QFile::exists(outputFile));
    QVERIFY(QFileInfo(outputFile).size() > 0);
}
