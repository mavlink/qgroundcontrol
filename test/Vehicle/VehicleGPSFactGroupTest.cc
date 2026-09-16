#include "VehicleGPSFactGroupTest.h"

#include <array>

#include <QtTest/QSignalSpy>

#include "GPSPositionPolicy.h"
#include "MAVLinkLib.h"
#include "MonotonicClock.h"
#include "QGCGeo.h"
#include "VehicleGPS2FactGroup.h"
#include "VehicleGPSAggregateFactGroup.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

namespace {

void sendRawTelemetry(VehicleGPSFactGroup& gps1, VehicleGPS2FactGroup& gps2, const mavlink_gps_raw_int_t& raw)
{
    mavlink_message_t message{};
    mavlink_msg_gps_raw_int_encode(1, 1, &message, &raw);
    gps1.handleMessage(nullptr, message);

    mavlink_gps2_raw_t raw2{};
    raw2.lat = raw.lat;
    raw2.lon = raw.lon;
    raw2.alt = raw.alt;
    raw2.time_usec = raw.time_usec;
    raw2.h_acc = raw.h_acc;
    raw2.v_acc = raw.v_acc;
    raw2.eph = raw.eph;
    raw2.epv = raw.epv;
    raw2.cog = raw.cog;
    raw2.yaw = raw.yaw;
    raw2.fix_type = raw.fix_type;
    raw2.satellites_visible = raw.satellites_visible;
    mavlink_msg_gps2_raw_encode(1, 1, &message, &raw2);
    gps2.handleMessage(nullptr, message);
}

}  // namespace

void VehicleGPSFactGroupTest::_sharedFactsAndMetadata()
{
    QObject metadataOwner;
    const auto expectedMetadata =
        FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Vehicle/GPSFact.json"), &metadataOwner);
    VehicleGPSFactGroup gps1;
    VehicleGPS2FactGroup gps2;
    const std::array<VehicleGPSFactGroup*, 2> groups{&gps1, &gps2};
    const QStringList names{"lat", "lon", "mgrs", "hdop", "vdop", "courseOverGround", "yaw", "lock", "count"};
    for (auto* group : groups) {
        const std::array<Fact*, 9> facts{group->lat(),  group->lon(),  group->mgrs(),
                                         group->hdop(), group->vdop(), group->courseOverGround(),
                                         group->yaw(),  group->lock(), group->count()};
        for (size_t i = 0; i < facts.size(); ++i) {
            Fact* fact = facts[i];
            QCOMPARE(group->getFact(names[i]), fact);
            QCOMPARE(group->property(qPrintable(names[i])).value<Fact*>(), fact);
            QCOMPARE(fact->metaData()->parent(), group);
            auto* metadata = expectedMetadata.value(names[i]);
            QVERIFY(metadata);
            QVERIFY(fact->metaData() != metadata);
            QCOMPARE(fact->decimalPlaces(), metadata->decimalPlaces());
            QCOMPARE(fact->rawUnits(), metadata->rawUnits());
            QCOMPARE(fact->enumValues(), metadata->enumValues());
            QCOMPARE(fact->enumStrings(), metadata->enumStrings());
        }
        QVERIFY(group->factGroupNames().isEmpty());
        QVERIFY(!group->telemetryAvailable());
        QCOMPARE(group->count()->type(), FactMetaData::valueTypeInt32);
        QCOMPARE(group->count()->rawValue().toInt(), 0);
        QCOMPARE(group->lock()->rawValue().toInt(), 0);
        QVERIFY(group->mgrs()->rawValue().toString().isEmpty());
        for (Fact* fact : {group->lat(), group->lon(), group->hdop(), group->vdop(), group->courseOverGround()}) {
            QVERIFY(qIsNaN(fact->rawValue().toDouble()));
        }
    }
    QCOMPARE(gps1.count()->metaData()->type(), FactMetaData::valueTypeUint32);
    QCOMPARE(gps2.count()->metaData()->type(), FactMetaData::valueTypeUint32);
    gps1.count()->setRawValue(UINT32_MAX);
    gps2.count()->setRawValue(UINT32_MAX);
    QCOMPARE(gps1.count()->rawValue().toUInt(), UINT32_MAX);
    QCOMPARE(gps2.count()->rawValue().toUInt(), UINT32_MAX);
    QCOMPARE(gps1.yaw()->rawValue().toDouble(), 0.0);
    QCOMPARE(gps2.yaw()->rawValue().toDouble(), 0.0);
    QCOMPARE(gps1.factNames().first(9), names);
    QCOMPARE(gps2.factNames(), gps1.factNames());
    QCOMPARE(gps1.factNames().size(), 17);
    for (const auto& name : gps1.factNames()) {
        QVERIFY(gps1.getFact(name) != gps2.getFact(name));
        QVERIFY(gps1.getFact(name)->metaData() != gps2.getFact(name)->metaData());
    }
}

void VehicleGPSFactGroupTest::_rawTelemetryEquivalence_data()
{
    QTest::addColumn<int>("dop");
    QTest::addColumn<int>("course");
    QTest::addColumn<int>("heading");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("fixType");
    QTest::newRow("scaled") << 125 << 23456 << 12345 << 18 << 6;
    QTest::newRow("sentinels") << int(UINT16_MAX) << int(UINT16_MAX) << int(UINT16_MAX) << 255 << 0;
    QTest::newRow("zero") << 0 << 0 << 0 << 0 << 1;
    QTest::newRow("north") << 100 << 36000 << 36000 << 12 << 7;
}

void VehicleGPSFactGroupTest::_rawTelemetryEquivalence()
{
    QFETCH(int, dop);
    QFETCH(int, course);
    QFETCH(int, heading);
    QFETCH(int, count);
    QFETCH(int, fixType);
    VehicleGPSFactGroup gps1;
    VehicleGPS2FactGroup gps2;
    QSignalSpy gps1Spy(&gps1, &FactGroup::telemetryAvailableChanged);
    QSignalSpy gps2Spy(&gps2, &FactGroup::telemetryAvailableChanged);
    QMap<QString, QPair<Fact*, FactMetaData*>> identities;
    for (const auto& name : gps1.factNames()) {
        auto* fact = gps1.getFact(name);
        identities.insert(name, {fact, fact->metaData()});
    }

    mavlink_gps_raw_int_t raw{};
    raw.lat = -353123456;
    raw.lon = 1491234567;
    raw.eph = dop;
    raw.epv = dop;
    raw.cog = course;
    raw.yaw = heading;
    raw.satellites_visible = count;
    raw.fix_type = fixType;
    sendRawTelemetry(gps1, gps2, raw);
    for (auto* group : std::array<VehicleGPSFactGroup*, 2>{&gps1, &gps2}) {
        QCOMPARE(group->lat()->rawValue().toDouble(), raw.lat * 1e-7);
        QCOMPARE(group->lon()->rawValue().toDouble(), raw.lon * 1e-7);
        QCOMPARE(group->mgrs()->rawValue().toString(),
                 QGCGeo::convertGeoToMGRS(QGeoCoordinate(raw.lat * 1e-7, raw.lon * 1e-7)));
        QCOMPARE(group->count()->rawValue().toInt(), count == 255 ? 0 : count);
        QCOMPARE(group->lock()->rawValue().toInt(), fixType);
        const std::array<std::pair<Fact*, int>, 4> scaled{
            {{group->hdop(), dop}, {group->vdop(), dop}, {group->courseOverGround(), course}, {group->yaw(), heading}}};
        for (const auto& [fact, value] : scaled) {
            if (value == UINT16_MAX) {
                QVERIFY(qIsNaN(fact->rawValue().toDouble()));
            } else {
                QCOMPARE(fact->rawValue().toDouble(), value / 100.0);
            }
        }
        QVERIFY(group->telemetryAvailable());
    }
    QCOMPARE(gps1Spy.count(), 1);
    QCOMPARE(gps2Spy.count(), 1);
    sendRawTelemetry(gps1, gps2, raw);
    QCOMPARE(gps1Spy.count(), 1);
    QCOMPARE(gps2Spy.count(), 1);
    for (auto it = identities.cbegin(); it != identities.cend(); ++it) {
        QCOMPARE(gps1.getFact(it.key()), it.value().first);
        QCOMPARE(gps1.getFact(it.key())->metaData(), it.value().second);
    }
}

void VehicleGPSFactGroupTest::_highLatencyPartialUpdates_data()
{
    QTest::addColumn<bool>("version2");
    QTest::addColumn<int>("dop");
    QTest::newRow("high-latency") << false << 0;
    QTest::newRow("high-latency2") << true << 23;
    QTest::newRow("high-latency2-unknown-dop") << true << int(UINT8_MAX);
}

void VehicleGPSFactGroupTest::_highLatencyPartialUpdates()
{
    QFETCH(bool, version2);
    QFETCH(int, dop);
    VehicleGPSFactGroup gps1;
    VehicleGPS2FactGroup gps2;
    mavlink_gps_raw_int_t raw{};
    raw.lat = 473123456;
    raw.lon = 85412345;
    raw.eph = 125;
    raw.epv = 175;
    raw.cog = 23456;
    raw.yaw = 12345;
    raw.satellites_visible = 18;
    raw.fix_type = 6;
    sendRawTelemetry(gps1, gps2, raw);
    const auto gps2Receipt = gps2.observation().monotonicTimestampUs;
    QMap<QString, QVariant> gps2Values;
    for (const auto& name : gps2.factNames()) {
        gps2Values.insert(name, gps2.getFact(name)->rawValue());
    }
    mavlink_message_t message{};
    constexpr int32_t LATITUDE = 453000000;
    constexpr int32_t LONGITUDE = 90000000;
    if (version2) {
        mavlink_high_latency2_t partial{};
        partial.latitude = LATITUDE;
        partial.longitude = LONGITUDE;
        partial.eph = dop;
        partial.epv = dop;
        mavlink_msg_high_latency2_encode(1, 1, &message, &partial);
    } else {
        mavlink_high_latency_t partial{};
        partial.latitude = LATITUDE;
        partial.longitude = LONGITUDE;
        mavlink_msg_high_latency_encode(1, 1, &message, &partial);
    }
    gps1.handleMessage(nullptr, message);
    gps2.handleMessage(nullptr, message);
    QVERIFY(!gps1.observation().position.isValid());
    QCOMPARE(gps2.observation().monotonicTimestampUs, gps2Receipt);
    QCOMPARE(gps1.lat()->rawValue().toDouble(), LATITUDE * 1e-7);
    QCOMPARE(gps1.lon()->rawValue().toDouble(), LONGITUDE * 1e-7);
    QCOMPARE(gps1.mgrs()->rawValue().toString(),
             QGCGeo::convertGeoToMGRS(QGeoCoordinate(LATITUDE * 1e-7, LONGITUDE * 1e-7)));
    QCOMPARE(gps1.count()->rawValue().toInt(), 0);
    QCOMPARE(gps1.lock()->rawValue().toInt(), 6);
    QCOMPARE(gps1.courseOverGround()->rawValue().toDouble(), 234.56);
    QCOMPARE(gps1.yaw()->rawValue().toDouble(), 123.45);
    if (!version2) {
        QCOMPARE(gps1.hdop()->rawValue().toDouble(), 1.25);
        QCOMPARE(gps1.vdop()->rawValue().toDouble(), 1.75);
    } else if (dop == UINT8_MAX) {
        QVERIFY(qIsNaN(gps1.hdop()->rawValue().toDouble()));
        QVERIFY(qIsNaN(gps1.vdop()->rawValue().toDouble()));
    } else {
        QCOMPARE(gps1.hdop()->rawValue().toDouble(), dop / 10.0);
        QCOMPARE(gps1.vdop()->rawValue().toDouble(), dop / 10.0);
    }
    for (auto it = gps2Values.cbegin(); it != gps2Values.cend(); ++it) {
        QCOMPARE(gps2.getFact(it.key())->rawValue(), it.value());
    }
}

void VehicleGPSFactGroupTest::_flatIntegrityNotifications()
{
    VehicleGPSFactGroup gps1;
    VehicleGPS2FactGroup gps2;
    VehicleGPSAggregateFactGroup aggregate;
    aggregate.bindToGps(&gps1, &gps2);
    QSignalSpy gps1Spy(&gps1, &VehicleGPSFactGroup::gnssIntegrityReceived);
    QSignalSpy gps2Spy(&gps2, &VehicleGPSFactGroup::gnssIntegrityReceived);
    mavlink_gnss_integrity_t report{};
    report.system_errors = 0x12345678;
    report.spoofing_state = 1;
    report.jamming_state = 2;
    report.authentication_state = 3;
    report.corrections_quality = 4;
    report.system_status_summary = 5;
    report.gnss_signal_quality = 6;
    report.post_processing_quality = 7;
    mavlink_message_t message{};
    const auto sendIntegrity = [&]() {
        mavlink_msg_gnss_integrity_encode(1, 1, &message, &report);
        gps1.handleMessage(nullptr, message);
        gps2.handleMessage(nullptr, message);
    };
    sendIntegrity();
    QCOMPARE(gps1Spy.count(), 1);
    QCOMPARE(gps2Spy.count(), 0);
    QCOMPARE(gps1.systemErrors()->rawValue().toUInt(), report.system_errors);
    QCOMPARE(gps1.getFact(QStringLiteral("spoofingState")), gps1.spoofingState());
    QCOMPARE(gps1.property("spoofingState").value<Fact*>(), gps1.spoofingState());
    QCOMPARE(gps1.correctionsQuality()->rawValue().toInt(), 4);
    QCOMPARE(gps1.systemQuality()->rawValue().toInt(), 5);
    QCOMPARE(gps1.gnssSignalQuality()->rawValue().toInt(), 6);
    QCOMPARE(gps1.postProcessingQuality()->rawValue().toInt(), 7);
    QVERIFY(!aggregate.isStale()->rawValue().toBool());
    QCOMPARE(aggregate.spoofingState()->rawValue().toInt(), 1);
    QCOMPARE(aggregate.jammingState()->rawValue().toInt(), 2);
    QCOMPARE(aggregate.authenticationState()->rawValue().toInt(), 3);

    report.id = 1;
    report.spoofing_state = 3;
    report.jamming_state = 1;
    report.authentication_state = 2;
    sendIntegrity();
    QCOMPARE(gps1Spy.count(), 1);
    QCOMPARE(gps2Spy.count(), 1);
    QCOMPARE(gps2.spoofingState()->rawValue().toInt(), 3);
    QCOMPARE(aggregate.spoofingState()->rawValue().toInt(), 3);
    QCOMPARE(aggregate.jammingState()->rawValue().toInt(), 2);
    QCOMPARE(aggregate.authenticationState()->rawValue().toInt(), 2);
    sendIntegrity();
    QCOMPARE(gps2Spy.count(), 2);
    report.id = 2;
    sendIntegrity();
    QCOMPARE(gps1Spy.count(), 1);
    QCOMPARE(gps2Spy.count(), 2);
    QVERIFY(!gps1.telemetryAvailable());
    QVERIFY(!gps2.telemetryAvailable());
}

void VehicleGPSFactGroupTest::_typedObservations_data()
{
    QTest::addColumn<int>("fixType");
    QTest::addColumn<GPSObservation::FixQuality>("quality");
    QTest::addColumn<bool>("fixValid");
    QTest::newRow("no-gps") << 0 << GPSObservation::FixQuality::NoFix << false;
    QTest::newRow("no-fix") << 1 << GPSObservation::FixQuality::NoFix << false;
    QTest::newRow("2d") << 2 << GPSObservation::FixQuality::Fix2D << true;
    QTest::newRow("3d") << 3 << GPSObservation::FixQuality::Fix3D << true;
    QTest::newRow("differential") << 4 << GPSObservation::FixQuality::Differential << true;
    QTest::newRow("rtk-float") << 5 << GPSObservation::FixQuality::RTKFloat << true;
    QTest::newRow("rtk-fixed") << 6 << GPSObservation::FixQuality::RTKFixed << true;
    QTest::newRow("static") << 7 << GPSObservation::FixQuality::Fix3D << true;
    QTest::newRow("ppp") << 8 << GPSObservation::FixQuality::Fix3D << true;
    QTest::newRow("unknown") << 255 << GPSObservation::FixQuality::Unknown << false;
}

void VehicleGPSFactGroupTest::_typedObservations()
{
    QFETCH(int, fixType);
    QFETCH(GPSObservation::FixQuality, quality);
    QFETCH(bool, fixValid);
    VehicleGPSFactGroup gps1;
    VehicleGPS2FactGroup gps2;
    mavlink_gps_raw_int_t raw{};
    raw.time_usec = 1234567;
    raw.lat = 473977000;
    raw.lon = 85456000;
    raw.alt = 456789;
    raw.fix_type = fixType;
    raw.satellites_visible = 19;
    const auto before = QDateTime::currentDateTimeUtc();
    sendRawTelemetry(gps1, gps2, raw);

    for (auto* group : std::array<VehicleGPSFactGroup*, 2>{&gps1, &gps2}) {
        const auto& observation = group->observation();
        QCOMPARE(observation.fixQuality, quality);
        QCOMPARE(observation.receiverFixValid, std::optional<bool>(fixValid));
        QCOMPARE(observation.position.coordinate().latitude(), raw.lat * 1e-7);
        QCOMPARE(observation.position.coordinate().longitude(), raw.lon * 1e-7);
        QVERIFY(observation.monotonicTimestampUs > 0);
        QVERIFY(observation.receivedAt >= before);
        QCOMPARE(observation.position.timestamp(), observation.receivedAt);
        QVERIFY(!observation.satellitesUsed.has_value());
        QVERIFY(!observation.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
        QVERIFY(!observation.usable());
        QCOMPARE(GPSPositionPolicy::project(observation, GPSObservation::PositionUse::Gga).has_value(), fixValid);
        if (fixValid && fixType >= GPS_FIX_TYPE_3D_FIX) {
            QCOMPARE(observation.position.coordinate().altitude(), 456.789);
            QCOMPARE(observation.altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
        } else {
            QVERIFY(qIsNaN(observation.position.coordinate().altitude()));
            QCOMPARE(observation.altitudeDatum, GPSAltitudeDatum::Unknown);
        }
    }
    QCOMPARE(gps1.observation().sourceId, QStringLiteral("VehicleGPS"));
    QCOMPARE(gps2.observation().sourceId, QStringLiteral("VehicleGPS2"));
}

void VehicleGPSFactGroupTest::_observationReceiptAndInvalidation()
{
    VehicleGPSFactGroup gps1;
    VehicleGPS2FactGroup gps2;
    mavlink_gps_raw_int_t raw{};
    raw.lat = 473977000;
    raw.lon = 85456000;
    raw.alt = 456789;
    raw.fix_type = GPS_FIX_TYPE_3D_FIX;
    raw.h_acc = 1500;
    raw.v_acc = 2500;
    sendRawTelemetry(gps1, gps2, raw);
    const auto original = gps1.observation();
    QCOMPARE(original.position.attribute(QGeoPositionInfo::HorizontalAccuracy), 1.5);
    QCOMPARE(original.position.attribute(QGeoPositionInfo::VerticalAccuracy), 2.5);
    QCOMPARE(original.accuracyTimestampUs, original.monotonicTimestampUs);
    gps1.lat()->rawValue();
    QCOMPARE(gps1.observation().monotonicTimestampUs, original.monotonicTimestampUs);

    const auto beforeReceipt = MonotonicClock::nowUs();
    sendRawTelemetry(gps1, gps2, raw);
    QCOMPARE(gps1.observation().position.coordinate(), original.position.coordinate());
    QVERIFY(gps1.observation().monotonicTimestampUs >= beforeReceipt);
    QVERIFY(gps2.observation().monotonicTimestampUs >= beforeReceipt);

    raw.alt = INT32_MAX;
    raw.h_acc = 0;
    raw.v_acc = UINT32_MAX;
    sendRawTelemetry(gps1, gps2, raw);
    for (auto* group : std::array<VehicleGPSFactGroup*, 2>{&gps1, &gps2}) {
        QVERIFY(qIsNaN(group->observation().position.coordinate().altitude()));
        QCOMPARE(group->observation().altitudeDatum, GPSAltitudeDatum::Unknown);
        QVERIFY(!group->observation().position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
        QVERIFY(!group->observation().position.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
        const auto latitude = group->lat()->rawValue();
        group->invalidateObservation();
        QVERIFY(!group->observation().position.isValid());
        QCOMPARE(group->observation().monotonicTimestampUs, quint64(0));
        QCOMPARE(group->lat()->rawValue(), latitude);
    }
}

UT_REGISTER_TEST(VehicleGPSFactGroupTest, TestLabel::Unit)
