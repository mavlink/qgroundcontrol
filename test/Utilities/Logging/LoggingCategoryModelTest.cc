#include "LoggingCategoryModelTest.h"
#include <QtTest/QSignalSpy>

#include "LoggingCategoryModel.h"


// ---------------------------------------------------------------------------
// QGCLoggingCategoryItem
// ---------------------------------------------------------------------------

void LoggingCategoryModelTest::_testItemConstructionDefaultsToWarning()
{
    // Items constructed with an explicit QtWarningMsg level must also report
    // enabled() == false, since enabled() is defined as logLevel <= QtDebugMsg.
    QGCLoggingCategoryItem item(QStringLiteral("Cat"), QStringLiteral("Test.Cat"),
                                QtWarningMsg);

    QCOMPARE(item.shortCategory, QStringLiteral("Cat"));
    QCOMPARE(item.fullCategory,  QStringLiteral("Test.Cat"));
    QCOMPARE(item.logLevel(),    static_cast<int>(QtWarningMsg));
    QVERIFY(!item.enabled());
}

void LoggingCategoryModelTest::_testItemEnabledReflectsLogLevel()
{
    // enabled() is true iff logLevel <= QtDebugMsg. Check the boundary.
    QGCLoggingCategoryItem debugItem(QStringLiteral("A"), QStringLiteral("A"), QtDebugMsg);
    QGCLoggingCategoryItem infoItem (QStringLiteral("B"), QStringLiteral("B"), QtInfoMsg);
    QGCLoggingCategoryItem warnItem (QStringLiteral("C"), QStringLiteral("C"), QtWarningMsg);
    QGCLoggingCategoryItem critItem (QStringLiteral("D"), QStringLiteral("D"), QtCriticalMsg);

    QVERIFY( debugItem.enabled());
    QVERIFY(!infoItem.enabled());
    QVERIFY(!warnItem.enabled());
    QVERIFY(!critItem.enabled());
}

void LoggingCategoryModelTest::_testSetLogLevelFromManagerEmitsOnTransition()
{
    QGCLoggingCategoryItem item(QStringLiteral("X"), QStringLiteral("UT.X"), QtWarningMsg);

    QSignalSpy levelSpy  (&item, &QGCLoggingCategoryItem::logLevelChanged);
    QSignalSpy enableSpy(&item, &QGCLoggingCategoryItem::enabledChanged);

    // Flipping from Warning (disabled) to Debug (enabled) must fire both signals.
    item.setLogLevelFromManager(QtDebugMsg);
    QCOMPARE(item.logLevel(),   static_cast<int>(QtDebugMsg));
    QVERIFY(item.enabled());
    QCOMPARE(levelSpy.count(),  1);
    QCOMPARE(enableSpy.count(), 1);

    // Going Debug → Info changes the level but keeps enabled==false; so
    // logLevelChanged must fire but enabledChanged must not.
    item.setLogLevelFromManager(QtInfoMsg);
    QCOMPARE(levelSpy.count(),  2);
    QCOMPARE(enableSpy.count(), 2); // Info is NOT <= QtDebugMsg — transition fires.

    // Going Info → Warning — still not-enabled; enabledChanged must not fire.
    item.setLogLevelFromManager(QtWarningMsg);
    QCOMPARE(levelSpy.count(),  3);
    QCOMPARE(enableSpy.count(), 2); // stays disabled through Info→Warning.
}

void LoggingCategoryModelTest::_testSetLogLevelFromManagerNoOp()
{
    QGCLoggingCategoryItem item(QStringLiteral("Y"), QStringLiteral("UT.Y"), QtInfoMsg);

    QSignalSpy levelSpy(&item, &QGCLoggingCategoryItem::logLevelChanged);

    // Same-level call must be a pure no-op — no signal, no state churn.
    item.setLogLevelFromManager(QtInfoMsg);
    QCOMPARE(levelSpy.count(), 0);
    QCOMPARE(item.logLevel(),  static_cast<int>(QtInfoMsg));
}

// ---------------------------------------------------------------------------
// LoggingCategoryFlatModel
// ---------------------------------------------------------------------------

void LoggingCategoryModelTest::_testFlatModelEmptyState()
{
    LoggingCategoryFlatModel model;
    QCOMPARE(model.rowCount(),       0);
    QCOMPARE(model.count(),          0);
    QVERIFY(!model.findByFullName(QStringLiteral("nope")));
    // data() on an empty model with any index must not crash and returns null.
    QVERIFY(!model.data(model.index(0), Qt::DisplayRole).isValid());
}

void LoggingCategoryModelTest::_testFlatModelInsertSortedAlphabetical()
{
    // Feed items deliberately out of order; the model must insert sorted by
    // fullCategory so that QML ListView consumers render in alpha order.
    LoggingCategoryFlatModel model;
    auto* c = new QGCLoggingCategoryItem(QStringLiteral("C"), QStringLiteral("Zeta"),   QtWarningMsg, &model);
    auto* a = new QGCLoggingCategoryItem(QStringLiteral("A"), QStringLiteral("Alpha"),  QtWarningMsg, &model);
    auto* b = new QGCLoggingCategoryItem(QStringLiteral("B"), QStringLiteral("Middle"), QtWarningMsg, &model);

    QSignalSpy rowsInsertedSpy(&model, &QAbstractItemModel::rowsInserted);

    model.insertSorted(c);
    model.insertSorted(a);
    model.insertSorted(b);

    QCOMPARE(rowsInsertedSpy.count(), 3);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.at(0)->fullCategory, QStringLiteral("Alpha"));
    QCOMPARE(model.at(1)->fullCategory, QStringLiteral("Middle"));
    QCOMPARE(model.at(2)->fullCategory, QStringLiteral("Zeta"));
}

void LoggingCategoryModelTest::_testFlatModelDataRoles()
{
    LoggingCategoryFlatModel model;
    auto* item = new QGCLoggingCategoryItem(QStringLiteral("Short"),
                                            QStringLiteral("Full.Name"),
                                            QtDebugMsg, &model);
    model.insertSorted(item);

    using Roles = LoggingCategoryFlatModel::Roles;
    const QModelIndex idx = model.index(0);

    QCOMPARE(model.data(idx, Qt::DisplayRole).toString(),
             QStringLiteral("Full.Name"));
    QCOMPARE(model.data(idx, static_cast<int>(Roles::FullNameRole)).toString(),
             QStringLiteral("Full.Name"));
    QCOMPARE(model.data(idx, static_cast<int>(Roles::ShortNameRole)).toString(),
             QStringLiteral("Short"));
    QCOMPARE(model.data(idx, static_cast<int>(Roles::EnabledRole)).toBool(),  true);
    QCOMPARE(model.data(idx, static_cast<int>(Roles::LogLevelRole)).toInt(),
             static_cast<int>(QtDebugMsg));
    // Unknown role falls through the switch and returns a null QVariant.
    QVERIFY(!model.data(idx, Qt::UserRole + 999).isValid());
}

void LoggingCategoryModelTest::_testFlatModelInvalidIndexReturnsNull()
{
    LoggingCategoryFlatModel model;
    model.insertSorted(new QGCLoggingCategoryItem(
        QStringLiteral("S"), QStringLiteral("F"), QtDebugMsg, &model));

    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(5), Qt::DisplayRole).isValid());
}

void LoggingCategoryModelTest::_testFlatModelFindByFullName()
{
    LoggingCategoryFlatModel model;
    auto* hit = new QGCLoggingCategoryItem(QStringLiteral("S"), QStringLiteral("Want.Me"), QtWarningMsg, &model);
    model.insertSorted(hit);
    model.insertSorted(new QGCLoggingCategoryItem(QStringLiteral("S"), QStringLiteral("Nope"), QtWarningMsg, &model));

    QCOMPARE(model.findByFullName(QStringLiteral("Want.Me")), hit);
    QCOMPARE(model.findByFullName(QStringLiteral("Missing")), nullptr);
}

void LoggingCategoryModelTest::_testFlatModelRoleNames()
{
    // Role-name mapping is consumed by QML; verify the exact QByteArray values.
    LoggingCategoryFlatModel model;
    const QHash<int, QByteArray> names = model.roleNames();
    using Roles = LoggingCategoryFlatModel::Roles;

    QCOMPARE(names.value(Qt::DisplayRole),                     QByteArrayLiteral("display"));
    QCOMPARE(names.value(static_cast<int>(Roles::ShortNameRole)),  QByteArrayLiteral("shortName"));
    QCOMPARE(names.value(static_cast<int>(Roles::FullNameRole)),   QByteArrayLiteral("fullName"));
    QCOMPARE(names.value(static_cast<int>(Roles::EnabledRole)),    QByteArrayLiteral("enabled"));
    QCOMPARE(names.value(static_cast<int>(Roles::LogLevelRole)),   QByteArrayLiteral("logLevel"));
}

void LoggingCategoryModelTest::_testFlatModelFlagsIsEditable()
{
    // Valid rows must advertise ItemIsEditable for the QML delegate to bind a
    // checkbox. Invalid indexes must not.
    LoggingCategoryFlatModel model;
    model.insertSorted(new QGCLoggingCategoryItem(
        QStringLiteral("S"), QStringLiteral("F"), QtWarningMsg, &model));

    QVERIFY(model.flags(model.index(0))  & Qt::ItemIsEditable);
    QVERIFY(!(model.flags(QModelIndex()) & Qt::ItemIsEditable));
}

// ---------------------------------------------------------------------------
// LoggingCategoryTreeModel
// ---------------------------------------------------------------------------

void LoggingCategoryModelTest::_testTreeModelEmptyState()
{
    LoggingCategoryTreeModel model;
    QCOMPARE(model.rowCount(),    0);
    QCOMPARE(model.columnCount(), 1);
    QVERIFY(!model.hasChildren());
    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
}

void LoggingCategoryModelTest::_testTreeModelInsertSingleLevel()
{
    LoggingCategoryTreeModel model;

    QSignalSpy rowsInsertedSpy(&model, &QAbstractItemModel::rowsInserted);

    auto* item = new QGCLoggingCategoryItem(QStringLiteral("Leaf"),
                                            QStringLiteral("Leaf"), QtWarningMsg, &model);
    model.insertCategory(QStringList{QStringLiteral("Leaf")}, QStringLiteral("Leaf"), item);

    QCOMPARE(rowsInsertedSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.hasChildren());

    const QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());
    QCOMPARE(model.data(idx, Qt::DisplayRole).toString(), QStringLiteral("Leaf"));
    // Leaf nodes have no children.
    QVERIFY(!model.hasChildren(idx));
}

void LoggingCategoryModelTest::_testTreeModelInsertNestedCreatesIntermediate()
{
    // A path like ["Foo","Bar","Baz"] should produce a 3-level tree where Foo
    // and Bar are auto-created group nodes.
    LoggingCategoryTreeModel model;
    auto* leaf = new QGCLoggingCategoryItem(QStringLiteral("Baz"),
                                            QStringLiteral("Foo.Bar.Baz"), QtWarningMsg, &model);
    model.insertCategory(QStringList{QStringLiteral("Foo"), QStringLiteral("Bar"), QStringLiteral("Baz")},
                         QStringLiteral("Foo.Bar.Baz"), leaf);

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex fooIdx = model.index(0, 0);
    QCOMPARE(model.data(fooIdx, Qt::DisplayRole).toString(), QStringLiteral("Foo"));
    QVERIFY(model.hasChildren(fooIdx));
    QCOMPARE(model.rowCount(fooIdx), 1);

    const QModelIndex barIdx = model.index(0, 0, fooIdx);
    QCOMPARE(model.data(barIdx, Qt::DisplayRole).toString(), QStringLiteral("Bar"));
    QVERIFY(model.hasChildren(barIdx));

    const QModelIndex bazIdx = model.index(0, 0, barIdx);
    QCOMPARE(model.data(bazIdx, Qt::DisplayRole).toString(), QStringLiteral("Baz"));
    QVERIFY(!model.hasChildren(bazIdx));

    // parent() chain leads back to root via fooIdx.
    QCOMPARE(model.parent(barIdx), fooIdx);
    QCOMPARE(model.parent(fooIdx), QModelIndex());
}

void LoggingCategoryModelTest::_testTreeModelDataRoles()
{
    LoggingCategoryTreeModel model;
    auto* item = new QGCLoggingCategoryItem(QStringLiteral("L"), QStringLiteral("Full.L"),
                                            QtDebugMsg, &model);
    model.insertCategory(QStringList{QStringLiteral("L")}, QStringLiteral("Full.L"), item);

    using Roles = LoggingCategoryTreeModel::Roles;
    const QModelIndex idx = model.index(0, 0);

    QCOMPARE(model.data(idx, Qt::DisplayRole).toString(), QStringLiteral("L"));
    QCOMPARE(model.data(idx, static_cast<int>(Roles::ShortNameRole)).toString(),
             QStringLiteral("L"));
    QCOMPARE(model.data(idx, static_cast<int>(Roles::FullNameRole)).toString(),
             QStringLiteral("Full.L"));
    QCOMPARE(model.data(idx, static_cast<int>(Roles::EnabledRole)).toBool(), true);
    QCOMPARE(model.data(idx, static_cast<int>(Roles::LogLevelRole)).toInt(),
             static_cast<int>(QtDebugMsg));
}

void LoggingCategoryModelTest::_testTreeModelRoleNames()
{
    LoggingCategoryTreeModel model;
    const QHash<int, QByteArray> names = model.roleNames();
    using Roles = LoggingCategoryTreeModel::Roles;

    QCOMPARE(names.value(Qt::DisplayRole),                        QByteArrayLiteral("display"));
    QCOMPARE(names.value(static_cast<int>(Roles::ShortNameRole)), QByteArrayLiteral("shortName"));
    QCOMPARE(names.value(static_cast<int>(Roles::FullNameRole)),  QByteArrayLiteral("fullName"));
    QCOMPARE(names.value(static_cast<int>(Roles::EnabledRole)),   QByteArrayLiteral("enabled"));
    QCOMPARE(names.value(static_cast<int>(Roles::LogLevelRole)),  QByteArrayLiteral("logLevel"));
}

UT_REGISTER_TEST(LoggingCategoryModelTest, TestLabel::Unit, TestLabel::Utilities)
