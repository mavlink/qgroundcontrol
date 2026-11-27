#include "TileRegionTest.h"

#include "TileRegion.h"

void TileRegionTest::_testDefaultConstruction()
{
    TileRegion set;
    QCOMPARE(set.tileX0, 0);
    QCOMPARE(set.tileX1, 0);
    QCOMPARE(set.tileY0, 0);
    QCOMPARE(set.tileY1, 0);
    QCOMPARE(set.tileCount, static_cast<quint64>(0));
    QCOMPARE(set.tileSize, static_cast<quint64>(0));
}

void TileRegionTest::_testClear()
{
    TileRegion set;
    set.tileX0 = 10;
    set.tileX1 = 20;
    set.tileY0 = 30;
    set.tileY1 = 40;
    set.tileCount = 100;
    set.tileSize = 5000;

    set.clear();

    QCOMPARE(set.tileX0, 0);
    QCOMPARE(set.tileX1, 0);
    QCOMPARE(set.tileY0, 0);
    QCOMPARE(set.tileY1, 0);
    QCOMPARE(set.tileCount, static_cast<quint64>(0));
    QCOMPARE(set.tileSize, static_cast<quint64>(0));
}

void TileRegionTest::_testPlusEqualsAccumulates()
{
    TileRegion a;
    a.tileCount = 10;
    a.tileSize = 1000;

    TileRegion b;
    b.tileCount = 5;
    b.tileSize = 500;

    TileRegion c;
    c.tileCount = 3;
    c.tileSize = 300;

    a += b;
    QCOMPARE(a.tileCount, static_cast<quint64>(15));
    QCOMPARE(a.tileSize, static_cast<quint64>(1500));

    a += c;
    QCOMPARE(a.tileCount, static_cast<quint64>(18));
    QCOMPARE(a.tileSize, static_cast<quint64>(1800));

    a += TileRegion();
    QCOMPARE(a.tileCount, static_cast<quint64>(18));
    QCOMPARE(a.tileSize, static_cast<quint64>(1800));
}

UT_REGISTER_TEST(TileRegionTest, TestLabel::Unit)
