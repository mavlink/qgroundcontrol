/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtGui/QColor>
#include <QtQuick3D/QQuick3DTextureData>

/// Procedural checkerboard texture for GeoMap debug rendering: deterministic
/// content for visually (and pixel-)verifying patch placement and UV mapping
/// without tile imagery or network access.
class CheckerboardTextureData : public QQuick3DTextureData
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int sizePixels READ sizePixels WRITE setSizePixels NOTIFY sizePixelsChanged)
    Q_PROPERTY(int cells READ cells WRITE setCells NOTIFY cellsChanged)
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY color1Changed)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY color2Changed)

public:
    explicit CheckerboardTextureData(QQuick3DObject* parent = nullptr);

    int sizePixels() const { return _sizePixels; }

    void setSizePixels(int sizePixels);

    int cells() const { return _cells; }

    void setCells(int cells);

    QColor color1() const { return _color1; }

    void setColor1(const QColor& color);

    QColor color2() const { return _color2; }

    void setColor2(const QColor& color);

signals:
    void sizePixelsChanged();
    void cellsChanged();
    void color1Changed();
    void color2Changed();

private:
    void _rebuild();

    int _sizePixels = 256;
    int _cells = 8;
    QColor _color1{0x40, 0x80, 0x40};
    QColor _color2{0xd0, 0xd0, 0xd0};
};
