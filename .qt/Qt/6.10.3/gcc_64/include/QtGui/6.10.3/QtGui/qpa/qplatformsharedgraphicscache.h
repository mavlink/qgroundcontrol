// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSHAREDGRAPHICSCACHE_H
#define QPLATFORMSHAREDGRAPHICSCACHE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPlatformSharedGraphicsCache: public QObject
{
    Q_OBJECT
public:
    enum PixelFormat
    {
        Alpha8
    };

    enum BufferType
    {
        OpenGLTexture
    };

    explicit QPlatformSharedGraphicsCache(QObject *parent = nullptr) : QObject(parent) {}

    virtual void beginRequestBatch() = 0;
    virtual void ensureCacheInitialized(const QByteArray &cacheId, BufferType bufferType,
                                                    PixelFormat pixelFormat) = 0;
    virtual void requestItems(const QByteArray &cacheId, const QList<quint32> &itemIds) = 0;
    virtual void insertItems(const QByteArray &cacheId, const QList<quint32> &itemIds,
                             const QList<QImage> &items) = 0;
    virtual void releaseItems(const QByteArray &cacheId, const QList<quint32> &itemIds) = 0;
    virtual void endRequestBatch() = 0;

    virtual bool requestBatchStarted() const = 0;

    virtual uint textureIdForBuffer(void *bufferId) = 0;
    virtual void referenceBuffer(void *bufferId) = 0;
    virtual bool dereferenceBuffer(void *bufferId) = 0;
    virtual QSize sizeOfBuffer(void *bufferId) = 0;
    virtual void *eglImageForBuffer(void *bufferId) = 0;

Q_SIGNALS:
    void itemsMissing(const QByteArray &cacheId, const QList<quint32> &itemIds);
    void itemsAvailable(const QByteArray &cacheId, void *bufferId, const QList<quint32> &itemIds,
                        const QList<QPoint> &positionsInBuffer);
    void itemsInvalidated(const QByteArray &cacheId, const QList<quint32> &itemIds);
    void itemsUpdated(const QByteArray &cacheId, void *bufferId, const QList<quint32> &itemIds,
                      const QList<QPoint> &positionsInBuffer);
};

QT_END_NAMESPACE

#endif // QPLATFORMSHAREDGRAPHICSCACHE_H
