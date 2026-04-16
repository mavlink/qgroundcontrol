#pragma once

#include "UnitTest.h"

/// Unit tests for LoggingCategoryFlatModel, LoggingCategoryTreeModel, and
/// QGCLoggingCategoryItem.
///
/// QGCLoggingCategoryItem::setLogLevel() / setEnabled() push the change into
/// the global QGCLoggingCategoryManager as a side effect, so these tests use
/// setLogLevelFromManager() — which suppresses the write-back — and otherwise
/// stick to fully-custom category names that are not used elsewhere in the
/// codebase. That way the test runner's real category filters stay untouched.
class LoggingCategoryModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testItemConstructionDefaultsToWarning();
    void _testItemEnabledReflectsLogLevel();
    void _testSetLogLevelFromManagerEmitsOnTransition();
    void _testSetLogLevelFromManagerNoOp();

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
