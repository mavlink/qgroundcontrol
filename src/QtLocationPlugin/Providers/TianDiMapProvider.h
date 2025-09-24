/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_TIANDI_STREET_MAP = 1297;
static constexpr const quint32 AVERAGE_TIANDI_SAT_MAP    = 19597;

class TianDiMapProvider : public MapProvider
{
protected:
    TianDiMapProvider(const QString &mapName, const QString &mapTypeCode, const QString &imageFormat, quint32 averageSize,
                    QGeoMapType::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://map.tianditu.gov.cn/"), imageFormat, averageSize, mapType)
        , _mapType(mapTypeCode) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapType;
    const QString _mapUrl = QStringLiteral("https://t%1.tianditu.gov.cn/DataServer?tk=%2&T=%3&x=%4&y=%5&l=%6");
};

class TianDiRoadMapProvider : public TianDiMapProvider
{
public:
    TianDiRoadMapProvider()
        : TianDiMapProvider(
            QObject::tr("TianDi Road"),
            QStringLiteral("cia_w"),
            QStringLiteral("png"),
            AVERAGE_TIANDI_STREET_MAP,
            QGeoMapType::StreetMap) {}
};

class TianDiSatelliteMapProvider : public TianDiMapProvider
{
public:
    TianDiSatelliteMapProvider()
        : TianDiMapProvider(
            QObject::tr("TianDi Satellite"),
            QStringLiteral("img_w"),
            QStringLiteral("jpg"),
            AVERAGE_TIANDI_SAT_MAP,
            QGeoMapType::SatelliteMapDay) {}
};

