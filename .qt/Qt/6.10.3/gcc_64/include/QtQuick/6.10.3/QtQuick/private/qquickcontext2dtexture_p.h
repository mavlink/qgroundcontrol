// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCONTEXT2DTEXTURE_P_H
#define QQUICKCONTEXT2DTEXTURE_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_canvas);

#include <QtQuick/qsgtexture.h>
#include "qquickcanvasitem_p.h"
#include "qquickcontext2d_p.h"
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>

QT_BEGIN_NAMESPACE

class QQuickContext2DTile;
class QQuickContext2DCommandBuffer;

class QQuickContext2DTexture : public QObject
{
    Q_OBJECT
public:
    class PaintEvent : public QEvent {
    public:
        PaintEvent(QQuickContext2DCommandBuffer *b) : QEvent(QEvent::Type(QEvent::User + 1)), buffer(b) {}
        QQuickContext2DCommandBuffer *buffer;
    };

    class CanvasChangeEvent : public QEvent {
    public:
        CanvasChangeEvent(const QSize &cSize,
                          const QSize &tSize,
                          const QRect &cWindow,
                          const QRect &dRect,
                          bool sm,
                          bool aa)
            : QEvent(QEvent::Type(QEvent::User + 2))
            , canvasSize(cSize)
            , tileSize(tSize)
            , canvasWindow(cWindow)
            , dirtyRect(dRect)
            , smooth(sm)
            , antialiasing(aa)
        {
        }
        QSize canvasSize;
        QSize tileSize;
        QRect canvasWindow;
        QRect dirtyRect;
        bool smooth;
        bool antialiasing;
    };

    QQuickContext2DTexture();
    ~QQuickContext2DTexture();

    virtual QQuickCanvasItem::RenderTarget renderTarget() const = 0;
    static QRect tiledRect(const QRectF& window, const QSize& tileSize);

    bool setCanvasSize(const QSize &size);
    bool setTileSize(const QSize &size);
    bool setCanvasWindow(const QRect& canvasWindow);
    void setSmooth(bool smooth);
    void setAntialiasing(bool antialiasing);
    bool setDirtyRect(const QRect &dirtyRect);
    bool canvasDestroyed();
    void setOnCustomThread(bool is) { m_onCustomThread = is; }
    bool isOnCustomThread() const { return m_onCustomThread; }

    // Called during sync() on the scene graph thread while GUI is blocked.
    virtual QSGTexture *textureForNextFrame(QSGTexture *lastFrame, QQuickWindow *window) = 0;
    bool event(QEvent *e) override;

Q_SIGNALS:
    void textureChanged();

public Q_SLOTS:
    void canvasChanged(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth, bool antialiasing);
    void paint(QQuickContext2DCommandBuffer *ccb);
    void markDirtyTexture();
    void setItem(QQuickCanvasItem* item);
    virtual void grabImage(const QRectF& region = QRectF()) = 0;

protected:
    virtual QVector2D scaleFactor() const { return QVector2D(1, 1); }

    void paintWithoutTiles(QQuickContext2DCommandBuffer *ccb);
    virtual QPaintDevice* beginPainting() {m_painting = true; return nullptr; }
    virtual void endPainting() {m_painting = false;}
    virtual QQuickContext2DTile* createTile() const = 0;
    virtual void compositeTile(QQuickContext2DTile* tile) = 0;

    void clearTiles();
    virtual QSize adjustedTileSize(const QSize &ts);
    QRect createTiles(const QRect& window);

    QList<QQuickContext2DTile*> m_tiles;
    QQuickContext2D *m_context;
    QSurface *m_surface;

    QQuickContext2D::State m_state;

    QQuickCanvasItem* m_item;
    QSize m_canvasSize;
    QSize m_tileSize;
    QRect m_canvasWindow;
    qreal m_canvasDevicePixelRatio;

    QMutex m_mutex;
    QWaitCondition m_condition;

    uint m_canvasWindowChanged : 1;
    uint m_dirtyTexture : 1;
    uint m_smooth : 1;
    uint m_antialiasing : 1;
    uint m_tiledCanvas : 1;
    uint m_painting : 1;
    uint m_onCustomThread : 1; // Not GUI and not SGRender
};

class QSGPlainTexture;
class QQuickContext2DImageTexture : public QQuickContext2DTexture
{
    Q_OBJECT

public:
    QQuickContext2DImageTexture();
    ~QQuickContext2DImageTexture();

    QQuickCanvasItem::RenderTarget renderTarget() const override;

    QQuickContext2DTile* createTile() const override;
    QPaintDevice* beginPainting() override;
    void endPainting() override;
    void compositeTile(QQuickContext2DTile* tile) override;

    QSGTexture *textureForNextFrame(QSGTexture *lastFrame, QQuickWindow *window) override;

public Q_SLOTS:
    void grabImage(const QRectF& region = QRectF()) override;

private:
    QImage m_image;
    QImage m_displayImage;
    QPainter m_painter;
};

QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DTEXTURE_P_H
