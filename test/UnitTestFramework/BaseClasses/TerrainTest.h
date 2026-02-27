#pragma once

#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoRectangle>

#include "TerrainQueryInterface.h"
#include "UnitTest.h"

/// @file
/// @brief Base class for terrain-related tests

/// Provides unit test terrain query responses.
/// It provides preset, emulated, 1 arc-second (SRTM1) resolution regions that are either
/// flat or sloped in a fashion that aids testing terrain-sensitive functionality. All emulated
/// regions are positioned around Point Nemo - should real terrain become useful and checked in one day.
class UnitTestTerrainQuery : public TerrainQueryInterface
{
public:
    explicit UnitTestTerrainQuery(QObject* parent = nullptr);
    ~UnitTestTerrainQuery() override;

    void requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates) final;
    void requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) final;
    void requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly) final;

    static constexpr double regionSizeDeg = 0.1;  ///< all regions are 0.1deg (~11km) square
    static constexpr double oneSecondDeg = 1.0 / 3600.;
    static constexpr double earthsRadiusMts = 6371000.;

    /// Region with constant 10m terrain elevation
    struct Flat10Region : public QGeoRectangle
    {
        explicit Flat10Region(const QGeoRectangle& region) : QGeoRectangle(region)
        {
        }

        static constexpr double amslElevation = 10.;
    };

    static const Flat10Region flat10Region;

    /// Region with a linear west to east slope raising at a rate of 100 meters per kilometer (-100m to 1000m)
    struct LinearSlopeRegion : public QGeoRectangle
    {
        explicit LinearSlopeRegion(const QGeoRectangle& region) : QGeoRectangle(region)
        {
        }

        static constexpr double minAMSLElevation = -100.;
        static constexpr double maxAMSLElevation = 1000.;
        static constexpr double totalElevationChange = maxAMSLElevation - minAMSLElevation;
    };

    static const LinearSlopeRegion linearSlopeRegion;

    /// Region with a hill (top half of a sphere) in the center.
    struct HillRegion : public QGeoRectangle
    {
        explicit HillRegion(const QGeoRectangle& region) : QGeoRectangle(region)
        {
        }

        static constexpr double radius = UnitTestTerrainQuery::regionSizeDeg / UnitTestTerrainQuery::oneSecondDeg;
    };

    static const HillRegion hillRegion;

    /// Path height info returned by terrain queries
    struct PathHeightInfo_t
    {
        QList<QGeoCoordinate> rgCoords;
        QList<double> rgHeights;
        double distanceBetween;
        double finalDistanceBetween;
    };

private:
    QList<double> _requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates);
    PathHeightInfo_t _requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord);
};

/*===========================================================================*/

/// Base class for terrain-related tests.
/// Provides access to UnitTestTerrainQuery for mock terrain data.
///
/// Example usage:
/// @code
/// class MyTerrainTest : public TerrainTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testTerrainQuery() {
///         QList<QGeoCoordinate> coords;
///         coords << pointNemo();
///         terrainQuery()->requestCoordinateHeights(coords);
///     }
/// };
/// @endcode
class TerrainTest : public UnitTest
{
    Q_OBJECT

public:
    explicit TerrainTest(QObject* parent = nullptr);
    ~TerrainTest() override = default;

    /// Point Nemo - furthest point from land, used as terrain test origin
    static QGeoCoordinate pointNemo()
    {
        return QGeoCoordinate(-48.875556, -123.392500);
    }

    /// Returns the test terrain query interface
    UnitTestTerrainQuery* terrainQuery() const
    {
        return _terrainQuery;
    }

    /// Access to flat 10m region
    static const UnitTestTerrainQuery::Flat10Region& flat10Region()
    {
        return UnitTestTerrainQuery::flat10Region;
    }

    /// Access to linear slope region
    static const UnitTestTerrainQuery::LinearSlopeRegion& linearSlopeRegion()
    {
        return UnitTestTerrainQuery::linearSlopeRegion;
    }

    /// Access to hill region
    static const UnitTestTerrainQuery::HillRegion& hillRegion()
    {
        return UnitTestTerrainQuery::hillRegion;
    }

protected slots:
    void init() override;
    void cleanup() override;

private:
    UnitTestTerrainQuery* _terrainQuery = nullptr;
};
