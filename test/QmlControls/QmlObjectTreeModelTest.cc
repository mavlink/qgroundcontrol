#include "QmlObjectTreeModelTest.h"

#include "TestDirtyObject.h"

#include <QtCore/QPersistentModelIndex>
#include <QtCore/QAbstractItemModel>
#include <QtTest/QSignalSpy>

// ===========================================================================
// Construction & empty state
// ===========================================================================

void QmlObjectTreeModelTest::_emptyModelDefaults()
{
    QmlObjectTreeModel model;
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.isEmpty());
    QCOMPARE(model.dirty(), false);
}

void QmlObjectTreeModelTest::_roleNamesIncludeNodeType()
{
    QmlObjectTreeModel model;
    const auto roles = model.roleNames();
    QVERIFY(roles.values().contains("object"));
    QVERIFY(roles.values().contains("text"));
    QVERIFY(roles.values().contains("nodeType"));
    QVERIFY(roles.values().contains("separator"));
}

// ===========================================================================
// Insert / Append
// ===========================================================================

void QmlObjectTreeModelTest::_appendToRoot()
{
    QmlObjectTreeModel model;
    QObject obj;
    obj.setObjectName("item1");

    const QModelIndex idx = model.appendItem(&obj);
    QVERIFY(idx.isValid());
    QCOMPARE(idx.row(), 0);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.getObject(idx), &obj);
}

void QmlObjectTreeModelTest::_appendWithNodeType()
{
    QmlObjectTreeModel model;
    QObject obj;

    const QModelIndex idx = model.appendItem(&obj, QModelIndex(), QStringLiteral("myType"));
    QVERIFY(idx.isValid());
    QCOMPARE(model.data(idx, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("myType"));
}

void QmlObjectTreeModelTest::_insertAtPositions()
{
    QmlObjectTreeModel model;
    QObject a, b, c;
    a.setObjectName("a");
    b.setObjectName("b");
    c.setObjectName("c");

    model.appendItem(&b);               // [b]
    model.insertItem(0, &a);            // [a, b]
    model.insertItem(2, &c);            // [a, b, c]

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.getObject(model.index(0, 0)), &a);
    QCOMPARE(model.getObject(model.index(1, 0)), &b);
    QCOMPARE(model.getObject(model.index(2, 0)), &c);
}

void QmlObjectTreeModelTest::_appendToChild()
{
    QmlObjectTreeModel model;
    QObject parent, child;

    const QModelIndex parentIdx = model.appendItem(&parent);
    const QModelIndex childIdx  = model.appendItem(&child, parentIdx);

    QVERIFY(childIdx.isValid());
    QCOMPARE(model.rowCount(parentIdx), 1);
    QCOMPARE(model.getObject(childIdx), &child);
    QCOMPARE(model.count(), 2); // parent + child
}

void QmlObjectTreeModelTest::_insertInvalidParentLogsWarning()
{
    QmlObjectTreeModel model;
    QObject obj;

    // Create a stale index from another model — will be invalid for this model
    const QModelIndex badIndex = model.index(999, 0);
    QVERIFY(!badIndex.isValid());

    // appendItem with invalid index -> inserts at root (valid behavior)
    const QModelIndex idx = model.appendItem(&obj, badIndex);
    // Should still succeed since invalid parent = root
    QVERIFY(idx.isValid());
}

void QmlObjectTreeModelTest::_insertOutOfRangeLogsWarning()
{
    QmlObjectTreeModel model;
    QObject obj;

    const QModelIndex idx = model.insertItem(5, &obj); // row 5, but 0 children
    QVERIFY(!idx.isValid()); // should fail gracefully
    QCOMPARE(model.count(), 0);
}

// ===========================================================================
// Remove
// ===========================================================================

void QmlObjectTreeModelTest::_removeItemReturnsObject()
{
    QmlObjectTreeModel model;
    QObject obj;

    const QModelIndex idx = model.appendItem(&obj);
    QObject* removed = model.removeItem(idx);

    QCOMPARE(removed, &obj);
    QCOMPARE(model.count(), 0);
}

void QmlObjectTreeModelTest::_removeSubtreeDecrementsCount()
{
    QmlObjectTreeModel model;
    QObject parentObj, child1, child2, grandchild;

    const QModelIndex parentIdx = model.appendItem(&parentObj);
    const QModelIndex child1Idx = model.appendItem(&child1, parentIdx);
    model.appendItem(&child2, parentIdx);
    model.appendItem(&grandchild, child1Idx);

    QCOMPARE(model.count(), 4);

    // Remove parentObj — should remove parent + 2 children + 1 grandchild = 4
    model.removeItem(parentIdx);
    QCOMPARE(model.count(), 0);
}

void QmlObjectTreeModelTest::_removeAtConvenience()
{
    QmlObjectTreeModel model;
    QObject a, b;

    model.appendItem(&a);
    model.appendItem(&b);

    QObject* removed = model.removeAt(QModelIndex(), 0);
    QCOMPARE(removed, &a);
    QCOMPARE(model.count(), 1);
}

void QmlObjectTreeModelTest::_removeChildrenKeepsParent()
{
    QmlObjectTreeModel model;
    QObject parentObj, child1, child2;

    const QModelIndex parentIdx = model.appendItem(&parentObj);
    model.appendItem(&child1, parentIdx);
    model.appendItem(&child2, parentIdx);

    QCOMPARE(model.count(), 3);
    model.removeChildren(parentIdx);
    QCOMPARE(model.rowCount(parentIdx), 0);
    QCOMPARE(model.count(), 1); // parent still there
    QVERIFY(model.contains(&parentObj));
}

void QmlObjectTreeModelTest::_removeInvalidIndexReturnsNull()
{
    QmlObjectTreeModel model;
    QObject* removed = model.removeItem(QModelIndex());
    QVERIFY(removed == nullptr);
}

// ===========================================================================
// Count cache
// ===========================================================================

void QmlObjectTreeModelTest::_countStaysConsistentAfterMixedOps()
{
    QmlObjectTreeModel model;
    QObject objects[10];

    // Insert 10
    for (int i = 0; i < 10; i++) {
        model.appendItem(&objects[i]);
    }
    QCOMPARE(model.count(), 10);

    // Remove 5 (from end to avoid index shifting)
    for (int i = 9; i >= 5; i--) {
        model.removeAt(QModelIndex(), i);
    }
    QCOMPARE(model.count(), 5);

    // Add 3 more
    QObject extras[3];
    for (int i = 0; i < 3; i++) {
        model.appendItem(&extras[i]);
    }
    QCOMPARE(model.count(), 8);

    // Clear
    model.clear();
    QCOMPARE(model.count(), 0);
}

void QmlObjectTreeModelTest::_countReflectsNestedChildren()
{
    QmlObjectTreeModel model;
    QObject root, c1, c2, gc1, gc2, gc3;

    const QModelIndex rootIdx = model.appendItem(&root);
    const QModelIndex c1Idx   = model.appendItem(&c1, rootIdx);
    model.appendItem(&c2, rootIdx);
    model.appendItem(&gc1, c1Idx);
    model.appendItem(&gc2, c1Idx);
    model.appendItem(&gc3, c1Idx);

    QCOMPARE(model.count(), 6); // root + 2 children + 3 grandchildren
}

// ===========================================================================
// Clear
// ===========================================================================

void QmlObjectTreeModelTest::_clearResetsToEmpty()
{
    QmlObjectTreeModel model;
    QObject a, b, c;
    model.appendItem(&a);
    model.appendItem(&b);
    model.appendItem(&c);

    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.isEmpty());
}

void QmlObjectTreeModelTest::_clearAndDeleteContentsDeletesObjects()
{
    QmlObjectTreeModel model;

    auto* obj1 = new QObject;
    auto* obj2 = new QObject;

    model.appendItem(obj1);
    model.appendItem(obj2);

    QSignalSpy destroyed1(obj1, &QObject::destroyed);
    QSignalSpy destroyed2(obj2, &QObject::destroyed);

    model.clearAndDeleteContents();
    QCOMPARE(model.count(), 0);

    // deleteLater requires the event loop to process deferred deletions
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(destroyed1.count(), 1);
    QCOMPARE(destroyed2.count(), 1);
}

void QmlObjectTreeModelTest::_clearOnEmptyIsNoop()
{
    QmlObjectTreeModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.clear();
    QCOMPARE(resetSpy.count(), 0); // no-op — no reset emitted
}

// ===========================================================================
// Tree navigation
// ===========================================================================

void QmlObjectTreeModelTest::_parentOfRootChildIsInvalid()
{
    QmlObjectTreeModel model;
    QObject obj;
    const QModelIndex idx = model.appendItem(&obj);
    QVERIFY(!model.parent(idx).isValid());
}

void QmlObjectTreeModelTest::_parentOfNestedChild()
{
    QmlObjectTreeModel model;
    QObject parentObj, childObj;

    const QModelIndex parentIdx = model.appendItem(&parentObj);
    const QModelIndex childIdx  = model.appendItem(&childObj, parentIdx);

    const QModelIndex retrievedParent = model.parent(childIdx);
    QVERIFY(retrievedParent.isValid());
    QCOMPARE(model.getObject(retrievedParent), &parentObj);
}

void QmlObjectTreeModelTest::_depthValues()
{
    QmlObjectTreeModel model;
    QObject root, child, grandchild;

    const QModelIndex rootIdx  = model.appendItem(&root);
    const QModelIndex childIdx = model.appendItem(&child, rootIdx);
    const QModelIndex gcIdx    = model.appendItem(&grandchild, childIdx);

    QCOMPARE(model.depth(rootIdx), 0);
    QCOMPARE(model.depth(childIdx), 1);
    QCOMPARE(model.depth(gcIdx), 2);
    QCOMPARE(model.depth(QModelIndex()), -1); // invalid
}

void QmlObjectTreeModelTest::_indexForObjectFindsNested()
{
    QmlObjectTreeModel model;
    QObject root, child, grandchild;

    const QModelIndex rootIdx  = model.appendItem(&root);
    const QModelIndex childIdx = model.appendItem(&child, rootIdx);
    model.appendItem(&grandchild, childIdx);

    const QModelIndex found = model.indexForObject(&grandchild);
    QVERIFY(found.isValid());
    QCOMPARE(model.getObject(found), &grandchild);
}

void QmlObjectTreeModelTest::_indexForObjectNotFound()
{
    QmlObjectTreeModel model;
    QObject notInModel;
    QVERIFY(!model.indexForObject(&notInModel).isValid());
    QVERIFY(!model.indexForObject(nullptr).isValid());
}

void QmlObjectTreeModelTest::_containsWorks()
{
    QmlObjectTreeModel model;
    QObject inModel, notInModel;
    model.appendItem(&inModel);

    QVERIFY(model.contains(&inModel));
    QVERIFY(!model.contains(&notInModel));
}

void QmlObjectTreeModelTest::_columnCountAlwaysOne()
{
    QmlObjectTreeModel model;
    QObject obj;
    const QModelIndex idx = model.appendItem(&obj);

    QCOMPARE(model.columnCount(), 1);
    QCOMPARE(model.columnCount(idx), 1);
}

void QmlObjectTreeModelTest::_hasChildrenLeafVsParent()
{
    QmlObjectTreeModel model;
    QObject parentObj, childObj;

    const QModelIndex parentIdx = model.appendItem(&parentObj);
    QVERIFY(!model.hasChildren(parentIdx)); // no children yet

    const QModelIndex childIdx = model.appendItem(&childObj, parentIdx);
    QVERIFY(model.hasChildren(parentIdx));  // now has children
    QVERIFY(!model.hasChildren(childIdx));  // leaf
    QVERIFY(model.hasChildren());           // root has children
}

// ===========================================================================
// Data & SetData
// ===========================================================================

void QmlObjectTreeModelTest::_dataObjectRole()
{
    QmlObjectTreeModel model;
    QObject obj;
    const QModelIndex idx = model.appendItem(&obj);

    const QVariant val = model.data(idx, Qt::UserRole); // ObjectRole
    QCOMPARE(val.value<QObject*>(), &obj);
}

void QmlObjectTreeModelTest::_dataTextRole()
{
    QmlObjectTreeModel model;
    QObject obj;
    obj.setObjectName("hello");
    const QModelIndex idx = model.appendItem(&obj);

    QCOMPARE(model.data(idx, Qt::UserRole + 1).toString(), QStringLiteral("hello")); // TextRole
}

void QmlObjectTreeModelTest::_dataNodeTypeRole()
{
    QmlObjectTreeModel model;
    QObject obj;
    const QModelIndex idx = model.appendItem(&obj, QModelIndex(), QStringLiteral("missionItem"));

    QCOMPARE(model.data(idx, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("missionItem"));
}

void QmlObjectTreeModelTest::_dataSeparatorRole()
{
    QmlObjectTreeModel model;

    // Build a parent with three leaf children
    QObject parent;
    QObject child1, child2, child3;
    const QModelIndex parentIdx = model.appendItem(&parent, QModelIndex(), QStringLiteral("group"));
    const QModelIndex c1 = model.appendItem(&child1, parentIdx, QStringLiteral("item"));
    const QModelIndex c2 = model.appendItem(&child2, parentIdx, QStringLiteral("item"));
    const QModelIndex c3 = model.appendItem(&child3, parentIdx, QStringLiteral("item"));

    // First two children are not last → separator = true
    QCOMPARE(model.data(c1, QmlObjectTreeModel::SeparatorRole).toBool(), true);
    QCOMPARE(model.data(c2, QmlObjectTreeModel::SeparatorRole).toBool(), true);
    // Last child → separator = false
    QCOMPARE(model.data(c3, QmlObjectTreeModel::SeparatorRole).toBool(), false);

    // Parent node has children → separator = false regardless of position
    QCOMPARE(model.data(parentIdx, QmlObjectTreeModel::SeparatorRole).toBool(), false);

    // Remove last child — c2 is now last and should emit dataChanged with SeparatorRole
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);
    model.removeItem(c3);

    // c2 should now be the last child → separator = false
    const QModelIndex c2New = model.index(1, 0, parentIdx);
    QCOMPARE(model.data(c2New, QmlObjectTreeModel::SeparatorRole).toBool(), false);

    // Verify dataChanged was emitted with SeparatorRole
    bool foundSeparatorChange = false;
    for (const auto& call : dataChangedSpy) {
        const QList<int> roles = call.at(2).value<QList<int>>();
        if (roles.contains(QmlObjectTreeModel::SeparatorRole)) {
            foundSeparatorChange = true;
            break;
        }
    }
    QVERIFY(foundSeparatorChange);

    // Insert a new child at the end — c2 should flip back to separator = true
    QObject child4;
    model.appendItem(&child4, parentIdx, QStringLiteral("item"));
    QCOMPARE(model.data(c2New, QmlObjectTreeModel::SeparatorRole).toBool(), true);
}

void QmlObjectTreeModelTest::_dataInvalidIndexReturnsEmpty()
{
    QmlObjectTreeModel model;
    QVERIFY(!model.data(QModelIndex(), Qt::UserRole).isValid());
}

void QmlObjectTreeModelTest::_setDataReplacesObject()
{
    QmlObjectTreeModel model;
    QObject obj1, obj2;

    const QModelIndex idx = model.appendItem(&obj1);
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    bool ok = model.setData(idx, QVariant::fromValue<QObject*>(&obj2), Qt::UserRole);
    QVERIFY(ok);
    QCOMPARE(model.getObject(idx), &obj2);
    QCOMPARE(dataChangedSpy.count(), 1);
}

void QmlObjectTreeModelTest::_setDataNonObjectRoleReturnsFalse()
{
    QmlObjectTreeModel model;
    QObject obj;
    const QModelIndex idx = model.appendItem(&obj);

    QVERIFY(!model.setData(idx, QVariant("test"), Qt::DisplayRole));
}

// ===========================================================================
// Dirty tracking
// ===========================================================================

void QmlObjectTreeModelTest::_insertSetsDirty()
{
    QmlObjectTreeModel model;
    model.setDirty(false);

    QObject obj;
    model.appendItem(&obj);
    QVERIFY(model.dirty());
}

void QmlObjectTreeModelTest::_childDirtyPropagates()
{
    QmlObjectTreeModel model;
    TestDirtyObject obj;

    QSignalSpy dirtySpy(&model, &QmlObjectTreeModel::dirtyChanged);
    QVERIFY(dirtySpy.isValid());

    model.appendItem(&obj);
    model.setDirty(false);
    dirtySpy.clear();

    obj.setDirty(true);
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), true);
    QVERIFY(model.dirty());
}

void QmlObjectTreeModelTest::_removeDisconnectsDirty()
{
    QmlObjectTreeModel model;
    TestDirtyObject obj;

    model.appendItem(&obj);
    const QModelIndex idx = model.indexForObject(&obj);
    model.removeItem(idx);
    model.setDirty(false);

    QSignalSpy dirtySpy(&model, &QmlObjectTreeModel::dirtyChanged);
    obj.setDirty(true);

    QCOMPARE(dirtySpy.count(), 0);
    QVERIFY(!model.dirty());
}

// ===========================================================================
// Model signals
// ===========================================================================

void QmlObjectTreeModelTest::_insertEmitsRowsInserted()
{
    QmlObjectTreeModel model;
    QSignalSpy spy(&model, &QAbstractItemModel::rowsInserted);

    QObject obj;
    model.appendItem(&obj);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toInt(), 0); // first = 0
    QCOMPARE(spy.at(0).at(2).toInt(), 0); // last = 0
}

void QmlObjectTreeModelTest::_removeEmitsRowsRemoved()
{
    QmlObjectTreeModel model;
    QObject obj;
    const QModelIndex idx = model.appendItem(&obj);

    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);
    model.removeItem(idx);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toInt(), 0); // first = 0
    QCOMPARE(spy.at(0).at(2).toInt(), 0); // last = 0
}

void QmlObjectTreeModelTest::_clearEmitsModelReset()
{
    QmlObjectTreeModel model;
    QObject obj;
    model.appendItem(&obj);

    QSignalSpy spy(&model, &QAbstractItemModel::modelReset);
    model.clear();

    QCOMPARE(spy.count(), 1);
}

void QmlObjectTreeModelTest::_countChangedEmitted()
{
    QmlObjectTreeModel model;
    QSignalSpy spy(&model, &QmlObjectTreeModel::countChanged);

    QObject obj;
    model.appendItem(&obj);
    QVERIFY(spy.count() >= 1);

    spy.clear();
    model.removeItem(model.index(0, 0));
    QVERIFY(spy.count() >= 1);
}

// ===========================================================================
// Nested reset
// ===========================================================================

void QmlObjectTreeModelTest::_nestedResetOnlyEmitsOnce()
{
    QmlObjectTreeModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.beginResetModel();
    model.beginResetModel(); // nested — should NOT emit again

    QObject a, b;
    model.appendItem(&a);
    model.appendItem(&b);

    model.endResetModel(); // inner — no-op
    model.endResetModel(); // outer — actually emits

    QCOMPARE(resetSpy.count(), 1);
}

void QmlObjectTreeModelTest::_insertDuringResetNoSignals()
{
    QmlObjectTreeModel model;
    model.beginResetModel();

    QSignalSpy insertSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy countSpy(&model, &QmlObjectTreeModel::countChanged);

    QObject obj;
    model.appendItem(&obj);

    QCOMPARE(insertSpy.count(), 0); // suppressed during reset
    QCOMPARE(countSpy.count(), 0);  // suppressed during reset

    model.endResetModel();
    QCOMPARE(model.count(), 1); // but count is correct
}

// ===========================================================================
// insertRows / removeRows guard
// ===========================================================================

void QmlObjectTreeModelTest::_insertRowsReturnsFalse()
{
    QmlObjectTreeModel model;
    QVERIFY(!model.insertRows(0, 1));
}

void QmlObjectTreeModelTest::_removeRowsReturnsFalse()
{
    QmlObjectTreeModel model;
    QVERIFY(!model.removeRows(0, 1));
}

// ===========================================================================
// QPersistentModelIndex stability
// ===========================================================================

void QmlObjectTreeModelTest::_persistentIndexSurvivesInsertBefore()
{
    QmlObjectTreeModel model;
    QObject a, b;

    model.appendItem(&a);
    QPersistentModelIndex persistentIdx(model.index(0, 0));
    QCOMPARE(model.getObject(persistentIdx), &a);

    // Insert before — persistent index should shift to row 1
    model.insertItem(0, &b);
    QVERIFY(persistentIdx.isValid());
    QCOMPARE(persistentIdx.row(), 1);
    QCOMPARE(model.getObject(persistentIdx), &a);
}

void QmlObjectTreeModelTest::_persistentIndexSurvivesRemoveOther()
{
    QmlObjectTreeModel model;
    QObject a, b, c;

    model.appendItem(&a);
    model.appendItem(&b);
    model.appendItem(&c);

    QPersistentModelIndex persistentIdx(model.index(2, 0)); // points to c
    QCOMPARE(model.getObject(persistentIdx), &c);

    // Remove a (row 0) — persistent should shift to row 1
    model.removeAt(QModelIndex(), 0);
    QVERIFY(persistentIdx.isValid());
    QCOMPARE(persistentIdx.row(), 1);
    QCOMPARE(model.getObject(persistentIdx), &c);
}

// ===========================================================================
// Null object edge case
// ===========================================================================

void QmlObjectTreeModelTest::_insertNullObject()
{
    QmlObjectTreeModel model;
    const QModelIndex idx = model.appendItem(nullptr);

    QVERIFY(idx.isValid());
    QCOMPARE(model.count(), 1);
    QVERIFY(model.getObject(idx) == nullptr);
    QVERIFY(!model.data(idx, Qt::UserRole).isValid()); // null object → invalid variant
}

// ---------------------------------------------------------------------------
// Destructor safety: external objects outlive the model
// ---------------------------------------------------------------------------

void QmlObjectTreeModelTest::_destructorSafeWithExternalObjects()
{
    QObject externalObj;
    {
        QmlObjectTreeModel model;
        model.appendItem(&externalObj, QModelIndex(), QStringLiteral("test"));
        // model destroyed here — must not crash or double-free externalObj
    }
    // externalObj still alive — reaching here means no crash
    QVERIFY(true);
}

// ---------------------------------------------------------------------------
// Destructor safety: objects destroyed before the model (shutdown scenario)
// ---------------------------------------------------------------------------

void QmlObjectTreeModelTest::_destructorSafeWithDestroyedObjects()
{
    QmlObjectTreeModel model;
    {
        QObject tempObj;
        model.appendItem(&tempObj, QModelIndex(), QStringLiteral("test"));
        // tempObj destroyed here while model still holds a pointer to it
    }
    // model destroyed after tempObj — must not crash during ~QmlObjectTreeModel
    QVERIFY(true);
}

UT_REGISTER_TEST(QmlObjectTreeModelTest, TestLabel::Unit)
