#include "VehicleGPSFactGroupTest.h"

#include <memory>

#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "Support/GPSTestHelpers.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

namespace {

mavlink_message_t gpsMessage(bool secondary, uint16_t yaw, uint8_t fixType)
{
    using GPSTest::GPSReceiverIndex;
    return GPSTest::gpsRawMessage({.latitudeE7 = 470000000,
                                   .longitudeE7 = 80000000,
                                   .altitudeMm = 123456,
                                   .fixType = fixType,
                                   .eph = 120,
                                   .epv = 240,
                                   .cog = 9000,
                                   .satellitesVisible = 18,
                                   .yaw = yaw,
                                   .horizontalAccuracyMm = 3500,
                                   .verticalAccuracyMm = 6000},
                                  secondary ? GPSReceiverIndex::Secondary : GPSReceiverIndex::Primary);
}

mavlink_message_t rtkMessage(bool secondary)
{
    const auto fill = [](auto& rtk) {
        rtk.baseline_a_mm = 3000;
        rtk.baseline_b_mm = -4000;
        rtk.baseline_c_mm = 0;
        rtk.rtk_rate = 1;
        rtk.nsats = 17;
    };
    mavlink_message_t message{};
    if (secondary) {
        mavlink_gps2_rtk_t rtk{};
        fill(rtk);
        mavlink_msg_gps2_rtk_encode(1, 1, &message, &rtk);
    } else {
        mavlink_gps_rtk_t rtk{};
        fill(rtk);
        mavlink_msg_gps_rtk_encode(1, 1, &message, &rtk);
    }
    return message;
}

std::unique_ptr<VehicleGPSFactGroup> receiver(bool secondary, RuntimeScheduler* scheduler)
{
    using Index = VehicleGPSFactGroup::ReceiverIndex;
    return std::make_unique<VehicleGPSFactGroup>(nullptr, scheduler, secondary ? Index::Secondary : Index::Primary);
}

}  // namespace

void VehicleGPSFactGroupTest::_rawNormalization_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::addColumn<uint16_t>("yaw");
    QTest::addColumn<double>("expected");
    for (const bool secondary : {false, true}) {
        const QByteArray prefix = secondary ? "gps2-" : "gps1-";
        QTest::newRow((prefix + "unavailable").constData()) << secondary << uint16_t(0) << qQNaN();
        QTest::newRow((prefix + "no-solution").constData()) << secondary << uint16_t(UINT16_MAX) << qQNaN();
        QTest::newRow((prefix + "north").constData()) << secondary << uint16_t(36000) << 0.0;
        QTest::newRow((prefix + "east").constData()) << secondary << uint16_t(9000) << 90.0;
    }
}

void VehicleGPSFactGroupTest::_rawNormalization()
{
    QFETCH(bool, secondary);
    QFETCH(uint16_t, yaw);
    QFETCH(double, expected);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QVERIFY(qIsNaN(gps->yaw()->rawValue().toDouble()));
    gps->handleMessage(nullptr, gpsMessage(secondary, yaw, GPS_FIX_TYPE_3D_FIX));
    if (qIsNaN(expected)) {
        QVERIFY(qIsNaN(gps->yaw()->rawValue().toDouble()));
    } else {
        QCOMPARE(gps->yaw()->rawValue().toDouble(), expected);
    }
    QCOMPARE(gps->hdop()->rawValue().toDouble(), 1.2);
    QCOMPARE(gps->vdop()->rawValue().toDouble(), 2.4);
    QCOMPARE(gps->horizontalAccuracy()->rawValue().toDouble(), 3.5);
    QCOMPARE(gps->verticalAccuracy()->rawValue().toDouble(), 6.0);
    QCOMPARE(gps->count()->rawValue().toInt(), 18);
}

void VehicleGPSFactGroupTest::_positionReported_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::newRow("gps1") << false;
    QTest::newRow("gps2") << true;
}

void VehicleGPSFactGroupTest::_positionReported()
{
    QFETCH(bool, secondary);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QSignalSpy reports(gps.get(), &VehicleGPSFactGroup::positionReported);
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_RTK_FIXED));
    QCOMPARE(reports.count(), 1);
    const auto position = reports.first().at(0).value<QGeoPositionInfo>();
    QCOMPARE(position.coordinate(), QGeoCoordinate(47, 8, 123.456));
    QVERIFY(position.timestamp().isValid());
    QCOMPARE(position.attribute(QGeoPositionInfo::HorizontalAccuracy), 3.5);
    QCOMPARE(position.attribute(QGeoPositionInfo::Direction), 90.0);
    QCOMPARE(reports.first().at(1).toInt(), int(GPS_FIX_TYPE_RTK_FIXED));
    // An unchanged report still reaches observers.
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_RTK_FIXED));
    QCOMPARE(reports.count(), 2);
}

void VehicleGPSFactGroupTest::_highLatency2KeepsReceiverState()
{
    ManualScheduler scheduler;
    VehicleGPSFactGroup gps(nullptr, &scheduler);
    gps.handleMessage(nullptr, gpsMessage(false, 9000, GPS_FIX_TYPE_3D_FIX));
    QSignalSpy reports(&gps, &VehicleGPSFactGroup::positionReported);
    mavlink_high_latency2_t raw{};
    raw.latitude = 480000000;
    raw.longitude = 90000000;
    raw.altitude = 123;
    raw.eph = 123;
    raw.epv = 234;
    mavlink_message_t message{};
    mavlink_msg_high_latency2_encode(1, 1, &message, &raw);
    gps.handleMessage(nullptr, message);
    QCOMPARE(gps.lat()->rawValue().toDouble(), 48.0);
    QCOMPARE(gps.horizontalAccuracy()->rawValue().toDouble(), 12.3);
    QCOMPARE(gps.verticalAccuracy()->rawValue().toDouble(), 23.4);
    // The message carries no receiver state, so the last GPS report's remains.
    QCOMPARE(gps.lock()->rawValue().toInt(), int(GPS_FIX_TYPE_3D_FIX));
    QCOMPARE(gps.count()->rawValue().toInt(), 18);
    QCOMPARE(gps.courseOverGround()->rawValue().toDouble(), 90.0);
    QCOMPARE(gps.yaw()->rawValue().toDouble(), 90.0);
    QCOMPARE(gps.hdop()->rawValue().toDouble(), 1.2);
    QCOMPARE(gps.vdop()->rawValue().toDouble(), 2.4);
    // Its estimated position does not count as a GPS fix.
    QCOMPARE(reports.count(), 1);
    QCOMPARE(reports.first().at(1).toInt(), int(GPS_FIX_TYPE_NO_GPS));
}

void VehicleGPSFactGroupTest::_receiverDispatch_data()
{
    _positionReported_data();
}

void VehicleGPSFactGroupTest::_receiverDispatch()
{
    QFETCH(bool, secondary);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QSignalSpy reports(gps.get(), &VehicleGPSFactGroup::positionReported);
    gps->handleMessage(nullptr, gpsMessage(!secondary, 0, GPS_FIX_TYPE_3D_FIX));
    QVERIFY(reports.isEmpty());
    QVERIFY(qIsNaN(gps->lat()->rawValue().toDouble()));
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_3D_FIX));
    QCOMPARE(reports.count(), 1);

    mavlink_high_latency_t highLatency{};
    highLatency.latitude = 480000000;
    highLatency.longitude = 90000000;
    highLatency.altitude_amsl = 300;
    highLatency.gps_fix_type = GPS_FIX_TYPE_3D_FIX;
    mavlink_message_t message{};
    mavlink_msg_high_latency_encode(1, 1, &message, &highLatency);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->lat()->rawValue().toDouble(), secondary ? 47.0 : 48.0);
    mavlink_high_latency2_t highLatency2{};
    highLatency2.latitude = 490000000;
    highLatency2.longitude = 100000000;
    highLatency2.altitude = 400;
    mavlink_msg_high_latency2_encode(1, 1, &message, &highLatency2);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->lat()->rawValue().toDouble(), secondary ? 47.0 : 49.0);

    mavlink_gnss_integrity_t integrity{};
    integrity.id = secondary ? 0 : 1;
    integrity.jamming_state = 3;
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->jammingState()->rawValue().toInt(), 255);
    integrity.id = secondary ? 1 : 0;
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->jammingState()->rawValue().toInt(), 3);
}

void VehicleGPSFactGroupTest::_systemErrorText_data()
{
    QTest::addColumn<quint32>("errors");
    QTest::addColumn<QString>("expected");
    QTest::newRow("none") << 0u << QString();
    QTest::newRow("antenna") << 8u << VehicleGPSFactGroup::tr("Antenna");
    // Several errors are listed together rather than collapsed.
    QTest::newRow("combined") << (1u | 32u)
                              << VehicleGPSFactGroup::tr("Incoming correction") + QStringLiteral(", ") +
                                     VehicleGPSFactGroup::tr("CPU overload");
    QTest::newRow("unknown-bit") << 0x100u << VehicleGPSFactGroup::tr("Other (0x%1)").arg(QStringLiteral("100"));
}

void VehicleGPSFactGroupTest::_systemErrorText()
{
    QFETCH(quint32, errors);
    QFETCH(QString, expected);
    VehicleGPSFactGroup gps;
    QSignalSpy changed(&gps, &VehicleGPSFactGroup::systemErrorTextChanged);
    gps.systemErrors()->setRawValue(errors);
    QCOMPARE(gps.systemErrorText(), expected);
    QCOMPARE(gps.property("systemErrorText").toString(), expected);
    QCOMPARE(changed.size(), errors ? 1 : 0);
}

void VehicleGPSFactGroupTest::_resilienceSummary_data()
{
    using Auth = VehicleGPSFactGroup::AuthenticationState;
    QTest::addColumn<int>("spoofing");
    QTest::addColumn<int>("jamming");
    QTest::addColumn<int>("authentication");
    QTest::addColumn<int>("interference");
    QTest::addColumn<int>("severity");
    // 0 (Unknown) and 255 (not reported) mean the receiver does not know.
    constexpr int NONE = VehicleGPSFactGroup::NOT_REPORTED;
    QTest::newRow("not-reported") << NONE << NONE << NONE << 0 << 0;
    QTest::newRow("unknown") << 0 << 0 << int(Auth::Unknown) << 0 << 0;
    QTest::newRow("worse-of-two") << 2 << 3 << int(Auth::Disabled) << 3 << 1;
    QTest::newRow("spoofing-only") << 1 << NONE << int(Auth::Initializing) << 1 << 2;
    QTest::newRow("authentication-ok") << NONE << 1 << int(Auth::Ok) << 1 << 3;
    QTest::newRow("authentication-error") << NONE << NONE << int(Auth::Error) << 0 << 4;
}

void VehicleGPSFactGroupTest::_resilienceSummary()
{
    QFETCH(int, spoofing);
    QFETCH(int, jamming);
    QFETCH(int, authentication);
    QFETCH(int, interference);
    QFETCH(int, severity);
    VehicleGPSFactGroup gps;
    QSignalSpy changed(&gps, &VehicleGPSFactGroup::resilienceChanged);
    gps.spoofingState()->setRawValue(spoofing);
    gps.jammingState()->setRawValue(jamming);
    gps.authenticationState()->setRawValue(authentication);
    QCOMPARE(gps.spoofingReported(), VehicleGPSFactGroup::reported(spoofing));
    QCOMPARE(gps.jammingReported(), VehicleGPSFactGroup::reported(jamming));
    QCOMPARE(gps.authenticationReported(), VehicleGPSFactGroup::reported(authentication));
    QCOMPARE(gps.interferenceState(), interference);
    QCOMPARE(gps.authenticationSeverity(), severity);
    QCOMPARE(changed.isEmpty(), spoofing == 255 && jamming == 255 && authentication == 255);
}

void VehicleGPSFactGroupTest::_integrityExpires()
{
    using namespace std::chrono_literals;
    ManualScheduler scheduler;
    auto gps = receiver(false, &scheduler);
    mavlink_gnss_integrity_t integrity{};
    integrity.id = 0;
    integrity.system_errors = 8;
    integrity.spoofing_state = 1;
    integrity.jamming_state = 3;
    integrity.authentication_state = 3;
    integrity.corrections_quality = 7;
    mavlink_message_t message{};
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->interferenceState(), 3);
    QVERIFY(scheduler.advanceBy(VehicleGPSFactGroup::GNSS_INTEGRITY_TIMEOUT - 1ms));
    // Each report restarts the timeout.
    gps->handleMessage(nullptr, message);
    QVERIFY(scheduler.advanceBy(VehicleGPSFactGroup::GNSS_INTEGRITY_TIMEOUT - 1ms));
    QCOMPARE(gps->jammingState()->rawValue().toInt(), 3);
    QSignalSpy changed(gps.get(), &VehicleGPSFactGroup::resilienceChanged);
    QVERIFY(scheduler.advanceBy(1ms));
    QVERIFY(!changed.isEmpty());
    QVERIFY(!gps->jammingReported() && !gps->spoofingReported() && !gps->authenticationReported());
    QCOMPARE(gps->interferenceState(), 0);
    QCOMPARE(gps->systemErrors()->rawValue().toUInt(), 0u);
    QVERIFY(gps->systemErrorText().isEmpty());
    QCOMPARE(gps->correctionsQuality()->rawValue().toInt(), VehicleGPSFactGroup::NOT_REPORTED);
}

void VehicleGPSFactGroupTest::_schedulerDestruction()
{
    auto scheduler = std::make_unique<ManualScheduler>();
    auto gps = receiver(false, scheduler.get());
    scheduler.reset();
    // Without a scheduler the reports still apply; they only stop expiring.
    mavlink_gnss_integrity_t integrity{};
    integrity.id = 0;
    integrity.jamming_state = 2;
    mavlink_message_t message{};
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->jammingState()->rawValue().toInt(), 2);
    gps->handleMessage(nullptr, rtkMessage(false));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), 17);
}

void VehicleGPSFactGroupTest::_rtkStatus_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::newRow("primary") << false;
    QTest::newRow("secondary") << true;
}

void VehicleGPSFactGroupTest::_rtkStatus()
{
    QFETCH(bool, secondary);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QVERIFY(qIsNaN(gps->rtkBaseline()->rawValue().toDouble()));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), -1);
    gps->handleMessage(nullptr, rtkMessage(!secondary));
    QVERIFY(qIsNaN(gps->rtkBaseline()->rawValue().toDouble()));
    gps->handleMessage(nullptr, rtkMessage(secondary));
    QCOMPARE(gps->rtkBaseline()->rawValue().toDouble(), 5.0);
    QCOMPARE(gps->rtkRate()->rawValue().toDouble(), 1.0);
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), 17);
    QVERIFY(scheduler.advanceBy(VehicleGPSFactGroup::RTK_STATUS_TIMEOUT - std::chrono::milliseconds(1)));
    gps->handleMessage(nullptr, rtkMessage(secondary));
    QVERIFY(scheduler.advanceBy(VehicleGPSFactGroup::RTK_STATUS_TIMEOUT - std::chrono::milliseconds(1)));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), 17);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(qIsNaN(gps->rtkBaseline()->rawValue().toDouble()));
    QVERIFY(qIsNaN(gps->rtkRate()->rawValue().toDouble()));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), -1);
}

UT_REGISTER_TEST(VehicleGPSFactGroupTest, TestLabel::Unit)
