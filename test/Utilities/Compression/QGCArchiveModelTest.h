#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

/// Tests for QGCArchiveModel (QAbstractListModel for archive contents)
class QGCArchiveModelTest : public UnitTest
{
    Q_OBJECT

public:
    QGCArchiveModelTest() = default;

private slots:
    void init() override;
    void cleanup() override;

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

private:
    QTemporaryDir *_tempDir = nullptr;
};
