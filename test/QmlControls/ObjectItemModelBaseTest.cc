#include "ObjectItemModelBaseTest.h"
#include <QtTest/QSignalSpy>

#include "ObjectItemModelBase.h"


// ---------------------------------------------------------------------------
// Concrete subclass — ObjectItemModelBase is abstract, so we need a minimal
// implementation to test the base-class logic.
// ---------------------------------------------------------------------------
namespace {

class ConcreteItemModel : public ObjectItemModelBase
{
    Q_OBJECT

public:
    explicit ConcreteItemModel(QObject* parent = nullptr)
        : ObjectItemModelBase(parent)
    {
    }

    // -- ObjectItemModelBase pure-virtuals --
    int count() const override { return _count; }
    void setDirty(bool dirty) override
    {
        if (_dirty == dirty) return;
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
    void clear() override
    {
        beginResetModel();
        _count = 0;
        endResetModel();
    }

    // -- QAbstractItemModel pure-virtuals --
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        if (row < 0 || row >= _count || column != 0) return {};
        return createIndex(row, 0);
    }
    QModelIndex parent(const QModelIndex& child) const override
    {
        Q_UNUSED(child);
        return {};
    }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return _count;
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return 1;
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        Q_UNUSED(index); Q_UNUSED(role);
        return {};
    }

    // Helper to manipulate count from tests
    void setCount(int c) { _count = c; }

    // Expose protected helpers for testing
    using ObjectItemModelBase::_signalCountChangedIfNotNested;
    using ObjectItemModelBase::_childDirtyChanged;
    using ObjectItemModelBase::roleNames;

private:
    int _count = 0;
};

class DirtyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

public:
    explicit DirtyObject(QObject* parent = nullptr) : QObject(parent) {}

    bool dirty() const { return _dirty; }
    void setDirty(bool dirty)
    {
        if (_dirty == dirty) return;
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }

signals:
    void dirtyChanged(bool dirty);

private:
    bool _dirty = false;
};

} // namespace

// ===========================================================================
// Nested beginResetModel / endResetModel
// ===========================================================================

void ObjectItemModelBaseTest::_singleResetCallsBaseOnce()
{
    ConcreteItemModel model;

    QSignalSpy resetBeginSpy(&model, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetEndSpy(&model, &QAbstractItemModel::modelReset);

    model.beginResetModel();
    QCOMPARE(resetBeginSpy.count(), 1);

    model.endResetModel();
    QCOMPARE(resetEndSpy.count(), 1);
}

void ObjectItemModelBaseTest::_nestedResetTwoDeep()
{
    ConcreteItemModel model;

    QSignalSpy resetBeginSpy(&model, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetEndSpy(&model, &QAbstractItemModel::modelReset);

    model.beginResetModel();
    model.beginResetModel(); // nested — should NOT emit again
    QCOMPARE(resetBeginSpy.count(), 1);

    model.endResetModel(); // back to depth 1 — should NOT emit yet
    QCOMPARE(resetEndSpy.count(), 0);

    model.endResetModel(); // depth 0 — now emit
    QCOMPARE(resetEndSpy.count(), 1);
}

void ObjectItemModelBaseTest::_nestedResetThreeDeep()
{
    ConcreteItemModel model;

    QSignalSpy resetBeginSpy(&model, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetEndSpy(&model, &QAbstractItemModel::modelReset);

    model.beginResetModel();
    model.beginResetModel();
    model.beginResetModel(); // depth 3
    QCOMPARE(resetBeginSpy.count(), 1);

    model.endResetModel(); // depth 2
    model.endResetModel(); // depth 1
    QCOMPARE(resetEndSpy.count(), 0);

    model.endResetModel(); // depth 0 — emit
    QCOMPARE(resetEndSpy.count(), 1);
}

void ObjectItemModelBaseTest::_endResetWithoutBeginLogsWarning()
{
    ConcreteItemModel model;

    QSignalSpy resetEndSpy(&model, &QAbstractItemModel::modelReset);

    // Should just log a warning and not crash
    model.endResetModel();
    QCOMPARE(resetEndSpy.count(), 0);
}

// ===========================================================================
// Count signal suppression during reset
// ===========================================================================

void ObjectItemModelBaseTest::_countChangedSuppressedInsideReset()
{
    ConcreteItemModel model;
    model.setCount(5);

    QSignalSpy countSpy(&model, &ObjectItemModelBase::countChanged);

    model.beginResetModel();

    // _signalCountChangedIfNotNested should be suppressed while in reset
    model._signalCountChangedIfNotNested();
    QCOMPARE(countSpy.count(), 0);

    model.endResetModel();
    // endResetModel itself emits countChanged
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(countSpy.takeFirst().at(0).toInt(), 5);
}

void ObjectItemModelBaseTest::_countChangedEmittedOnEndReset()
{
    ConcreteItemModel model;
    model.setCount(3);

    model.beginResetModel();
    model.setCount(7);

    QSignalSpy countSpy(&model, &ObjectItemModelBase::countChanged);

    model.endResetModel();
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(countSpy.takeFirst().at(0).toInt(), 7);
}

// ===========================================================================
// Dirty tracking via _childDirtyChanged
// ===========================================================================

void ObjectItemModelBaseTest::_childDirtySetsDirtyTrue()
{
    ConcreteItemModel model;
    QCOMPARE(model.dirty(), false);

    model._childDirtyChanged(true);
    QCOMPARE(model.dirty(), true);
}

void ObjectItemModelBaseTest::_childDirtyDoesNotClearDirty()
{
    ConcreteItemModel model;
    model._childDirtyChanged(true);
    QCOMPARE(model.dirty(), true);

    // _childDirtyChanged(false) should NOT reset the model's dirty flag
    // because _dirty |= dirty means it's sticky
    model._childDirtyChanged(false);
    QCOMPARE(model.dirty(), true);
}

void ObjectItemModelBaseTest::_childDirtyEmitsDirtyChanged()
{
    ConcreteItemModel model;

    QSignalSpy dirtySpy(&model, &ObjectItemModelBase::dirtyChanged);

    model._childDirtyChanged(true);
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), true);

    // Calling again with true — dirty is already true, |= true is still true,
    // but the signal is still emitted each time _childDirtyChanged is called
    model._childDirtyChanged(true);
    QCOMPARE(dirtySpy.count(), 1);
}

// ===========================================================================
// roleNames
// ===========================================================================

void ObjectItemModelBaseTest::_roleNamesContainObjectAndText()
{
    ConcreteItemModel model;
    const auto roles = model.roleNames();

    QVERIFY(roles.values().contains("object"));
    QVERIFY(roles.values().contains("text"));
}

UT_REGISTER_TEST(ObjectItemModelBaseTest, TestLabel::Unit)

#include "ObjectItemModelBaseTest.moc"
