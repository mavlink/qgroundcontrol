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

#include <TerrainQuery.h>

/// @brief Provides unit test terrain query responses.
/// @details It provides preset, emulated, 1 arc-second (SRTM1) resolution regions that are either
/// flat or sloped in a fashion that aids testing terrain-sensitive functionality. All emulated
/// regions are positioned around Point Nemo - should real terrain became useful and checked in one day.
class UnitTestTerrainQuery : public TerrainQueryInterface {
public:

    static constexpr double regionSizeDeg     = 0.1;      // all regions are 0.1deg (~11km) square
    static constexpr double one_second_deg    = 1.0/3600;
    static constexpr double earths_radius_mts = 6371000.;

    /// Point Nemo is a point on Earth furthest from land
    static const QGeoCoordinate pointNemo;

    /// Region with constant 10m terrain elevation
    struct Flat10Region : public QGeoRectangle
    {
        Flat10Region(const QGeoRectangle& region)
            : QGeoRectangle(region)
        {

        }

        static const double amslElevation;
    };
    static const Flat10Region flat10Region;

    /// Region with a linear west to east slope raising at a rate of 100 meters per kilometer (-100m to 1000m)
    struct LinearSlopeRegion : public QGeoRectangle
    {
        LinearSlopeRegion(const QGeoRectangle& region)
            : QGeoRectangle(region)
        {

        }

        static const double minAMSLElevation;
        static const double maxAMSLElevation;
        static const double totalElevationChange;
    };
    static const LinearSlopeRegion linearSlopeRegion;

    /// Region with a hill (top half of a sphere) in the center.
    struct HillRegion : public QGeoRectangle
    {
        HillRegion(const QGeoRectangle& region)
            : QGeoRectangle(region)
        {
        }

        static const double radius;
    };
    static const HillRegion hillRegion;

    UnitTestTerrainQuery(TerrainQueryInterface* parent = nullptr);

    // Overrides from TerrainQueryInterface
    void requestCoordinateHeights   (const QList<QGeoCoordinate>& coordinates) override;
    void requestPathHeights         (const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) override;
    void requestCarpetHeights       (const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly) override;

private:
    typedef struct {
        QList<QGeoCoordinate>   rgCoords;
        QList<double>           rgHeights;
        double                  distanceBetween;
        double                  finalDistanceBetween;
    } PathHeightInfo_t;

    QList<double> _requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates);
    PathHeightInfo_t _requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord);
};
