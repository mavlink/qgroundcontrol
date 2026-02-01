#include "GeoTest.h"

#include <QtGui/QVector3D>

#include "Benchmarking.h"
#include "PropertyTesting.h"
#include "QGCGeo.h"

static bool compareDoubles(double actual, double expected, double epsilon = 0.00001)
{
    return (qAbs(actual - expected) <= epsilon);
}

void GeoTest::_convertGeoToNed_test()
{
    const QGeoCoordinate coord(47.364869, 8.594398, 0.0);
    // Expected values from WGS84 ellipsoidal model (LocalCartesian)
    const double expectedX = -1280.954612;
    const double expectedY = 3497.196961;
    const double expectedZ = 1.085830;
    double x = 0., y = 0., z = 0.;
    QGCGeo::convertGeoToNed(coord, m_origin, x, y, z);
    // Use 0.01m tolerance (instead of default epsilon) because the WGS84 ellipsoidal
    // model and reference implementation can differ at the centimeter level due to
    // numerical precision and model approximations.
    QVERIFY(compareDoubles(x, expectedX, 0.01));
    QVERIFY(compareDoubles(y, expectedY, 0.01));
    QVERIFY(compareDoubles(z, expectedZ, 0.01));
}

void GeoTest::_convertGeoToNedAtOrigin_test()
{
    QGeoCoordinate coord(m_origin);
    coord.setAltitude(10.);  // offset altitude to test z
    const double expectedX = 0.;
    const double expectedY = 0.;
    const double expectedZ = -10.;
    double x = 0., y = 0., z = 0.;
    QGCGeo::convertGeoToNed(coord, m_origin, x, y, z);
    QVERIFY(compareDoubles(x, expectedX));
    QVERIFY(compareDoubles(y, expectedY));
    QVERIFY(compareDoubles(z, expectedZ));
}

void GeoTest::_convertNedToGeo_test()
{
    // Use values from WGS84 ellipsoidal model
    const double x = -1280.954612;
    const double y = 3497.196961;
    const double z = 1.085830;
    QGeoCoordinate coord(0, 0, 0);
    QGCGeo::convertNedToGeo(x, y, z, m_origin, coord);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), 47.364869, 0.00001));
    QVERIFY(compareDoubles(coord.longitude(), 8.594398, 0.00001));
    QVERIFY(compareDoubles(coord.altitude(), 0.0, 0.01));
}

void GeoTest::_convertNedToGeoAtOrigin_test()
{
    const double x = 0.0;
    const double y = 0.0;
    const double z = 0.0;
    QGeoCoordinate coord(0, 0, 0);
    QGCGeo::convertNedToGeo(x, y, z, m_origin, coord);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(coord.altitude(), m_origin.altitude()));
}

void GeoTest::_convertGeoToUTM_test()
{
    const QGeoCoordinate coord(m_origin);
    double easting = 0.;
    double northing = 0.;
    const int zone = QGCGeo::convertGeoToUTM(coord, easting, northing);
    QCOMPARE(zone, 32);
    QVERIFY(compareDoubles(easting, 465886.092246));
    QVERIFY(compareDoubles(northing, 5247092.44892));
}

void GeoTest::_convertUTMToGeo_test()
{
    const double easting = 465886.092246;
    const double northing = 5247092.44892;
    const int zone = 32;
    const bool southhemi = false;
    QGeoCoordinate coord(0, 0, 0);
    const bool result = QGCGeo::convertUTMToGeo(easting, northing, zone, southhemi, coord);
    QVERIFY(result);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(coord.altitude(), m_origin.altitude()));
}

void GeoTest::_convertGeoToMGRS_test()
{
    const QGeoCoordinate coord(m_origin);
    const QString result = QGCGeo::convertGeoToMGRS(coord);
    QCOMPARE(result, "32TMT 65886 47092");
}

void GeoTest::_convertMGRSToGeo_test()
{
    const QString mgrs("32T MT 65886 47092");
    QGeoCoordinate coord(0, 0, 0);
    const bool result = QGCGeo::convertMGRSToGeo(mgrs, coord);
    QVERIFY(result);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(coord.altitude(), m_origin.altitude()));
}

void GeoTest::_convertGeodeticToEcef_test()
{
    const QGeoCoordinate coord(m_origin.latitude(), m_origin.longitude(), 100.0);
    const QVector3D ecef = QGCGeo::convertGeodeticToEcef(coord);
    // ECEF coordinates for ETH campus at 100m altitude
    QVERIFY(compareDoubles(ecef.x(), 4278990.18, 1.0));
    QVERIFY(compareDoubles(ecef.y(), 643172.29, 1.0));
    QVERIFY(compareDoubles(ecef.z(), 4670276.60, 1.0));
}

void GeoTest::_convertEcefToGeodetic_test()
{
    const QVector3D ecef(4278990.18, 643172.29, 4670276.60);
    const QGeoCoordinate coord = QGCGeo::convertEcefToGeodetic(ecef);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude(), 0.0001));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude(), 0.0001));
    QVERIFY(compareDoubles(coord.altitude(), 100.0, 1.0));
}

void GeoTest::_convertGpsToEnu_test()
{
    const QGeoCoordinate coord(47.364869, 8.594398, 50.0);
    const QVector3D enu = QGCGeo::convertGpsToEnu(coord, m_origin);
    // ENU: East, North, Up (WGS84 ellipsoidal)
    QVERIFY(compareDoubles(enu.x(), 3497.22, 1.0));   // East
    QVERIFY(compareDoubles(enu.y(), -1280.96, 1.0));  // North
    QVERIFY(compareDoubles(enu.z(), 48.91, 0.1));     // Up (ellipsoidal height)
}

void GeoTest::_convertEnuToGps_test()
{
    const QVector3D enu(3497.22, -1280.96, 48.91);
    const QGeoCoordinate coord = QGCGeo::convertEnuToGps(enu, m_origin);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), 47.364869, 0.0001));
    QVERIFY(compareDoubles(coord.longitude(), 8.594398, 0.0001));
    QVERIFY(compareDoubles(coord.altitude(), 50.0, 0.2));
}

void GeoTest::_convertEcefToEnu_test()
{
    // Convert a known geodetic point to ECEF first
    const QGeoCoordinate coord(47.364869, 8.594398, 50.0);
    const QVector3D ecef = QGCGeo::convertGeodeticToEcef(coord);
    // Then convert ECEF to ENU relative to origin
    const QVector3D enu = QGCGeo::convertEcefToEnu(ecef, m_origin);
    // Note: QVector3D uses float, so ECEF round-trips have reduced precision
    QVERIFY(compareDoubles(enu.x(), 3497.22, 2.0));   // East
    QVERIFY(compareDoubles(enu.y(), -1280.96, 2.0));  // North
    QVERIFY(compareDoubles(enu.z(), 48.91, 1.0));     // Up
}

void GeoTest::_convertEnuToEcef_test()
{
    const QVector3D enu(3497.22, -1280.96, 48.91);
    const QVector3D ecef = QGCGeo::convertEnuToEcef(enu, m_origin);
    // Convert back to geodetic to verify
    // Note: QVector3D uses float, so ECEF round-trips have reduced precision
    const QGeoCoordinate coord = QGCGeo::convertEcefToGeodetic(ecef);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), 47.364869, 0.001));
    QVERIFY(compareDoubles(coord.longitude(), 8.594398, 0.001));
    QVERIFY(compareDoubles(coord.altitude(), 50.0, 1.0));
}

void GeoTest::_geodesicDistance_test()
{
    // Test distance from ETH Zurich to Eiffel Tower (Paris)
    const QGeoCoordinate zurich(47.3764, 8.5481, 0.0);
    const QGeoCoordinate paris(48.8584, 2.2945, 0.0);
    const double distance = QGCGeo::geodesicDistance(zurich, paris);
    // Geodesic distance is approximately 493.7 km
    QVERIFY(compareDoubles(distance, 493739.0, 100.0));
    // Compare with Qt's spherical approximation to show difference
    const double qtDistance = zurich.distanceTo(paris);
    // Qt uses spherical model, should be slightly different
    QVERIFY(qAbs(distance - qtDistance) < 2000.0);  // Within 2km for this distance
}

void GeoTest::_geodesicAzimuth_test()
{
    // Test azimuth from ETH Zurich heading northwest to Paris
    const QGeoCoordinate zurich(47.3764, 8.5481, 0.0);
    const QGeoCoordinate paris(48.8584, 2.2945, 0.0);
    const double azimuth = QGCGeo::geodesicAzimuth(zurich, paris);
    // Azimuth is approximately 291.8 degrees (northwest)
    QVERIFY(compareDoubles(azimuth, 291.8, 1.0));
    // Test due north
    const QGeoCoordinate north(48.3764, 8.5481, 0.0);
    const double northAzimuth = QGCGeo::geodesicAzimuth(zurich, north);
    QVERIFY(compareDoubles(northAzimuth, 0.0, 0.1));
    // Test due east
    const QGeoCoordinate east(47.3764, 9.5481, 0.0);
    const double eastAzimuth = QGCGeo::geodesicAzimuth(zurich, east);
    QVERIFY(compareDoubles(eastAzimuth, 90.0, 1.0));
}

void GeoTest::_geodesicDestination_test()
{
    const QGeoCoordinate start(47.3764, 8.5481, 100.0);
    // Go 1000m north
    const QGeoCoordinate northDest = QGCGeo::geodesicDestination(start, 0.0, 1000.0);
    QVERIFY(northDest.isValid());
    QVERIFY(northDest.latitude() > start.latitude());
    QVERIFY(compareDoubles(northDest.longitude(), start.longitude(), 0.0001));
    QVERIFY(compareDoubles(northDest.altitude(), start.altitude(), 0.01));
    // Go 1000m east
    const QGeoCoordinate eastDest = QGCGeo::geodesicDestination(start, 90.0, 1000.0);
    QVERIFY(eastDest.isValid());
    QVERIFY(compareDoubles(eastDest.latitude(), start.latitude(), 0.001));
    QVERIFY(eastDest.longitude() > start.longitude());
}

void GeoTest::_geodesicRoundTrip_test()
{
    // Verify that destination + inverse gives back original distance/azimuth
    const QGeoCoordinate start(47.3764, 8.5481, 0.0);
    const double originalAzimuth = 45.0;
    const double originalDistance = 5000.0;
    // Calculate destination
    const QGeoCoordinate dest = QGCGeo::geodesicDestination(start, originalAzimuth, originalDistance);
    // Calculate distance and azimuth back
    const double calculatedDistance = QGCGeo::geodesicDistance(start, dest);
    const double calculatedAzimuth = QGCGeo::geodesicAzimuth(start, dest);
    QVERIFY(compareDoubles(calculatedDistance, originalDistance, 0.01));
    QVERIFY(compareDoubles(calculatedAzimuth, originalAzimuth, 0.0001));
}

void GeoTest::_pathLength_test()
{
    // Empty and single-point paths
    QCOMPARE(QGCGeo::pathLength({}), 0.0);
    QCOMPARE(QGCGeo::pathLength({m_origin}), 0.0);
    // Simple path: origin -> 1km north -> 1km east
    const QGeoCoordinate north = QGCGeo::geodesicDestination(m_origin, 0.0, 1000.0);
    const QGeoCoordinate northEast = QGCGeo::geodesicDestination(north, 90.0, 1000.0);
    const QList<QGeoCoordinate> path = {m_origin, north, northEast};
    const double length = QGCGeo::pathLength(path);
    // Should be approximately 2000m
    QVERIFY(compareDoubles(length, 2000.0, 1.0));
}

void GeoTest::_polygonArea_test()
{
    // Fewer than 3 points
    QCOMPARE(QGCGeo::polygonArea({}), 0.0);
    QCOMPARE(QGCGeo::polygonArea({m_origin}), 0.0);
    QCOMPARE(QGCGeo::polygonArea({m_origin, m_origin}), 0.0);
    // Create a roughly 1km x 1km square
    const QGeoCoordinate sw = m_origin;
    const QGeoCoordinate nw = QGCGeo::geodesicDestination(sw, 0.0, 1000.0);
    const QGeoCoordinate ne = QGCGeo::geodesicDestination(nw, 90.0, 1000.0);
    const QGeoCoordinate se = QGCGeo::geodesicDestination(sw, 90.0, 1000.0);
    const QList<QGeoCoordinate> square = {sw, nw, ne, se};
    const double area = QGCGeo::polygonArea(square);
    // Should be approximately 1,000,000 m² (1 km²)
    QVERIFY(compareDoubles(area, 1000000.0, 1000.0));  // Within 0.1%
}

void GeoTest::_polygonPerimeter_test()
{
    // Fewer than 2 points
    QCOMPARE(QGCGeo::polygonPerimeter({}), 0.0);
    QCOMPARE(QGCGeo::polygonPerimeter({m_origin}), 0.0);
    // Create a roughly 1km x 1km square
    const QGeoCoordinate sw = m_origin;
    const QGeoCoordinate nw = QGCGeo::geodesicDestination(sw, 0.0, 1000.0);
    const QGeoCoordinate ne = QGCGeo::geodesicDestination(nw, 90.0, 1000.0);
    const QGeoCoordinate se = QGCGeo::geodesicDestination(sw, 90.0, 1000.0);
    const QList<QGeoCoordinate> square = {sw, nw, ne, se};
    const double perimeter = QGCGeo::polygonPerimeter(square);
    // Should be approximately 4000m
    QVERIFY(compareDoubles(perimeter, 4000.0, 1.0));
}

void GeoTest::_interpolatePath_test()
{
    // Test with same start and end
    const auto samePoints = QGCGeo::interpolatePath(m_origin, m_origin, 5);
    QCOMPARE(samePoints.size(), 5);
    for (const auto& coord : samePoints) {
        QCOMPARE(coord, m_origin);
    }
    // Test with 2 points (minimum)
    const QGeoCoordinate dest = QGCGeo::geodesicDestination(m_origin, 45.0, 10000.0);
    const auto twoPoints = QGCGeo::interpolatePath(m_origin, dest, 2);
    QCOMPARE(twoPoints.size(), 2);
    QVERIFY(compareDoubles(twoPoints.first().latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(twoPoints.first().longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(twoPoints.last().latitude(), dest.latitude()));
    QVERIFY(compareDoubles(twoPoints.last().longitude(), dest.longitude()));
    // Test with 11 points (10 segments of 1000m each)
    const QGeoCoordinate end = QGCGeo::geodesicDestination(m_origin, 0.0, 10000.0);
    const auto elevenPoints = QGCGeo::interpolatePath(m_origin, end, 11);
    QCOMPARE(elevenPoints.size(), 11);
    // Each segment should be approximately 1000m
    for (int i = 1; i < elevenPoints.size(); ++i) {
        const double segmentDist = QGCGeo::geodesicDistance(elevenPoints[i - 1], elevenPoints[i]);
        QVERIFY(compareDoubles(segmentDist, 1000.0, 0.1));
    }
    // Total path length should be 10000m
    const double totalLen = QGCGeo::pathLength(elevenPoints);
    QVERIFY(compareDoubles(totalLen, 10000.0, 0.1));
    // Test altitude interpolation
    const QGeoCoordinate startAlt(m_origin.latitude(), m_origin.longitude(), 100.0);
    const QGeoCoordinate endAlt(end.latitude(), end.longitude(), 200.0);
    const auto altPoints = QGCGeo::interpolatePath(startAlt, endAlt, 11);
    QVERIFY(compareDoubles(altPoints[0].altitude(), 100.0, 0.01));
    QVERIFY(compareDoubles(altPoints[5].altitude(), 150.0, 0.01));
    QVERIFY(compareDoubles(altPoints[10].altitude(), 200.0, 0.01));
}

void GeoTest::_interpolateAtDistance_test()
{
    // Test at distance 0
    const QGeoCoordinate dest = QGCGeo::geodesicDestination(m_origin, 0.0, 5000.0);
    const auto atZero = QGCGeo::interpolateAtDistance(m_origin, dest, 0.0);
    QVERIFY(compareDoubles(atZero.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(atZero.longitude(), m_origin.longitude()));
    // Test at distance beyond end (should clamp to end)
    const auto beyondEnd = QGCGeo::interpolateAtDistance(m_origin, dest, 10000.0);
    QVERIFY(compareDoubles(beyondEnd.latitude(), dest.latitude()));
    QVERIFY(compareDoubles(beyondEnd.longitude(), dest.longitude()));
    // Test midpoint
    const auto midpoint = QGCGeo::interpolateAtDistance(m_origin, dest, 2500.0);
    const double distToMid = QGCGeo::geodesicDistance(m_origin, midpoint);
    const double distFromMid = QGCGeo::geodesicDistance(midpoint, dest);
    QVERIFY(compareDoubles(distToMid, 2500.0, 0.1));
    QVERIFY(compareDoubles(distFromMid, 2500.0, 0.1));
    // Test altitude interpolation at midpoint
    const QGeoCoordinate startAlt(m_origin.latitude(), m_origin.longitude(), 0.0);
    const QGeoCoordinate endAlt(dest.latitude(), dest.longitude(), 100.0);
    const auto midAlt = QGCGeo::interpolateAtDistance(startAlt, endAlt, 2500.0);
    QVERIFY(compareDoubles(midAlt.altitude(), 50.0, 0.1));
    // Test same start and end
    const auto same = QGCGeo::interpolateAtDistance(m_origin, m_origin, 100.0);
    QCOMPARE(same, m_origin);
}

void GeoTest::_distanceProperties_test()
{
    RC_QT_PROP("distance is always non-negative", [] {
        const auto lat1 = *rc::qgc::latitude();
        const auto lon1 = *rc::qgc::longitude();
        const auto lat2 = *rc::qgc::latitude();
        const auto lon2 = *rc::qgc::longitude();

        const QGeoCoordinate c1(lat1, lon1);
        const QGeoCoordinate c2(lat2, lon2);

        const double distance = QGCGeo::geodesicDistance(c1, c2);
        RC_ASSERT(distance >= 0.0);
    });

    RC_QT_PROP("distance is symmetric", [] {
        const auto lat1 = *rc::qgc::latitude();
        const auto lon1 = *rc::qgc::longitude();
        const auto lat2 = *rc::qgc::latitude();
        const auto lon2 = *rc::qgc::longitude();

        const QGeoCoordinate c1(lat1, lon1);
        const QGeoCoordinate c2(lat2, lon2);

        const double d1 = QGCGeo::geodesicDistance(c1, c2);
        const double d2 = QGCGeo::geodesicDistance(c2, c1);

        RC_ASSERT(qAbs(d1 - d2) < 0.001);
    });

    RC_QT_PROP("distance to self is zero", [] {
        const auto lat = *rc::qgc::latitude();
        const auto lon = *rc::qgc::longitude();

        const QGeoCoordinate c(lat, lon);
        const double distance = QGCGeo::geodesicDistance(c, c);

        RC_ASSERT(qAbs(distance) < 0.001);
    });
}

void GeoTest::_nedRoundtripProperty_test()
{
    RC_QT_PROP("NED conversion roundtrips", [] {
        // Use origin near the test coordinate
        const QGeoCoordinate origin(47.0, 8.0, 0.0);

        // Generate coordinate close to origin (within ~10km)
        const auto latOffset = *rc::gen::inRange(-100, 100);
        const auto lonOffset = *rc::gen::inRange(-100, 100);
        const auto alt = *rc::gen::inRange(0, 1000);

        const double lat = origin.latitude() + (latOffset / 1000.0);
        const double lon = origin.longitude() + (lonOffset / 1000.0);
        const QGeoCoordinate original(lat, lon, alt);

        // Convert to NED
        double n = 0, e = 0, d = 0;
        QGCGeo::convertGeoToNed(original, origin, n, e, d);

        // Convert back to Geo
        QGeoCoordinate result;
        QGCGeo::convertNedToGeo(n, e, d, origin, result);

        // Should match within reasonable tolerance
        RC_ASSERT(result.isValid());
        RC_ASSERT(qAbs(result.latitude() - original.latitude()) < 0.0001);
        RC_ASSERT(qAbs(result.longitude() - original.longitude()) < 0.0001);
        RC_ASSERT(qAbs(result.altitude() - original.altitude()) < 0.1);
    });
}

void GeoTest::_benchmarkCoordinateConversions()
{
    const QGeoCoordinate coord(47.364869, 8.594398, 100.0);
    double n = 0, e = 0, d = 0;
    QGeoCoordinate result;

    auto bench = qgc::bench::ciConfig();
    bench.relative(true);

    bench.run("convertGeoToNed", [&] {
        QGCGeo::convertGeoToNed(coord, m_origin, n, e, d);
        ankerl::nanobench::doNotOptimizeAway(n);
    });

    bench.run("convertNedToGeo", [&] {
        QGCGeo::convertNedToGeo(n, e, d, m_origin, result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("geodesicDistance", [&] {
        const double dist = QGCGeo::geodesicDistance(m_origin, coord);
        ankerl::nanobench::doNotOptimizeAway(dist);
    });

    bench.run("geodesicAzimuth", [&] {
        const double az = QGCGeo::geodesicAzimuth(m_origin, coord);
        ankerl::nanobench::doNotOptimizeAway(az);
    });

    bench.run("geodesicDestination", [&] {
        const auto dest = QGCGeo::geodesicDestination(m_origin, 45.0, 1000.0);
        ankerl::nanobench::doNotOptimizeAway(dest);
    });
}

void GeoTest::_qbenchmarkGeodesicDistance()
{
    const QGeoCoordinate coord(47.364869, 8.594398, 100.0);
    double dist = 0;

    QBENCHMARK {
        dist = QGCGeo::geodesicDistance(m_origin, coord);
        QT_BENCHMARK_KEEP(dist);
    }

    // Verify result is valid
    QVERIFY(dist > 0);
}

UT_REGISTER_TEST(GeoTest, TestLabel::Unit, TestLabel::Utilities)
