#include "QGCArchiveModelTest.h"
#include "QGCArchiveModel.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void QGCArchiveModelTest::init()
{
    UnitTest::init();

    _tempDir = new QTemporaryDir();
    QVERIFY(_tempDir->isValid());
}

void QGCArchiveModelTest::cleanup()
{
    delete _tempDir;
    _tempDir = nullptr;

    UnitTest::cleanup();
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
}

void QGCArchiveModelTest::_testLoadArchive()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);

    model.setArchivePath(zipResource);

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
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

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
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

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

// ============================================================================
// Properties
// ============================================================================

void QGCArchiveModelTest::_testArchivePathProperty()
{
    QGCArchiveModel model;
    QSignalSpy pathSpy(&model, &QGCArchiveModel::archivePathChanged);

    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    model.setArchivePath(zipResource);
    QCOMPARE(model.archivePath(), zipResource);
    QCOMPARE(pathSpy.count(), 1);

    // Setting same path should not emit signal
    model.setArchivePath(zipResource);
    QCOMPARE(pathSpy.count(), 1);

    // Setting different path should emit signal
    model.setArchivePath(QString());
    QCOMPARE(pathSpy.count(), 2);
    QVERIFY(model.archivePath().isEmpty());
}

void QGCArchiveModelTest::_testCountProperties()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    QSignalSpy countSpy(&model, &QGCArchiveModel::countChanged);
    QSignalSpy fileCountSpy(&model, &QGCArchiveModel::fileCountChanged);

    model.setArchivePath(zipResource);

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
    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.zip"));
    QVERIFY(!model.loading());

    // Loading signal should have been emitted (true then false)
    QVERIFY(loadingSpy.count() >= 2);
}

void QGCArchiveModelTest::_testErrorProperty()
{
    QGCArchiveModel model;
    QSignalSpy errorSpy(&model, &QGCArchiveModel::errorStringChanged);

    // No error initially
    QVERIFY(model.errorString().isEmpty());

    // Valid archive should have no error
    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.zip"));
    QVERIFY(model.errorString().isEmpty());
}

// ============================================================================
// Filtering
// ============================================================================

void QGCArchiveModelTest::_testFilterModeAll()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

    // Default filter mode is AllEntries
    QCOMPARE(model.filterMode(), QGCArchiveModel::AllEntries);
    QCOMPARE(model.count(), model.fileCount() + model.directoryCount());
}

void QGCArchiveModelTest::_testFilterModeFilesOnly()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

    const int totalFiles = model.fileCount();
    const int totalDirs = model.directoryCount();

    QSignalSpy filterSpy(&model, &QGCArchiveModel::filterModeChanged);
    QSignalSpy countSpy(&model, &QGCArchiveModel::countChanged);

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
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

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
    // Bytes
    QCOMPARE(QGCArchiveModel::formatSize(0), QStringLiteral("0 B"));
    QCOMPARE(QGCArchiveModel::formatSize(100), QStringLiteral("100 B"));
    QCOMPARE(QGCArchiveModel::formatSize(1023), QStringLiteral("1023 B"));

    // Kilobytes
    QString kb = QGCArchiveModel::formatSize(1024);
    QVERIFY(kb.contains("KB"));

    QString kb2 = QGCArchiveModel::formatSize(1536); // 1.5 KB
    QVERIFY(kb2.contains("KB"));

    // Megabytes
    QString mb = QGCArchiveModel::formatSize(1024 * 1024);
    QVERIFY(mb.contains("MB"));

    // Gigabytes
    QString gb = QGCArchiveModel::formatSize(1024LL * 1024 * 1024);
    QVERIFY(gb.contains("GB"));

    // Negative (edge case)
    QCOMPARE(QGCArchiveModel::formatSize(-1), QStringLiteral("--"));
}

void QGCArchiveModelTest::_testContains()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

    // File that exists
    QVERIFY(model.contains("manifest.json"));

    // File that doesn't exist
    QVERIFY(!model.contains("nonexistent.txt"));

    // Empty string
    QVERIFY(!model.contains(""));
}

void QGCArchiveModelTest::_testClear()
{
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

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
    const QString zipResource = QStringLiteral(":/unittest/manifest.json.zip");

    QGCArchiveModel model;
    model.setArchivePath(zipResource);

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
    // Create a corrupt archive
    const QString corruptPath = _tempDir->path() + "/corrupt.zip";
    QFile corrupt(corruptPath);
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("This is not a valid ZIP file");
    corrupt.close();

    QGCArchiveModel model;
    QSignalSpy errorSpy(&model, &QGCArchiveModel::errorStringChanged);
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
    model.setArchivePath(corruptPath);

    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), false);
    QVERIFY(!model.errorString().isEmpty());
    QCOMPARE(model.count(), 0);
}

void QGCArchiveModelTest::_testNonExistentArchive()
{
    QGCArchiveModel model;
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*"));
    model.setArchivePath("/nonexistent/path/archive.zip");

    QCOMPARE(loadingSpy.count(), 1);
    QCOMPARE(loadingSpy.first().first().toBool(), false);
    QVERIFY(!model.errorString().isEmpty());
    QCOMPARE(model.count(), 0);
}

// ============================================================================
// QUrl Support
// ============================================================================

void QGCArchiveModelTest::_testArchiveUrl()
{
    QGCArchiveModel model;
    QSignalSpy pathSpy(&model, &QGCArchiveModel::archivePathChanged);
    QSignalSpy loadingSpy(&model, &QGCArchiveModel::loadingComplete);

    // Test with qrc:/ URL scheme (Qt resources)
    const QUrl qrcUrl(QStringLiteral("qrc:/unittest/manifest.json.zip"));
    model.setArchiveUrl(qrcUrl);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(model.archivePath(), QStringLiteral(":/unittest/manifest.json.zip"));
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
    const QString tempZipPath = _tempDir->path() + "/test_url.zip";
    QFile::copy(QStringLiteral(":/unittest/manifest.json.zip"), tempZipPath);
    QVERIFY(QFileInfo::exists(tempZipPath));

    const QUrl fileUrl = QUrl::fromLocalFile(tempZipPath);
    model.setArchiveUrl(fileUrl);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(model.archivePath(), tempZipPath);
    QVERIFY(loadingSpy.count() >= 1);
    QVERIFY(model.count() > 0);

    // Test that plain Qt resource path works (already tested in other tests)
    model.clear();
    pathSpy.clear();
    loadingSpy.clear();

    // QML commonly passes paths as URLs with the :/ prefix
    const QUrl resourcePathUrl(QStringLiteral(":/unittest/manifest.json.zip"));
    model.setArchiveUrl(resourcePathUrl);

    QCOMPARE(pathSpy.count(), 1);
    QVERIFY(model.count() > 0);
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
    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.zip"));

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

    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.zip"));

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
    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.zip"));
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
    model.setArchivePath(QStringLiteral(":/unittest/manifest.json.zip"));
    QVERIFY(model.rowCount() > 0);
}
