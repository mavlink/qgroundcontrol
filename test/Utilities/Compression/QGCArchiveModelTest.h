#pragma once

#include <functional>

#include "UnitTest.h"

class QGCArchiveModel;

/// Tests for QGCArchiveModel (QAbstractListModel for archive contents)
class QGCArchiveModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Basic model functionality
    void _testEmptyModel();
    void _testLoadArchive();
    void _testRoleNames();
    void _testDataAccess();
    void _testGetMethod();
    void _testGetAgreesWithData();

    // Properties
    void _testArchivePathProperty();
    void _testCountProperties();
    void _testLoadingProperty();
    void _testAbandonedLoad_data();
    void _testAbandonedLoad();
    void _testErrorClearedWithArchive_data();
    void _testErrorClearedWithArchive();

    // Filtering
    void _testFilterModeFilesOnly();
    void _testFilterModeDirectoriesOnly();

    // Utilities
    void _testFormatSize();
    void _testContains();
    void _testClear();
    void _testRefresh();

    // Edge cases
    void _testInvalidArchive_data();
    void _testInvalidArchive();
    void _testNonExistentArchive();
    void _testEmptyArchive();
    void _testDirectoryEntries();

    // QUrl support
    void _testArchiveUrl();
    void _testArchiveUrlRejectsRemote();

    // Model invariants (QAbstractItemModelTester)
    void _testModelTesterEmpty();
    void _testModelTesterLoaded();
    void _testModelTesterFilterChange();
    void _testModelTesterClearAndReload();

private:
    /// Runs @a startLoad, which has to start a load, and waits for that load to end
    /// @return The outcome that loadingComplete reported, false if no load ended in time
    static bool _load(QGCArchiveModel& model, const std::function<void()>& startLoad);
    static bool _load(QGCArchiveModel& model, const QString& archivePath);

    static inline const QString _zipResource = QStringLiteral(":/unittest/manifest.json.zip");
};
