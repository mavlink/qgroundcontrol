/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtGui/QImage>
#include <QtQuick3D/QQuick3DTextureData>

/// Feeds a QImage (a map tile from SurfacePatchModel's tileImage role) to a
/// Qt Quick 3D Texture. A null image produces an empty texture; consumers
/// switch materials on image validity rather than rendering it.
class PatchTextureData : public QQuick3DTextureData
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)

public:
    explicit PatchTextureData(QQuick3DObject* parent = nullptr);

    QImage image() const { return _image; }

    void setImage(const QImage& image);

signals:
    void imageChanged();

private:
    void _rebuild();

    QImage _image;
};
