// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMBACKINGSTORE_H
#define QPLATFORMBACKINGSTORE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qrect.h>
#include <QtCore/qobject.h>

#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQpaBackingStore, Q_GUI_EXPORT)

class QRegion;
class QRect;
class QPoint;
class QImage;
class QPlatformBackingStorePrivate;
class QPlatformTextureList;
class QPlatformTextureListPrivate;
class QPlatformGraphicsBuffer;
class QRhi;
class QRhiTexture;
class QRhiResourceUpdateBatch;

struct Q_GUI_EXPORT QPlatformBackingStoreRhiConfig
{
    Q_GADGET
public:
    enum Api {
        OpenGL,
        Metal,
        Vulkan,
        D3D11,
        D3D12,
        Null
    };
    Q_ENUM(Api)

    QPlatformBackingStoreRhiConfig()
        : m_enable(false)
    { }

    QPlatformBackingStoreRhiConfig(Api api)
        : m_enable(true),
          m_api(api)
    { }

    bool isEnabled() const { return m_enable; }
    void setEnabled(bool enable) { m_enable = enable; }

    Api api() const { return m_api; }
    void setApi(Api api) { m_api = api; }

    bool isDebugLayerEnabled() const { return m_debugLayer; }
    void setDebugLayer(bool enable) { m_debugLayer = enable; }

private:
    bool m_enable;
    Api m_api = Null;
    bool m_debugLayer = false;
    friend bool operator==(const QPlatformBackingStoreRhiConfig &a, const QPlatformBackingStoreRhiConfig &b);
};

inline bool operator==(const QPlatformBackingStoreRhiConfig &a, const QPlatformBackingStoreRhiConfig &b)
{
    return a.m_enable == b.m_enable
            && a.m_api == b.m_api
            && a.m_debugLayer == b.m_debugLayer;
}

inline bool operator!=(const QPlatformBackingStoreRhiConfig &a, const QPlatformBackingStoreRhiConfig &b)
{
    return !(a == b);
}

class Q_GUI_EXPORT QPlatformTextureList : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPlatformTextureList)
public:
    enum Flag {
        StacksOnTop = 0x01,
        TextureIsSrgb = 0x02,
        NeedsPremultipliedAlphaBlending = 0x04,
        MirrorVertically = 0x08
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    explicit QPlatformTextureList(QObject *parent = nullptr);
    ~QPlatformTextureList();

    int count() const;
    bool isEmpty() const { return count() == 0; }
    QRhiTexture *texture(int index) const;
    QRhiTexture *textureExtra(int index) const;
    QRect geometry(int index) const;
    QRect clipRect(int index) const;
    void *source(int index);
    Flags flags(int index) const;
    void lock(bool on);
    bool isLocked() const;

    void appendTexture(void *source, QRhiTexture *texture, const QRect &geometry,
                       const QRect &clipRect = QRect(), Flags flags = { });

    void appendTexture(void *source, QRhiTexture *textureLeft, QRhiTexture *textureRight, const QRect &geometry,
                       const QRect &clipRect = QRect(), Flags flags = { });
    void clear();

 Q_SIGNALS:
    void locked(bool);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QPlatformTextureList::Flags)

class Q_GUI_EXPORT QPlatformBackingStore
{
public:
    enum FlushResult {
        FlushSuccess,
        FlushFailed,
        FlushFailedDueToLostDevice
    };

    explicit QPlatformBackingStore(QWindow *window);
    virtual ~QPlatformBackingStore();

    QWindow *window() const;
    QBackingStore *backingStore() const;

    virtual QPaintDevice *paintDevice() = 0;

    virtual void flush(QWindow *window, const QRegion &region, const QPoint &offset);

    virtual FlushResult rhiFlush(QWindow *window,
                                 qreal sourceDevicePixelRatio,
                                 const QRegion &region,
                                 const QPoint &offset,
                                 QPlatformTextureList *textures,
                                 bool translucentBackground,
                                 qreal sourceTransformFactor = 0);

    virtual QImage toImage() const;

    enum TextureFlag {
        TextureSwizzle = 0x01,
        TextureFlip = 0x02,
        TexturePremultiplied = 0x04
    };
    Q_DECLARE_FLAGS(TextureFlags, TextureFlag)
    virtual QRhiTexture *toTexture(QRhiResourceUpdateBatch *resourceUpdates,
                                   const QRegion &dirtyRegion,
                                   TextureFlags *flags) const;

    virtual QPlatformGraphicsBuffer *graphicsBuffer() const;

    virtual void resize(const QSize &size, const QRegion &staticContents) = 0;

    virtual bool scroll(const QRegion &area, int dx, int dy);

    virtual void beginPaint(const QRegion &);
    virtual void endPaint();

    void createRhi(QWindow *window, QPlatformBackingStoreRhiConfig config);
    QRhi *rhi(QWindow *window) const;
    void surfaceAboutToBeDestroyed();
    void graphicsDeviceReportedLost(QWindow *window);

private:
    QPlatformBackingStorePrivate *d_ptr;

    void setBackingStore(QBackingStore *);
    friend class QBackingStore;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QPlatformBackingStore::TextureFlags)

QT_END_NAMESPACE

#endif // QPLATFORMBACKINGSTORE_H
