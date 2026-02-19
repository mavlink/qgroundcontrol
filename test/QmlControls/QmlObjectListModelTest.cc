#include "QmlObjectListModelTest.h"

#include <QtTest/QSignalSpy>

namespace {

class DirtyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

public:
    explicit DirtyObject(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool dirty() const
    {
        return _dirty;
    }

    void setDirty(bool dirty)
    {
        if (_dirty == dirty) {
            return;
        }

        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }

signals:
    void dirtyChanged(bool dirty);

private:
    bool _dirty = false;
};

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
    DirtyObject object;

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
    DirtyObject object;

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

    DirtyObject firstObject;
    DirtyObject secondObject;

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

#include "QmlObjectListModelTest.moc"
