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

/// QCOMPARE_COORDS with a tolerance that has to be given; 0 or less stands for 1 m, as there. It is written out
/// instead of expanding QCOMPARE_COORDS, whose __VA_OPT__ MSVC only takes with /Zc:preprocessor: the portable
/// tests, which are built without that option, use this macro.
#define QVERIFY_COORDS_NEAR(actual, expected, toleranceMeters)                                                   \
    do {                                                                                                         \
        QString _qcn_msg;                                                                                        \
        QVERIFY2(QTest::compareCoords((actual), (expected), (toleranceMeters), _qcn_msg), qPrintable(_qcn_msg)); \
    } while (false)

namespace QTest {

inline bool compareCoords(const QGeoCoordinate &actual, const QGeoCoordinate &expected, double toleranceMeters, QString &outMessage)
{
    const double tolerance = (toleranceMeters > 0.0) ? toleranceMeters : 1.0;
    // distanceTo() is 0 for a coordinate that is not valid, which is near no other
    const bool comparable = actual.isValid() && expected.isValid();
    const double distance = actual.distanceTo(expected);
    if (!comparable || (distance > tolerance)) {
        outMessage = QStringLiteral(
                         "Coordinates %1 (tolerance: %2m)\n"
                         "  Actual:   (%3, %4, %5)\n"
                         "  Expected: (%6, %7, %8)")
                         .arg(comparable ? QStringLiteral("differ by %1m").arg(distance, 0, 'f', 3)
                                         : QStringLiteral("are not both valid"))
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
