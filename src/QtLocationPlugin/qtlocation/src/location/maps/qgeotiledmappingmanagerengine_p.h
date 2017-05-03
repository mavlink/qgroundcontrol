/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGEOTILEDMAPPINGMANAGERENGINE_H
#define QGEOTILEDMAPPINGMANAGERENGINE_H

#include <QObject>
#include <QSize>
#include <QPair>
#include <QtLocation/qlocationglobal.h>
#include "qgeomaptype_p.h"
#include "qgeomappingmanagerengine_p.h"

QT_BEGIN_NAMESPACE

class QGeoTiledMappingManagerEnginePrivate;
class QGeoMapRequestOptions;
class QGeoTileFetcher;
class QGeoTileTexture;

class QGeoTileSpec;
class QGeoTiledMap;
class QGeoTileCache;

class Q_LOCATION_EXPORT QGeoTiledMappingManagerEngine : public QGeoMappingManagerEngine
{
    Q_OBJECT

public:
    enum CacheArea {
        DiskCache = 0x01,
        MemoryCache = 0x02,
        AllCaches = 0xFF
    };
    Q_DECLARE_FLAGS(CacheAreas, CacheArea)

    explicit QGeoTiledMappingManagerEngine(QObject *parent = 0);
    virtual ~QGeoTiledMappingManagerEngine();

    QGeoTileFetcher *tileFetcher();

    QGeoMap *createMap() Q_DECL_OVERRIDE;
    void registerMap(QGeoMap *map) Q_DECL_OVERRIDE;
    void deregisterMap(QGeoMap *map) Q_DECL_OVERRIDE;

    QSize tileSize() const;

    void updateTileRequests(QGeoTiledMap *map,
                            const QSet<QGeoTileSpec> &tilesAdded,
                            const QSet<QGeoTileSpec> &tilesRemoved);

    QGeoTileCache *tileCache(); // TODO: check this is still used
    QSharedPointer<QGeoTileTexture> getTileTexture(const QGeoTileSpec &spec);


    QGeoTiledMappingManagerEngine::CacheAreas cacheHint() const;

private Q_SLOTS:
    void engineTileFinished(const QGeoTileSpec &spec, const QByteArray &bytes, const QString &format);
    void engineTileError(const QGeoTileSpec &spec, const QString &errorString);

Q_SIGNALS:
    void tileError(const QGeoTileSpec &spec, const QString &errorString);
    void mapVersionChanged();

protected:
    void setTileFetcher(QGeoTileFetcher *fetcher);
    void setTileSize(const QSize &tileSize);
    void setCacheHint(QGeoTiledMappingManagerEngine::CacheAreas cacheHint);

    QGeoTileCache *createTileCacheWithDir(const QString &cacheDirectory);

private:
    QGeoTiledMappingManagerEnginePrivate *d_ptr;

    Q_DECLARE_PRIVATE(QGeoTiledMappingManagerEngine)
    Q_DISABLE_COPY(QGeoTiledMappingManagerEngine)

    friend class QGeoTileFetcher;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoTiledMappingManagerEngine::CacheAreas)

QT_END_NAMESPACE

#endif
