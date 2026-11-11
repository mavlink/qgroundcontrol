#include "GeoidTest.h"

#include <cmath>
#include <limits>

#include <QtCore/QString>
#include <QtTest/QTest>

#include "QGCGeoid.h"

namespace {

enum class GeoidReferenceKind
{
    Node,
    Sample,
    Steep,
};

struct GeoidReference
{
    GeoidReferenceKind kind;
    double latitude;
    double longitude;
    double separation;
};

// Points of GeographicLib's egm96-5 dataset, written by src/Utilities/Geo/generate_egm96_grid.py.
#define EGM96_REFERENCE(kind, latitude, longitude, separation) \
    {GeoidReferenceKind::kind, latitude, longitude, separation},
constexpr GeoidReference EGM96_REFERENCES[] = {
#include "EGM96Reference.inc"
};
#undef EGM96_REFERENCE

// The grid stores centimeters.
constexpr double GEOID_GRID_ROUNDING_METERS = 0.006;

double geoidReferenceError(const GeoidReference& reference)
{
    return std::abs(QGCGeo::geoidSeparation(reference.latitude, reference.longitude) - reference.separation);
}

QByteArray geoidReferenceText(const GeoidReference& reference)
{
    return QStringLiteral("%1, %2: expected %3, got %4")
        .arg(reference.latitude)
        .arg(reference.longitude)
        .arg(reference.separation)
        .arg(QGCGeo::geoidSeparation(reference.latitude, reference.longitude))
        .toUtf8();
}

}  // namespace

void GeoidTest::_reproducesGridPoints()
{
    // Whole and half degrees, among them 19.5N 155.5W on Hawaii, which a 1 degree grid misses by 13.7 m.
    int nodes = 0;
    for (const GeoidReference& reference : EGM96_REFERENCES) {
        if (reference.kind == GeoidReferenceKind::Node) {
            ++nodes;
            QVERIFY2(geoidReferenceError(reference) <= GEOID_GRID_ROUNDING_METERS,
                     geoidReferenceText(reference).constData());
        }
    }
    QVERIFY(nodes > 0);
}

void GeoidTest::_staysWithinStatedError()
{
    // Against all 9.3 million points of the dataset the grid is off by 0.14 m RMS and by 3.93 m at most, at
    // 10.75N 73.75W, which is one of the steep reference points. The sampled points are off by 0.126 m RMS and by
    // 0.70 m at most. The limits leave a margin of 0.07 m on the maximum, 0.3 m on a sampled point and 0.024 m on
    // the RMS; the grid itself rounds to 0.005 m.
    constexpr double MAXIMUM_ERROR_METERS = 4.0;
    constexpr double SAMPLE_MAXIMUM_ERROR_METERS = 1.0;
    constexpr double SAMPLE_RMS_METERS = 0.15;
    double squares = 0.0;
    int samples = 0;
    for (const GeoidReference& reference : EGM96_REFERENCES) {
        const bool sampled = reference.kind == GeoidReferenceKind::Sample;
        const double error = geoidReferenceError(reference);
        QVERIFY2(error <= (sampled ? SAMPLE_MAXIMUM_ERROR_METERS : MAXIMUM_ERROR_METERS),
                 geoidReferenceText(reference).constData());
        if (sampled) {
            squares += error * error;
            ++samples;
        }
    }
    QVERIFY(samples > 0);
    const double rms = std::sqrt(squares / samples);
    QVERIFY2(rms <= SAMPLE_RMS_METERS, qPrintable(QString::number(rms)));
}

void GeoidTest::_wrapsLongitude_data()
{
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::newRow("date-line") << 10.5 << 180.0;
    QTest::newRow("last-cell") << -40.25 << 179.75;
    QTest::newRow("greenwich") << 51.5 << 0.0;
    QTest::newRow("west") << -23.4 << -46.6;
}

void GeoidTest::_wrapsLongitude()
{
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    const double separation = QGCGeo::geoidSeparation(latitude, longitude);
    QVERIFY(std::isfinite(separation));
    for (const double turns : {-2.0, -1.0, 1.0, 2.0}) {
        QVERIFY(std::abs(QGCGeo::geoidSeparation(latitude, longitude + turns * 360.0) - separation) < 1e-6);
    }
}

void GeoidTest::_isContinuousAtTheDateLine()
{
    for (double latitude = -90.0; latitude <= 90.0; latitude += 7.5) {
        const double west = QGCGeo::geoidSeparation(latitude, std::nextafter(180.0, 0.0));
        QVERIFY(std::abs(west - QGCGeo::geoidSeparation(latitude, -180.0)) < 1e-6);
    }
}

void GeoidTest::_polesHaveOneValue_data()
{
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("separation");
    QTest::newRow("north") << 90.0 << 13.605;
    QTest::newRow("south") << -90.0 << -29.535;
}

void GeoidTest::_polesHaveOneValue()
{
    QFETCH(double, latitude);
    QFETCH(double, separation);
    for (const double longitude : {-180.0, -77.3, 0.0, 12.5, 179.99}) {
        QVERIFY(std::abs(QGCGeo::geoidSeparation(latitude, longitude) - separation) <= GEOID_GRID_ROUNDING_METERS);
    }
}

void GeoidTest::_rejectsInvalidCoordinates_data()
{
    constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
    constexpr double INFINITE = std::numeric_limits<double>::infinity();
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::newRow("latitude-nan") << NOT_A_NUMBER << 8.0;
    QTest::newRow("longitude-nan") << 47.0 << NOT_A_NUMBER;
    QTest::newRow("latitude-infinite") << INFINITE << 8.0;
    QTest::newRow("longitude-infinite") << 47.0 << -INFINITE;
    QTest::newRow("beyond-north-pole") << 90.001 << 8.0;
    QTest::newRow("beyond-south-pole") << -91.0 << 8.0;
}

void GeoidTest::_rejectsInvalidCoordinates()
{
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QVERIFY(std::isnan(QGCGeo::geoidSeparation(latitude, longitude)));
}

QGC_REGISTER_PORTABLE_TEST(GeoidTest, TestLabel::Unit, TestLabel::Utilities)
