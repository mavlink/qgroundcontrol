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
        : MapboxMapProvider(QStringLiteral("mapbox.streets"), AVERAGE_MAPBOX_STREET_MAP,
                            QGeoMapType::StreetMap, parent) {}
};

class MapboxLightMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxLightMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.light"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxDarkMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxDarkMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.dark"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxSatelliteMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxSatelliteMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.satellite"), AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::SatelliteMapDay, parent) {}
};

class MapboxHybridMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxHybridMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.hybrid"), AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::HybridMap, parent) {}
};

class MapboxWheatPasteMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxWheatPasteMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.wheatpaste"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxStreetsBasicMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxStreetsBasicMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.streets-basic"), AVERAGE_TILE_SIZE,
                            QGeoMapType::StreetMap, parent) {}
};

class MapboxComicMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxComicMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.comic"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxOutdoorsMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxOutdoorsMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.outdoors"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxRunBikeHikeMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxRunBikeHikeMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.run-bike-hike"), AVERAGE_MAPBOX_STREET_MAP,
                            QGeoMapType::CycleMap, parent) {}
};

class MapboxPencilMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxPencilMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.pencil"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxPiratesMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxPiratesMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.pirates"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxEmeraldMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxEmeraldMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.emerald"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxHighContrastMapProvider : public MapboxMapProvider {
    Q_OBJECT

public:
    MapboxHighContrastMapProvider(QObject* parent = nullptr)
        : MapboxMapProvider(QStringLiteral("mapbox.high-contrast"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};
