// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARELAYER_H
#define QSGSOFTWARELAYER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgtexture_p.h>

QT_BEGIN_NAMESPACE

class QSGSoftwarePixmapRenderer;

class QSGSoftwareLayer : public QSGLayer
{
    Q_OBJECT
public:
    QSGSoftwareLayer(QSGRenderContext *renderContext);
    ~QSGSoftwareLayer();

    const QPixmap &pixmap() const { return m_pixmap; }

    // QSGTexture interface
public:
    qint64 comparisonKey() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;

    // QSGDynamicTexture interface
public:
    bool updateTexture() override;

    // QSGLayer interface
public:
    void setItem(QSGNode *item) override;
    void setRect(const QRectF &rect) override;
    void setSize(const QSize &size) override;
    void scheduleUpdate() override;
    QImage toImage() const override;
    void setLive(bool live) override;
    void setRecursive(bool recursive) override;
    void setFormat(Format) override;
    void setHasMipmaps(bool) override;
    void setDevicePixelRatio(qreal ratio) override;
    void setMirrorHorizontal(bool mirror) override;
    void setMirrorVertical(bool mirror) override;
    void setSamples(int) override { }

public Q_SLOTS:
    void markDirtyTexture() override;
    void invalidated() override;

private:
    void grab();

    QSGNode *m_item;
    QSGRenderContext *m_context;
    QSGSoftwarePixmapRenderer *m_renderer;
    QRectF m_rect;
    QSize m_size;
    QPixmap m_pixmap;
    qreal m_device_pixel_ratio;
    bool m_mirrorHorizontal;
    bool m_mirrorVertical;
    bool m_live;
    bool m_grab;
    bool m_recursive;
    bool m_dirtyTexture;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARELAYER_H
