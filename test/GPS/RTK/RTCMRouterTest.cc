#include "RTCMRouterTest.h"
#include "RTCMRouter.h"
#include "RTCMMavlink.h"
#include "UnitTest.h"

void RTCMRouterTest::testRouteToVehicles()
{
    RTCMMavlink mavlink;
    RTCMRouter router(&mavlink);

    const QByteArray data = QByteArrayLiteral("test rtcm data");
    router.routeToVehicles(data);
}

void RTCMRouterTest::testRouteToBaseStation()
{
    RTCMMavlink mavlink;
    RTCMRouter router(&mavlink);

    // No GPS RTK set — should not crash
    const QByteArray data = QByteArrayLiteral("test rtcm data");
    router.routeToBaseStation(data);
}

void RTCMRouterTest::testRouteAll()
{
    RTCMMavlink mavlink;
    RTCMRouter router(&mavlink);

    const QByteArray data = QByteArrayLiteral("test rtcm data");
    // Should route to both without crashing
    router.routeAll(data);
}

void RTCMRouterTest::testNullSafety()
{
    RTCMRouter router(nullptr);
    router.routeToVehicles(QByteArrayLiteral("data"));
    router.routeToBaseStation(QByteArrayLiteral("data"));
    router.routeAll(QByteArrayLiteral("data"));
}

UT_REGISTER_TEST(RTCMRouterTest, TestLabel::Unit)
