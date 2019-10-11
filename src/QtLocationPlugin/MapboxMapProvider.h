#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

const quint32 AVERAGE_MAPBOX_SAT_MAP     = 15739;
const quint32 AVERAGE_MAPBOX_STREET_MAP  = 5648;

class MapboxMapProvider : public MapProvider {
    Q_OBJECT
  public:
    MapboxMapProvider(QString mapName, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent);
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
  protected:
    QString mapboxName;
};

class MapboxStreetMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxStreetMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.streets", AVERAGE_MAPBOX_STREET_MAP,
                            QGeoMapType::StreetMap, parent) {}
};

class MapboxLightMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxLightMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.light", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxDarkMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxDarkMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.dark", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxSatelliteMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxSatelliteMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.satellite", AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::SatelliteMapDay, parent) {}
};

class MapboxHybridMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxHybridMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.hybrid", AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::HybridMap, parent) {}
};

class MapboxWheatPasteMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxWheatPasteMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.wheatpaste", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxStreetsBasicMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxStreetsBasicMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.streets-basic", AVERAGE_TILE_SIZE,
                            QGeoMapType::StreetMap, parent) {}
};

class MapboxComicMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxComicMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.comic", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxOutdoorsMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxOutdoorsMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.outdoors", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxRunBikeHikeMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxRunBikeHikeMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.run-bike-hike", AVERAGE_MAPBOX_STREET_MAP,
                            QGeoMapType::CycleMap, parent) {}
};

class MapboxPencilMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxPencilMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.pencil", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxPiratesMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxPiratesMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.pirates", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxEmeraldMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxEmeraldMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.emerald", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};

class MapboxHighContrastMapProvider : public MapboxMapProvider {
    Q_OBJECT
  public:
    MapboxHighContrastMapProvider(QObject* parent)
        : MapboxMapProvider("mapbox.high-contrast", AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}
};
