#pragma once

// Split from UnitTest.h so QtPositioning isn't pulled into every test TU.

#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

inline void _qCompareCoords(const QGeoCoordinate &actual, const QGeoCoordinate &expected, double toleranceMeters = 1.0)
{
    const double tolerance = (toleranceMeters > 0.0) ? toleranceMeters : 1.0;
    const double distance = actual.distanceTo(expected);
    if (distance > tolerance) {
        const QString message = QStringLiteral(
                                    "Coordinates differ by %1m (tolerance: %2m)\n"
                                    "  Actual:   (%3, %4, %5)\n"
                                    "  Expected: (%6, %7, %8)")
                                    .arg(distance, 0, 'f', 3)
                                    .arg(tolerance, 0, 'f', 3)
                                    .arg(actual.latitude(), 0, 'f', 7)
                                    .arg(actual.longitude(), 0, 'f', 7)
                                    .arg(actual.altitude(), 0, 'f', 2)
                                    .arg(expected.latitude(), 0, 'f', 7)
                                    .arg(expected.longitude(), 0, 'f', 7)
                                    .arg(expected.altitude(), 0, 'f', 2);
        QFAIL(qPrintable(message));
    }
}

/// Compare two QGeoCoordinates with meter tolerance (default 1.0m)
#define QCOMPARE_COORDS(...) _qCompareCoords(__VA_ARGS__)

/// Verify coordinates are equal within tolerance (less verbose than QCOMPARE_COORDS)
#define QVERIFY_COORDS_NEAR(actual, expected, toleranceMeters)   \
    QVERIFY2((actual).distanceTo(expected) <= (toleranceMeters), \
             qPrintable(QString("Coordinates differ by %1m").arg((actual).distanceTo(expected))))

namespace QTest {
// Readable QCOMPARE diagnostics for QGeoCoordinate.
template <>
inline char* toString(const QGeoCoordinate& c)
{
    return qstrdup(qPrintable(QStringLiteral("(%1, %2, %3)")
                                  .arg(c.latitude(), 0, 'f', 7)
                                  .arg(c.longitude(), 0, 'f', 7)
                                  .arg(c.altitude(), 0, 'f', 2)));
}
}  // namespace QTest
