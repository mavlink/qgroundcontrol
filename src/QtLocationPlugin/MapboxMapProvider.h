/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MapProvider.h"

static const quint32 AVERAGE_MAPBOX_SAT_MAP     = 15739;
static const quint32 AVERAGE_MAPBOX_STREET_MAP  = 5648;

class MapboxMapProvider : public MapProvider {
    Q_OBJECT

public:
    MapboxMapProvider(const QString& mapName, const quint32 averageSize, const QGeoMapType::MapStyle mapType, QObject* parent = nullptr);

protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;

    QString _mapboxName;
};

class MapboxStreetMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxStreetMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("streets-v10"), AVERAGE_MAPBOX_STREET_MAP,
                            QGeoMapType::StreetMap, parent) {}
};

class MapboxLightMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxLightMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("light-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxDarkMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxDarkMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("dark-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxSatelliteMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxSatelliteMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("satellite-v9"), AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::SatelliteMapDay, parent) {}
};

class MapboxHybridMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxHybridMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("satellite-streets-v10"), AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::HybridMap, parent) {}
};

class MapboxBrightMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxBrightMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("bright-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxStreetsBasicMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxStreetsBasicMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("basic-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::StreetMap, parent) {}
};

class MapboxOutdoorsMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxOutdoorsMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("outdoors-v10"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxCustomMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxCustomMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.custom"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};
