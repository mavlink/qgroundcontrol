#pragma once

#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoRectangle>

#include "UnitTest.h"

/// @file
/// @brief Base class and synthetic terrain data for terrain-related tests

/// Synthetic terrain regions used to generate deterministic elevation data for unit tests.
/// Preset, emulated, 1 arc-second (SRTM1) resolution regions that are either flat or sloped
/// in a fashion that aids testing terrain-sensitive functionality. All emulated regions are
/// positioned around Point Nemo - should real terrain become useful and checked in one day.
/// Any coordinate outside the regions has 0 height.
class UnitTestTerrainData
{
public:
    static constexpr double regionSizeDeg = 0.1;  ///< all regions are 0.1deg (~11km) square
    static constexpr double oneSecondDeg = 1.0 / 3600.;
    static constexpr double earthsRadiusMts = 6371000.;

    /// Region with constant 10m terrain elevation
    struct Flat10Region : public QGeoRectangle
    {
        explicit Flat10Region(const QGeoRectangle& region) : QGeoRectangle(region) {}

        static constexpr double amslElevation = 10.;
    };

    static const Flat10Region flat10Region;

    /// Region with a linear west to east slope raising at a rate of 100 meters per kilometer (-100m to 1000m)
    struct LinearSlopeRegion : public QGeoRectangle
    {
        explicit LinearSlopeRegion(const QGeoRectangle& region) : QGeoRectangle(region) {}

        static constexpr double minAMSLElevation = -100.;
        static constexpr double maxAMSLElevation = 1000.;
        static constexpr double totalElevationChange = maxAMSLElevation - minAMSLElevation;
    };

    static const LinearSlopeRegion linearSlopeRegion;

    /// Region with a hill (top half of a sphere) in the center.
    struct HillRegion : public QGeoRectangle
    {
        explicit HillRegion(const QGeoRectangle& region) : QGeoRectangle(region) {}

        /// Sphere radius in meters (also the hill's peak height)
        static constexpr double radiusMts = 360.;
    };

    static const HillRegion hillRegion;

    /// Returns the synthetic terrain height for the coordinate. Coordinates outside
    /// the test regions return 0.
    static double heightAt(const QGeoCoordinate& coordinate);
};

/*===========================================================================*/

/// Base class for terrain-related tests.
/// Synthetic elevation data for the UnitTestTerrainData regions is served to all unit
/// tests at the tile cache layer (see UnitTestTileGenerator), so production terrain
/// queries resolve against it without any network access.
class TerrainTest : public UnitTest
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerrainTest)

public:
    explicit TerrainTest(QObject* parent = nullptr);
    ~TerrainTest() override = default;

    /// Point Nemo - furthest point from land, used as terrain test origin
    static QGeoCoordinate pointNemo() { return QGeoCoordinate(-48.875556, -123.392500); }

    /// Access to flat 10m region
    static const UnitTestTerrainData::Flat10Region& flat10Region() { return UnitTestTerrainData::flat10Region; }

    /// Access to linear slope region
    static const UnitTestTerrainData::LinearSlopeRegion& linearSlopeRegion()
    {
        return UnitTestTerrainData::linearSlopeRegion;
    }

    /// Access to hill region
    static const UnitTestTerrainData::HillRegion& hillRegion() { return UnitTestTerrainData::hillRegion; }
};
