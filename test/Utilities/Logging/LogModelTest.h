#pragma once

#include "UnitTest.h"

class LogModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _appendAndRowCount();
    void _batchFlush();
    void _trimExcessOnOverflow();
    void _categories();
    void _clear();
    void _maxEntries();
    void _filterByLevel();
    void _filterByCategory();
    void _filterByText();
    void _filterByRegex();
    void _clearFilters();
    void _filteredEntries();
    void _categoriesComboList();
    void _setFilterTextDeferred();
    void _allEntriesSnapshot();
    void _invalidRegex();
    void _columnCount();
    void _multiData();
};
