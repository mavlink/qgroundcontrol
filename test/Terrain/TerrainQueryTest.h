/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoRectangle>

#include "UnitTest.h"
#include "TerrainQueryInterface.h"

/// Provides unit test terrain query responses.
/// It provides preset, emulated, 1 arc-second (SRTM1) resolution regions that are either
/// flat or sloped in a fashion that aids testing terrain-sensitive functionality. All emulated
/// regions are positioned around Point Nemo - should real terrain became useful and checked in one day.
class UnitTestTerrainQuery : public TerrainQueryInterface
{
public:
    explicit UnitTestTerrainQuery(QObject *parent = nullptr);
    ~UnitTestTerrainQuery();

    void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates) final;
    void requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord) final;
    void requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly) final;

    static constexpr double regionSizeDeg = 0.1;           ///< all regions are 0.1deg (~11km) square
    static constexpr double oneSecondDeg = 1.0 / 3600.;
    static constexpr double earthsRadiusMts = 6371000.;

    /// Region with constant 10m terrain elevation
    struct Flat10Region : public QGeoRectangle
    {
        explicit Flat10Region(const QGeoRectangle &region)
            : QGeoRectangle(region) {}

        static constexpr double amslElevation = 10.;
    };
    static const Flat10Region flat10Region;

    /// Region with a linear west to east slope raising at a rate of 100 meters per kilometer (-100m to 1000m)
    struct LinearSlopeRegion : public QGeoRectangle
    {
        explicit LinearSlopeRegion(const QGeoRectangle &region)
            : QGeoRectangle(region) {}

        static constexpr double minAMSLElevation = -100.;
        static constexpr double maxAMSLElevation = 1000.;
        static constexpr double totalElevationChange = maxAMSLElevation - minAMSLElevation;
    };
    static const LinearSlopeRegion linearSlopeRegion;

    /// Region with a hill (top half of a sphere) in the center.
    struct HillRegion : public QGeoRectangle
    {
        explicit HillRegion(const QGeoRectangle &region)
            : QGeoRectangle(region) {}

        static constexpr double radius = UnitTestTerrainQuery::regionSizeDeg / UnitTestTerrainQuery::oneSecondDeg;
    };
    static const HillRegion hillRegion;

private:
    QList<double> _requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates);

    struct PathHeightInfo_t {
        QList<QGeoCoordinate> rgCoords;
        QList<double> rgHeights;
        double distanceBetween;
        double finalDistanceBetween;
    };
    PathHeightInfo_t _requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord);
};

/*===========================================================================*/

class TerrainQueryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRequestCoordinateHeights();
    void _testRequestPathHeights();
    void _testRequestCarpetHeights();
    // void _testTerrainAtCoordinateQuery();
};
