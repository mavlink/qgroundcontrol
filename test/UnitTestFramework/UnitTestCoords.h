#pragma once

// Split from UnitTest.h so QtPositioning isn't pulled into every test TU.

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

/// Compare two QGeoCoordinates with meter tolerance (default 1.0m)
#define QCOMPARE_COORDS(actual, expected, ...) \
    do { \
        const double _qcc_tol = [] { \
            const double _v = (0.0 __VA_OPT__(+) __VA_ARGS__); \
            return _v > 0.0 ? _v : 1.0; \
        }(); \
        QString _qcc_msg; \
        QVERIFY2(QTest::compareCoords((actual), (expected), _qcc_tol, _qcc_msg), qPrintable(_qcc_msg)); \
    } while (false)

/// Verify coordinates are equal within tolerance (less verbose than QCOMPARE_COORDS)
#define QVERIFY_COORDS_NEAR(actual, expected, toleranceMeters)   \
    QVERIFY2((actual).distanceTo(expected) <= (toleranceMeters), \
             qPrintable(QString("Coordinates differ by %1m").arg((actual).distanceTo(expected))))

namespace QTest {

inline bool compareCoords(const QGeoCoordinate &actual, const QGeoCoordinate &expected, double toleranceMeters, QString &outMessage)
{
    const double tolerance = (toleranceMeters > 0.0) ? toleranceMeters : 1.0;
    const double distance = actual.distanceTo(expected);
    if (distance > tolerance) {
        outMessage = QStringLiteral(
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
        return false;
    }
    return true;
}

inline bool compareCoords(const QGeoCoordinate &actual, const QGeoCoordinate &expected, double toleranceMeters = 1.0)
{
    QString message;
    const bool ok = compareCoords(actual, expected, toleranceMeters, message);
    if (!ok) {
        qDebug("%s", qPrintable(message));
    }
    return ok;
}

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
