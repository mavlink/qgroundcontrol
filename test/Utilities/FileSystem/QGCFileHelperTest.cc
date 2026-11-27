#include "QGCFileHelperTest.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>

#include "Compression.h"
#include "QGCFileHelper.h"

// ============================================================================
// exists() tests
// ============================================================================
void QGCFileHelperTest::_testExistsRegularFile()
{
    QTemporaryDir tempDir;
    // Create a test file
    const QString filePath = tempDir.filePath("testfile.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();
    QVERIFY(QGCFileHelper::exists(filePath));
}

void QGCFileHelperTest::_testExistsDirectory()
{
    QTemporaryDir tempDir;
    // The temp directory should exist
    QVERIFY(QGCFileHelper::exists(tempDir.path()));
    // Create a subdirectory
    const QString subDir = tempDir.filePath("subdir");
    QVERIFY(QDir().mkdir(subDir));
    QVERIFY(QGCFileHelper::exists(subDir));
}

void QGCFileHelperTest::_testExistsNonExistent()
{
    QTemporaryDir tempDir;
    QVERIFY(!QGCFileHelper::exists("/nonexistent/path/that/does/not/exist"));
    QVERIFY(!QGCFileHelper::exists(tempDir.filePath("nonexistent.txt")));
}

void QGCFileHelperTest::_testExistsQtResource()
{
    // Qt resources with :/ prefix should be handled specially
    // exists() returns true for any :/ path (Qt resource system)
    QVERIFY(QGCFileHelper::exists(":/unittest/manifest.json.zip"));
    // Even non-existent resource paths return true (by design - Qt handles validation)
    // This is intentional: the function assumes :/ paths are valid resources
    QVERIFY(QGCFileHelper::exists(":/any/resource/path"));
}

// ============================================================================
// isInDirectory() tests
// ============================================================================
void QGCFileHelperTest::_testIsInDirectory_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("inside");

    // "/" or the drive, whose path ends in the separator as no other directory's does
    const QString root = QDir::rootPath();
    const QString directory = root + QStringLiteral("output");

    QTest::newRow("the directory") << directory << directory << true;
    QTest::newRow("below it") << directory << directory + QStringLiteral("/nested/file.txt") << true;
    QTest::newRow("its name continued") << directory << directory + QStringLiteral("-other/file.txt") << false;
    QTest::newRow("beside it") << directory << root + QStringLiteral("file.txt") << false;
    QTest::newRow("above it") << directory << root << false;
    QTest::newRow("the root") << root << root << true;
    QTest::newRow("below the root") << root << directory + QStringLiteral("/file.txt") << true;
}

void QGCFileHelperTest::_testIsInDirectory()
{
    QFETCH(QString, directory);
    QFETCH(QString, path);
    QFETCH(bool, inside);

    QCOMPARE(QGCFileHelper::isInDirectory(directory, path), inside);
}

// ============================================================================
// fileNameIssue() and localFileName() tests
// ============================================================================
namespace {

QString issueName(QGCFileHelper::FileNameIssue issue)
{
    switch (issue) {
        case QGCFileHelper::FileNameIssue::None:
            return QStringLiteral("none");
        case QGCFileHelper::FileNameIssue::Empty:
            return QStringLiteral("empty");
        case QGCFileHelper::FileNameIssue::ForbiddenCharacter:
            return QStringLiteral("character");
        case QGCFileHelper::FileNameIssue::TrailingDotOrSpace:
            return QStringLiteral("ending");
        case QGCFileHelper::FileNameIssue::DeviceName:
            return QStringLiteral("device");
        case QGCFileHelper::FileNameIssue::TooLong:
            return QStringLiteral("length");
    }
    return QStringLiteral("unknown");
}

}  // namespace

void QGCFileHelperTest::_testFileNameIssue_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("issue");

    const auto row = [](const char* tag, const QString& name, const char* issue) {
        QTest::newRow(tag) << name << QString::fromLatin1(issue);
    };

    row("a name", QStringLiteral("parameters.json.xz"), "none");
    row("empty", QString(), "empty");

    // Windows does not allow these characters, and a separator ends a name everywhere
    row("an alternate data stream", QStringLiteral("evil.json:stream"), "character");
    row("relative to a drive", QStringLiteral("C:evil.json"), "character");
    for (const char* const character : {"<", ">", "\"", "|", "?", "*", "/", "\\"}) {
        row(qPrintable(QStringLiteral("the character %1").arg(QLatin1String(character))),
            QStringLiteral("a%1b").arg(QLatin1String(character)), "character");
    }
    row("a line feed", QStringLiteral("evil\n.json"), "character");
    row("a bell", QStringLiteral("bell\a.txt"), "character");
    row("U+0000", QStringLiteral("a") + QChar(0) + QStringLiteral("b"), "character");
    row("U+001F", QStringLiteral("a") + QChar(0x1F) + QStringLiteral("b"), "character");
    // Not a control character for Windows, which forbids 1 to 31 only
    row("U+007F", QStringLiteral("a") + QChar(0x7F) + QStringLiteral("b"), "none");

    // Windows drops a trailing dot or space
    row("a trailing dot", QStringLiteral("evil.json."), "ending");
    row("a trailing space", QStringLiteral("evil.json "), "ending");
    row("dot", QStringLiteral("."), "ending");
    row("dot dot", QStringLiteral(".."), "ending");
    row("three dots", QStringLiteral("..."), "ending");
    row("dots and a space", QStringLiteral(".. "), "ending");
    row("a leading dot", QStringLiteral(".hidden"), "none");
    row("a leading space", QStringLiteral(" leading"), "none");

    // Device names of Windows, in any case, also with an extension and with spaces before it
    for (const char* const device : {"CON", "PRN", "AUX", "NUL", "COM0", "COM1", "COM9", "LPT0", "LPT1", "LPT9",
                                     "CONIN$", "CONOUT$", "con", "Nul", "conout$"}) {
        row(qPrintable(QStringLiteral("the device %1").arg(QLatin1String(device))), QLatin1String(device), "device");
    }
    row("a device with an extension", QStringLiteral("com1.json"), "device");
    row("a device with the extension of a tile set", QStringLiteral("COM1.db"), "device");
    row("a device with two extensions", QStringLiteral("NUL.tar.gz"), "device");
    row("console input with an extension", QStringLiteral("conin$.txt"), "device");
    row("a device with a space before the extension", QStringLiteral("CON .json"), "device");
    row("a device with spaces before the extension", QStringLiteral("aux  .log"), "device");
    row("a serial port with a superscript 1", QStringLiteral("COM\u00B9"), "device");
    row("a printer port with a superscript 2", QStringLiteral("lpt\u00B2.txt"), "device");
    row("a printer port with a superscript 3", QStringLiteral("LPT\u00B3"), "device");
    // The first issue counts
    row("a device with a trailing dot", QStringLiteral("CON."), "ending");
    row("a device with a colon", QStringLiteral("CON:"), "character");
    // Names that only start like a device
    row("a serial port of two digits", QStringLiteral("COM10.json"), "none");
    row("a serial port with a superscript 4", QStringLiteral("COM\u2074"), "none");
    row("a serial port without a digit", QStringLiteral("COM"), "none");
    row("a device name inside a word", QStringLiteral("console.txt"), "none");
    row("a device name before a hyphen", QStringLiteral("nul-device.txt"), "none");
    row("a device name after a dot", QStringLiteral(".CON"), "none");

    // At most 255 bytes in UTF-8, whatever the number of characters
    row("255 bytes", QString(255, QLatin1Char('a')), "none");
    row("256 bytes", QString(256, QLatin1Char('a')), "length");
    row("255 bytes of UTF-8", QString(127, QChar(0x00E9)) + QLatin1Char('a'), "none");
    row("256 bytes of UTF-8", QString(128, QChar(0x00E9)), "length");
    row("255 bytes of three-byte characters", QString(85, QChar(0x65E5)), "none");
    row("258 bytes of three-byte characters", QString(86, QChar(0x65E5)), "length");
    row("255 bytes with four-byte characters", QString::fromUcs4(U"\U0001F600").repeated(63) + QStringLiteral("abc"),
        "none");
    row("256 bytes of four-byte characters", QString::fromUcs4(U"\U0001F600").repeated(64), "length");
}

void QGCFileHelperTest::_testFileNameIssue()
{
    QFETCH(QString, name);
    QFETCH(QString, issue);

    QCOMPARE(issueName(QGCFileHelper::fileNameIssue(name)), issue);
}

void QGCFileHelperTest::_testLocalFileName_data()
{
    QTest::addColumn<QString>("name");
    // Empty if the name is refused
    QTest::addColumn<QString>("fileName");

    QTest::newRow("a name") << "parameters.json.xz" << "parameters.json.xz";
    QTest::newRow("a path") << "/etc/extras/parameters.json.xz" << "parameters.json.xz";
    QTest::newRow("backslashes") << "..\\..\\evil.json" << "evil.json";
    QTest::newRow("both separators") << "a/..\\b\\../evil.json" << "evil.json";
    QTest::newRow("a drive path") << "C:\\Windows\\evil.json" << "evil.json";
    // Callers give a URL's name decoded: what is left encoded is a part of the name
    QTest::newRow("an encoded separator") << "..%5C..%5Cevil.json" << "..%5C..%5Cevil.json";
    QTest::newRow("empty") << "" << "";
    QTest::newRow("dot") << "." << "";
    QTest::newRow("dot dot") << "a\\.." << "";
    QTest::newRow("a trailing slash") << "dir/" << "";
    QTest::newRow("a trailing backslash") << "dir\\" << "";
    // The last component has to pass fileNameIssue(), whose cases _testFileNameIssue checks
    QTest::newRow("a device") << "dir/NUL" << "";
    QTest::newRow("an alternate data stream") << "evil.json:stream" << "";
    QTest::newRow("a device name and more") << "COM10.json" << "COM10.json";
    QTest::newRow("256 bytes of UTF-8") << QString(128, QChar(0x00E9)) << "";
}

void QGCFileHelperTest::_testLocalFileName()
{
    QFETCH(QString, name);
    QFETCH(QString, fileName);

    QCOMPARE(QGCFileHelper::localFileName(name), fileName);
}

// ============================================================================
// ensureDirectoryExists() tests
// ============================================================================
void QGCFileHelperTest::_testEnsureDirectoryExistsAlreadyExists()
{
    QTemporaryDir tempDir;
    // Temp directory already exists
    QVERIFY(QGCFileHelper::ensureDirectoryExists(tempDir.path()));
    QVERIFY(QDir(tempDir.path()).exists());
}

void QGCFileHelperTest::_testEnsureDirectoryExistsCreate()
{
    QTemporaryDir tempDir;
    const QString newDir = tempDir.filePath("newdir");
    QVERIFY(!QDir(newDir).exists());
    QVERIFY(QGCFileHelper::ensureDirectoryExists(newDir));
    QVERIFY(QDir(newDir).exists());
}

void QGCFileHelperTest::_testEnsureDirectoryExistsNested()
{
    QTemporaryDir tempDir;
    const QString nestedDir = tempDir.filePath("level1/level2/level3");
    QVERIFY(!QDir(nestedDir).exists());
    QVERIFY(QGCFileHelper::ensureDirectoryExists(nestedDir));
    QVERIFY(QDir(nestedDir).exists());
}

// ============================================================================
// ensureParentExists() tests
// ============================================================================
void QGCFileHelperTest::_testEnsureParentExistsAlreadyExists()
{
    QTemporaryDir tempDir;
    // Parent of a file in temp dir already exists
    const QString filePath = tempDir.filePath("somefile.txt");
    QVERIFY(QGCFileHelper::ensureParentExists(filePath));
    QVERIFY(QDir(tempDir.path()).exists());
}

void QGCFileHelperTest::_testEnsureParentExistsCreate()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("newparent/somefile.txt");
    const QString parentDir = tempDir.filePath("newparent");
    QVERIFY(!QDir(parentDir).exists());
    QVERIFY(QGCFileHelper::ensureParentExists(filePath));
    QVERIFY(QDir(parentDir).exists());
}

void QGCFileHelperTest::_testEnsureParentExistsNested()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("a/b/c/somefile.txt");
    const QString parentDir = tempDir.filePath("a/b/c");
    QVERIFY(!QDir(parentDir).exists());
    QVERIFY(QGCFileHelper::ensureParentExists(filePath));
    QVERIFY(QDir(parentDir).exists());
}

// ============================================================================
// optimalBufferSize() tests
// ============================================================================
void QGCFileHelperTest::_testOptimalBufferSizeConstants()
{
    // Verify constants are properly defined
    QVERIFY(QGCFileHelper::kBufferSizeMin == 16384);      // 16KB
    QVERIFY(QGCFileHelper::kBufferSizeMax == 131072);     // 128KB
    QVERIFY(QGCFileHelper::kBufferSizeDefault == 65536);  // 64KB
    // Sanity: min < default < max
    QVERIFY(QGCFileHelper::kBufferSizeMin < QGCFileHelper::kBufferSizeDefault);
    QVERIFY(QGCFileHelper::kBufferSizeDefault < QGCFileHelper::kBufferSizeMax);
}

void QGCFileHelperTest::_testOptimalBufferSizeWithinBounds()
{
    const size_t size = QGCFileHelper::optimalBufferSize();
    QVERIFY(size >= QGCFileHelper::kBufferSizeMin);
    QVERIFY(size <= QGCFileHelper::kBufferSizeMax);
}

void QGCFileHelperTest::_testOptimalBufferSizeCached()
{
    QTemporaryDir tempDir;
    // Multiple calls should return the same value (cached)
    const size_t first = QGCFileHelper::optimalBufferSize();
    const size_t second = QGCFileHelper::optimalBufferSize();
    const size_t third = QGCFileHelper::optimalBufferSize(tempDir.path());
    QCOMPARE(first, second);
    QCOMPARE(second, third);
}

void QGCFileHelperTest::_testOptimalBufferSizeWithPath()
{
    QTemporaryDir tempDir;
    // With path argument, should still return valid size within bounds
    const size_t size = QGCFileHelper::optimalBufferSize(tempDir.path());
    QVERIFY(size >= QGCFileHelper::kBufferSizeMin);
    QVERIFY(size <= QGCFileHelper::kBufferSizeMax);
}

// ============================================================================
// atomicWrite() tests
// ============================================================================
void QGCFileHelperTest::_testAtomicWriteBasic()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("atomic_basic.txt");
    const QByteArray data = "Hello, atomic write!";
    QVERIFY(!QFile::exists(filePath));
    QVERIFY(QGCFileHelper::atomicWrite(filePath, data));
    QVERIFY(QFile::exists(filePath));
    // Verify content
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), data);
}

void QGCFileHelperTest::_testAtomicWriteOverwrite()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("atomic_overwrite.txt");
    const QByteArray originalData = "Original content";
    const QByteArray newData = "New content after overwrite";
    // Write original
    QVERIFY(QGCFileHelper::atomicWrite(filePath, originalData));
    // Overwrite
    QVERIFY(QGCFileHelper::atomicWrite(filePath, newData));
    // Verify new content
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), newData);
    // Verify no temp file left behind
    QVERIFY(!QFile::exists(filePath + ".tmp"));
}

void QGCFileHelperTest::_testAtomicWriteCreatesParent()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("nested/dirs/atomic.txt");
    const QByteArray data = "Data in nested directory";
    QVERIFY(!QFile::exists(filePath));
    QVERIFY(QGCFileHelper::atomicWrite(filePath, data));
    QVERIFY(QFile::exists(filePath));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), data);
}

void QGCFileHelperTest::_testAtomicWriteEmptyPath()
{
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("file path is empty"));
    QVERIFY(!QGCFileHelper::atomicWrite(QString(), QByteArray("data")));
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testAtomicWriteEmptyData()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("atomic_empty.txt");
    // Empty data should succeed and create empty file
    QVERIFY(QGCFileHelper::atomicWrite(filePath, QByteArray()));
    QVERIFY(QFile::exists(filePath));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.size(), 0);
}

void QGCFileHelperTest::_testReadCompressedFile()
{
    // The test manifest is a few bytes
    constexpr qint64 maxManifestBytes = 1024;
    QByteArray data;
    const Compression::Outcome read =
        Compression::decompressFileToMemory(":/unittest/manifest.json.gz", data, maxManifestBytes);
    QVERIFY2(read, qPrintable(read.description()));
    QVERIFY(data.contains("\"name\""));
}

// ============================================================================
// Disk space utilities tests
// ============================================================================
void QGCFileHelperTest::_testAvailableDiskSpaceBasic()
{
    QTemporaryDir tempDir;
    // Should return positive value for valid path
    const qint64 available = QGCFileHelper::availableDiskSpace(tempDir.path());
    QVERIFY(available > 0);
    // Should work for file path too (uses parent directory)
    const QString filePath = tempDir.filePath("testfile.txt");
    const qint64 available2 = QGCFileHelper::availableDiskSpace(filePath);
    QVERIFY(available2 > 0);
}

void QGCFileHelperTest::_testAvailableDiskSpaceEmptyPath()
{
    // Empty path should return -1
    QCOMPARE(QGCFileHelper::availableDiskSpace(QString()), qint64(-1));
}

void QGCFileHelperTest::_testHasSufficientDiskSpaceBasic()
{
    QTemporaryDir tempDir;
    // Should have space for 1 byte
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), 1));
    // Should have space for 1KB
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), 1024));
    // Should probably not have space for 1 exabyte (10^18 bytes)
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("Insufficient disk space"));
    QVERIFY(!QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), 1000000000000000000LL));
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testHasSufficientDiskSpaceZeroBytes()
{
    QTemporaryDir tempDir;
    // Zero bytes should always succeed
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), 0));
    // Negative bytes should also succeed (treated as unknown/nothing to check)
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), -1));
}

void QGCFileHelperTest::_testHasSufficientDiskSpaceWithMargin()
{
    QTemporaryDir tempDir;
    // Get available space
    const qint64 available = QGCFileHelper::availableDiskSpace(tempDir.path());
    QVERIFY(available > 0);
    // Should have space for half the available (with 10% margin)
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), available / 2, 1.1));
    // Should not have space for 2x available
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("Insufficient disk space"));
    QVERIFY(!QGCFileHelper::hasSufficientDiskSpace(tempDir.path(), available * 2, 1.0));
    verifyExpectedLogMessage();
}

// ============================================================================
// URL/Path utilities tests
// ============================================================================
void QGCFileHelperTest::_testToLocalPathPlainPaths()
{
    // Plain paths should pass through unchanged
    QCOMPARE(QGCFileHelper::toLocalPath("/path/to/file.txt"), QString("/path/to/file.txt"));
    QCOMPARE(QGCFileHelper::toLocalPath("relative/path.txt"), QString("relative/path.txt"));
    QCOMPARE(QGCFileHelper::toLocalPath(""), QString(""));
    // Qt resource paths should pass through
    QCOMPARE(QGCFileHelper::toLocalPath(":/resource/path"), QString(":/resource/path"));
}

void QGCFileHelperTest::_testToLocalPathFileUrls()
{
    // file:// URLs should be converted to local paths
    QCOMPARE(QGCFileHelper::toLocalPath("file:///path/to/file.txt"), QString("/path/to/file.txt"));
    QCOMPARE(QGCFileHelper::toLocalPath("file:///home/user/test.zip"), QString("/home/user/test.zip"));
    // QUrl overload
    QUrl fileUrl = QUrl::fromLocalFile("/path/to/file.txt");
    QCOMPARE(QGCFileHelper::toLocalPath(fileUrl), QString("/path/to/file.txt"));
}

void QGCFileHelperTest::_testToLocalPathQrcUrls()
{
    // qrc:// URLs should be converted to :/ paths
    QCOMPARE(QGCFileHelper::toLocalPath("qrc:/resource/path"), QString(":/resource/path"));
    QCOMPARE(QGCFileHelper::toLocalPath("qrc:///resource/path"), QString(":/resource/path"));
    // QUrl overload
    QUrl qrcUrl;
    qrcUrl.setScheme("qrc");
    qrcUrl.setPath("/test/file.txt");
    QCOMPARE(QGCFileHelper::toLocalPath(qrcUrl), QString(":/test/file.txt"));

    // QUrl(":/resource/path") is invalid in Qt but should still round-trip to resource path.
    QCOMPARE(QGCFileHelper::toLocalPath(QUrl(QStringLiteral(":/resource/path"))), QStringLiteral(":/resource/path"));
}

void QGCFileHelperTest::_testToLocalPathCompressionInterop()
{
    QTemporaryDir tempDir;
    const QString archivePath = QGCFileHelper::toLocalPath(QUrl(QStringLiteral("qrc:/unittest/manifest.json.zip")));
    QCOMPARE(archivePath, QStringLiteral(":/unittest/manifest.json.zip"));
    QVERIFY(Compression::isArchiveFile(archivePath));

    const QString extractDir = tempDir.filePath("from_url_extract");
    QVERIFY(Compression::extractArchive(archivePath, extractDir));
    QVERIFY(QFile::exists(extractDir + QStringLiteral("/manifest.json")));

    const QString gzipPath = QGCFileHelper::toLocalPath(QUrl(QStringLiteral("qrc:/unittest/manifest.json.gz")));
    QCOMPARE(gzipPath, QStringLiteral(":/unittest/manifest.json.gz"));

    // The test manifest is a few bytes
    constexpr qint64 maxManifestBytes = 1024;
    QByteArray decompressed;
    QVERIFY(Compression::decompressFileToMemory(gzipPath, decompressed, maxManifestBytes));
    QVERIFY(decompressed.contains("\"name\""));
}

void QGCFileHelperTest::_testIsLocalPath()
{
    // Local paths
    QVERIFY(QGCFileHelper::isLocalPath("/path/to/file"));
    QVERIFY(QGCFileHelper::isLocalPath("relative/path"));
    QVERIFY(QGCFileHelper::isLocalPath(":/resource/path"));
    QVERIFY(QGCFileHelper::isLocalPath("file:///path/to/file"));
    QVERIFY(QGCFileHelper::isLocalPath("qrc:/resource"));
    // Empty path
    QVERIFY(!QGCFileHelper::isLocalPath(""));
    // Network URLs (not local)
    QVERIFY(!QGCFileHelper::isLocalPath("http://example.com/file.txt"));
    QVERIFY(!QGCFileHelper::isLocalPath("https://example.com/file.txt"));
}

void QGCFileHelperTest::_testIsQtResource()
{
    // Qt resource paths
    QVERIFY(QGCFileHelper::isQtResource(":/path/to/resource"));
    QVERIFY(QGCFileHelper::isQtResource(":/"));
    QVERIFY(QGCFileHelper::isQtResource("qrc:/path/to/resource"));
    QVERIFY(QGCFileHelper::isQtResource("QRC:/path/to/resource"));  // Case insensitive
    // Not Qt resource paths
    QVERIFY(!QGCFileHelper::isQtResource("/path/to/file"));
    QVERIFY(!QGCFileHelper::isQtResource("file:///path"));
    QVERIFY(!QGCFileHelper::isQtResource("relative/path"));
    QVERIFY(!QGCFileHelper::isQtResource(""));
}

// ============================================================================
// Checksum utilities tests
// ============================================================================
void QGCFileHelperTest::_testComputeHash()
{
    // Known test vectors
    // SHA-256 of empty string
    QCOMPARE(QGCFileHelper::computeHash(QByteArray()),
             QString("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    // SHA-256 of "hello"
    QCOMPARE(QGCFileHelper::computeHash(QByteArray("hello")),
             QString("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"));
    // MD5 of "hello"
    QCOMPARE(QGCFileHelper::computeHash(QByteArray("hello"), QCryptographicHash::Md5),
             QString("5d41402abc4b2a76b9719d911017c592"));
    // SHA-1 of "hello"
    QCOMPARE(QGCFileHelper::computeHash(QByteArray("hello"), QCryptographicHash::Sha1),
             QString("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d"));
}

void QGCFileHelperTest::_testComputeFileHash()
{
    QTemporaryDir tempDir;
    // Create a test file
    const QString filePath = tempDir.filePath("hashtest.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("hello");
    file.close();
    // Compute hash of file
    const QString hash = QGCFileHelper::computeFileHash(filePath);
    QCOMPARE(hash, QString("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"));
    // Different algorithm
    const QString md5Hash = QGCFileHelper::computeFileHash(filePath, QCryptographicHash::Md5);
    QCOMPARE(md5Hash, QString("5d41402abc4b2a76b9719d911017c592"));
    // Qt resource file
    const QString resourceHash = QGCFileHelper::computeFileHash(":/unittest/manifest.json.gz");
    QVERIFY(!resourceHash.isEmpty());
    QCOMPARE(resourceHash.length(), 64);  // SHA-256 = 32 bytes = 64 hex chars
    // Non-existent file should return empty
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("computeFileHash: failed to open"));
    const QString noFileHash = QGCFileHelper::computeFileHash("/nonexistent/file.txt");
    QVERIFY(noFileHash.isEmpty());
    verifyExpectedLogMessage();
    // Empty path should return empty
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("computeFileHash: empty file path"));
    QVERIFY(QGCFileHelper::computeFileHash(QString()).isEmpty());
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testVerifyFileHash()
{
    QTemporaryDir tempDir;
    // Create a test file
    const QString filePath = tempDir.filePath("verifytest.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("hello");
    file.close();
    const QString correctHash = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
    const QString wrongHash = "0000000000000000000000000000000000000000000000000000000000000000";
    // Correct hash should verify
    QVERIFY(QGCFileHelper::verifyFileHash(filePath, correctHash));
    // Case insensitive
    QVERIFY(QGCFileHelper::verifyFileHash(filePath, correctHash.toUpper()));
    // Wrong hash should fail
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("verifyFileHash: hash mismatch"));
    QVERIFY(!QGCFileHelper::verifyFileHash(filePath, wrongHash));
    verifyExpectedLogMessage();
    // Empty expected hash should fail
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("verifyFileHash: empty expected hash"));
    QVERIFY(!QGCFileHelper::verifyFileHash(filePath, QString()));
    verifyExpectedLogMessage();
    // Non-existent file should fail
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("computeFileHash: failed to open"));
    QVERIFY(!QGCFileHelper::verifyFileHash("/nonexistent/file.txt", correctHash));
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testHashAlgorithmName()
{
    QCOMPARE(QGCFileHelper::hashAlgorithmName(QCryptographicHash::Md5), QString("MD5"));
    QCOMPARE(QGCFileHelper::hashAlgorithmName(QCryptographicHash::Sha1), QString("SHA-1"));
    QCOMPARE(QGCFileHelper::hashAlgorithmName(QCryptographicHash::Sha256), QString("SHA-256"));
    QCOMPARE(QGCFileHelper::hashAlgorithmName(QCryptographicHash::Sha512), QString("SHA-512"));
    QCOMPARE(QGCFileHelper::hashAlgorithmName(QCryptographicHash::Blake2b_256), QString("BLAKE2b-256"));
}

// ============================================================================
// Temporary file utilities tests
// ============================================================================
void QGCFileHelperTest::_testTempDirectory()
{
    const QString tempDir = QGCFileHelper::tempDirectory();
    QVERIFY(!tempDir.isEmpty());
    QVERIFY(QDir(tempDir).exists());
}

void QGCFileHelperTest::_testUniqueTempPath()
{
    // Basic usage
    const QString path1 = QGCFileHelper::uniqueTempPath();
    QVERIFY(!path1.isEmpty());
    QVERIFY(path1.startsWith(QGCFileHelper::tempDirectory()));
    QVERIFY(!QFile::exists(path1));  // File should not exist
    // Multiple calls should return unique paths
    const QString path2 = QGCFileHelper::uniqueTempPath();
    QVERIFY(path1 != path2);
    // With template
    const QString path3 = QGCFileHelper::uniqueTempPath("test_XXXXXX.json");
    QVERIFY(path3.endsWith(".json"));
    QVERIFY(!QFile::exists(path3));
    // Template without XXXXXX gets it added
    const QString path4 = QGCFileHelper::uniqueTempPath("noplaceholder.txt");
    QVERIFY(path4.contains("noplaceholder"));
    QVERIFY(path4.endsWith(".txt"));
}

void QGCFileHelperTest::_testCreateTempFile()
{
    const QByteArray testData = "Hello, temporary file!";
    auto temp = QGCFileHelper::createTempFile(testData);
    QVERIFY(temp != nullptr);
    QVERIFY(temp->isOpen());
    QVERIFY(QFile::exists(temp->fileName()));
    // File should be positioned at start for reading
    QCOMPARE(temp->pos(), qint64(0));
    QCOMPARE(temp->readAll(), testData);
    // File should be deleted when temp goes out of scope
    const QString tempPath = temp->fileName();
    temp.reset();
    QVERIFY(!QFile::exists(tempPath));
}

void QGCFileHelperTest::_testCreateTempFileWithTemplate()
{
    const QByteArray testData = "Test data";
    // Template with extension
    auto temp1 = QGCFileHelper::createTempFile(testData, "myapp_XXXXXX.json");
    QVERIFY(temp1 != nullptr);
    QVERIFY(temp1->fileName().endsWith(".json"));
    // Template without XXXXXX
    auto temp2 = QGCFileHelper::createTempFile(testData, "simple.dat");
    QVERIFY(temp2 != nullptr);
    QVERIFY(temp2->fileName().contains("simple"));
    QVERIFY(temp2->fileName().endsWith(".dat"));
    // Empty data should work
    auto temp3 = QGCFileHelper::createTempFile(QByteArray());
    QVERIFY(temp3 != nullptr);
    QCOMPARE(temp3->size(), qint64(0));
}

void QGCFileHelperTest::_testCreateTempCopy()
{
    QTemporaryDir tempDir;
    // Create source file
    const QString sourcePath = tempDir.filePath("source.txt");
    const QByteArray sourceData = "Source file content";
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write(sourceData);
    source.close();
    // Create temp copy
    auto copy = QGCFileHelper::createTempCopy(sourcePath);
    QVERIFY(copy != nullptr);
    QVERIFY(copy->isOpen());
    // Verify content matches
    QCOMPARE(copy->readAll(), sourceData);
    // Copy with custom template
    auto copy2 = QGCFileHelper::createTempCopy(sourcePath, "backup_XXXXXX.txt");
    QVERIFY(copy2 != nullptr);
    QVERIFY(copy2->fileName().contains("backup"));
    // Non-existent source should return nullptr
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("createTempCopy: failed to open source"));
    auto noCopy = QGCFileHelper::createTempCopy("/nonexistent/file.txt");
    QVERIFY(noCopy == nullptr);
    verifyExpectedLogMessage();
    // Empty path should return nullptr
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("createTempCopy: source path is empty"));
    auto emptyCopy = QGCFileHelper::createTempCopy(QString());
    QVERIFY(emptyCopy == nullptr);
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testReplaceFileFromTemp()
{
    QTemporaryDir tempDir;
    const QString targetPath = tempDir.filePath("target.txt");
    const QByteArray newContent = "New file content";
    // Create temp file with new content
    auto temp = QGCFileHelper::createTempFile(newContent);
    QVERIFY(temp != nullptr);
    // Replace (target doesn't exist yet)
    QVERIFY(QGCFileHelper::replaceFileFromTemp(temp.get(), targetPath));
    temp.release();  // Ownership transferred
    // Verify target has new content
    QFile target(targetPath);
    QVERIFY(target.open(QIODevice::ReadOnly));
    QCOMPARE(target.readAll(), newContent);
    target.close();
    // Replace again (target exists)
    const QByteArray newerContent = "Even newer content";
    auto temp2 = QGCFileHelper::createTempFile(newerContent);
    QVERIFY(QGCFileHelper::replaceFileFromTemp(temp2.get(), targetPath));
    temp2.release();
    QVERIFY(target.open(QIODevice::ReadOnly));
    QCOMPARE(target.readAll(), newerContent);
}

void QGCFileHelperTest::_testReplaceFileFromTempWithBackup()
{
    QTemporaryDir tempDir;
    const QString targetPath = tempDir.filePath("targetWithBackup.txt");
    const QString backupPath = tempDir.filePath("targetWithBackup.txt.bak");
    const QByteArray originalContent = "Original content";
    const QByteArray newContent = "New content";
    // Create original target
    QFile original(targetPath);
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write(originalContent);
    original.close();
    // Create temp with new content and replace with backup
    auto temp = QGCFileHelper::createTempFile(newContent);
    QVERIFY(QGCFileHelper::replaceFileFromTemp(temp.get(), targetPath, backupPath));
    temp.release();
    // Verify target has new content
    QFile target(targetPath);
    QVERIFY(target.open(QIODevice::ReadOnly));
    QCOMPARE(target.readAll(), newContent);
    target.close();
    // Verify backup has original content
    QFile backup(backupPath);
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), originalContent);
}

void QGCFileHelperTest::_testCopyDirectoryRecursively()
{
    QTemporaryDir tempDir;
    const QString sourceDir = tempDir.filePath("copy_src");
    const QString nestedDir = sourceDir + "/nested";
    const QString destDir = tempDir.filePath("copy_dst");

    QVERIFY(QDir().mkpath(nestedDir));

    QFile file1(sourceDir + "/root.txt");
    QVERIFY(file1.open(QIODevice::WriteOnly));
    file1.write("root");
    file1.close();

    QFile file2(nestedDir + "/child.txt");
    QVERIFY(file2.open(QIODevice::WriteOnly));
    file2.write("child");
    file2.close();

    QVERIFY(QGCFileHelper::copyDirectoryRecursively(sourceDir, destDir));
    QVERIFY(QFile::exists(destDir + "/root.txt"));
    QVERIFY(QFile::exists(destDir + "/nested/child.txt"));
}

void QGCFileHelperTest::_testMoveFileOrCopyFile()
{
    QTemporaryDir tempDir;
    const QString sourcePath = tempDir.filePath("move_source.txt");
    const QString destPath = tempDir.filePath("move_dest.txt");

    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("move-me");
    source.close();

    QVERIFY(QGCFileHelper::moveFileOrCopy(sourcePath, destPath));
    QVERIFY(!QFile::exists(sourcePath));
    QVERIFY(QFile::exists(destPath));
}

void QGCFileHelperTest::_testMoveFileOrCopyDirectory()
{
    QTemporaryDir tempDir;
    const QString sourceDir = tempDir.filePath("move_dir_src");
    const QString destDir = tempDir.filePath("move_dir_dst");
    QVERIFY(QDir().mkpath(sourceDir));

    QFile sourceFile(sourceDir + "/data.txt");
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.write("dir-data");
    sourceFile.close();

    QVERIFY(QGCFileHelper::moveFileOrCopy(sourceDir, destDir));
    QVERIFY(!QFile::exists(sourceDir));
    QVERIFY(QFile::exists(destDir + "/data.txt"));
}

void QGCFileHelperTest::_testReplaceFileFromTempInvalidArgs()
{
    QTemporaryDir tempDir;
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("replaceFileFromTemp: temp file is null or not open"));
    QVERIFY(!QGCFileHelper::replaceFileFromTemp(nullptr, tempDir.filePath("out.txt")));
    verifyExpectedLogMessage();

    auto temp = QGCFileHelper::createTempFile(QByteArrayLiteral("data"));
    QVERIFY(temp != nullptr);
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("replaceFileFromTemp: target path is empty"));
    QVERIFY(!QGCFileHelper::replaceFileFromTemp(temp.get(), QString()));
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(QGCFileHelperTest, TestLabel::Unit, TestLabel::Utilities)
