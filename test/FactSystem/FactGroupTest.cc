#include "FactGroupTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "FactGroup.h"

/// Testable subclass exposing protected members
class TestableFactGroup : public FactGroup
{
    Q_OBJECT
public:
    explicit TestableFactGroup(QObject *parent = nullptr, bool ignoreCamelCase = false)
        : FactGroup(0 /* immediate updates */, parent, ignoreCamelCase)
    {
    }

    using FactGroup::_addFact;
    using FactGroup::_addFactGroup;
    using FactGroup::_setTelemetryAvailable;
};

void FactGroupTest::_addFactAndLookup_test()
{
    TestableFactGroup group;
    Fact fact(0, "testFact", FactMetaData::valueTypeDouble, &group);

    group._addFact(&fact, QStringLiteral("testFact"));

    QVERIFY(group.factExists(QStringLiteral("testFact")));
    QCOMPARE(group.getFact(QStringLiteral("testFact")), &fact);
}

void FactGroupTest::_factExistsNonExistent_test()
{
    TestableFactGroup group;
    QVERIFY(!group.factExists(QStringLiteral("noSuchFact")));
}

void FactGroupTest::_getFactNonExistent_test()
{
    TestableFactGroup group;
    // Returns nullptr and emits a warning (expected)
    Fact *result = group.getFact(QStringLiteral("noSuchFact"));
    QVERIFY(result == nullptr);
}

void FactGroupTest::_duplicateFact_test()
{
    TestableFactGroup group;
    Fact fact1(0, "dup", FactMetaData::valueTypeDouble, &group);
    Fact fact2(0, "dup", FactMetaData::valueTypeDouble, &group);

    group._addFact(&fact1, QStringLiteral("dup"));
    // Adding duplicate should warn but not crash; original stays
    group._addFact(&fact2, QStringLiteral("dup"));
    QCOMPARE(group.getFact(QStringLiteral("dup")), &fact1);
}

void FactGroupTest::_addFactGroupAndLookup_test()
{
    TestableFactGroup parent;
    TestableFactGroup *child = new TestableFactGroup(&parent);

    parent._addFactGroup(child, QStringLiteral("childGroup"));

    QCOMPARE(parent.getFactGroup(QStringLiteral("childGroup")), child);
}

void FactGroupTest::_getFactGroupNonExistent_test()
{
    TestableFactGroup group;
    FactGroup *result = group.getFactGroup(QStringLiteral("noSuchGroup"));
    QVERIFY(result == nullptr);
}

void FactGroupTest::_duplicateFactGroup_test()
{
    TestableFactGroup parent;
    TestableFactGroup *child1 = new TestableFactGroup(&parent);
    TestableFactGroup *child2 = new TestableFactGroup(&parent);

    parent._addFactGroup(child1, QStringLiteral("dup"));
    parent._addFactGroup(child2, QStringLiteral("dup"));
    QCOMPARE(parent.getFactGroup(QStringLiteral("dup")), child1);
}

void FactGroupTest::_dotNotationFact_test()
{
    TestableFactGroup parent;
    TestableFactGroup *child = new TestableFactGroup(&parent, true /* ignoreCamelCase */);
    Fact fact(0, "speed", FactMetaData::valueTypeDouble, child);

    child->_addFact(&fact, QStringLiteral("speed"));
    parent._addFactGroup(child, QStringLiteral("gps"));

    // Dot notation: "gps.speed" should resolve through child group
    QVERIFY(parent.factExists(QStringLiteral("gps.speed")));
    QCOMPARE(parent.getFact(QStringLiteral("gps.speed")), &fact);
}

void FactGroupTest::_dotNotationFactNotFound_test()
{
    TestableFactGroup parent;
    TestableFactGroup *child = new TestableFactGroup(&parent, true /* ignoreCamelCase */);
    parent._addFactGroup(child, QStringLiteral("gps"));

    QVERIFY(!parent.factExists(QStringLiteral("gps.noSuch")));
    QVERIFY(parent.getFact(QStringLiteral("gps.noSuch")) == nullptr);
}

void FactGroupTest::_dotNotationTooDeep_test()
{
    TestableFactGroup group;
    // More than one dot level is unsupported
    QVERIFY(!group.factExists(QStringLiteral("a.b.c")));
    QVERIFY(group.getFact(QStringLiteral("a.b.c")) == nullptr);
}

void FactGroupTest::_camelCaseConversion_test()
{
    // Default: camelCase enabled (ignoreCamelCase=false)
    TestableFactGroup group;
    Fact fact(0, "myFact", FactMetaData::valueTypeDouble, &group);

    group._addFact(&fact, QStringLiteral("myFact"));

    // "MyFact" should be converted to "myFact" via camelCase
    QVERIFY(group.factExists(QStringLiteral("MyFact")));
    QCOMPARE(group.getFact(QStringLiteral("MyFact")), &fact);

    // "myFact" should also work (already camelCase)
    QVERIFY(group.factExists(QStringLiteral("myFact")));
}

void FactGroupTest::_ignoreCamelCase_test()
{
    TestableFactGroup group(nullptr, true /* ignoreCamelCase */);
    Fact fact(0, "MyFact", FactMetaData::valueTypeDouble, &group);

    group._addFact(&fact, QStringLiteral("MyFact"));

    // With ignoreCamelCase, lookup uses name as-is
    QVERIFY(group.factExists(QStringLiteral("MyFact")));
    // "myFact" should NOT match since camelCase conversion is disabled
    QVERIFY(!group.factExists(QStringLiteral("myFact")));
}

void FactGroupTest::_factNamesSignal_test()
{
    TestableFactGroup group;
    QSignalSpy spy(&group, &FactGroup::factNamesChanged);
    QVERIFY(spy.isValid());

    Fact fact(0, "f1", FactMetaData::valueTypeDouble, &group);
    group._addFact(&fact, QStringLiteral("f1"));
    QCOMPARE(spy.count(), 1);
}

void FactGroupTest::_factGroupNamesSignal_test()
{
    TestableFactGroup parent;
    QSignalSpy spy(&parent, &FactGroup::factGroupNamesChanged);
    QVERIFY(spy.isValid());

    TestableFactGroup *child = new TestableFactGroup(&parent);
    parent._addFactGroup(child, QStringLiteral("sub"));
    QCOMPARE(spy.count(), 1);
}

void FactGroupTest::_telemetryAvailable_test()
{
    TestableFactGroup group;
    QVERIFY(!group.telemetryAvailable());

    QSignalSpy spy(&group, &FactGroup::telemetryAvailableChanged);
    QVERIFY(spy.isValid());

    group._setTelemetryAvailable(true);
    QVERIFY(group.telemetryAvailable());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);

    // Setting same value should not emit again
    group._setTelemetryAvailable(true);
    QCOMPARE(spy.count(), 1);

    group._setTelemetryAvailable(false);
    QVERIFY(!group.telemetryAvailable());
    QCOMPARE(spy.count(), 2);
}

void FactGroupTest::_factNames_test()
{
    TestableFactGroup group;
    Fact f1(0, "alpha", FactMetaData::valueTypeDouble, &group);
    Fact f2(0, "beta", FactMetaData::valueTypeInt32, &group);

    group._addFact(&f1, QStringLiteral("alpha"));
    group._addFact(&f2, QStringLiteral("beta"));

    const QStringList names = group.factNames();
    QCOMPARE(names.count(), 2);
    QVERIFY(names.contains(QStringLiteral("alpha")));
    QVERIFY(names.contains(QStringLiteral("beta")));
}

void FactGroupTest::_factGroupNames_test()
{
    TestableFactGroup parent;
    TestableFactGroup *c1 = new TestableFactGroup(&parent);
    TestableFactGroup *c2 = new TestableFactGroup(&parent);

    parent._addFactGroup(c1, QStringLiteral("sub1"));
    parent._addFactGroup(c2, QStringLiteral("sub2"));

    const QStringList names = parent.factGroupNames();
    QCOMPARE(names.count(), 2);
    QVERIFY(names.contains(QStringLiteral("sub1")));
    QVERIFY(names.contains(QStringLiteral("sub2")));
}

#include "FactGroupTest.moc"

UT_REGISTER_TEST(FactGroupTest, TestLabel::Unit)
