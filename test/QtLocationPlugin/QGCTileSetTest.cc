#include "QGCTileSetTest.h"

#include "QGCTileSet.h"

void QGCTileSetTest::_testDefaultConstruction()
{
    QGCTileSet set;
    QCOMPARE(set.tileX0, 0);
    QCOMPARE(set.tileX1, 0);
    QCOMPARE(set.tileY0, 0);
    QCOMPARE(set.tileY1, 0);
    QCOMPARE(set.tileCount, static_cast<quint64>(0));
    QCOMPARE(set.tileSize, static_cast<quint64>(0));
}

void QGCTileSetTest::_testClear()
{
    QGCTileSet set;
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

void QGCTileSetTest::_testPlusEqualsAccumulates()
{
    QGCTileSet a;
    a.tileCount = 10;
    a.tileSize = 1000;

    QGCTileSet b;
    b.tileCount = 5;
    b.tileSize = 500;

    a += b;

    QCOMPARE(a.tileCount, static_cast<quint64>(15));
    QCOMPARE(a.tileSize, static_cast<quint64>(1500));
}

void QGCTileSetTest::_testPlusEqualsWithZero()
{
    QGCTileSet a;
    a.tileCount = 42;
    a.tileSize = 9999;

    a += QGCTileSet();

    QCOMPARE(a.tileCount, static_cast<quint64>(42));
    QCOMPARE(a.tileSize, static_cast<quint64>(9999));
}

void QGCTileSetTest::_testPlusEqualsChaining()
{
    QGCTileSet a;
    a.tileCount = 1;
    a.tileSize = 100;

    QGCTileSet b;
    b.tileCount = 2;
    b.tileSize = 200;

    QGCTileSet c;
    c.tileCount = 3;
    c.tileSize = 300;

    a += b;
    a += c;

    QCOMPARE(a.tileCount, static_cast<quint64>(6));
    QCOMPARE(a.tileSize, static_cast<quint64>(600));
}

UT_REGISTER_TEST(QGCTileSetTest, TestLabel::Unit)
