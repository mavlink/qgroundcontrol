#pragma once

// Split from UnitTest.h so QtPositioning isn't pulled into every test TU.

#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

/// Compare two QGeoCoordinates with meter tolerance (default 1.0m)
#define QCOMPARE_COORDS(actual, expected, ...)                                                                 \
    do {                                                                                                       \
        const QGeoCoordinate _actual = (actual);                                                               \
        const QGeoCoordinate _expected = (expected);                                                           \
        const double _tolerance = (0.0 __VA_OPT__(+) __VA_ARGS__) > 0 ? (0.0 __VA_OPT__(+) __VA_ARGS__) : 1.0; \
        const double _distance = _actual.distanceTo(_expected);                                                \
        if (_distance > _tolerance) {                                                                          \
            const QString _msg = QString(                                                                      \
                                     "Coordinates differ by %1m (tolerance: %2m)\n"                            \
                                     "  Actual:   (%3, %4, %5)\n"                                              \
                                     "  Expected: (%6, %7, %8)")                                               \
                                     .arg(_distance, 0, 'f', 3)                                                \
                                     .arg(_tolerance, 0, 'f', 3)                                               \
                                     .arg(_actual.latitude(), 0, 'f', 7)                                       \
                                     .arg(_actual.longitude(), 0, 'f', 7)                                      \
                                     .arg(_actual.altitude(), 0, 'f', 2)                                       \
                                     .arg(_expected.latitude(), 0, 'f', 7)                                     \
                                     .arg(_expected.longitude(), 0, 'f', 7)                                    \
                                     .arg(_expected.altitude(), 0, 'f', 2);                                    \
            QFAIL(qPrintable(_msg));                                                                           \
        }                                                                                                      \
    } while (false)

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
