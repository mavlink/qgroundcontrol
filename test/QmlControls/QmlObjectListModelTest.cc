#include "QmlObjectListModelTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "TestDirtyObject.h"

namespace {

class TestQmlObjectListModel : public QmlObjectListModel
{
public:
    explicit TestQmlObjectListModel(QObject* parent = nullptr) : QmlObjectListModel(parent) {}

    void setSkipDirtyFirstItemForTest(bool skip) { _skipDirtyFirstItem = skip; }

    bool insertEmptyRows(int position, int rows) { return insertRows(position, rows); }

    bool removeRowsForTest(int position, int rows) { return removeRows(position, rows); }

    bool setObject(const QModelIndex& index, QObject* object)
    {
        return setData(index, QVariant::fromValue(object), ObjectRole);
    }

    static constexpr int textRole() { return TextRole; }

    QString textAt(int row) const { return data(index(row, 0), TextRole).toString(); }
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

    QCOMPARE(modelDirtySpy.size(), 1);
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
    QCOMPARE(modelDirtySpy.size(), 0);
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
    QCOMPARE(modelDirtySpy.size(), 0);
    QCOMPARE(model.dirty(), false);

    secondObject.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 1);
    QCOMPARE(modelDirtySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(model.dirty(), true);

    model.setDirty(false);
    modelDirtySpy.clear();
    QVERIFY(model.insertEmptyRows(0, 1));
    model.setDirty(false);
    modelDirtySpy.clear();
    firstObject.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 1);

    model.setDirty(false);
    modelDirtySpy.clear();
    QCOMPARE(model.removeAt(0), nullptr);
    model.setDirty(false);
    modelDirtySpy.clear();
    firstObject.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 0);
}

void QmlObjectListModelTest::_duplicateObjectDirtyPropagation()
{
    TestQmlObjectListModel model;
    TestDirtyObject duplicate;
    TestDirtyObject replacement;
    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);

    model.append({&duplicate, &duplicate});
    model.setDirty(false);
    modelDirtySpy.clear();
    duplicate.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 1);

    model.setDirty(false);
    modelDirtySpy.clear();
    QCOMPARE(model.removeAt(0), static_cast<QObject*>(&duplicate));
    model.setDirty(false);
    modelDirtySpy.clear();
    duplicate.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 1);

    model.setDirty(false);
    model.append(&duplicate);
    QVERIFY(model.setObject(model.index(0, 0), &replacement));
    model.setDirty(false);
    modelDirtySpy.clear();
    duplicate.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 1);

    model.clear();
}

void QmlObjectListModelTest::_objectNameChangesNotifyTextRole()
{
    TestQmlObjectListModel model;
    QObject duplicate;
    QObject other;
    model.append({&duplicate, &other, &duplicate});
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    duplicate.setObjectName(QStringLiteral("renamed"));
    QCOMPARE(dataChangedSpy.size(), 2);
    QCOMPARE(dataChangedSpy.at(0).at(0).toModelIndex().row(), 0);
    QCOMPARE(dataChangedSpy.at(1).at(0).toModelIndex().row(), 2);
    for (const QList<QVariant>& arguments : dataChangedSpy) {
        QCOMPARE(arguments.at(2).value<QList<int>>(), QList<int>{TestQmlObjectListModel::textRole()});
    }
    QCOMPARE(model.textAt(0), QStringLiteral("renamed"));

    model.move(2, 1);
    dataChangedSpy.clear();
    duplicate.setObjectName(QStringLiteral("renamed after move"));
    QCOMPARE(dataChangedSpy.size(), 2);
    QCOMPARE(dataChangedSpy.at(0).at(0).toModelIndex().row(), 0);
    QCOMPARE(dataChangedSpy.at(1).at(0).toModelIndex().row(), 1);

    QCOMPARE(model.removeAt(0), static_cast<QObject*>(&duplicate));
    dataChangedSpy.clear();
    duplicate.setObjectName(QStringLiteral("renamed after remove"));
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(dataChangedSpy.at(0).at(0).toModelIndex().row(), 0);

    dataChangedSpy.clear();
    QObject inserted;
    const QMetaObject::Connection connection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                [&duplicate]() { duplicate.setObjectName(QStringLiteral("renamed during insert")); });
    model.append(&inserted);
    (void) disconnect(connection);
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(model.textAt(0), QStringLiteral("renamed during insert"));

    model.clear();
    dataChangedSpy.clear();
    duplicate.setObjectName(QStringLiteral("detached"));
    QCOMPARE(dataChangedSpy.size(), 0);
}

void QmlObjectListModelTest::_insertLifetimeDuringNotification()
{
    TestQmlObjectListModel model;

    auto* const deletedSingleObject = new QObject;
    const QMetaObject::Connection singleConnection = connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                                                             [deletedSingleObject]() { delete deletedSingleObject; });
    model.append(deletedSingleObject);
    (void) disconnect(singleConnection);
    QCOMPARE(model.count(), 0);

    auto* const survivingObject = new QObject;
    auto* const deletedBatchObject = new QObject;
    const QMetaObject::Connection batchConnection = connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                                                            [deletedBatchObject]() { delete deletedBatchObject; });
    model.append({survivingObject, deletedBatchObject});
    (void) disconnect(batchConnection);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.get(0), survivingObject);

    model.clear();
    delete survivingObject;
}

void QmlObjectListModelTest::_reentrantSwapRejected()
{
    TestQmlObjectListModel model;
    QObject original;
    QObject inserted;
    QObject replacement;
    model.append(&original);

    QObjectList displacedObjects;
    const QMetaObject::Connection connection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model, [&]() {
            expectLogMessage("API.QmlObjectListModel", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Cannot swap object list during a model change")));
            displacedObjects = model.swapObjectList({&replacement});
            verifyExpectedLogMessage();
        });

    model.append(&inserted);
    (void) disconnect(connection);

    QVERIFY(displacedObjects.isEmpty());
    QCOMPARE(model.objectList(), QObjectList({&original, &inserted}));
}

void QmlObjectListModelTest::_insertNotificationReentrancy()
{
    {
        TestQmlObjectListModel model;
        QObject inserted;
        bool containsInserted = false;
        int insertedIndex = -1;
        const QMetaObject::Connection connection =
            connect(&model, &QAbstractItemModel::rowsInserted, &model,
                    [&model, &inserted, &containsInserted, &insertedIndex]() {
                        containsInserted = model.contains(&inserted);
                        insertedIndex = model.indexOf(&inserted);
                    });

        model.append(&inserted);
        (void) disconnect(connection);
        QVERIFY(containsInserted);
        QCOMPARE(insertedIndex, 0);
    }

    TestQmlObjectListModel model;
    model.setSkipDirtyFirstItemForTest(true);
    TestDirtyObject formerFirst;
    TestDirtyObject insertedFirst;
    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);

    model.append(&formerFirst);
    const QMetaObject::Connection insertionConnection =
        connect(&model, &QAbstractItemModel::rowsInserted, &model,
                [&model, &formerFirst]() { QCOMPARE(model.removeOne(&formerFirst), nullptr); });
    model.insert(0, &insertedFirst);
    (void) disconnect(insertionConnection);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.get(0), static_cast<QObject*>(&insertedFirst));

    model.setDirty(false);
    modelDirtySpy.clear();
    formerFirst.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 0);
    QVERIFY(!model.dirty());
    model.clear();
}

void QmlObjectListModelTest::_firstItemLifetimeDuringNotification()
{
    {
        TestQmlObjectListModel model;
        model.setSkipDirtyFirstItemForTest(true);
        auto* const previousFirst = new TestDirtyObject;
        model.append(previousFirst);

        const QMetaObject::Connection connection = connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                                                           [previousFirst]() { delete previousFirst; });
        QVERIFY(model.insertEmptyRows(0, 1));
        (void) disconnect(connection);

        QCOMPARE(model.count(), 1);
        QVERIFY(!model.get(0));
    }

    {
        TestQmlObjectListModel model;
        model.setSkipDirtyFirstItemForTest(true);
        auto* const previousFirst = new TestDirtyObject;
        TestDirtyObject insertedFirst;
        model.append(previousFirst);

        const QMetaObject::Connection connection = connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                                                           [previousFirst]() { delete previousFirst; });
        model.insert(0, &insertedFirst);
        (void) disconnect(connection);

        QCOMPARE(model.objectList(), QObjectList({&insertedFirst}));
    }

    {
        TestQmlObjectListModel model;
        model.setSkipDirtyFirstItemForTest(true);
        auto* const previousFirst = new TestDirtyObject;
        TestDirtyObject second;
        model.append({previousFirst, &second});

        const QMetaObject::Connection connection = connect(&model, &QAbstractItemModel::rowsAboutToBeMoved, &model,
                                                           [previousFirst]() { delete previousFirst; });
        model.move(0, 1);
        (void) disconnect(connection);

        QCOMPARE(model.objectList(), QObjectList({&second}));
        model.setDirty(false);
        QSignalSpy dirtySpy(&model, &QmlObjectListModel::dirtyChanged);
        second.setDirty(true);
        QCOMPARE(dirtySpy.size(), 0);
    }
}

void QmlObjectListModelTest::_clearDisconnectsDirtyPropagation()
{
    TestQmlObjectListModel model;
    TestDirtyObject clearedObject;
    TestDirtyObject swappedObject;
    TestDirtyObject replacement;
    QSignalSpy dirtySpy(&model, &QmlObjectListModel::dirtyChanged);

    model.append(&clearedObject);
    model.clear();
    model.setDirty(false);
    dirtySpy.clear();
    clearedObject.setDirty(true);
    QCOMPARE(dirtySpy.size(), 0);
    QVERIFY(!model.dirty());

    model.append(&swappedObject);
    (void) model.swapObjectList({&replacement});
    model.setDirty(false);
    dirtySpy.clear();
    swappedObject.setDirty(true);
    QCOMPARE(dirtySpy.size(), 0);
    QVERIFY(!model.dirty());

    replacement.setDirty(true);
    QCOMPARE(dirtySpy.size(), 1);
    QVERIFY(model.dirty());
}

void QmlObjectListModelTest::_destroyedObjectsAreRemoved()
{
    {
        TestQmlObjectListModel model;
        auto* const object = new QObject;
        const QMetaObject::Connection connection =
            connect(&model, &QAbstractItemModel::rowsInserted, &model, [object]() { delete object; });
        model.append(object);
        (void) disconnect(connection);
        QCOMPARE(model.count(), 0);
    }

    {
        TestQmlObjectListModel model;
        QVERIFY(model.insertEmptyRows(0, 1));
        auto* const object = new QObject;
        const QMetaObject::Connection connection =
            connect(&model, &QAbstractItemModel::dataChanged, &model, [object]() { delete object; });
        QVERIFY(model.setObject(model.index(0, 0), object));
        (void) disconnect(connection);
        QCOMPARE(model.count(), 0);
    }

    {
        TestQmlObjectListModel model;
        auto* const object = new QObject;
        const QMetaObject::Connection connection =
            connect(&model, &QAbstractItemModel::modelReset, &model, [object]() { delete object; });
        (void) model.swapObjectList({object});
        (void) disconnect(connection);
        QCOMPARE(model.count(), 0);
    }

    {
        TestQmlObjectListModel model;
        model.setSkipDirtyFirstItemForTest(true);
        auto* const firstObject = new QObject;
        TestDirtyObject secondObject;
        QSignalSpy dirtySpy(&model, &QmlObjectListModel::dirtyChanged);
        model.append({firstObject, &secondObject});

        delete firstObject;

        QTRY_COMPARE(model.objectList(), QObjectList({&secondObject}));
        model.setDirty(false);
        dirtySpy.clear();
        secondObject.setDirty(true);
        QCOMPARE(dirtySpy.size(), 0);
    }
}

void QmlObjectListModelTest::_destroyedObjectNotificationReentrancy()
{
    TestQmlObjectListModel model;
    auto* const firstObject = new QObject;
    auto* const secondObject = new QObject;
    model.append({firstObject, secondObject});

    bool deletedSecondObject = false;
    connect(&model, &QmlObjectListModel::countChanged, &model, [&](int count) {
        if ((count == 1) && !deletedSecondObject) {
            deletedSecondObject = true;
            delete secondObject;
        }
    });

    delete firstObject;

    QTRY_COMPARE(model.count(), 0);
    QVERIFY(deletedSecondObject);
}

void QmlObjectListModelTest::_destroyedObjectCleanupIsCoalesced()
{
    TestQmlObjectListModel model;
    QList<QObject*> objects;
    constexpr int objectCount = 128;
    objects.reserve(objectCount);
    for (int i = 0; i < objectCount; ++i) {
        objects.append(new QObject);
    }
    model.append(objects);
    QSignalSpy rowsRemovedSpy(&model, &QAbstractItemModel::rowsRemoved);

    qDeleteAll(objects);

    QCOMPARE(model.count(), objectCount);
    QTRY_COMPARE(model.count(), 0);
    QCOMPARE(rowsRemovedSpy.size(), 1);
}

void QmlObjectListModelTest::_structuralNotificationReentrancy()
{
    TestQmlObjectListModel model;
    model.setSkipDirtyFirstItemForTest(true);
    TestDirtyObject formerFirst;
    TestDirtyObject formerSecond;
    TestDirtyObject insertedFirst;
    QSignalSpy modelDirtySpy(&model, &QmlObjectListModel::dirtyChanged);

    model.append({&formerFirst, &formerSecond});
    QObject* removedObject = nullptr;
    bool setDataAccepted = true;
    bool removeRowsAccepted = true;
    const QMetaObject::Connection connection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                [&model, &insertedFirst, &removedObject, &setDataAccepted, &removeRowsAccepted]() {
                    setDataAccepted = model.setObject(model.index(0, 0), &insertedFirst);
                    removeRowsAccepted = model.removeRowsForTest(0, 1);
                    removedObject = model.removeAt(0);
                });
    model.insert(0, &insertedFirst);
    (void) disconnect(connection);

    QVERIFY(!setDataAccepted);
    QVERIFY(!removeRowsAccepted);
    QCOMPARE(removedObject, nullptr);
    QCOMPARE(model.objectList(), QObjectList({&insertedFirst, &formerSecond}));

    model.setDirty(false);
    modelDirtySpy.clear();
    formerFirst.setDirty(true);
    insertedFirst.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 0);
    formerSecond.setDirty(true);
    QCOMPARE(modelDirtySpy.size(), 1);
}

void QmlObjectListModelTest::_directConnectionModelDeletion()
{
    {
        QPointer<TestQmlObjectListModel> model = new TestQmlObjectListModel;
        QObject object;
        connect(
            model, &QmlObjectListModel::countChanged, this, [model](int) { delete model.data(); },
            Qt::DirectConnection);
        model->append(&object);
        QVERIFY(!model);
    }

    {
        QPointer<TestQmlObjectListModel> model = new TestQmlObjectListModel;
        QVERIFY(model->insertEmptyRows(0, 1));
        QObject object;
        connect(
            model, &QAbstractItemModel::dataChanged, this, [model]() { delete model.data(); }, Qt::DirectConnection);
        QVERIFY(model->setObject(model->index(0, 0), &object));
        QVERIFY(!model);
    }
}

void QmlObjectListModelTest::_moveContract()
{
    TestQmlObjectListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    QObject first;
    QObject second;
    QObject third;
    QObject fourth;
    model.append({&first, &second, &third, &fourth});

    QPersistentModelIndex firstIndex(model.index(0, 0));
    QPersistentModelIndex secondIndex(model.index(1, 0));
    model.setDirty(false);
    QSignalSpy dirtySpy(&model, &QmlObjectListModel::dirtyChanged);

    model.move(0, 3);
    QCOMPARE(model.objectList(), QObjectList({&second, &third, &fourth, &first}));
    QCOMPARE(firstIndex.row(), 3);
    QCOMPARE(secondIndex.row(), 0);
    QCOMPARE(dirtySpy.size(), 1);

    model.move(3, 0);
    QCOMPARE(model.objectList(), QObjectList({&first, &second, &third, &fourth}));
    QCOMPARE(firstIndex.row(), 0);
    QCOMPARE(secondIndex.row(), 1);

    model.move(1, 2);
    QCOMPARE(model.objectList(), QObjectList({&first, &third, &second, &fourth}));
    QCOMPARE(secondIndex.row(), 2);

    model.move(2, 1);
    QCOMPARE(model.objectList(), QObjectList({&first, &second, &third, &fourth}));
    QCOMPARE(secondIndex.row(), 1);

    model.move(0, 2);
    QCOMPARE(model.objectList(), QObjectList({&second, &third, &first, &fourth}));
    QCOMPARE(firstIndex.row(), 2);

    QSignalSpy rowsMovedSpy(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy modelResetSpy(&model, &QAbstractItemModel::modelReset);
    model.beginResetModel();
    model.move(0, 1);
    model.endResetModel();
    QCOMPARE(rowsMovedSpy.size(), 0);
    QCOMPARE(modelResetSpy.size(), 1);
    QCOMPARE(model.objectList(), QObjectList({&third, &second, &first, &fourth}));
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

    QCOMPARE(modelDirtySpy.size(), 1);
    QCOMPARE(modelDirtySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(model.dirty(), true);
}

void QmlObjectListModelTest::_modelContract()
{
    TestQmlObjectListModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    QSignalSpy countSpy(&model, &QmlObjectListModel::countChanged);
    QObject first;
    QObject second;

    model.append(&first);
    model.append(&second);
    QCOMPARE(model.count(), 2);

    const QModelIndex firstIndex = model.index(0, 0);
    QVERIFY(firstIndex.isValid());
    QCOMPARE(static_cast<QAbstractItemModel*>(&model)->rowCount(firstIndex), 0);

    QCOMPARE(model.removeOne(&first), &first);
    QCOMPARE(model.removeOne(&second), &second);
    QCOMPARE(model.count(), 0);

    QVERIFY(model.insertEmptyRows(0, 2));
    QCOMPARE(model.count(), 2);
    QVERIFY(!model.get(0));
    model.setDirty(false);

    auto* const replacement = new QObject(&model);
    replacement->setObjectName(QStringLiteral("replacement"));
    QVERIFY(model.setObject(model.index(0, 0), replacement));
    QCOMPARE(model.get(0), replacement);
    QCOMPARE(model.index(0, 0).data(TestQmlObjectListModel::textRole()).toString(), replacement->objectName());

    countSpy.clear();
    model.clearAndDeleteContents();
    QCOMPARE(model.count(), 0);
    QCOMPARE(countSpy.size(), 1);

    auto* const removedDuringNotification = new QObject;
    model.append(removedDuringNotification);
    const QMetaObject::Connection removeConnection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeRemoved, &model,
                [removedDuringNotification]() { delete removedDuringNotification; });
    QCOMPARE(model.removeAt(0), nullptr);
    (void) disconnect(removeConnection);
    QCOMPARE(model.count(), 0);

    auto* const clearedDuringNotification = new QObject;
    model.append(clearedDuringNotification);
    const QMetaObject::Connection clearConnection =
        connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model,
                [clearedDuringNotification]() { delete clearedDuringNotification; });
    countSpy.clear();
    model.clearAndDeleteContents();
    (void) disconnect(clearConnection);
    QCOMPARE(model.count(), 0);
    QCOMPARE(countSpy.size(), 1);

    auto* const swappedDuringNotification = new QObject;
    auto* const replacementAfterSwap = new QObject(&model);
    model.append(swappedDuringNotification);
    const QMetaObject::Connection swapConnection =
        connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model,
                [swappedDuringNotification]() { delete swappedDuringNotification; });
    countSpy.clear();
    const QObjectList oldObjects = model.swapObjectList({replacementAfterSwap});
    (void) disconnect(swapConnection);
    QCOMPARE(oldObjects.size(), 1);
    QVERIFY(!oldObjects.constFirst());
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.get(0), replacementAfterSwap);
    QCOMPARE(countSpy.size(), 1);
    model.clearAndDeleteContents();
}

void QmlObjectListModelTest::_lowerBoundIndex()
{
    QmlObjectListModel model;
    QObject alpha;
    QObject charlie;
    QObject bravo;
    alpha.setObjectName(QStringLiteral("alpha"));
    bravo.setObjectName(QStringLiteral("bravo"));
    charlie.setObjectName(QStringLiteral("charlie"));
    model.append({&alpha, &charlie});

    const auto compare = [](const QObject* left, const QObject* right) {
        return left && right && (left->objectName() < right->objectName());
    };
    QCOMPARE(model.lowerBoundIndex(&bravo, compare), 1);

    model.insert(model.lowerBoundIndex(&bravo, compare), &bravo);
    QCOMPARE(model.objectList(), QObjectList({&alpha, &bravo, &charlie}));
}

UT_REGISTER_TEST(QmlObjectListModelTest, TestLabel::Unit)
