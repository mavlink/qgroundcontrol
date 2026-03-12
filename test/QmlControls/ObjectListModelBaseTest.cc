#include "ObjectListModelBaseTest.h"

#include "QmlObjectListModel.h"

// ===========================================================================
// index() overrides
// ===========================================================================

void ObjectListModelBaseTest::_indexValidRow()
{
    QmlObjectListModel model;
    QObject a, b;
    model.append(&a);
    model.append(&b);

    const QModelIndex idx0 = model.index(0, 0);
    QVERIFY(idx0.isValid());
    QCOMPARE(idx0.row(), 0);
    QCOMPARE(idx0.column(), 0);

    const QModelIndex idx1 = model.index(1, 0);
    QVERIFY(idx1.isValid());
    QCOMPARE(idx1.row(), 1);
}

void ObjectListModelBaseTest::_indexInvalidRow()
{
    QmlObjectListModel model;
    QObject a;
    model.append(&a);

    // Negative row
    QVERIFY(!model.index(-1, 0).isValid());

    // Beyond count
    QVERIFY(!model.index(1, 0).isValid());
    QVERIFY(!model.index(100, 0).isValid());
}

void ObjectListModelBaseTest::_indexNonZeroColumnInvalid()
{
    QmlObjectListModel model;
    QObject a;
    model.append(&a);

    // Column must be 0 for a list model
    QVERIFY(!model.index(0, 1).isValid());
    QVERIFY(!model.index(0, 2).isValid());
}

void ObjectListModelBaseTest::_indexWithValidParentInvalid()
{
    QmlObjectListModel model;
    QObject a, b;
    model.append(&a);
    model.append(&b);

    const QModelIndex parentIdx = model.index(0, 0);
    QVERIFY(parentIdx.isValid());

    // A flat list model should return invalid for any child of a valid parent
    QVERIFY(!model.index(0, 0, parentIdx).isValid());
}

// ===========================================================================
// parent() override
// ===========================================================================

void ObjectListModelBaseTest::_parentAlwaysInvalid()
{
    QmlObjectListModel model;
    QObject a;
    model.append(&a);

    const QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());

    // parent() should always return invalid for a flat list
    QVERIFY(!model.parent(idx).isValid());
}

// ===========================================================================
// columnCount() override
// ===========================================================================

void ObjectListModelBaseTest::_columnCountAlwaysOne()
{
    QmlObjectListModel model;

    // Empty model
    QCOMPARE(model.columnCount(), 1);
    QCOMPARE(model.columnCount(QModelIndex()), 1);

    // With items
    QObject a;
    model.append(&a);
    QCOMPARE(model.columnCount(), 1);

    // With a valid parent index — still 1
    const QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.columnCount(idx), 1);
}

// ===========================================================================
// hasChildren() override
// ===========================================================================

void ObjectListModelBaseTest::_hasChildrenRootEmpty()
{
    QmlObjectListModel model;

    // Root of an empty list has no children
    QVERIFY(!model.hasChildren(QModelIndex()));
}

void ObjectListModelBaseTest::_hasChildrenRootWithItems()
{
    QmlObjectListModel model;
    QObject a;
    model.append(&a);

    // Root of a non-empty list has children
    QVERIFY(model.hasChildren(QModelIndex()));
}

void ObjectListModelBaseTest::_hasChildrenNonRootAlwaysFalse()
{
    QmlObjectListModel model;
    QObject a;
    model.append(&a);

    const QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());

    // Non-root item should never have children in a flat list
    QVERIFY(!model.hasChildren(idx));
}

UT_REGISTER_TEST(ObjectListModelBaseTest, TestLabel::Unit)
