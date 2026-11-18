#pragma once

#include "UnitTest.h"

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

    // Filtering
    void _testFilterModeFilesOnly();
    void _testFilterModeDirectoriesOnly();

    // Utilities
    void _testFormatSize();
    void _testContains();
    void _testClear();
    void _testRefresh();

    // Edge cases
    void _testInvalidArchive();
    void _testNonExistentArchive();

    // QUrl support
    void _testArchiveUrl();
    void _testArchiveUrlRejectsRemote();

    // Model invariants (QAbstractItemModelTester)
    void _testModelTesterEmpty();
    void _testModelTesterLoaded();
    void _testModelTesterFilterChange();
    void _testModelTesterClearAndReload();

private:
    static inline const QString _zipResource = QStringLiteral(":/unittest/manifest.json.zip");
};
