#include "QGCArchiveModelTest.h"

#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "QGCArchiveModel.h"
#include "QGCCompressionTest.h"
#include "UnitTest.h"

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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(archivePath);

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
    model.setArchivePath(_zipResource);
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
    // Loading should be false initially
    QVERIFY(!model.loading());
    // After setArchivePath completes synchronously, loading should be false
    model.setArchivePath(_zipResource);
    QVERIFY(!model.loading());
    // Loading signal should have been emitted (true then false)
    QVERIFY(loadingSpy.count() >= 2);
}

// ============================================================================
// Filtering
// ============================================================================
void QGCArchiveModelTest::_testFilterModeFilesOnly()
{
    QGCArchiveModel model;
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
    const int originalCount = model.count();
    QVERIFY(originalCount > 0);
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    model.refresh();
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(model.count(), originalCount);
}

// ============================================================================
// Edge Cases
// ============================================================================
void QGCArchiveModelTest::_testInvalidArchive()
{
    QTemporaryDir tempDir;
    // Create a corrupt archive
    const QString corruptPath = tempDir.path() + "/corrupt.zip";
    QVERIFY(TestFixtures::writeFile(corruptPath, "This is not a valid ZIP file"));
    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);
    // libarchive's error text (e.g. "Unrecognized archive format") and the number of
    // warnings emitted vary across platforms/libarchive versions, so suppress the noise
    // and assert the failure via loadingComplete/errorString instead.
    ignoreLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("Failed to open file:"));
    model.setArchivePath(corruptPath);
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
    verifyExpectedLogMessage();
    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), false);
    QCOMPARE(model.errorString(), QStringLiteral("File does not exist: /nonexistent/path/archive.zip"));
    QCOMPARE(model.count(), 0);
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
    QCOMPARE(pathSpy.count(), 1);
    QVERIFY(model.count() > 0);
}

void QGCArchiveModelTest::_testArchiveUrlRejectsRemote()
{
    QGCArchiveModel model;
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
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
    model.setArchivePath(_zipResource);
    const int originalCount = model.rowCount();
    QVERIFY(originalCount > 0);
    // Clear - tester verifies reset signals (but path remains set)
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    // Refresh reloads from existing path (path was not cleared)
    model.refresh();
    QCOMPARE(model.rowCount(), originalCount);
    // Clear and set empty path
    model.clear();
    model.setArchivePath(QString());
    QCOMPARE(model.rowCount(), 0);
    // Now refresh should do nothing (no path)
    model.refresh();
    QCOMPARE(model.rowCount(), 0);
    // Set path again
    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.7z"));
    QVERIFY(model.rowCount() > 0);
    // Refresh with valid path
    const int count = model.rowCount();
    model.refresh();
    QCOMPARE(model.rowCount(), count);
    // Change path - tester verifies reset signals
    model.setArchivePath(_zipResource);
    QVERIFY(model.rowCount() > 0);
}

UT_REGISTER_TEST(QGCArchiveModelTest, TestLabel::Unit, TestLabel::Utilities)
