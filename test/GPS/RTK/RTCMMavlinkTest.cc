#include "RTCMMavlinkTest.h"
#include "RTCMMavlink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <mavlink_types.h>
#include <common/mavlink_msg_gps_rtcm_data.h>

void RTCMMavlinkTest::testInitialState()
{
    RTCMMavlink mavlink;

    QCOMPARE(mavlink.bandwidthKBps(), 0.0);
    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(0));
}

void RTCMMavlinkTest::testTotalBytesSent()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&sent](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    const QByteArray data(50, 'A');
    mavlink.RTCMDataUpdate(data);

    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(50));
    QVERIFY(!sent.isEmpty());
}

void RTCMMavlinkTest::testSmallMessageNotFragmented()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&sent](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    const QByteArray data(100, 'B');
    mavlink.RTCMDataUpdate(data);

    QCOMPARE(sent.size(), 1);
    QCOMPARE(sent[0].len, static_cast<uint8_t>(100));
    QCOMPARE(sent[0].flags & 0x01, 0);
}

void RTCMMavlinkTest::testLargeMessageFragmented()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&sent](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    const QByteArray data(MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN + 50, 'C');
    mavlink.RTCMDataUpdate(data);

    QVERIFY(sent.size() > 1);

    for (const auto &msg : sent) {
        QCOMPARE(msg.flags & 0x01, 1);
    }

    int totalLen = 0;
    for (const auto &msg : sent) {
        totalLen += msg.len;
    }
    QCOMPARE(totalLen, data.size());
}

void RTCMMavlinkTest::testSequenceIdIncrements()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&sent](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    const QByteArray data(50, 'D');
    mavlink.RTCMDataUpdate(data);
    mavlink.RTCMDataUpdate(data);

    QCOMPARE(sent.size(), 2);
    const uint8_t seq0 = (sent[0].flags >> 3) & 0x1F;
    const uint8_t seq1 = (sent[1].flags >> 3) & 0x1F;
    QCOMPARE(seq1, static_cast<uint8_t>((seq0 + 1) & 0x1F));
}

UT_REGISTER_TEST(RTCMMavlinkTest, TestLabel::Unit)
