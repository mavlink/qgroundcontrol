#include "TerrainTest.h"

#include <cmath>

// Use the canonical definition from TerrainTest::pointNemo()
static const QGeoCoordinate pointNemo = TerrainTest::pointNemo();

const UnitTestTerrainData::Flat10Region UnitTestTerrainData::flat10Region{
    {pointNemo, QGeoCoordinate{pointNemo.latitude() - UnitTestTerrainData::regionSizeDeg,
                               pointNemo.longitude() + UnitTestTerrainData::regionSizeDeg}}};

const UnitTestTerrainData::LinearSlopeRegion UnitTestTerrainData::linearSlopeRegion{
    {flat10Region.topRight(),
     QGeoCoordinate{flat10Region.topRight().latitude() - UnitTestTerrainData::regionSizeDeg,
                    flat10Region.topRight().longitude() + UnitTestTerrainData::regionSizeDeg}}};

const UnitTestTerrainData::HillRegion UnitTestTerrainData::hillRegion{
    {linearSlopeRegion.topRight(),
     QGeoCoordinate{linearSlopeRegion.topRight().latitude() - UnitTestTerrainData::regionSizeDeg,
                    linearSlopeRegion.topRight().longitude() + UnitTestTerrainData::regionSizeDeg}}};

double UnitTestTerrainData::heightAt(const QGeoCoordinate& coordinate)
{
    if (flat10Region.contains(coordinate)) {
        return Flat10Region::amslElevation;
    }

    if (linearSlopeRegion.contains(coordinate)) {
        // cast to oneSecondDeg grid and round to int to emulate SRTM1 even better
        const double x = (coordinate.longitude() - linearSlopeRegion.topLeft().longitude()) / oneSecondDeg;
        const double dx = regionSizeDeg / oneSecondDeg;
        const double fraction = x / dx;
        return std::round(LinearSlopeRegion::minAMSLElevation + (fraction * LinearSlopeRegion::totalElevationChange));
    }

    if (hillRegion.contains(coordinate)) {
        // Distance from the hill center in meters, compared against the sphere radius in meters.
        // Longitude degrees are scaled by cos(latitude) so the hill is truly spherical.
        const double metersPerDeg = earthsRadiusMts * (M_PI / 180.);
        const double x = (coordinate.latitude() - hillRegion.center().latitude()) * metersPerDeg;
        const double y = (coordinate.longitude() - hillRegion.center().longitude()) * metersPerDeg *
                         cos(hillRegion.center().latitude() * (M_PI / 180.));
        const double x2y2 = (x * x) + (y * y);
        const double r2 = HillRegion::radiusMts * HillRegion::radiusMts;
        return (x2y2 <= r2) ? sqrt(r2 - x2y2) : Flat10Region::amslElevation;
    }

    return 0.;
}

/*===========================================================================*/

TerrainTest::TerrainTest(QObject* parent) : UnitTest(parent) {}
