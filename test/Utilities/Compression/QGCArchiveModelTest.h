#pragma once

#include "BaseClasses/TempDirectoryTest.h"

/// Tests for QGCArchiveModel (QAbstractListModel for archive contents)
class QGCArchiveModelTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    // Basic model functionality
    void _testEmptyModel();
    void _testLoadArchive();
    void _testRoleNames();
    void _testDataAccess();
    void _testGetMethod();

    // Properties
    void _testArchivePathProperty();
    void _testCountProperties();
    void _testLoadingProperty();
    void _testErrorProperty();

    // Filtering
    void _testFilterModeAll();
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

    // Model invariants (QAbstractItemModelTester)
    void _testModelTesterEmpty();
    void _testModelTesterLoaded();
    void _testModelTesterFilterChange();
    void _testModelTesterClearAndReload();
};
