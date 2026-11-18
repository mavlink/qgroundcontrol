#include "QGCFileHelperTest.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>

#include "FileFixtures.h"
#include "QGCCompression.h"
#include "QGCFileHelper.h"

// ============================================================================
// exists() tests
// ============================================================================
void QGCFileHelperTest::_testExistsRegularFile()
{
    QTemporaryDir tempDir;
    // Create a test file
    const QString filePath = tempDir.filePath("testfile.txt");
    QVERIFY(TestFixtures::writeFile(filePath, "test content"));
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
// ensureDirectoryExists() and ensureParentExists() tests
// ============================================================================
void QGCFileHelperTest::_testEnsureDirectory_data()
{
    QTest::addColumn<bool>("parentOfFile");
    QTest::addColumn<QString>("directory");  // below the temporary directory; empty: that directory itself

    QTest::newRow("directory, exists") << false << QString();
    QTest::newRow("directory, new") << false << QStringLiteral("newdir");
    QTest::newRow("directory, nested") << false << QStringLiteral("level1/level2/level3");
    QTest::newRow("parent of a file, exists") << true << QString();
    QTest::newRow("parent of a file, new") << true << QStringLiteral("newparent");
    QTest::newRow("parent of a file, nested") << true << QStringLiteral("a/b/c");
}

void QGCFileHelperTest::_testEnsureDirectory()
{
    QFETCH(bool, parentOfFile);
    QFETCH(QString, directory);

    QTemporaryDir tempDir;
    const QDir dir(directory.isEmpty() ? tempDir.path() : tempDir.filePath(directory));
    QCOMPARE(dir.exists(), directory.isEmpty());
    if (parentOfFile) {
        QVERIFY(QGCFileHelper::ensureParentExists(dir.filePath("somefile.txt")));
    } else {
        QVERIFY(QGCFileHelper::ensureDirectoryExists(dir.path()));
    }
    QVERIFY(dir.exists());
}

// ============================================================================
// optimalBufferSize() tests
// ============================================================================
void QGCFileHelperTest::_testOptimalBufferSize()
{
    QTemporaryDir tempDir;
    const size_t size = QGCFileHelper::optimalBufferSize();
    QVERIFY(size >= QGCFileHelper::kBufferSizeMin);
    QVERIFY(size <= QGCFileHelper::kBufferSizeMax);
    // The size is worked out once: every later call returns it, with a path or without
    QCOMPARE(QGCFileHelper::optimalBufferSize(), size);
    QCOMPARE(QGCFileHelper::optimalBufferSize(tempDir.path()), size);
}

// ============================================================================
// atomicWrite() tests
// ============================================================================
void QGCFileHelperTest::_testAtomicWrite_data()
{
    QTest::addColumn<QString>("path");             // below the temporary directory
    QTest::addColumn<QByteArrayList>("contents");  // written one after the other

    QTest::newRow("new file") << QStringLiteral("atomic_basic.txt") << QByteArrayList{"Hello, atomic write!"};
    QTest::newRow("overwrite") << QStringLiteral("atomic_overwrite.txt")
                               << QByteArrayList{"Original content", "New content after overwrite"};
    QTest::newRow("creates the parent") << QStringLiteral("nested/dirs/atomic.txt")
                                        << QByteArrayList{"Data in nested directory"};
    QTest::newRow("empty data") << QStringLiteral("atomic_empty.txt") << QByteArrayList{QByteArray()};
}

void QGCFileHelperTest::_testAtomicWrite()
{
    QFETCH(QString, path);
    QFETCH(QByteArrayList, contents);

    QTemporaryDir tempDir;
    const QFileInfo file(tempDir.filePath(path));
    QVERIFY(!QFile::exists(file.filePath()));
    for (const QByteArray& content : contents) {
        QVERIFY(QGCFileHelper::atomicWrite(file.filePath(), content));
        QVERIFY(QFile::exists(file.filePath()));
        QCOMPARE(TestFixtures::readFile(file.filePath()), content);
    }
    // No temporary file is left behind
    QCOMPARE(file.dir().entryList(QDir::Files | QDir::Hidden), QStringList{file.fileName()});
}

void QGCFileHelperTest::_testAtomicWriteEmptyPath()
{
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("file path is empty"));
    QVERIFY(!QGCFileHelper::atomicWrite(QString(), QByteArray("data")));
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testReadCompressedFile()
{
    QString error;
    const QByteArray data = QGCCompression::readFile(":/unittest/manifest.json.gz", &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains("\"name\""));
}

void QGCFileHelperTest::_testReadCompressedFileMaxBytes()
{
    QString error;
    const QByteArray partial = QGCCompression::readFile(":/unittest/manifest.json.gz", &error, 8);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!partial.isEmpty());
    QVERIFY(partial.size() <= 8);
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
void QGCFileHelperTest::_testToLocalPathFromUrl()
{
    QCOMPARE(QGCFileHelper::toLocalPath(QUrl::fromLocalFile("/path/to/file.txt")), QString("/path/to/file.txt"));

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
    QCOMPARE(QGCCompression::detectFormat(archivePath), QGCCompression::Format::ZIP);
    QVERIFY(QGCCompression::validateArchive(archivePath));

    QStringList entries;
    QVERIFY(QGCCompression::listArchive(archivePath, entries));
    QVERIFY(entries.contains(QStringLiteral("manifest.json")));

    const QString extractDir = tempDir.filePath("from_url_extract");
    QVERIFY(QGCCompression::extractArchive(archivePath, extractDir));
    QVERIFY(QFile::exists(extractDir + QStringLiteral("/manifest.json")));

    const QString gzipPath = QGCFileHelper::toLocalPath(QUrl(QStringLiteral("qrc:/unittest/manifest.json.gz")));
    QCOMPARE(gzipPath, QStringLiteral(":/unittest/manifest.json.gz"));

    const QString decompressedPath = tempDir.filePath("from_url_manifest.json");
    QVERIFY(QGCCompression::decompressFile(gzipPath, decompressedPath));
    QVERIFY(QFile::exists(decompressedPath));
}

void QGCFileHelperTest::_testLocalPaths_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("localPath");
    QTest::addColumn<bool>("isLocal");

    const auto addUnchanged = [](const char* tag, const QString& input, bool isLocal) {
        QTest::newRow(tag) << input << input << isLocal;
    };

    addUnchanged("absolute path", QStringLiteral("/path/to/file.txt"), true);
    addUnchanged("relative path", QStringLiteral("relative/path.txt"), true);
    addUnchanged("empty", QString(), false);
    addUnchanged("resource path", QStringLiteral(":/resource/path"), true);
    QTest::newRow("file URL") << QStringLiteral("file:///path/to/file.txt") << QStringLiteral("/path/to/file.txt")
                              << true;
    QTest::newRow("file URL, other path")
        << QStringLiteral("file:///home/user/test.zip") << QStringLiteral("/home/user/test.zip") << true;
    QTest::newRow("qrc URL") << QStringLiteral("qrc:/resource/path") << QStringLiteral(":/resource/path") << true;
    QTest::newRow("qrc URL, three slashes")
        << QStringLiteral("qrc:///resource/path") << QStringLiteral(":/resource/path") << true;
    addUnchanged("http", QStringLiteral("http://example.com/file.txt"), false);
    addUnchanged("https", QStringLiteral("https://example.com/file.txt"), false);

    // QUrl parses a drive letter as a URL scheme
    addUnchanged("forward slashes", QStringLiteral("C:/dir/file.txt"), true);
    addUnchanged("backslashes", QStringLiteral("C:\\dir\\file.txt"), true);
    addUnchanged("lower case drive", QStringLiteral("d:/dir/file.txt"), true);
    addUnchanged("URL delimiters", QStringLiteral("C:/dir/a b#c?d.txt"), true);
    addUnchanged("drive relative", QStringLiteral("C:file.txt"), true);
    addUnchanged("drive only", QStringLiteral("C:"), true);

    // Only a single leading letter is a drive
    const QString fileUrl = QStringLiteral("file:///C:/dir/file.txt");
    QTest::newRow("file URL with drive") << fileUrl << QUrl(fileUrl).toLocalFile() << true;
    addUnchanged("two letter scheme", QStringLiteral("ab://host/file.txt"), false);
    addUnchanged("http URL", QStringLiteral("http://host/C:/file.txt"), false);
}

void QGCFileHelperTest::_testLocalPaths()
{
    QFETCH(QString, input);
    QFETCH(QString, localPath);
    QFETCH(bool, isLocal);

    QCOMPARE(QGCFileHelper::toLocalPath(input), localPath);
    QCOMPARE(QGCFileHelper::isLocalPath(input), isLocal);
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
    QVERIFY(TestFixtures::writeFile(filePath, "hello"));
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
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^failed to open file:"));
    const QString noFileHash = QGCFileHelper::computeFileHash("/nonexistent/file.txt");
    QVERIFY(noFileHash.isEmpty());
    verifyExpectedLogMessage();
    // Empty path should return empty
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^empty file path$"));
    QVERIFY(QGCFileHelper::computeFileHash(QString()).isEmpty());
    verifyExpectedLogMessage();
}

void QGCFileHelperTest::_testComputeDecompressedFileHash()
{
    const QString gzPath = QStringLiteral(":/unittest/manifest.json.gz");
    QString error;
    const QByteArray plainData = QGCCompression::readFile(gzPath, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!plainData.isEmpty());

    const QString expectedPlainHash = QGCFileHelper::computeHash(plainData);
    QVERIFY(!expectedPlainHash.isEmpty());

    const QString decompressedHash = QGCCompression::computeFileHash(gzPath);
    QVERIFY(!decompressedHash.isEmpty());
    QCOMPARE(decompressedHash, expectedPlainHash);

    // For uncompressed files, QGCCompression::computeFileHash should match QGCFileHelper::computeFileHash().
    const QString passthroughPath = QStringLiteral(":/unittest/arducopter.apj");
    const QString regularHash = QGCFileHelper::computeFileHash(passthroughPath);
    const QString passthroughHash = QGCCompression::computeFileHash(passthroughPath);
    QCOMPARE(passthroughHash, regularHash);
}

void QGCFileHelperTest::_testVerifyFileHash()
{
    QTemporaryDir tempDir;
    // Create a test file
    const QString filePath = tempDir.filePath("verifytest.txt");
    QVERIFY(TestFixtures::writeFile(filePath, "hello"));
    const QString correctHash = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
    const QString wrongHash = "0000000000000000000000000000000000000000000000000000000000000000";
    // Correct hash should verify
    QVERIFY(QGCFileHelper::verifyFileHash(filePath, correctHash));
    // Case insensitive
    QVERIFY(QGCFileHelper::verifyFileHash(filePath, correctHash.toUpper()));
    // Wrong hash should fail
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^hash mismatch for"));
    QVERIFY(!QGCFileHelper::verifyFileHash(filePath, wrongHash));
    verifyExpectedLogMessage();
    // Empty expected hash should fail
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^empty expected hash$"));
    QVERIFY(!QGCFileHelper::verifyFileHash(filePath, QString()));
    verifyExpectedLogMessage();
    // Non-existent file should fail
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^failed to open file:"));
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
    QVERIFY(TestFixtures::writeFile(sourcePath, sourceData));
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
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^failed to open source:"));
    auto noCopy = QGCFileHelper::createTempCopy("/nonexistent/file.txt");
    QVERIFY(noCopy == nullptr);
    verifyExpectedLogMessage();
    // Empty path should return nullptr
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^source path is empty$"));
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
    QCOMPARE(TestFixtures::readFile(targetPath), newContent);
    // Replace again (target exists)
    const QByteArray newerContent = "Even newer content";
    auto temp2 = QGCFileHelper::createTempFile(newerContent);
    QVERIFY(QGCFileHelper::replaceFileFromTemp(temp2.get(), targetPath));
    temp2.release();
    QCOMPARE(TestFixtures::readFile(targetPath), newerContent);
}

void QGCFileHelperTest::_testReplaceFileFromTempWithBackup()
{
    QTemporaryDir tempDir;
    const QString targetPath = tempDir.filePath("targetWithBackup.txt");
    const QString backupPath = tempDir.filePath("targetWithBackup.txt.bak");
    const QByteArray originalContent = "Original content";
    const QByteArray newContent = "New content";
    // Create original target
    QVERIFY(TestFixtures::writeFile(targetPath, originalContent));
    // Create temp with new content and replace with backup
    auto temp = QGCFileHelper::createTempFile(newContent);
    QVERIFY(QGCFileHelper::replaceFileFromTemp(temp.get(), targetPath, backupPath));
    temp.release();
    QCOMPARE(TestFixtures::readFile(targetPath), newContent);
    QCOMPARE(TestFixtures::readFile(backupPath), originalContent);
}

void QGCFileHelperTest::_testCopyDirectoryRecursively()
{
    QTemporaryDir tempDir;
    const QString sourceDir = tempDir.filePath("copy_src");
    const QString nestedDir = sourceDir + "/nested";
    const QString destDir = tempDir.filePath("copy_dst");

    QVERIFY(QDir().mkpath(nestedDir));

    QVERIFY(TestFixtures::writeFile(sourceDir + "/root.txt", "root"));

    QVERIFY(TestFixtures::writeFile(nestedDir + "/child.txt", "child"));

    QVERIFY(QGCFileHelper::copyDirectoryRecursively(sourceDir, destDir));
    QVERIFY(QFile::exists(destDir + "/root.txt"));
    QVERIFY(QFile::exists(destDir + "/nested/child.txt"));
}

void QGCFileHelperTest::_testMoveFileOrCopyFile()
{
    QTemporaryDir tempDir;
    const QString sourcePath = tempDir.filePath("move_source.txt");
    const QString destPath = tempDir.filePath("move_dest.txt");

    QVERIFY(TestFixtures::writeFile(sourcePath, "move-me"));

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

    QVERIFY(TestFixtures::writeFile(sourceDir + "/data.txt", "dir-data"));

    QVERIFY(QGCFileHelper::moveFileOrCopy(sourceDir, destDir));
    QVERIFY(!QFile::exists(sourceDir));
    QVERIFY(QFile::exists(destDir + "/data.txt"));
}

void QGCFileHelperTest::_testReplaceFileFromTempInvalidArgs()
{
    QTemporaryDir tempDir;
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^temp file is null or not open$"));
    QVERIFY(!QGCFileHelper::replaceFileFromTemp(nullptr, tempDir.filePath("out.txt")));
    verifyExpectedLogMessage();

    auto temp = QGCFileHelper::createTempFile(QByteArrayLiteral("data"));
    QVERIFY(temp != nullptr);
    expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg, QRegularExpression("^target path is empty$"));
    QVERIFY(!QGCFileHelper::replaceFileFromTemp(temp.get(), QString()));
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(QGCFileHelperTest, TestLabel::Unit, TestLabel::Utilities)
