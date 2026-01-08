#include "QGCFileHelperTest.h"
#include "QGCFileHelper.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtTest/QTest>

void QGCFileHelperTest::init()
{
    UnitTest::init();
    _tempDir = new QTemporaryDir();
    QVERIFY(_tempDir->isValid());
}

void QGCFileHelperTest::cleanup()
{
    delete _tempDir;
    _tempDir = nullptr;
    UnitTest::cleanup();
}

// ============================================================================
// exists() tests
// ============================================================================

void QGCFileHelperTest::_testExistsRegularFile()
{
    // Create a test file
    const QString filePath = _tempDir->filePath("testfile.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    QVERIFY(QGCFileHelper::exists(filePath));
}

void QGCFileHelperTest::_testExistsDirectory()
{
    // The temp directory should exist
    QVERIFY(QGCFileHelper::exists(_tempDir->path()));

    // Create a subdirectory
    const QString subDir = _tempDir->filePath("subdir");
    QVERIFY(QDir().mkdir(subDir));
    QVERIFY(QGCFileHelper::exists(subDir));
}

void QGCFileHelperTest::_testExistsNonExistent()
{
    QVERIFY(!QGCFileHelper::exists("/nonexistent/path/that/does/not/exist"));
    QVERIFY(!QGCFileHelper::exists(_tempDir->filePath("nonexistent.txt")));
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
// ensureDirectoryExists() tests
// ============================================================================

void QGCFileHelperTest::_testEnsureDirectoryExistsAlreadyExists()
{
    // Temp directory already exists
    QVERIFY(QGCFileHelper::ensureDirectoryExists(_tempDir->path()));
    QVERIFY(QDir(_tempDir->path()).exists());
}

void QGCFileHelperTest::_testEnsureDirectoryExistsCreate()
{
    const QString newDir = _tempDir->filePath("newdir");
    QVERIFY(!QDir(newDir).exists());

    QVERIFY(QGCFileHelper::ensureDirectoryExists(newDir));
    QVERIFY(QDir(newDir).exists());
}

void QGCFileHelperTest::_testEnsureDirectoryExistsNested()
{
    const QString nestedDir = _tempDir->filePath("level1/level2/level3");
    QVERIFY(!QDir(nestedDir).exists());

    QVERIFY(QGCFileHelper::ensureDirectoryExists(nestedDir));
    QVERIFY(QDir(nestedDir).exists());
}

// ============================================================================
// ensureParentExists() tests
// ============================================================================

void QGCFileHelperTest::_testEnsureParentExistsAlreadyExists()
{
    // Parent of a file in temp dir already exists
    const QString filePath = _tempDir->filePath("somefile.txt");
    QVERIFY(QGCFileHelper::ensureParentExists(filePath));
    QVERIFY(QDir(_tempDir->path()).exists());
}

void QGCFileHelperTest::_testEnsureParentExistsCreate()
{
    const QString filePath = _tempDir->filePath("newparent/somefile.txt");
    const QString parentDir = _tempDir->filePath("newparent");
    QVERIFY(!QDir(parentDir).exists());

    QVERIFY(QGCFileHelper::ensureParentExists(filePath));
    QVERIFY(QDir(parentDir).exists());
}

void QGCFileHelperTest::_testEnsureParentExistsNested()
{
    const QString filePath = _tempDir->filePath("a/b/c/somefile.txt");
    const QString parentDir = _tempDir->filePath("a/b/c");
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
    // Multiple calls should return the same value (cached)
    const size_t first = QGCFileHelper::optimalBufferSize();
    const size_t second = QGCFileHelper::optimalBufferSize();
    const size_t third = QGCFileHelper::optimalBufferSize(_tempDir->path());

    QCOMPARE(first, second);
    QCOMPARE(second, third);
}

void QGCFileHelperTest::_testOptimalBufferSizeWithPath()
{
    // With path argument, should still return valid size within bounds
    const size_t size = QGCFileHelper::optimalBufferSize(_tempDir->path());

    QVERIFY(size >= QGCFileHelper::kBufferSizeMin);
    QVERIFY(size <= QGCFileHelper::kBufferSizeMax);
}

// ============================================================================
// atomicWrite() tests
// ============================================================================

void QGCFileHelperTest::_testAtomicWriteBasic()
{
    const QString filePath = _tempDir->filePath("atomic_basic.txt");
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
    const QString filePath = _tempDir->filePath("atomic_overwrite.txt");
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
    const QString filePath = _tempDir->filePath("nested/dirs/atomic.txt");
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
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("file path is empty"));
    QVERIFY(!QGCFileHelper::atomicWrite(QString(), QByteArray("data")));
}

void QGCFileHelperTest::_testAtomicWriteEmptyData()
{
    const QString filePath = _tempDir->filePath("atomic_empty.txt");

    // Empty data should succeed and create empty file
    QVERIFY(QGCFileHelper::atomicWrite(filePath, QByteArray()));
    QVERIFY(QFile::exists(filePath));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.size(), 0);
}

// ============================================================================
// Disk space utilities tests
// ============================================================================

void QGCFileHelperTest::_testAvailableDiskSpaceBasic()
{
    // Should return positive value for valid path
    const qint64 available = QGCFileHelper::availableDiskSpace(_tempDir->path());
    QVERIFY(available > 0);

    // Should work for file path too (uses parent directory)
    const QString filePath = _tempDir->filePath("testfile.txt");
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
    // Should have space for 1 byte
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), 1));

    // Should have space for 1KB
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), 1024));

    // Should probably not have space for 1 exabyte (10^18 bytes)
    QVERIFY(!QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), 1000000000000000000LL));
}

void QGCFileHelperTest::_testHasSufficientDiskSpaceZeroBytes()
{
    // Zero bytes should always succeed
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), 0));

    // Negative bytes should also succeed (treated as unknown/nothing to check)
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), -1));
}

void QGCFileHelperTest::_testHasSufficientDiskSpaceWithMargin()
{
    // Get available space
    const qint64 available = QGCFileHelper::availableDiskSpace(_tempDir->path());
    QVERIFY(available > 0);

    // Should have space for half the available (with 10% margin)
    QVERIFY(QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), available / 2, 1.1));

    // Should not have space for 2x available
    QVERIFY(!QGCFileHelper::hasSufficientDiskSpace(_tempDir->path(), available * 2, 1.0));
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
    // Note: http:// URLs are not "local" but toLocalPath will return the URL string
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
    // Create a test file
    const QString filePath = _tempDir->filePath("hashtest.txt");
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
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("computeFileHash: failed to open"));
    const QString noFileHash = QGCFileHelper::computeFileHash("/nonexistent/file.txt");
    QVERIFY(noFileHash.isEmpty());

    // Empty path should return empty
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("computeFileHash: empty file path"));
    QVERIFY(QGCFileHelper::computeFileHash(QString()).isEmpty());
}

void QGCFileHelperTest::_testVerifyFileHash()
{
    // Create a test file
    const QString filePath = _tempDir->filePath("verifytest.txt");
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
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("verifyFileHash: hash mismatch"));
    QVERIFY(!QGCFileHelper::verifyFileHash(filePath, wrongHash));

    // Empty expected hash should fail
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("verifyFileHash: empty expected hash"));
    QVERIFY(!QGCFileHelper::verifyFileHash(filePath, QString()));

    // Non-existent file should fail
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("computeFileHash: failed to open"));
    QVERIFY(!QGCFileHelper::verifyFileHash("/nonexistent/file.txt", correctHash));
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
    // Create source file
    const QString sourcePath = _tempDir->filePath("source.txt");
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
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("createTempCopy: failed to open source"));
    auto noCopy = QGCFileHelper::createTempCopy("/nonexistent/file.txt");
    QVERIFY(noCopy == nullptr);

    // Empty path should return nullptr
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("createTempCopy: source path is empty"));
    auto emptyCopy = QGCFileHelper::createTempCopy(QString());
    QVERIFY(emptyCopy == nullptr);
}

void QGCFileHelperTest::_testReplaceFileFromTemp()
{
    const QString targetPath = _tempDir->filePath("target.txt");
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
    const QString targetPath = _tempDir->filePath("targetWithBackup.txt");
    const QString backupPath = _tempDir->filePath("targetWithBackup.txt.bak");
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

