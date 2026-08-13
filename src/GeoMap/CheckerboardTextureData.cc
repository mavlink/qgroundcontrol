#include "CheckerboardTextureData.h"

#include <QtCore/QSize>

#include <algorithm>

CheckerboardTextureData::CheckerboardTextureData(QQuick3DObject* parent) : QQuick3DTextureData(parent)
{
    _rebuild();
}

void CheckerboardTextureData::setSizePixels(int sizePixels)
{
    const int clamped = std::clamp(sizePixels, 2, 2048);
    if (clamped == _sizePixels) {
        return;
    }
    _sizePixels = clamped;
    emit sizePixelsChanged();
    _rebuild();
}

void CheckerboardTextureData::setCells(int cells)
{
    // Fixed range (no cross-property dependence: QML property init order is unspecified);
    // _rebuild() degrades cells > sizePixels to 1-pixel cells
    const int clamped = std::clamp(cells, 1, 2048);
    if (clamped == _cells) {
        return;
    }
    _cells = clamped;
    emit cellsChanged();
    _rebuild();
}

void CheckerboardTextureData::setColor1(const QColor& color)
{
    if (color == _color1) {
        return;
    }
    _color1 = color;
    emit color1Changed();
    _rebuild();
}

void CheckerboardTextureData::setColor2(const QColor& color)
{
    if (color == _color2) {
        return;
    }
    _color2 = color;
    emit color2Changed();
    _rebuild();
}

void CheckerboardTextureData::_rebuild()
{
    const int size = _sizePixels;
    const int cellPixels = std::max(1, size / _cells);

    QByteArray data(qsizetype(size) * size * 4, Qt::Uninitialized);
    uchar* p = reinterpret_cast<uchar*>(data.data());
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const bool odd = (((x / cellPixels) + (y / cellPixels)) % 2) != 0;
            const QColor& color = odd ? _color2 : _color1;
            *p++ = static_cast<uchar>(color.red());
            *p++ = static_cast<uchar>(color.green());
            *p++ = static_cast<uchar>(color.blue());
            *p++ = 255;
        }
    }

    setSize(QSize(size, size));
    setFormat(QQuick3DTextureData::RGBA8);
    setHasTransparency(false);
    setTextureData(data);
}
