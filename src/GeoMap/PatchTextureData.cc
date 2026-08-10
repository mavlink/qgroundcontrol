/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PatchTextureData.h"

PatchTextureData::PatchTextureData(QQuick3DObject* parent) : QQuick3DTextureData(parent) {}

void PatchTextureData::setImage(const QImage& image)
{
    if (image == _image) {
        return;
    }
    _image = image;
    emit imageChanged();
    _rebuild();
}

void PatchTextureData::_rebuild()
{
    if (_image.isNull()) {
        setSize(QSize());
        setTextureData(QByteArray());
        return;
    }

    const QImage converted = _image.convertToFormat(QImage::Format_RGBA8888);
    setSize(converted.size());
    setFormat(QQuick3DTextureData::RGBA8);
    // The conversion always yields an alpha channel; only the source knows opacity
    setHasTransparency(_image.hasAlphaChannel());
    // Deep copy: the converted image's buffer dies with this scope
    setTextureData(QByteArray(reinterpret_cast<const char*>(converted.constBits()), converted.sizeInBytes()));
}
