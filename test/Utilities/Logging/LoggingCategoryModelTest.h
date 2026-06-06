#pragma once

#include "UnitTest.h"

/// Unit tests for LoggingCategoryFlatModel, LoggingCategoryTreeModel, and
/// QGCLoggingCategoryItem.
///
/// QGCLoggingCategoryItem::setEnabled() pushes the change into the global
/// QGCLoggingCategoryManager as a side effect, so these tests use
/// setEnabledFromManager() — which suppresses the write-back — and otherwise
/// stick to fully-custom category names that are not used elsewhere in the
/// codebase. That way the test runner's real category filters stay untouched.
class LoggingCategoryModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testItemConstructionDisabled();
    void _testItemConstructionEnabled();
    void _testSetEnabledFromManagerEmitsSignal();
    void _testSetEnabledFromManagerNoOp();

    void _testFlatModelEmptyState();
    void _testFlatModelInsertSortedAlphabetical();
    void _testFlatModelDataRoles();
    void _testFlatModelInvalidIndexReturnsNull();
    void _testFlatModelFindByFullName();
    void _testFlatModelRoleNames();
    void _testFlatModelFlagsIsEditable();

    void _testTreeModelEmptyState();
    void _testTreeModelInsertSingleLevel();
    void _testTreeModelInsertNestedCreatesIntermediate();
    void _testTreeModelDataRoles();
    void _testTreeModelRoleNames();
};
