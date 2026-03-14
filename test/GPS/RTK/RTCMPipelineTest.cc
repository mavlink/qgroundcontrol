#include "RTCMPipelineTest.h"
#include "RTCMMavlink.h"
#include "RTCMRouter.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <mavlink_types.h>
#include <common/mavlink_msg_gps_rtcm_data.h>

void RTCMPipelineTest::testRouteToVehicles()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&sent](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    RTCMRouter router(&mavlink);

    const QByteArray data(100, 'R');
    router.routeToVehicles(data);

    QVERIFY(!sent.isEmpty());
    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(100));
}

void RTCMPipelineTest::testRouteAll()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&sent](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    RTCMRouter router(&mavlink);

    const QByteArray data(50, 'X');
    router.routeAll(data);

    QVERIFY(!sent.isEmpty());
    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(50));
}

UT_REGISTER_TEST(RTCMPipelineTest, TestLabel::Unit)
