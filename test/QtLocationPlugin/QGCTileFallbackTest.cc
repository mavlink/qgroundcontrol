#include "QGCTileFallbackTest.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>

#include "QGCTileFallback.h"

namespace {
const QRgb kRed = qRgb(255, 0, 0);
const QRgb kGreen = qRgb(0, 255, 0);
const QRgb kBlue = qRgb(0, 0, 255);
const QRgb kYellow = qRgb(255, 255, 0);

QImage makeQuadrantAncestor()
{
    QImage img(256, 256, QImage::Format_ARGB32);
    {
        QPainter p(&img);
        p.fillRect(0, 0, 128, 128, QColor(kRed));        // cell (0,0)
        p.fillRect(128, 0, 128, 128, QColor(kGreen));    // cell (1,0)
        p.fillRect(0, 128, 128, 128, QColor(kBlue));     // cell (0,1)
        p.fillRect(128, 128, 128, 128, QColor(kYellow)); // cell (1,1)
    }
    return img;
}

QRgb centerPixel(const QImage &img)
{
    return img.pixel(img.width() / 2, img.height() / 2);
}
} // namespace

void QGCTileFallbackTest::_testScaleLevelDelta1Quadrants()
{
    const QImage ancestor = makeQuadrantAncestor();
    const QSize tileSize(256, 256);

    struct Case { int x; int y; QRgb color; };
    const Case cases[] = {
        {0, 0, kRed},
        {1, 0, kGreen},
        {0, 1, kBlue},
        {1, 1, kYellow},
        {2, 2, kRed},   // x%2,y%2 == (0,0)
        {3, 0, kGreen}, // (1,0)
    };

    for (const Case &c : cases) {
        const QImage out = scaleAncestorToChild(ancestor, c.x, c.y, 1, tileSize);
        QVERIFY(!out.isNull());
        QCOMPARE(out.size(), tileSize);
        QCOMPARE(centerPixel(out), c.color);
    }
}

void QGCTileFallbackTest::_testScaleLevelDelta2SubCell()
{
    // levelDelta=2 => divisions=4, each sub-cell is 64x64 of the 256 ancestor.
    // cell (0,0) lies entirely inside the red top-left quadrant.
    const QImage ancestor = makeQuadrantAncestor();
    const QSize tileSize(256, 256);

    const QImage topLeft = scaleAncestorToChild(ancestor, 0, 0, 2, tileSize);
    QVERIFY(!topLeft.isNull());
    QCOMPARE(topLeft.size(), tileSize);
    QCOMPARE(centerPixel(topLeft), kRed);

    // cell (3,3) lies inside the yellow bottom-right quadrant.
    const QImage bottomRight = scaleAncestorToChild(ancestor, 3, 3, 2, tileSize);
    QVERIFY(!bottomRight.isNull());
    QCOMPARE(bottomRight.size(), tileSize);
    QCOMPARE(centerPixel(bottomRight), kYellow);
}

void QGCTileFallbackTest::_testScaleInvalidInputs()
{
    const QImage ancestor = makeQuadrantAncestor();
    const QSize tileSize(256, 256);

    QVERIFY(scaleAncestorToChild(QImage(), 0, 0, 1, tileSize).isNull());
    QVERIFY(scaleAncestorToChild(ancestor, 0, 0, 0, tileSize).isNull());
    QVERIFY(scaleAncestorToChild(ancestor, 0, 0, -1, tileSize).isNull());
    QVERIFY(scaleAncestorToChild(ancestor, 0, 0, 1, QSize()).isNull());

    // levelDelta so large the sub-square rounds to zero pixels => null.
    QVERIFY(scaleAncestorToChild(ancestor, 0, 0, 20, tileSize).isNull());
}

void QGCTileFallbackTest::_testEncodeFallbackTile()
{
    const QImage ancestor = makeQuadrantAncestor();
    const QByteArray png = encodeFallbackTile(ancestor);
    QVERIFY(!png.isEmpty());
    QCOMPARE(png.left(8), QByteArray::fromHex("89504e470d0a1a0a"));

    QVERIFY(encodeFallbackTile(QImage()).isEmpty());
}

UT_REGISTER_TEST(QGCTileFallbackTest, TestLabel::Unit)
