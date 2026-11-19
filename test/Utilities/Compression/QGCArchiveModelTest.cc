#include "QGCArchiveModelTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThreadPool>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "QGCArchiveModel.h"
#include "QGCCompressionTest.h"
#include "UnitTest.h"

bool QGCArchiveModelTest::_load(QGCArchiveModel& model, const std::function<void()>& startLoad)
{
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    startLoad();
    return waitForSignalCount(loadingSpy, 1, TestTimeout::mediumMs(), QStringLiteral("loadingComplete")) &&
           loadingSpy.first().first().toBool();
}

bool QGCArchiveModelTest::_load(QGCArchiveModel& model, const QString& archivePath)
{
    return _load(model, [&model, &archivePath] { model.setArchivePath(archivePath); });
}

// ============================================================================
// Basic Model Functionality
// ============================================================================
void QGCArchiveModelTest::_testEmptyModel()
{
    QGCArchiveModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.fileCount(), 0);
    QCOMPARE(model.directoryCount(), 0);
    QCOMPARE(model.totalSize(), 0);
    QVERIFY(model.archivePath().isEmpty());
    QVERIFY(model.errorString().isEmpty());
    QVERIFY(!model.loading());
    QCOMPARE(model.filterMode(), QGCArchiveModel::AllEntries);
}

void QGCArchiveModelTest::_testLoadArchive()
{
    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    model.setArchivePath(_zipResource);
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), true);
    QVERIFY(model.rowCount() > 0);
    QVERIFY(model.count() > 0);
    QVERIFY(model.fileCount() > 0);
    QVERIFY(model.errorString().isEmpty());
}

void QGCArchiveModelTest::_testRoleNames()
{
    QGCArchiveModel model;
    const QHash<int, QByteArray> roles = model.roleNames();
    QVERIFY(roles.contains(QGCArchiveModel::NameRole));
    QVERIFY(roles.contains(QGCArchiveModel::SizeRole));
    QVERIFY(roles.contains(QGCArchiveModel::ModifiedRole));
    QVERIFY(roles.contains(QGCArchiveModel::IsDirectoryRole));
    QVERIFY(roles.contains(QGCArchiveModel::PermissionsRole));
    QVERIFY(roles.contains(QGCArchiveModel::FileNameRole));
    QVERIFY(roles.contains(QGCArchiveModel::DirectoryRole));
    QVERIFY(roles.contains(QGCArchiveModel::FormattedSizeRole));
    QCOMPARE(roles.value(QGCArchiveModel::NameRole), QByteArray("name"));
    QCOMPARE(roles.value(QGCArchiveModel::SizeRole), QByteArray("size"));
    QCOMPARE(roles.value(QGCArchiveModel::IsDirectoryRole), QByteArray("isDirectory"));
}

void QGCArchiveModelTest::_testDataAccess()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    QVERIFY(model.rowCount() > 0);
    // Test data() with valid index
    const QModelIndex index = model.index(0);
    QVERIFY(index.isValid());
    const QVariant name = model.data(index, QGCArchiveModel::NameRole);
    QVERIFY(!name.toString().isEmpty());
    const QVariant size = model.data(index, QGCArchiveModel::SizeRole);
    QVERIFY(size.toLongLong() >= 0);
    const QVariant isDir = model.data(index, QGCArchiveModel::IsDirectoryRole);
    QVERIFY(isDir.canConvert<bool>());
    const QVariant formattedSize = model.data(index, QGCArchiveModel::FormattedSizeRole);
    QVERIFY(!formattedSize.toString().isEmpty());
    // Test data() with invalid index
    const QModelIndex invalidIndex = model.index(-1);
    QVERIFY(!invalidIndex.isValid());
    const QModelIndex outOfBounds = model.index(model.rowCount() + 1);
    QVERIFY(model.data(outOfBounds, QGCArchiveModel::NameRole).isNull());
}

void QGCArchiveModelTest::_testGetMethod()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    QVERIFY(model.count() > 0);
    // Test get() with valid index
    const QVariantMap entry = model.get(0);
    QVERIFY(!entry.isEmpty());
    QVERIFY(entry.contains("name"));
    QVERIFY(entry.contains("size"));
    QVERIFY(entry.contains("isDirectory"));
    QVERIFY(entry.contains("formattedSize"));
    QVERIFY(entry.contains("fileName"));
    QVERIFY(entry.contains("directory"));
    // Test get() with invalid index
    const QVariantMap invalid = model.get(-1);
    QVERIFY(invalid.isEmpty());
    const QVariantMap outOfBounds = model.get(model.count() + 1);
    QVERIFY(outOfBounds.isEmpty());
}

void QGCArchiveModelTest::_testGetAgreesWithData()
{
    // A directory entry, a file in that directory and a file beside it
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath("tree.tar");
    QVERIFY(
        TestFixtures::writeFile(archivePath, QGCCompressionTest::_makeTar({{.name = "sub/", .type = '5'},
                                                                           {.name = "sub/nested.txt", .data = "nested"},
                                                                           {.name = "top.txt", .data = "top"}})));
    QGCArchiveModel model;
    QVERIFY(_load(model, archivePath));

    struct Expected
    {
        const char* name;
        const char* fileName;
        const char* directory;
    };

    const QList<Expected> expected{
        {"sub/", "sub/", "sub"}, {"sub/nested.txt", "nested.txt", "sub"}, {"top.txt", "top.txt", ""}};
    QCOMPARE(model.count(), expected.size());

    // get() tells of an entry what the roles tell of it, also where its name ends in the separator
    const QHash<int, QByteArray> roles = model.roleNames();
    for (int row = 0; row < model.count(); ++row) {
        const QVariantMap entry = model.get(row);
        QCOMPARE(entry.size(), roles.size());
        for (const auto [role, roleName] : roles.asKeyValueRange()) {
            QCOMPARE(entry.value(QString::fromLatin1(roleName)), model.data(model.index(row), role));
        }
        QCOMPARE(entry.value("name").toString(), QLatin1String(expected.at(row).name));
        QCOMPARE(entry.value("fileName").toString(), QLatin1String(expected.at(row).fileName));
        QCOMPARE(entry.value("directory").toString(), QLatin1String(expected.at(row).directory));
    }
}

// ============================================================================
// Properties
// ============================================================================
void QGCArchiveModelTest::_testArchivePathProperty()
{
    QGCArchiveModel model;
    QSignalSpy pathSpy(&model, &QGCArchiveModel::archivePathChanged);
    model.setArchivePath(_zipResource);
    QCOMPARE(model.archivePath(), _zipResource);
    QCOMPARE(pathSpy.count(), 1);
    // Setting same path should not emit signal
    model.setArchivePath(_zipResource);
    QCOMPARE(pathSpy.count(), 1);
    // Setting different path should emit signal
    model.setArchivePath(QString());
    QCOMPARE(pathSpy.count(), 2);
    QVERIFY(model.archivePath().isEmpty());
}

void QGCArchiveModelTest::_testCountProperties()
{
    QGCArchiveModel model;
    QSignalSpy countSpy(&model, &QGCArchiveModel::countChanged);
    QVERIFY(_load(model, _zipResource));
    QVERIFY(countSpy.count() > 0);
    QVERIFY(model.count() > 0);
    QCOMPARE(model.count(), model.rowCount());
    // count = fileCount + directoryCount
    QCOMPARE(model.count(), model.fileCount() + model.directoryCount());
}

void QGCArchiveModelTest::_testLoadingProperty()
{
    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingChanged);
    QSignalSpy completeSpy(&model, &QGCArchiveModel::loadingComplete);
    QVERIFY(!model.loading());

    // The archive is read in the background: the call returns with the load in progress
    model.setArchivePath(_zipResource);
    QVERIFY(model.loading());
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(completeSpy.count(), 0);
    QCOMPARE(model.count(), 0);

    QVERIFY_SIGNAL_COUNT_WAIT(completeSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!model.loading());
    QCOMPARE(loadingSpy.count(), 2);
    QVERIFY(model.count() > 0);
}

void QGCArchiveModelTest::_testAbandonedLoad_data()
{
    QTest::addColumn<bool>("startNextLoad");

    QTest::newRow("path changed") << true;
    QTest::newRow("cleared") << false;
}

void QGCArchiveModelTest::_testAbandonedLoad()
{
    QFETCH(bool, startNextLoad);

    QTemporaryDir tempDir;
    const QString tarPath = tempDir.filePath("scripts.tar");
    QVERIFY(TestFixtures::writeFile(tarPath,
                                    QGCCompressionTest::_makeTar({{.name = "startup.lua", .data = "-- startup"}})));

    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingChanged);
    QSignalSpy completeSpy(&model, &QGCArchiveModel::loadingComplete);
    model.setArchivePath(_zipResource);
    QVERIFY(model.loading());

    if (startNextLoad) {
        // The model stays loading from the first load to the end of the second
        model.setArchivePath(tarPath);
        QVERIFY(model.loading());
        QCOMPARE(loadingSpy.count(), 1);
        QVERIFY_SIGNAL_COUNT_WAIT(completeSpy, 1, TestTimeout::mediumMs());
    } else {
        model.clear();
        QVERIFY(!model.loading());
    }

    // Let the abandoned load run to its end and deliver whatever it still has to deliver
    QVERIFY(waitForCondition([] { return QThreadPool::globalInstance()->activeThreadCount() == 0; },
                             TestTimeout::mediumMs(), QStringLiteral("global thread pool drained")));
    QCoreApplication::processEvents();

    QVERIFY(!model.loading());
    QCOMPARE(loadingSpy.count(), 2);
    QVERIFY(!model.contains("manifest.json"));
    QCOMPARE(completeSpy.count(), startNextLoad ? 1 : 0);
    QCOMPARE(model.contains("startup.lua"), startNextLoad);
}

void QGCArchiveModelTest::_testErrorClearedWithArchive_data()
{
    QTest::addColumn<bool>("clearPath");

    QTest::newRow("path cleared") << true;
    QTest::newRow("model cleared") << false;
}

void QGCArchiveModelTest::_testErrorClearedWithArchive()
{
    QFETCH(bool, clearPath);

    QTemporaryDir tempDir;
    const QString corruptPath = tempDir.filePath("corrupt.zip");
    QVERIFY(TestFixtures::writeFile(corruptPath, "This is not a valid ZIP file"));

    QGCArchiveModel model;
    // As in _testInvalidArchive: how many warnings libarchive gives for this file is not fixed
    ignoreLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to open file:"));
    QVERIFY(!_load(model, corruptPath));
    QVERIFY(!model.errorString().isEmpty());

    // The failed load left no entries to clear, and its error goes all the same
    if (clearPath) {
        model.setArchivePath(QString());
    } else {
        model.clear();
    }
    QVERIFY(model.errorString().isEmpty());
}

// ============================================================================
// Filtering
// ============================================================================
void QGCArchiveModelTest::_testFilterModeFilesOnly()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    const int totalFiles = model.fileCount();
    const int totalDirs = model.directoryCount();
    QSignalSpy filterSpy(&model, &QGCArchiveModel::filterModeChanged);
    model.setFilterMode(QGCArchiveModel::FilesOnly);
    QCOMPARE(filterSpy.count(), 1);
    QCOMPARE(model.filterMode(), QGCArchiveModel::FilesOnly);
    // Count should now equal file count only
    QCOMPARE(model.count(), totalFiles);
    // All entries should be files (not directories)
    for (int i = 0; i < model.count(); ++i) {
        const QVariantMap entry = model.get(i);
        QVERIFY2(!entry.value("isDirectory").toBool(),
                 qPrintable(QString("Entry %1 should not be a directory").arg(entry.value("name").toString())));
    }
    // File/directory counts should not change (those are totals)
    QCOMPARE(model.fileCount(), totalFiles);
    QCOMPARE(model.directoryCount(), totalDirs);
}

void QGCArchiveModelTest::_testFilterModeDirectoriesOnly()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    const int totalDirs = model.directoryCount();
    model.setFilterMode(QGCArchiveModel::DirectoriesOnly);
    QCOMPARE(model.filterMode(), QGCArchiveModel::DirectoriesOnly);
    QCOMPARE(model.count(), totalDirs);
    // All entries should be directories
    for (int i = 0; i < model.count(); ++i) {
        const QVariantMap entry = model.get(i);
        QVERIFY2(entry.value("isDirectory").toBool(),
                 qPrintable(QString("Entry %1 should be a directory").arg(entry.value("name").toString())));
    }
}

// ============================================================================
// Utilities
// ============================================================================
void QGCArchiveModelTest::_testFormatSize()
{
    const QLocale previous;
    QLocale::setDefault(QLocale(QStringLiteral("en_US")));
    const auto restoreLocale = qScopeGuard([&previous]() { QLocale::setDefault(previous); });

    // The text is that of QGC::bigSizeToString()
    QCOMPARE(QGCArchiveModel::formatSize(0), QStringLiteral("0B"));
    QCOMPARE(QGCArchiveModel::formatSize(100), QStringLiteral("100B"));
    QCOMPARE(QGCArchiveModel::formatSize(1023), QStringLiteral("1,023B"));
    QCOMPARE(QGCArchiveModel::formatSize(1024), QStringLiteral("1.0KB"));
    QCOMPARE(QGCArchiveModel::formatSize(1536), QStringLiteral("1.5KB"));
    // The unit follows the rounded number
    QCOMPARE(QGCArchiveModel::formatSize(1024 * 1024 - 1), QStringLiteral("1.0MB"));
    QCOMPARE(QGCArchiveModel::formatSize(1024 * 1024), QStringLiteral("1.0MB"));
    QCOMPARE(QGCArchiveModel::formatSize(1024LL * 1024 * 1024), QStringLiteral("1.0GB"));
    // A size that is not known has no number to show
    QCOMPARE(QGCArchiveModel::formatSize(-1), QStringLiteral("--"));
}

void QGCArchiveModelTest::_testContains()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    // File that exists
    QVERIFY(model.contains("manifest.json"));
    // File that doesn't exist
    QVERIFY(!model.contains("nonexistent.txt"));
    // Empty string
    QVERIFY(!model.contains(""));
}

void QGCArchiveModelTest::_testClear()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    QVERIFY(model.count() > 0);
    QVERIFY(model.fileCount() > 0);
    QSignalSpy countSpy(&model, &QGCArchiveModel::countChanged);
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.fileCount(), 0);
    QCOMPARE(model.directoryCount(), 0);
    QCOMPARE(model.totalSize(), 0);
    QVERIFY(countSpy.count() > 0);
}

void QGCArchiveModelTest::_testRefresh()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    const int originalCount = model.count();
    QVERIFY(originalCount > 0);
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    model.refresh();
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(model.count(), originalCount);
}

// ============================================================================
// Edge Cases
// ============================================================================
void QGCArchiveModelTest::_testInvalidArchive_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("content");

    QTest::newRow("not an archive") << "corrupt.zip" << QByteArray("This is not a valid ZIP file");
    // The first entry is listed before the damage is met, which is in its data
    QTest::newRow("truncated archive")
        << "truncated.tar"
        << QGCCompressionTest::_makeTar({{.name = "first.txt", .data = QByteArray(600, 'a')}, {.name = "second.txt"}})
               .left(512 + 100);
}

void QGCArchiveModelTest::_testInvalidArchive()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, content);

    QTemporaryDir tempDir;
    const QString corruptPath = tempDir.filePath(fileName);
    QVERIFY(TestFixtures::writeFile(corruptPath, content));
    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    // libarchive's error text (e.g. "Unrecognized archive format") and the number of
    // warnings emitted vary across platforms/libarchive versions, so suppress the noise
    // and assert the failure via loadingComplete/errorString instead.
    ignoreLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to open file:"));
    ignoreLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to read archive entry:"));
    model.setArchivePath(corruptPath);
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), false);
    QCOMPARE(model.errorString(), QStringLiteral("Invalid or corrupt archive: ") + corruptPath);
    QCOMPARE(model.count(), 0);
}

void QGCArchiveModelTest::_testNonExistentArchive()
{
    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    // The archive is read once: the listing tells why it failed
    expectLogMessage("Utilities.QGCCompression", QtWarningMsg, QRegularExpression("File does not exist"));
    model.setArchivePath("/nonexistent/path/archive.zip");
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), false);
    QCOMPARE(model.errorString(), QStringLiteral("File does not exist: /nonexistent/path/archive.zip"));
    QCOMPARE(model.count(), 0);
}

void QGCArchiveModelTest::_testEmptyArchive()
{
    QTemporaryDir tempDir;
    const QString emptyPath = tempDir.filePath("empty.zip");
    // An end of central directory record with no entries
    QVERIFY(TestFixtures::writeFile(emptyPath, QByteArray::fromHex("504b0506") + QByteArray(18, '\0')));

    QGCArchiveModel model;
    QVERIFY(_load(model, emptyPath));
    QCOMPARE(model.count(), 0);
    QVERIFY(model.errorString().isEmpty());
}

void QGCArchiveModelTest::_testDirectoryEntries()
{
    QTemporaryDir tempDir;
    const QString tarPath = tempDir.filePath("scripts.tar");
    QVERIFY(TestFixtures::writeFile(
        tarPath, QGCCompressionTest::_makeTar({{.name = "scripts/", .type = '5'},
                                               {.name = "scripts/startup.lua", .data = "-- startup"},
                                               {.name = "readme.txt", .data = QByteArray(2048, 'r')}})));

    QGCArchiveModel model;
    QVERIFY(_load(model, tarPath));
    QCOMPARE(model.count(), 3);
    QCOMPARE(model.directoryCount(), 1);
    QCOMPARE(model.fileCount(), 2);
    QCOMPARE(model.totalSize(), 2058);

    // Entries are sorted by name
    const QVariantMap file = model.get(0);
    QCOMPARE(file.value("name").toString(), QStringLiteral("readme.txt"));
    QCOMPARE(file.value("isDirectory").toBool(), false);
    QCOMPARE(file.value("formattedSize").toString(), QGCArchiveModel::formatSize(2048));

    const QVariantMap directory = model.get(1);
    QCOMPARE(directory.value("name").toString(), QStringLiteral("scripts/"));
    QCOMPARE(directory.value("isDirectory").toBool(), true);

    model.setFilterMode(QGCArchiveModel::DirectoriesOnly);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0), QGCArchiveModel::NameRole).toString(), QStringLiteral("scripts/"));

    model.setFilterMode(QGCArchiveModel::FilesOnly);
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.data(model.index(1), QGCArchiveModel::NameRole).toString(), QStringLiteral("scripts/startup.lua"));
}

// ============================================================================
// QUrl Support
// ============================================================================
void QGCArchiveModelTest::_testArchiveUrl()
{
    QTemporaryDir tempDir;
    QGCArchiveModel model;
    QSignalSpy pathSpy(&model, &QGCArchiveModel::archivePathChanged);
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    // Test with qrc:/ URL scheme (Qt resources)
    const QUrl qrcUrl(QStringLiteral("qrc:/unittest/manifest.json.zip"));
    model.setArchiveUrl(qrcUrl);
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(model.archivePath(), _zipResource);
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), true);
    QVERIFY(model.count() > 0);
    // Verify archiveUrl getter returns a file:// URL
    const QUrl returnedUrl = model.archiveUrl();
    QVERIFY(returnedUrl.isLocalFile() || returnedUrl.toString().startsWith(':'));
    // Clear and test with file:// URL
    model.clear();
    pathSpy.clear();
    loadingSpy.clear();
    // Create a temp file to test file:// URL
    const QString tempZipPath = tempDir.path() + "/test_url.zip";
    QFile::copy(_zipResource, tempZipPath);
    QVERIFY(QFileInfo::exists(tempZipPath));
    const QUrl fileUrl = QUrl::fromLocalFile(tempZipPath);
    model.setArchiveUrl(fileUrl);
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(QDir::cleanPath(model.archivePath()), QDir::cleanPath(tempZipPath));
    QVERIFY(loadingSpy.count() >= 1);
    QVERIFY(model.count() > 0);
    // Test that plain Qt resource path works (already tested in other tests)
    model.clear();
    pathSpy.clear();
    loadingSpy.clear();
    // QML commonly passes paths as URLs with the :/ prefix
    const QUrl resourcePathUrl(_zipResource);
    model.setArchiveUrl(resourcePathUrl);
    QVERIFY_SIGNAL_COUNT_WAIT(loadingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pathSpy.count(), 1);
    QVERIFY(model.count() > 0);
}

void QGCArchiveModelTest::_testArchiveUrlRejectsRemote()
{
    QGCArchiveModel model;
    QVERIFY(_load(model, _zipResource));
    QVERIFY(model.count() > 0);

    QSignalSpy pathSpy(&model, &QGCArchiveModel::archivePathChanged);
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);

    expectLogMessage("Utilities.QGCArchiveModel", QtWarningMsg, QRegularExpression(".*Unsupported archive URL.*"));
    model.setArchiveUrl(QUrl(QStringLiteral("https://example.com/archive.zip")));

    QCOMPARE(pathSpy.count(), 1);
    QVERIFY(model.archivePath().isEmpty());
    QCOMPARE(model.count(), 0);
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), true);
    verifyExpectedLogMessage();
}

// ============================================================================
// Model Invariants (QAbstractItemModelTester)
// ============================================================================
void QGCArchiveModelTest::_testModelTesterEmpty()
{
    // Test an empty model passes all QAbstractItemModel invariants
    QGCArchiveModel model;
    // QAbstractItemModelTester will assert if model violates any invariants
    // FailureReportingMode::Fatal makes failures cause test to abort
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    // Verify empty state
    QCOMPARE(model.rowCount(), 0);
    // Access invalid indices - tester will catch any issues
    const QModelIndex invalid = model.index(-1);
    QVERIFY(!invalid.isValid());
    const QModelIndex outOfBounds = model.index(100);
    QVERIFY(!outOfBounds.isValid());
}

void QGCArchiveModelTest::_testModelTesterLoaded()
{
    // Test a loaded model passes all invariants
    QGCArchiveModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    // Load archive - tester monitors all signals during load
    QVERIFY(_load(model, _zipResource));
    QVERIFY(model.rowCount() > 0);
    // Access all rows - tester will verify data() returns valid values
    for (int i = 0; i < model.rowCount(); ++i) {
        const QModelIndex idx = model.index(i);
        QVERIFY(idx.isValid());
        QCOMPARE(idx.row(), i);
        QCOMPARE(idx.column(), 0);
        // Access data for all roles
        QVERIFY(!model.data(idx, QGCArchiveModel::NameRole).isNull());
        QVERIFY(!model.data(idx, QGCArchiveModel::SizeRole).isNull());
        QVERIFY(!model.data(idx, QGCArchiveModel::IsDirectoryRole).isNull());
    }
}

void QGCArchiveModelTest::_testModelTesterFilterChange()
{
    // Test that filter changes emit proper signals
    QGCArchiveModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    QVERIFY(_load(model, _zipResource));
    const int originalCount = model.rowCount();
    QVERIFY(originalCount > 0);
    // Change filter - tester will verify beginResetModel/endResetModel signals
    model.setFilterMode(QGCArchiveModel::FilesOnly);
    // Verify rows are accessible after filter change
    for (int i = 0; i < model.rowCount(); ++i) {
        const QModelIndex idx = model.index(i);
        QVERIFY(idx.isValid());
        QVERIFY(!model.data(idx, QGCArchiveModel::IsDirectoryRole).toBool());
    }
    // Change to directories only
    model.setFilterMode(QGCArchiveModel::DirectoriesOnly);
    for (int i = 0; i < model.rowCount(); ++i) {
        const QModelIndex idx = model.index(i);
        QVERIFY(idx.isValid());
        QVERIFY(model.data(idx, QGCArchiveModel::IsDirectoryRole).toBool());
    }
    // Back to all entries
    model.setFilterMode(QGCArchiveModel::AllEntries);
    QCOMPARE(model.rowCount(), originalCount);
}

void QGCArchiveModelTest::_testModelTesterClearAndReload()
{
    // Test clear and reload emit proper signals
    QGCArchiveModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    // Load
    QVERIFY(_load(model, _zipResource));
    const int originalCount = model.rowCount();
    QVERIFY(originalCount > 0);
    // Clear - tester verifies reset signals (but path remains set)
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    // Refresh reloads from existing path (path was not cleared)
    QVERIFY(_load(model, [&model] { model.refresh(); }));
    QCOMPARE(model.rowCount(), originalCount);
    // Clear and set empty path
    model.clear();
    model.setArchivePath(QString());
    QCOMPARE(model.rowCount(), 0);
    // Now refresh should do nothing (no path)
    model.refresh();
    QCOMPARE(model.rowCount(), 0);
    // Set path again
    QVERIFY(_load(model, QStringLiteral(":/unittest/manifest.json.7z")));
    QVERIFY(model.rowCount() > 0);
    // Refresh with valid path
    const int count = model.rowCount();
    QVERIFY(_load(model, [&model] { model.refresh(); }));
    QCOMPARE(model.rowCount(), count);
    // Change path - tester verifies reset signals
    QVERIFY(_load(model, _zipResource));
    QVERIFY(model.rowCount() > 0);
}

UT_REGISTER_TEST(QGCArchiveModelTest, TestLabel::Unit, TestLabel::Utilities)
