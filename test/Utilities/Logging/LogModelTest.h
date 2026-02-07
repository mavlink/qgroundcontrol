#pragma once

#include "UnitTest.h"

class LogModel;

class LogModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testInitialState();
    void _testAppendEntry();
    void _testAppendMultiple();
    void _testClear();
    void _testRoleNames();
    void _testDataAccess();
    void _testDataAccessSourceLocation();
    void _testMaxEntries();
    void _testAllFormatted();
    void _testCountChanged();
    void _testFilterLevel();
    void _testFilterCategory();
    void _testFilterCategoryWildcard();
    void _testFilterText();
    void _testClearFilters();
    void _testFilteredFormatted();
    void _testFilteredEntries();
    void _testCategories();
    void _testTotalCountVsCount();

private:
    LogModel* _model = nullptr;
};
