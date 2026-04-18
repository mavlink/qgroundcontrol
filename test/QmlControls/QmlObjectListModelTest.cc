#include "QmlObjectListModelTest.h"
#include <QtTest/QSignalSpy>

#include "TestDirtyObject.h"


namespace {

class TestQmlObjectListModel : public QmlObjectListModel
{
public:
    explicit TestQmlObjectListModel(QObject* parent = nullptr)
        : QmlObjectListModel(parent)
    {
    }

    void setSkipDirtyFirstItemForTest(bool skip)
    {
        _skipDirtyFirstItem = skip;
    }
};

}  // namespace

void QmlObjectListModelTest::_childDirtyPropagates()
{
    TestQmlObjectListModel model;
    TestDirtyObject object;

    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);
    QVERIFY(modelDirtySpy.isValid());

    model.append(&object);
    model.setDirty(false);
    modelDirtySpy.clear();

    object.setDirty(true);

    QCOMPARE(modelDirtySpy.count(), 1);
    QCOMPARE(modelDirtySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(model.dirty(), true);
}

void QmlObjectListModelTest::_removeDisconnectsDirtyPropagation()
{
    TestQmlObjectListModel model;
    TestDirtyObject object;

    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);
    QVERIFY(modelDirtySpy.isValid());

    model.append(&object);
    model.setDirty(false);
    modelDirtySpy.clear();

    QObject* removed = model.removeAt(0);
    QCOMPARE(removed, static_cast<QObject*>(&object));
    model.setDirty(false);
    modelDirtySpy.clear();

    object.setDirty(true);
    QCOMPARE(modelDirtySpy.count(), 0);
    QCOMPARE(model.dirty(), false);
}

void QmlObjectListModelTest::_skipDirtyFirstItemSkipsFirstConnection()
{
    TestQmlObjectListModel model;
    model.setSkipDirtyFirstItemForTest(true);

    TestDirtyObject firstObject;
    TestDirtyObject secondObject;

    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);
    QVERIFY(modelDirtySpy.isValid());

    model.append(&firstObject);
    model.append(&secondObject);
    model.setDirty(false);
    modelDirtySpy.clear();

    firstObject.setDirty(true);
    QCOMPARE(modelDirtySpy.count(), 0);
    QCOMPARE(model.dirty(), false);

    secondObject.setDirty(true);
    QCOMPARE(modelDirtySpy.count(), 1);
    QCOMPARE(modelDirtySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(model.dirty(), true);
}

void QmlObjectListModelTest::_appendObjectWithoutDirtySignal()
{
    TestQmlObjectListModel model;
    QObject objectWithoutDirtySignal;

    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);
    QVERIFY(modelDirtySpy.isValid());

    model.append(&objectWithoutDirtySignal);
    QCOMPARE(model.count(), 1);

    model.setDirty(false);
    modelDirtySpy.clear();

    QObject* removed = model.removeAt(0);
    QCOMPARE(removed, &objectWithoutDirtySignal);

    QCOMPARE(modelDirtySpy.count(), 1);
    QCOMPARE(modelDirtySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(model.dirty(), true);
}

UT_REGISTER_TEST(QmlObjectListModelTest, TestLabel::Unit)
