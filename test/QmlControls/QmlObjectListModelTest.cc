#include "QmlObjectListModelTest.h"
#include "QmlObjectListModel.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void QmlObjectListModelTest::_initialStateTest()
{
    QmlObjectListModel model;

    QCOMPARE(model.count(), 0);
    QVERIFY(model.isEmpty());
}

void QmlObjectListModelTest::_appendTest()
{
    QmlObjectListModel model;
    QObject obj1;
    QObject obj2;

    model.append(&obj1);
    QCOMPARE(model.count(), 1);
    QVERIFY(!model.isEmpty());

    model.append(&obj2);
    QCOMPARE(model.count(), 2);
}

void QmlObjectListModelTest::_appendListTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;
    QList<QObject*> objects = {&obj1, &obj2, &obj3};

    model.append(objects);
    QCOMPARE(model.count(), 3);
}

void QmlObjectListModelTest::_insertTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;

    model.append(&obj1);
    model.append(&obj3);
    model.insert(1, &obj2);

    QCOMPARE(model.count(), 3);
    QCOMPARE(model[0], &obj1);
    QCOMPARE(model[1], &obj2);
    QCOMPARE(model[2], &obj3);
}

void QmlObjectListModelTest::_removeAtTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;

    model.append(&obj1);
    model.append(&obj2);
    model.append(&obj3);

    QObject* removed = model.removeAt(1);
    QCOMPARE(removed, &obj2);
    QCOMPARE(model.count(), 2);
    QCOMPARE(model[0], &obj1);
    QCOMPARE(model[1], &obj3);
}

void QmlObjectListModelTest::_removeOneTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;

    model.append(&obj1);
    model.append(&obj2);
    model.append(&obj3);

    QObject* removed = model.removeOne(&obj2);
    QCOMPARE(removed, &obj2);
    QCOMPARE(model.count(), 2);
    QVERIFY(!model.contains(&obj2));
}

void QmlObjectListModelTest::_clearTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2;

    model.append(&obj1);
    model.append(&obj2);
    QCOMPARE(model.count(), 2);

    model.clear();
    QCOMPARE(model.count(), 0);
    QVERIFY(model.isEmpty());
}

void QmlObjectListModelTest::_getTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2;

    model.append(&obj1);
    model.append(&obj2);

    QCOMPARE(model.get(0), &obj1);
    QCOMPARE(model.get(1), &obj2);
}

void QmlObjectListModelTest::_operatorBracketTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2;

    model.append(&obj1);
    model.append(&obj2);

    QCOMPARE(model[0], &obj1);
    QCOMPARE(model[1], &obj2);

    // Const version
    const QmlObjectListModel& constModel = model;
    QCOMPARE(constModel[0], &obj1);
}

void QmlObjectListModelTest::_indexOfTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;

    model.append(&obj1);
    model.append(&obj2);

    QCOMPARE(model.indexOf(&obj1), 0);
    QCOMPARE(model.indexOf(&obj2), 1);
    QCOMPARE(model.indexOf(&obj3), -1);  // Not in list
}

void QmlObjectListModelTest::_containsTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;

    model.append(&obj1);
    model.append(&obj2);

    QVERIFY(model.contains(&obj1));
    QVERIFY(model.contains(&obj2));
    QVERIFY(!model.contains(&obj3));
}

void QmlObjectListModelTest::_countTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2;

    QCOMPARE(model.count(), 0);

    model.append(&obj1);
    QCOMPARE(model.count(), 1);

    model.append(&obj2);
    QCOMPARE(model.count(), 2);

    model.removeAt(0);
    QCOMPARE(model.count(), 1);
}

void QmlObjectListModelTest::_rowCountTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2;

    // rowCount is protected, so we use count() instead
    QCOMPARE(model.count(), 0);

    model.append(&obj1);
    model.append(&obj2);
    QCOMPARE(model.count(), 2);
}

void QmlObjectListModelTest::_dataTest()
{
    QmlObjectListModel model;
    QObject obj1;
    obj1.setObjectName("TestObject");

    model.append(&obj1);

    // Use public get() method instead of protected data()
    QObject* objPtr = model.get(0);
    QVERIFY(objPtr != nullptr);
    QCOMPARE(objPtr, &obj1);
}

void QmlObjectListModelTest::_moveTest()
{
    QmlObjectListModel model;
    QObject obj1, obj2, obj3;

    model.append(&obj1);
    model.append(&obj2);
    model.append(&obj3);

    // Move obj1 from index 0 to index 2
    model.move(0, 2);

    QCOMPARE(model[0], &obj2);
    QCOMPARE(model[1], &obj3);
    QCOMPARE(model[2], &obj1);
}

void QmlObjectListModelTest::_clearAndDeleteContentsTest()
{
    QmlObjectListModel model;
    QObject* obj1 = new QObject();
    QObject* obj2 = new QObject();
    QSignalSpy spy1(obj1, &QObject::destroyed);
    QSignalSpy spy2(obj2, &QObject::destroyed);

    model.append(obj1);
    model.append(obj2);
    QCOMPARE(model.count(), 2);

    model.clearAndDeleteContents();

    QCOMPARE(model.count(), 0);
    // Objects should be scheduled for deletion
    QTRY_COMPARE(spy1.count(), 1);
    QTRY_COMPARE(spy2.count(), 1);
}

void QmlObjectListModelTest::_dirtyFlagTest()
{
    QmlObjectListModel model;
    QSignalSpy dirtySpy(&model, &QmlObjectListModel::dirtyChanged);

    QVERIFY(!model.dirty());

    model.setDirty(true);
    QVERIFY(model.dirty());
    QCOMPARE(dirtySpy.count(), 1);

    model.setDirty(false);
    QVERIFY(!model.dirty());
    QCOMPARE(dirtySpy.count(), 2);

    // Setting to same value should not emit
    model.setDirty(false);
    QCOMPARE(dirtySpy.count(), 2);
}
