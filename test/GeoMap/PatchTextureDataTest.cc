#include "PatchTextureDataTest.h"

#include <QtGui/QColor>
#include <QtGui/QImage>

#include "PatchTextureData.h"

void PatchTextureDataTest::_sizeFormatAndByteCount()
{
    PatchTextureData texture;
    QImage image(8, 4, QImage::Format_RGB32);
    image.fill(QColor(10, 20, 30));
    texture.setImage(image);

    QCOMPARE(texture.size(), QSize(8, 4));
    QCOMPARE(texture.format(), QQuick3DTextureData::RGBA8);
    QCOMPARE(texture.textureData().size(), 8 * 4 * 4);

    texture.setImage(QImage());
    QCOMPARE(texture.size(), QSize());
    QCOMPARE(texture.textureData().size(), 0);
}

void PatchTextureDataTest::_transparencyTracksSourceImage()
{
    PatchTextureData texture;

    // Opaque map tiles (JPEG decodes without alpha) must not be flagged
    // transparent even though the upload format is RGBA8
    QImage opaque(4, 4, QImage::Format_RGB32);
    opaque.fill(QColor(10, 20, 30));
    texture.setImage(opaque);
    QVERIFY(!texture.hasTransparency());

    QImage translucent(4, 4, QImage::Format_ARGB32);
    translucent.fill(QColor(10, 20, 30, 128));
    texture.setImage(translucent);
    QVERIFY(texture.hasTransparency());
}

UT_REGISTER_TEST(PatchTextureDataTest, TestLabel::Unit)
