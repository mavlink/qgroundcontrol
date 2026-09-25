#include <memory>

#include <QtCore/QObject>
#include <QtTest/QTest>

#include "GPSRevision.h"
#include "UnitTest.h"

namespace {

struct Owner : QObject
{
    GPSRevision revision;
};

}  // namespace

class GPSRevisionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _newerOperationSupersedes();
    void _ownerDeletionEndsOperation();
};

void GPSRevisionTest::_newerOperationSupersedes()
{
    Owner owner;
    const auto first = owner.revision.current(&owner);
    QVERIFY(first.isCurrent());
    QVERIFY(owner.revision.current(&owner).isCurrent());
    QVERIFY(first.isCurrent());

    const auto second = owner.revision.advance(&owner);
    QVERIFY(!first.isCurrent());
    QVERIFY(second.isCurrent());
    QCOMPARE(second.value(), first.value() + 1);

    owner.revision.invalidate();
    QVERIFY(!second.isCurrent());
    QCOMPARE(owner.revision.value(), second.value() + 1);
}

void GPSRevisionTest::_ownerDeletionEndsOperation()
{
    auto owner = std::make_unique<Owner>();
    const auto operation = owner->revision.advance(owner.get());
    const auto copy = operation;
    QVERIFY(copy.isCurrent());
    owner.reset();
    QVERIFY(!operation.isCurrent());
    QVERIFY(!copy.isCurrent());
}

UT_REGISTER_TEST(GPSRevisionTest, TestLabel::Unit)

#include "GPSRevisionTest.moc"
