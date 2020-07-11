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

class BingMapProvider : public MapProvider {
    Q_OBJECT

public:
    BingMapProvider(const QString &imageFormat, const quint32 averageSize,
                    const QGeoMapType::MapStyle mapType, QObject* parent = nullptr);

    ~BingMapProvider() = default;

    bool _isBingProvider() const override { return true; }


protected:
    const QString _versionBingMaps = QStringLiteral("563");
};

static const quint32 AVERAGE_BING_STREET_MAP = 1297;
static const quint32 AVERAGE_BING_SAT_MAP    = 19597;

// -----------------------------------------------------------
// Bing Road Map

class BingRoadMapProvider : public BingMapProvider {
    Q_OBJECT

public:
    BingRoadMapProvider(QObject* parent = nullptr)
        : BingMapProvider(QStringLiteral("png"), AVERAGE_BING_STREET_MAP, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Bing Satellite Map

class BingSatelliteMapProvider : public BingMapProvider {
    Q_OBJECT

public:
    BingSatelliteMapProvider(QObject* parent = nullptr)
        : BingMapProvider(QStringLiteral("jpg"), AVERAGE_BING_SAT_MAP, QGeoMapType::SatelliteMapDay, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Bing Hybrid Map

class BingHybridMapProvider : public BingMapProvider {
    Q_OBJECT
public:
    BingHybridMapProvider(QObject* parent = nullptr)
        : BingMapProvider(QStringLiteral("jpg"),AVERAGE_BING_SAT_MAP, QGeoMapType::HybridMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};
