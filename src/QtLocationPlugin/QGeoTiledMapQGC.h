/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGeoTiledMapQGCLog)

class QGeoTiledMappingManagerEngineQGC;

class QGeoTiledMapQGC : public QGeoTiledMap
{
    Q_OBJECT

public:
    explicit QGeoTiledMapQGC(QGeoTiledMappingManagerEngineQGC *engine, QObject *parent = nullptr);
    ~QGeoTiledMapQGC();

    QGeoMap::Capabilities capabilities() const final;

private:
    // void evaluateCopyrights(const QSet<QGeoTileSpec> &visibleTiles) final;
};
