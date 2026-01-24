#pragma once

#include "TestFixtures.h"

/// Unit tests for QmlObjectListModel list operations.
class QmlObjectListModelTest : public OfflineTest
{
    Q_OBJECT

public:
    QmlObjectListModelTest() = default;

private slots:
    // Basic operations
    void _initialStateTest();
    void _appendTest();
    void _appendListTest();
    void _insertTest();
    void _removeAtTest();
    void _removeOneTest();
    void _clearTest();

    // Access operations
    void _getTest();
    void _operatorBracketTest();
    void _indexOfTest();
    void _containsTest();

    // Model operations
    void _countTest();
    void _rowCountTest();
    void _dataTest();
    void _moveTest();

    // Ownership
    void _clearAndDeleteContentsTest();

    // Dirty flag
    void _dirtyFlagTest();
};
