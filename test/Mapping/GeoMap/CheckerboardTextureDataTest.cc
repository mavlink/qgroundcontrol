#include "CheckerboardTextureDataTest.h"

#include "CheckerboardTextureData.h"

namespace {

// RGBA8 pixel at (x, y)
const uchar* pixelAt(const QByteArray& data, int size, int x, int y)
{
    return reinterpret_cast<const uchar*>(data.constData()) + (((y * size) + x) * 4);
}

bool pixelIs(const QByteArray& data, int size, int x, int y, const QColor& color)
{
    const uchar* p = pixelAt(data, size, x, y);
    return (p[0] == color.red()) && (p[1] == color.green()) && (p[2] == color.blue()) && (p[3] == 255);
}

}  // namespace

void CheckerboardTextureDataTest::_sizeFormatAndByteCount()
{
    CheckerboardTextureData texture;
    texture.setSizePixels(64);
    texture.setCells(4);

    QCOMPARE(texture.size(), QSize(64, 64));
    QCOMPARE(texture.format(), QQuick3DTextureData::RGBA8);
    QCOMPARE(texture.textureData().size(), 64 * 64 * 4);
    QVERIFY(!texture.hasTransparency());
}

void CheckerboardTextureDataTest::_checkerPatternAndChannelOrder()
{
    CheckerboardTextureData texture;
    texture.setSizePixels(64);
    texture.setCells(4);  // 16px cells
    const QColor red(0xff, 0x00, 0x00);
    const QColor blue(0x00, 0x00, 0xff);
    texture.setColor1(red);
    texture.setColor2(blue);

    const QByteArray data = texture.textureData();
    // First cell is color1; neighbors right/below are color2; diagonal back to color1
    QVERIFY(pixelIs(data, 64, 0, 0, red));
    QVERIFY(pixelIs(data, 64, 15, 15, red));
    QVERIFY(pixelIs(data, 64, 16, 0, blue));
    QVERIFY(pixelIs(data, 64, 0, 16, blue));
    QVERIFY(pixelIs(data, 64, 16, 16, red));
}

void CheckerboardTextureDataTest::_deterministic()
{
    CheckerboardTextureData first;
    CheckerboardTextureData second;
    QCOMPARE(first.textureData(), second.textureData());

    first.setCells(5);
    second.setCells(5);
    QCOMPARE(first.textureData(), second.textureData());
}

void CheckerboardTextureDataTest::_clamps()
{
    CheckerboardTextureData texture;

    texture.setSizePixels(1);
    QCOMPARE(texture.sizePixels(), 2);
    texture.setSizePixels(1000000);
    QCOMPARE(texture.sizePixels(), 2048);

    // Fixed range independent of sizePixels (QML property init order is unspecified)
    texture.setCells(0);
    QCOMPARE(texture.cells(), 1);
    texture.setCells(1000000);
    QCOMPARE(texture.cells(), 2048);

    // cells > sizePixels degrades gracefully (1px cells), still a valid texture
    texture.setSizePixels(4);
    QCOMPARE(texture.textureData().size(), 4 * 4 * 4);
}

void CheckerboardTextureDataTest::_propertyChangeRegenerates()
{
    CheckerboardTextureData texture;
    const QByteArray before = texture.textureData();
    texture.setColor1(QColor(0x12, 0x34, 0x56));
    QVERIFY(texture.textureData() != before);
}

UT_REGISTER_TEST(CheckerboardTextureDataTest, TestLabel::Unit)
