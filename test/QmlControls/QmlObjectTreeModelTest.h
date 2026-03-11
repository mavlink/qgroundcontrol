#pragma once

#include <QtCore/QObject>

#include "QmlObjectTreeModel.h"
#include "UnitTest.h"

class QmlObjectTreeModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Construction & empty state
    void _emptyModelDefaults();
    void _roleNamesIncludeNodeType();

    // Insert / Append
    void _appendToRoot();
    void _appendWithNodeType();
    void _insertAtPositions();
    void _appendToChild();
    void _insertInvalidParentLogsWarning();
    void _insertOutOfRangeLogsWarning();

    // Remove
    void _removeItemReturnsObject();
    void _removeSubtreeDecrementsCount();
    void _removeAtConvenience();
    void _removeChildrenKeepsParent();
    void _removeInvalidIndexReturnsNull();

    // Count cache
    void _countStaysConsistentAfterMixedOps();
    void _countReflectsNestedChildren();

    // Clear
    void _clearResetsToEmpty();
    void _clearAndDeleteContentsDeletesObjects();
    void _clearOnEmptyIsNoop();

    // Tree navigation
    void _parentOfRootChildIsInvalid();
    void _parentOfNestedChild();
    void _depthValues();
    void _indexForObjectFindsNested();
    void _indexForObjectNotFound();
    void _containsWorks();
    void _columnCountAlwaysOne();
    void _hasChildrenLeafVsParent();

    // Data & SetData
    void _dataObjectRole();
    void _dataTextRole();
    void _dataNodeTypeRole();
    void _dataSeparatorRole();
    void _dataInvalidIndexReturnsEmpty();
    void _setDataReplacesObject();
    void _setDataNonObjectRoleReturnsFalse();

    // Dirty tracking
    void _insertSetsDirty();
    void _childDirtyPropagates();
    void _removeDisconnectsDirty();

    // Model signals
    void _insertEmitsRowsInserted();
    void _removeEmitsRowsRemoved();
    void _clearEmitsModelReset();
    void _countChangedEmitted();

    // Nested reset
    void _nestedResetOnlyEmitsOnce();
    void _insertDuringResetNoSignals();

    // insertRows / removeRows guard
    void _insertRowsReturnsFalse();
    void _removeRowsReturnsFalse();

    // QPersistentModelIndex stability
    void _persistentIndexSurvivesInsertBefore();
    void _persistentIndexSurvivesRemoveOther();

    // Null object edge case
    void _insertNullObject();

    // Destructor safe when external objects outlive the model
    void _destructorSafeWithExternalObjects();

    // Destructor safe when objects destroyed before the model
    void _destructorSafeWithDestroyedObjects();
};
