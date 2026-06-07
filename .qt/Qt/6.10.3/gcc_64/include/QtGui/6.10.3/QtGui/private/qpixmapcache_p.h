// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIXMAPCACHE_P_H
#define QPIXMAPCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qpixmapcache.h"
#include "qpaintengine.h"
#include <private/qimage_p.h>
#include <private/qpixmap_raster_p.h>
#include "qcache.h"

QT_BEGIN_NAMESPACE

class QPixmapCache::KeyData
{
public:
    KeyData() : isValid(true), key(0), ref(1) {}
    KeyData(const KeyData &other)
     : isValid(other.isValid), key(other.key), ref(1) {}
    ~KeyData() {}

    QString stringKey;
    bool isValid;
    int key;
    int ref;
};

// XXX: hw: is this a general concept we need to abstract?
class QPixmapCacheEntry : public QPixmap
{
public:
    QPixmapCacheEntry(const QPixmapCache::Key &key, const QPixmap &pix) : QPixmap(pix), key(key)
    {
        QPlatformPixmap *pd = handle();
        if (pd && pd->classId() == QPlatformPixmap::RasterClass) {
            QRasterPlatformPixmap *d = static_cast<QRasterPlatformPixmap*>(pd);
            if (!d->image.isNull() && d->image.d->paintEngine
                && !d->image.d->paintEngine->isActive())
            {
                delete d->image.d->paintEngine;
                d->image.d->paintEngine = nullptr;
            }
        }
    }
    ~QPixmapCacheEntry();
    QPixmapCache::Key key;
};

QT_END_NAMESPACE

#endif // QPIXMAPCACHE_P_H
