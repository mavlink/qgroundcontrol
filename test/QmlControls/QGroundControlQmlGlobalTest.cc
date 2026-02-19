#include "QGroundControlQmlGlobalTest.h"

#include "QGroundControlQmlGlobal.h"

#include <QtPositioning/QGeoCoordinate>

#include <limits>

void QGroundControlQmlGlobalTest::_testPositionManagerAccessor()
{
    QGroundControlQmlGlobal global;

    QVERIFY(global.qgcPositionManager() != nullptr);
}

void QGroundControlQmlGlobalTest::_testFlightMapPositionValidation()
{
    QGroundControlQmlGlobal global;

    const QGeoCoordinate oldPosition = QGroundControlQmlGlobal::flightMapPosition();

    const QGeoCoordinate validCoordinate(47.397742, 8.545594, 488.0);
    global.setFlightMapPosition(validCoordinate);
    QCOMPARE(QGroundControlQmlGlobal::flightMapPosition(), validCoordinate);

    const QGeoCoordinate invalidCoordinate;
    global.setFlightMapPosition(invalidCoordinate);
    QCOMPARE(QGroundControlQmlGlobal::flightMapPosition(), validCoordinate);

    global.setFlightMapPosition(oldPosition);
}

void QGroundControlQmlGlobalTest::_testFlightMapZoomValidation()
{
    QGroundControlQmlGlobal global;

    const double oldZoom = QGroundControlQmlGlobal::flightMapZoom();

    const double validZoom = 15.0;
    global.setFlightMapZoom(validZoom);
    QCOMPARE(QGroundControlQmlGlobal::flightMapZoom(), validZoom);

    global.setFlightMapZoom(-1.0);
    QCOMPARE(QGroundControlQmlGlobal::flightMapZoom(), validZoom);

    global.setFlightMapZoom(std::numeric_limits<double>::quiet_NaN());
    QCOMPARE(QGroundControlQmlGlobal::flightMapZoom(), validZoom);

    if (oldZoom > 0.0) {
        global.setFlightMapZoom(oldZoom);
    }
}

#include "UnitTest.h"

UT_REGISTER_TEST(QGroundControlQmlGlobalTest, TestLabel::Unit, TestLabel::QmlControls)
